/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "esp_log.h"
#include "dev_camera.h"
#include "esp_board_manager.h"
#include "bmgr_test_names.h"
#include "test_dev_camera_auto.h"

#define CAMERA_AUTO_BUFFER_COUNT  1
#define CAMERA_AUTO_MEMORY_TYPE   V4L2_MEMORY_MMAP

typedef struct {
    void   *data;
    size_t  length;
} camera_auto_mmap_buffer_t;

static const char *TAG = "TEST_DEV_CAMERA_AUTO";
static test_dev_camera_auto_summary_t s_last_summary;

static uint32_t camera_auto_checksum(const uint8_t *data, size_t size)
{
    uint32_t hash = 2166136261u;

    for (size_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 16777619u;
    }
    return hash;
}

static float camera_auto_mean_luma(const uint8_t *data, size_t size)
{
    uint64_t sum = 0;

    if (data == NULL || size == 0) {
        return 0.0f;
    }

    for (size_t i = 0; i < size; i++) {
        sum += data[i];
    }
    return (float)((double)sum / (double)size);
}

static esp_err_t camera_auto_open_device(const char **dev_path, int *fd)
{
    dev_camera_handle_t *camera_handle = NULL;
    esp_err_t ret = ESP_OK;

    if (dev_path == NULL || fd == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_CAMERA, (void **)&camera_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get camera device handle: %s", esp_err_to_name(ret));
        return ret;
    }
    if (camera_handle == NULL || camera_handle->dev_path == NULL) {
        ESP_LOGE(TAG, "Camera device handle/path is NULL");
        return ESP_ERR_INVALID_STATE;
    }

    *dev_path = camera_handle->dev_path;
    *fd = open(camera_handle->dev_path, O_RDONLY);
    if (*fd < 0) {
        ESP_LOGE(TAG, "Failed to open camera device: %s", camera_handle->dev_path);
        return ESP_FAIL;
    }
    return ESP_OK;
}

const test_dev_camera_auto_summary_t *test_dev_camera_auto_get_last_summary(void)
{
    return &s_last_summary;
}

void test_dev_camera_auto_clear(void)
{
    memset(&s_last_summary, 0, sizeof(s_last_summary));
}

