/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "linux/videodev2.h"
#include "esp_imgfx_color_convert.h"
#include "test_dev_camera_convert.h"

#define CAMERA_CONVERT_COLOR_SPACE   ESP_IMGFX_COLOR_SPACE_STD_BT601
#define CAMERA_CONVERT_BUFFER_ALIGN  64
#define CAMERA_CONVERT_RGB565_BYTES  2

static const char *TAG = "TEST_CAMERA_CONVERT";

const char *camera_convert_fourcc_to_str(uint32_t fourcc, char text[5])
{
    text[0] = (char)(fourcc & 0xff);
    text[1] = (char)((fourcc >> 8) & 0xff);
    text[2] = (char)((fourcc >> 16) & 0xff);
    text[3] = (char)((fourcc >> 24) & 0xff);
    text[4] = '\0';
    return text;
}

bool camera_convert_v4l2_to_imgfx_fmt(uint32_t v4l2_fmt, esp_imgfx_pixel_fmt_t *out_fmt)
{
    if (out_fmt == NULL) {
        return false;
    }

    switch (v4l2_fmt) {
        case V4L2_PIX_FMT_RGB565:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE;
            return true;
        case V4L2_PIX_FMT_RGB565X:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_BE;
            return true;
        case V4L2_PIX_FMT_YUYV:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_YUYV;
            return true;
        case V4L2_PIX_FMT_UYVY:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_UYVY;
            return true;
        case V4L2_PIX_FMT_GREY:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_Y;
            return true;
        case V4L2_PIX_FMT_RGB24:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_RGB888;
            return true;
        case V4L2_PIX_FMT_BGR24:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_BGR888;
            return true;
        case V4L2_PIX_FMT_YUV420:
            *out_fmt = ESP_IMGFX_PIXEL_FMT_I420;
            return true;
        default:
            return false;
    }
}

bool camera_convert_get_tight_stride(uint32_t v4l2_fmt, uint32_t width, uint32_t *stride)
{
    uint32_t bytes_per_pixel = 0;

    if (stride == NULL) {
        return false;
    }

    switch (v4l2_fmt) {
        case V4L2_PIX_FMT_GREY:
            bytes_per_pixel = 1;
            break;
        case V4L2_PIX_FMT_RGB565:
        case V4L2_PIX_FMT_RGB565X:
        case V4L2_PIX_FMT_YUYV:
        case V4L2_PIX_FMT_UYVY:
            bytes_per_pixel = 2;
            break;
        case V4L2_PIX_FMT_RGB24:
        case V4L2_PIX_FMT_BGR24:
            bytes_per_pixel = 3;
            break;
        default:
            return false;
    }

    *stride = width * bytes_per_pixel;
    return true;
}

