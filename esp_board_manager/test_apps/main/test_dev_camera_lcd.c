/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "linux/videodev2.h"
#include "sdkconfig.h"
#include "dev_camera.h"
#include "dev_display_lcd.h"
#include "esp_board_manager.h"
#include "esp_imgfx_color_convert.h"
#include "esp_imgfx_types.h"
#include "bmgr_test_names.h"
#include "test_dev_camera_convert.h"
#include "test_dev_camera_lcd.h"

#define CAMERA_LCD_BUFFER_COUNT     2
#define CAMERA_LCD_PREVIEW_SECONDS  30
#define CAMERA_LCD_COLOR_SPACE      ESP_IMGFX_COLOR_SPACE_STD_BT601
#define CAMERA_LCD_BYTES_PER_PIXEL  2
#define CAMERA_LCD_BUFFER_ALIGN     64

static const char *TAG = "TEST_CAMERA_LCD";

typedef struct {
    uint8_t *data;
    size_t   length;
} camera_lcd_mmap_buffer_t;

typedef struct {
    uint32_t  width;
    uint32_t  height;
    uint32_t  pixelformat;
    uint32_t  bytesperline;
    uint32_t  sizeimage;
} camera_lcd_video_format_t;

typedef struct {
    esp_imgfx_pixel_fmt_t             input_fmt;
    esp_imgfx_pixel_fmt_t             output_fmt;
    uint8_t                          *packed_input_buf;
    uint8_t                          *converted_buf;
    uint8_t                          *draw_buf;
    uint32_t                          input_size;
    uint32_t                          converted_size;
    uint32_t                          draw_size;
    uint32_t                          source_stride;
    uint32_t                          tight_rgb565_stride;
    uint32_t                          draw_w;
    uint32_t                          draw_h;
    uint32_t                          draw_x;
    uint32_t                          draw_y;
    uint32_t                          src_x;
    uint32_t                          src_y;
    bool                              need_convert;
    bool                              need_pack_input;
    bool                              need_copy_window;
    esp_imgfx_color_convert_handle_t  convert_handle;
} camera_lcd_frame_pipeline_t;

static esp_err_t camera_lcd_imgfx_to_esp_err(esp_imgfx_err_t err)
{
    switch (err) {
        case ESP_IMGFX_ERR_OK:
            return ESP_OK;
        case ESP_IMGFX_ERR_MEM_LACK:
            return ESP_ERR_NO_MEM;
        case ESP_IMGFX_ERR_INVALID_PARAMETER:
            return ESP_ERR_INVALID_ARG;
        case ESP_IMGFX_ERR_NOT_SUPPORTED:
            return ESP_ERR_NOT_SUPPORTED;
        case ESP_IMGFX_ERR_BUFF_NOT_ENOUGH:
            return ESP_ERR_INVALID_SIZE;
        case ESP_IMGFX_ERR_DATA_LACK:
            return ESP_ERR_INVALID_SIZE;
        case ESP_IMGFX_ERR_FAIL:
        default:
            return ESP_FAIL;
    }
}

static uint32_t camera_lcd_get_source_stride(const camera_lcd_video_format_t *camera_fmt)
{
    uint32_t tight_stride = 0;

    if (camera_fmt == NULL ||
        !camera_convert_get_tight_stride(camera_fmt->pixelformat, camera_fmt->width, &tight_stride)) {
        return 0;
    }

    return camera_fmt->bytesperline != 0 ? camera_fmt->bytesperline : tight_stride;
}

static bool camera_lcd_get_output_fmt(const dev_display_lcd_config_t *lcd_cfg,
                                      esp_imgfx_pixel_fmt_t *output_fmt)
{
    if (lcd_cfg == NULL || output_fmt == NULL) {
        return false;
    }

    switch (lcd_cfg->frame_format) {
    case DEV_DISPLAY_LCD_FRAME_FORMAT_RGB565_LE:
        *output_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE;
        return true;
    case DEV_DISPLAY_LCD_FRAME_FORMAT_RGB565_BE:
        *output_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_BE;
        return true;
    default:
        ESP_LOGE(TAG, "LCD frame format %d is unsupported by the RGB565 camera preview", lcd_cfg->frame_format);
        return false;
    }
}

static void *camera_lcd_aligned_calloc(size_t size)
{
    uint32_t caps = MALLOC_CAP_8BIT;
#ifdef CONFIG_SPIRAM
    caps |= MALLOC_CAP_SPIRAM;
#endif  /* CONFIG_SPIRAM */
    return heap_caps_aligned_calloc(CAMERA_LCD_BUFFER_ALIGN, 1, size, caps);
}