esp_err_t test_dev_camera_auto_capture(test_dev_camera_auto_summary_t *summary)
{
    const char *dev_path = NULL;
    camera_auto_mmap_buffer_t mapped_buffer = {0};
    struct v4l2_capability capability = {0};
    struct v4l2_format format = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
    };
    struct v4l2_requestbuffers req = {0};
    struct v4l2_buffer buf = {0};
    uint8_t *frame_data = NULL;
    uint32_t capability_flags = 0;
    int fd = -1;
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bool stream_on = false;
    esp_err_t ret = ESP_OK;

    test_dev_camera_auto_clear();

    ret = camera_auto_open_device(&dev_path, &fd);
    if (ret != ESP_OK) {
        return ret;
    }

    if (ioctl(fd, VIDIOC_QUERYCAP, &capability) != 0) {
        ESP_LOGE(TAG, "Failed to query camera capability: %s", dev_path);
        ret = ESP_FAIL;
        goto cleanup;
    }

    capability_flags = (capability.capabilities & V4L2_CAP_DEVICE_CAPS) ? capability.device_caps : capability.capabilities;
    if ((capability_flags & V4L2_CAP_VIDEO_CAPTURE) == 0 || (capability_flags & V4L2_CAP_STREAMING) == 0) {
        ESP_LOGE(TAG, "Camera device does not support capture streaming");
        ret = ESP_ERR_NOT_SUPPORTED;
        goto cleanup;
    }

    if (ioctl(fd, VIDIOC_G_FMT, &format) != 0) {
        ESP_LOGE(TAG, "Failed to get camera format");
        ret = ESP_FAIL;
        goto cleanup;
    }

    if (ioctl(fd, VIDIOC_S_FMT, &format) != 0) {
        ESP_LOGE(TAG, "Failed to set camera format");
        ret = ESP_FAIL;
        goto cleanup;
    }

    req.count = CAMERA_AUTO_BUFFER_COUNT;
    req.type = type;
    req.memory = CAMERA_AUTO_MEMORY_TYPE;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "Failed to request camera buffers");
        ret = ESP_FAIL;
        goto cleanup;
    }
    if (req.count < CAMERA_AUTO_BUFFER_COUNT) {
        ESP_LOGE(TAG, "Camera driver returned too few buffers: %u", (unsigned)req.count);
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    buf.type = type;
    buf.memory = CAMERA_AUTO_MEMORY_TYPE;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) != 0) {
        ESP_LOGE(TAG, "Failed to query camera buffer");
        ret = ESP_FAIL;
        goto cleanup;
    }

    mapped_buffer.length = buf.length;
    mapped_buffer.data = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (mapped_buffer.data == MAP_FAILED) {
        mapped_buffer.data = NULL;
        mapped_buffer.length = 0;
        ESP_LOGE(TAG, "Failed to mmap camera buffer");
        ret = ESP_FAIL;
        goto cleanup;
    }

    if (ioctl(fd, VIDIOC_QBUF, &buf) != 0) {
        ESP_LOGE(TAG, "Failed to queue camera buffer");
        ret = ESP_FAIL;
        goto cleanup;
    }

    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "Failed to start camera stream");
        ret = ESP_FAIL;
        goto cleanup;
    }
    stream_on = true;

    memset(&buf, 0, sizeof(buf));
    buf.type = type;
    buf.memory = CAMERA_AUTO_MEMORY_TYPE;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) != 0) {
        ESP_LOGE(TAG, "Failed to dequeue camera frame");
        ret = ESP_FAIL;
        goto cleanup;
    }
    if ((buf.flags & V4L2_BUF_FLAG_ERROR) != 0) {
        ESP_LOGE(TAG, "Camera frame marked with V4L2_BUF_FLAG_ERROR");
        ret = ESP_FAIL;
        goto cleanup;
    }
    if (buf.index >= CAMERA_AUTO_BUFFER_COUNT || mapped_buffer.data == NULL) {
        ESP_LOGE(TAG, "Invalid camera buffer index: %u", (unsigned)buf.index);
        ret = ESP_ERR_INVALID_RESPONSE;
        goto cleanup;
    }
    if ((size_t)buf.bytesused > mapped_buffer.length) {
        ESP_LOGE(TAG, "Captured frame exceeds mapped buffer: %u > %u",
                 (unsigned)buf.bytesused, (unsigned)mapped_buffer.length);
        ret = ESP_ERR_INVALID_SIZE;
        goto cleanup;
    }

    frame_data = (uint8_t *)mapped_buffer.data;
    s_last_summary.width = format.fmt.pix.width;
    s_last_summary.height = format.fmt.pix.height;
    s_last_summary.pixelformat = format.fmt.pix.pixelformat;
    s_last_summary.bytesused = buf.bytesused;
    s_last_summary.checksum = camera_auto_checksum(frame_data, buf.bytesused);
    s_last_summary.mean_luma = camera_auto_mean_luma(frame_data, buf.bytesused);

    if (summary != NULL) {
        *summary = s_last_summary;
    }

cleanup:
    if (stream_on && ioctl(fd, VIDIOC_STREAMOFF, &type) != 0) {
        ESP_LOGE(TAG, "Failed to stop camera stream");
        if (ret == ESP_OK) {
            ret = ESP_FAIL;
        }
    }
    if (mapped_buffer.data != NULL) {
        munmap(mapped_buffer.data, mapped_buffer.length);
    }
    if (fd >= 0) {
        close(fd);
    }
    if (ret != ESP_OK && summary != NULL) {
        memset(summary, 0, sizeof(*summary));
    }
    return ret;
}