esp_err_t camera_convert_pack_rows(uint8_t *dst, const uint8_t *src, size_t src_len,
                                   uint32_t src_stride, uint32_t tight_stride, uint32_t height)
{
    size_t needed_size = (size_t)src_stride * height;

    if (dst == NULL || src == NULL || src_stride < tight_stride || tight_stride == 0 || height == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (src_len < needed_size) {
        ESP_LOGE(TAG, "Camera frame too small for padded rows: len=%u, need=%u",
                 (unsigned)src_len, (unsigned)needed_size);
        return ESP_ERR_INVALID_SIZE;
    }

    for (uint32_t y = 0; y < height; y++) {
        memcpy(dst + y * tight_stride, src + y * src_stride, tight_stride);
    }
    return ESP_OK;
}

static esp_err_t camera_convert_imgfx_to_esp_err(esp_imgfx_err_t err)
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

static void *camera_convert_aligned_calloc(size_t size)
{
    uint32_t caps = MALLOC_CAP_8BIT;
#ifdef CONFIG_SPIRAM
    caps |= MALLOC_CAP_SPIRAM;
#endif  /* CONFIG_SPIRAM */
    return heap_caps_aligned_calloc(CAMERA_CONVERT_BUFFER_ALIGN, 1, size, caps);
}

esp_err_t camera_convert_to_rgb565(uint32_t v4l2_fmt, uint32_t width, uint32_t height, uint32_t bytesperline,
                                   const uint8_t *src, size_t src_len,
                                   uint8_t **out_buf, size_t *out_len)
{
    if (src == NULL || out_buf == NULL || out_len == NULL || width == 0 || height == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_imgfx_pixel_fmt_t input_fmt;
    if (!camera_convert_v4l2_to_imgfx_fmt(v4l2_fmt, &input_fmt)) {
        char fourcc[5];
        ESP_LOGE(TAG, "No ESP-IMGFX equivalent for camera format: %s", camera_convert_fourcc_to_str(v4l2_fmt, fourcc));
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_imgfx_resolution_t res = {
        .width = width,
        .height = height,
    };

    uint32_t input_size = 0;
    esp_imgfx_err_t imgfx_ret = esp_imgfx_get_image_size(input_fmt, &res, &input_size);
    ESP_RETURN_ON_FALSE(imgfx_ret == ESP_IMGFX_ERR_OK, camera_convert_imgfx_to_esp_err(imgfx_ret), TAG,
                        "Failed to get input image size");

    uint8_t *convert_input = (uint8_t *)src;
    size_t convert_input_len = src_len;
    uint8_t *packed_input_buf = NULL;

    uint32_t tight_stride = 0;
    uint32_t source_stride = bytesperline;
    if (camera_convert_get_tight_stride(v4l2_fmt, width, &tight_stride) &&
        source_stride != 0 && source_stride != tight_stride) {
        packed_input_buf = camera_convert_aligned_calloc(input_size);
        if (packed_input_buf == NULL) {
            return ESP_ERR_NO_MEM;
        }
        esp_err_t pack_ret = camera_convert_pack_rows(packed_input_buf, src, src_len, source_stride, tight_stride, height);
        if (pack_ret != ESP_OK) {
            heap_caps_free(packed_input_buf);
            return pack_ret;
        }
        convert_input = packed_input_buf;
        convert_input_len = input_size;
    }

    uint32_t converted_size = 0;
    imgfx_ret = esp_imgfx_get_image_size(ESP_IMGFX_PIXEL_FMT_RGB565_LE, &res, &converted_size);
    if (imgfx_ret != ESP_IMGFX_ERR_OK) {
        if (packed_input_buf != NULL) {
            heap_caps_free(packed_input_buf);
        }
        return camera_convert_imgfx_to_esp_err(imgfx_ret);
    }

    uint8_t *converted_buf = camera_convert_aligned_calloc(converted_size);
    if (converted_buf == NULL) {
        if (packed_input_buf != NULL) {
            heap_caps_free(packed_input_buf);
        }
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = ESP_OK;
    esp_imgfx_color_convert_handle_t convert_handle = NULL;
    esp_imgfx_color_convert_cfg_t convert_cfg = {
        .in_res = res,
        .in_pixel_fmt = input_fmt,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .color_space_std = CAMERA_CONVERT_COLOR_SPACE,
    };
    imgfx_ret = esp_imgfx_color_convert_open(&convert_cfg, &convert_handle);
    if (imgfx_ret != ESP_IMGFX_ERR_OK) {
        ret = camera_convert_imgfx_to_esp_err(imgfx_ret);
        goto exit;
    }

    esp_imgfx_data_t in_image = {
        .data = convert_input,
        .data_len = convert_input_len,
    };
    esp_imgfx_data_t out_image = {
        .data = converted_buf,
        .data_len = converted_size,
    };
    imgfx_ret = esp_imgfx_color_convert_process(convert_handle, &in_image, &out_image);
    if (imgfx_ret != ESP_IMGFX_ERR_OK) {
        ESP_LOGE(TAG, "Color convert failed: %d", imgfx_ret);
        ret = camera_convert_imgfx_to_esp_err(imgfx_ret);
        goto exit;
    }

    *out_buf = converted_buf;
    *out_len = converted_size;

exit:
    esp_imgfx_color_convert_close(convert_handle);
    if (packed_input_buf != NULL) {
        heap_caps_free(packed_input_buf);
    }
    if (ret != ESP_OK) {
        heap_caps_free(converted_buf);
    }
    return ret;
}