static esp_err_t camera_lcd_query_format(int fd, camera_lcd_video_format_t *fmt)
{
    if (fmt == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    struct v4l2_format v4l2_fmt = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
    };
    if (ioctl(fd, VIDIOC_G_FMT, &v4l2_fmt) != 0) {
        ESP_LOGE(TAG, "Failed to get camera format");
        return ESP_FAIL;
    }

    fmt->width = v4l2_fmt.fmt.pix.width;
    fmt->height = v4l2_fmt.fmt.pix.height;
    fmt->pixelformat = v4l2_fmt.fmt.pix.pixelformat;
    fmt->bytesperline = v4l2_fmt.fmt.pix.bytesperline;
    fmt->sizeimage = v4l2_fmt.fmt.pix.sizeimage;

    char fourcc[5];
    ESP_LOGI(TAG, "Camera format: %" PRIu32 "x%" PRIu32 " %s, bytesperline=%" PRIu32 ", sizeimage=%" PRIu32,
             fmt->width, fmt->height, camera_convert_fourcc_to_str(fmt->pixelformat, fourcc),
             fmt->bytesperline, fmt->sizeimage);
    return ESP_OK;
}

static esp_err_t camera_lcd_request_mmap_buffers(int fd, camera_lcd_mmap_buffer_t buffers[], size_t buffer_count)
{
    if (buffers == NULL || buffer_count == 0 || buffer_count > UINT32_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    struct v4l2_requestbuffers req = {
        .count = buffer_count,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP,
    };
    if (ioctl(fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "Failed to request camera buffers");
        return ESP_FAIL;
    }
    if (req.count < buffer_count) {
        ESP_LOGE(TAG, "Camera returned too few buffers: %" PRIu32, req.count);
        return ESP_ERR_NO_MEM;
    }

    for (size_t i = 0; i < buffer_count; i++) {
        struct v4l2_buffer buf = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
            .index = i,
        };
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) != 0) {
            ESP_LOGE(TAG, "Failed to query camera buffer %u", (unsigned)i);
            return ESP_FAIL;
        }

        buffers[i].data = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].data == MAP_FAILED) {
            ESP_LOGE(TAG, "Failed to mmap camera buffer %u", (unsigned)i);
            buffers[i].data = NULL;
            return ESP_FAIL;
        }
        buffers[i].length = buf.length;
        ESP_LOGI(TAG, "Camera buffer %u: length=%" PRIu32 ", bytesused=%" PRIu32,
                 (unsigned)i, buf.length, buf.bytesused);

        if (ioctl(fd, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "Failed to queue camera buffer %u", (unsigned)i);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

static void camera_lcd_release_mmap_buffers(camera_lcd_mmap_buffer_t buffers[], size_t buffer_count)
{
    if (buffers == NULL) {
        return;
    }
    for (size_t i = 0; i < buffer_count; i++) {
        if (buffers[i].data != NULL && buffers[i].length > 0) {
            munmap(buffers[i].data, buffers[i].length);
            buffers[i].data = NULL;
            buffers[i].length = 0;
        }
    }
}

static void camera_lcd_calc_viewport(const camera_lcd_video_format_t *camera_fmt,
                                     const dev_display_lcd_config_t *lcd_cfg,
                                     camera_lcd_frame_pipeline_t *pipeline)
{
    pipeline->draw_w = camera_fmt->width < lcd_cfg->lcd_width ? camera_fmt->width : lcd_cfg->lcd_width;
    pipeline->draw_h = camera_fmt->height < lcd_cfg->lcd_height ? camera_fmt->height : lcd_cfg->lcd_height;
    pipeline->draw_x = (lcd_cfg->lcd_width - pipeline->draw_w) / 2;
    pipeline->draw_y = (lcd_cfg->lcd_height - pipeline->draw_h) / 2;
    pipeline->src_x = (camera_fmt->width - pipeline->draw_w) / 2;
    pipeline->src_y = (camera_fmt->height - pipeline->draw_h) / 2;
    pipeline->need_copy_window = (pipeline->draw_w != camera_fmt->width) || (pipeline->draw_h != camera_fmt->height) || (pipeline->src_x != 0) || (pipeline->src_y != 0);
}

static esp_err_t camera_lcd_pipeline_init(const camera_lcd_video_format_t *camera_fmt,
                                          const dev_display_lcd_config_t *lcd_cfg,
                                          camera_lcd_frame_pipeline_t *pipeline)
{
    if (camera_fmt == NULL || lcd_cfg == NULL || pipeline == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memset(pipeline, 0, sizeof(*pipeline));

    if (!camera_convert_v4l2_to_imgfx_fmt(camera_fmt->pixelformat, &pipeline->input_fmt)) {
        char fourcc[5];
        ESP_LOGE(TAG, "Unsupported camera pixel format for LCD preview: %s",
                 camera_convert_fourcc_to_str(camera_fmt->pixelformat, fourcc));
        return ESP_ERR_NOT_SUPPORTED;
    }

    camera_lcd_calc_viewport(camera_fmt, lcd_cfg, pipeline);
    if (!camera_lcd_get_output_fmt(lcd_cfg, &pipeline->output_fmt)) {
        return ESP_ERR_NOT_SUPPORTED;
    }
    pipeline->need_convert = pipeline->input_fmt != pipeline->output_fmt;
    pipeline->tight_rgb565_stride = camera_fmt->width * CAMERA_LCD_BYTES_PER_PIXEL;
    pipeline->source_stride = pipeline->need_convert ? pipeline->tight_rgb565_stride : camera_lcd_get_source_stride(camera_fmt);
    if (pipeline->source_stride == 0) {
        pipeline->source_stride = pipeline->tight_rgb565_stride;
    }
    pipeline->need_copy_window = pipeline->need_copy_window || (!pipeline->need_convert && pipeline->source_stride != pipeline->tight_rgb565_stride);

    esp_imgfx_resolution_t camera_res = {
        .width = camera_fmt->width,
        .height = camera_fmt->height,
    };
    esp_imgfx_err_t imgfx_ret = esp_imgfx_get_image_size(pipeline->input_fmt,
                                                         &camera_res,
                                                         &pipeline->input_size);
    if (imgfx_ret != ESP_IMGFX_ERR_OK) {
        return camera_lcd_imgfx_to_esp_err(imgfx_ret);
    }

    imgfx_ret = esp_imgfx_get_image_size(pipeline->output_fmt,
                                         &camera_res,
                                         &pipeline->converted_size);
    if (imgfx_ret != ESP_IMGFX_ERR_OK) {
        return camera_lcd_imgfx_to_esp_err(imgfx_ret);
    }

    pipeline->draw_size = pipeline->draw_w * pipeline->draw_h * CAMERA_LCD_BYTES_PER_PIXEL;

    uint32_t input_tight_stride = 0;
    uint32_t input_source_stride = camera_lcd_get_source_stride(camera_fmt);
    if (pipeline->need_convert &&
        camera_convert_get_tight_stride(camera_fmt->pixelformat, camera_fmt->width, &input_tight_stride) &&
        input_source_stride != 0 &&
        input_source_stride != input_tight_stride) {
        pipeline->need_pack_input = true;
        pipeline->packed_input_buf = camera_lcd_aligned_calloc(pipeline->input_size);
        if (pipeline->packed_input_buf == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    if (pipeline->need_convert) {
        pipeline->converted_buf = camera_lcd_aligned_calloc(pipeline->converted_size);
        if (pipeline->converted_buf == NULL) {
            return ESP_ERR_NO_MEM;
        }

        esp_imgfx_color_convert_cfg_t convert_cfg = {
            .in_res = camera_res,
            .in_pixel_fmt = pipeline->input_fmt,
            .out_pixel_fmt = pipeline->output_fmt,
            .color_space_std = CAMERA_LCD_COLOR_SPACE,
        };
        imgfx_ret = esp_imgfx_color_convert_open(&convert_cfg, &pipeline->convert_handle);
        if (imgfx_ret != ESP_IMGFX_ERR_OK) {
            return camera_lcd_imgfx_to_esp_err(imgfx_ret);
        }
    }

    if (pipeline->need_copy_window) {
        pipeline->draw_buf = camera_lcd_aligned_calloc(pipeline->draw_size);
        if (pipeline->draw_buf == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }

    ESP_LOGI(TAG, "LCD preview viewport: src=%" PRIu32 "x%" PRIu32 " draw=%" PRIu32 "x%" PRIu32 " at (%" PRIu32 ",%" PRIu32 "), input="
             ESP_IMGFX_FOURCC_FMT ", output=" ESP_IMGFX_FOURCC_FMT ", stride=%" PRIu32 ", convert=%d, pack=%d, copy=%d",
             camera_fmt->width, camera_fmt->height, pipeline->draw_w, pipeline->draw_h,
             pipeline->draw_x, pipeline->draw_y,
             ESP_IMGFX_FOURCC_ARG(pipeline->input_fmt), ESP_IMGFX_FOURCC_ARG(pipeline->output_fmt),
             pipeline->source_stride, pipeline->need_convert, pipeline->need_pack_input,
             pipeline->need_copy_window);
    return ESP_OK;
}

static void camera_lcd_pipeline_deinit(camera_lcd_frame_pipeline_t *pipeline)
{
    if (pipeline == NULL) {
        return;
    }
    esp_imgfx_color_convert_close(pipeline->convert_handle);
    if (pipeline->packed_input_buf != NULL) {
        heap_caps_free(pipeline->packed_input_buf);
    }
    if (pipeline->converted_buf != NULL) {
        heap_caps_free(pipeline->converted_buf);
    }
    if (pipeline->draw_buf != NULL) {
        heap_caps_free(pipeline->draw_buf);
    }
    memset(pipeline, 0, sizeof(*pipeline));
}

static void camera_lcd_copy_rgb565_window(uint8_t *dst, const uint8_t *src,
                                          uint32_t src_stride, uint32_t src_x, uint32_t src_y,
                                          uint32_t draw_w, uint32_t draw_h)
{
    size_t dst_stride = draw_w * CAMERA_LCD_BYTES_PER_PIXEL;
    const uint8_t *src_start = src + (src_y * src_stride) + (src_x * CAMERA_LCD_BYTES_PER_PIXEL);
    for (uint32_t y = 0; y < draw_h; y++) {
        memcpy(dst + y * dst_stride, src_start + y * src_stride, dst_stride);
    }
}

static esp_err_t camera_lcd_prepare_rgb565_frame(camera_lcd_frame_pipeline_t *pipeline,
                                                 const camera_lcd_video_format_t *camera_fmt,
                                                 uint8_t *camera_buf,
                                                 size_t camera_len,
                                                 uint8_t **out_draw_buf)
{
    if (pipeline == NULL || camera_fmt == NULL || camera_buf == NULL || out_draw_buf == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *rgb565_frame = camera_buf;
    if (pipeline->need_convert) {
        uint8_t *convert_input = camera_buf;
        size_t convert_input_len = camera_len;

        if (pipeline->need_pack_input) {
            uint32_t input_tight_stride = 0;
            uint32_t input_source_stride = camera_lcd_get_source_stride(camera_fmt);
            if (!camera_convert_get_tight_stride(camera_fmt->pixelformat, camera_fmt->width, &input_tight_stride)) {
                return ESP_ERR_NOT_SUPPORTED;
            }
            ESP_RETURN_ON_ERROR(camera_convert_pack_rows(pipeline->packed_input_buf, camera_buf, camera_len,
                                                         input_source_stride, input_tight_stride, camera_fmt->height),
                                TAG, "Failed to pack camera frame rows");
            convert_input = pipeline->packed_input_buf;
            convert_input_len = pipeline->input_size;
        }

        esp_imgfx_data_t in_image = {
            .data = convert_input,
            .data_len = convert_input_len,
        };
        esp_imgfx_data_t out_image = {
            .data = pipeline->converted_buf,
            .data_len = pipeline->converted_size,
        };
        esp_imgfx_err_t imgfx_ret = esp_imgfx_color_convert_process(pipeline->convert_handle, &in_image, &out_image);
        if (imgfx_ret != ESP_IMGFX_ERR_OK) {
            ESP_LOGE(TAG, "Color convert failed: %d", imgfx_ret);
            return camera_lcd_imgfx_to_esp_err(imgfx_ret);
        }
        rgb565_frame = pipeline->converted_buf;
    }

    if (pipeline->need_copy_window) {
        camera_lcd_copy_rgb565_window(pipeline->draw_buf, rgb565_frame,
                                      pipeline->source_stride, pipeline->src_x, pipeline->src_y,
                                      pipeline->draw_w, pipeline->draw_h);
        *out_draw_buf = pipeline->draw_buf;
    } else {
        if (camera_len < pipeline->draw_size && !pipeline->need_convert) {
            ESP_LOGE(TAG, "Camera frame too small for direct draw: len=%u, need=%" PRIu32,
                     (unsigned)camera_len, pipeline->draw_size);
            return ESP_ERR_INVALID_SIZE;
        }
        *out_draw_buf = rgb565_frame;
    }

    return ESP_OK;
}

static esp_err_t camera_lcd_stream_to_panel(int fd,
                                            const camera_lcd_video_format_t *camera_fmt,
                                            camera_lcd_mmap_buffer_t buffers[],
                                            size_t buffer_count,
                                            esp_lcd_panel_handle_t panel_handle,
                                            camera_lcd_frame_pipeline_t *pipeline)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "Failed to start camera stream");
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_OK;
    uint32_t frame_count = 0;
    int64_t start_time_us = esp_timer_get_time();
    while (esp_timer_get_time() - start_time_us < CAMERA_LCD_PREVIEW_SECONDS * 1000 * 1000) {
        struct v4l2_buffer buf = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
        };
        if (ioctl(fd, VIDIOC_DQBUF, &buf) != 0) {
            ESP_LOGE(TAG, "Failed to dequeue camera frame");
            ret = ESP_FAIL;
            break;
        }

        if ((buf.flags & V4L2_BUF_FLAG_DONE) && buf.index < buffer_count && buffers[buf.index].data != NULL) {
            if (frame_count == 0) {
                ESP_LOGI(TAG, "First camera frame: index=%" PRIu32 ", bytesused=%" PRIu32 ", buffer_length=%u",
                         buf.index, buf.bytesused, (unsigned)buffers[buf.index].length);
            }
            uint8_t *draw_buf = NULL;
            ret = camera_lcd_prepare_rgb565_frame(pipeline, camera_fmt,
                                                  buffers[buf.index].data,
                                                  buf.bytesused,
                                                  &draw_buf);
            if (ret == ESP_OK) {
                ret = esp_lcd_panel_draw_bitmap(panel_handle,
                                                pipeline->draw_x,
                                                pipeline->draw_y,
                                                pipeline->draw_x + pipeline->draw_w,
                                                pipeline->draw_y + pipeline->draw_h,
                                                draw_buf);
            }
            if (ret == ESP_OK) {
                frame_count++;
            }
        }

        if (ioctl(fd, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "Failed to requeue camera frame");
            ret = ESP_FAIL;
            break;
        }
        if (ret != ESP_OK) {
            break;
        }
    }

    if (ioctl(fd, VIDIOC_STREAMOFF, &type) != 0) {
        ESP_LOGW(TAG, "Failed to stop camera stream");
        if (ret == ESP_OK) {
            ret = ESP_FAIL;
        }
    }

    ESP_LOGI(TAG, "Camera LCD preview displayed %" PRIu32 " frame(s)", frame_count);
    return ret;
}

esp_err_t test_dev_camera_lcd(void)
{
    dev_camera_handle_t *camera_handle = NULL;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_handle(BMGR_TEST_NAME_CAMERA, (void **)&camera_handle),
                        TAG, "Failed to get camera device handle");
    if (camera_handle == NULL || camera_handle->dev_path == NULL) {
        ESP_LOGE(TAG, "Camera device handle/path is NULL");
        return ESP_ERR_INVALID_STATE;
    }

    dev_display_lcd_handles_t *lcd_handles = NULL;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_handle(BMGR_TEST_NAME_DISPLAY_LCD, (void **)&lcd_handles),
                        TAG, "Failed to get LCD device handle");
    if (lcd_handles == NULL || lcd_handles->panel_handle == NULL) {
        ESP_LOGE(TAG, "LCD panel handle is NULL");
        return ESP_ERR_INVALID_STATE;
    }

    dev_display_lcd_config_t *lcd_cfg = NULL;
    ESP_RETURN_ON_ERROR(esp_board_manager_get_device_config(BMGR_TEST_NAME_DISPLAY_LCD, (void **)&lcd_cfg),
                        TAG, "Failed to get LCD device config");
    if (lcd_cfg == NULL || lcd_cfg->lcd_width == 0 || lcd_cfg->lcd_height == 0) {
        ESP_LOGE(TAG, "Invalid LCD config");
        return ESP_ERR_INVALID_STATE;
    }

    int fd = open(camera_handle->dev_path, O_RDONLY);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open camera device: %s", camera_handle->dev_path);
        return ESP_FAIL;
    }

    esp_err_t ret = ESP_OK;
    camera_lcd_video_format_t camera_fmt = {0};
    camera_lcd_mmap_buffer_t buffers[CAMERA_LCD_BUFFER_COUNT] = {0};
    camera_lcd_frame_pipeline_t pipeline = {0};

    ret = camera_lcd_query_format(fd, &camera_fmt);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = camera_lcd_pipeline_init(&camera_fmt, lcd_cfg, &pipeline);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = camera_lcd_request_mmap_buffers(fd, buffers, CAMERA_LCD_BUFFER_COUNT);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = camera_lcd_stream_to_panel(fd, &camera_fmt, buffers, CAMERA_LCD_BUFFER_COUNT,
                                     lcd_handles->panel_handle, &pipeline);

cleanup:
    camera_lcd_release_mmap_buffers(buffers, CAMERA_LCD_BUFFER_COUNT);
    camera_lcd_pipeline_deinit(&pipeline);
    close(fd);
    return ret;
}
