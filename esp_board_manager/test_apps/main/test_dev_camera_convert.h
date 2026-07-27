/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_imgfx_types.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Render a V4L2 FourCC as a human-readable 4-character string.
 *
 * @param[in]   fourcc  V4L2 pixel format FourCC value
 * @param[out]  text    Buffer of at least 5 bytes to receive the NUL-terminated string
 *
 * @return
 *       - `text`  for convenient use in printf-style calls
 */
const char *camera_convert_fourcc_to_str(uint32_t fourcc, char text[5]);

/**
 * @brief  Map a V4L2 pixel format to the equivalent ESP-IMGFX pixel format.
 *
 * @param[in]   v4l2_fmt  V4L2 pixel format FourCC
 * @param[out]  out_fmt   Resulting ESP-IMGFX pixel format
 *
 * @return
 *       - true   `v4l2_fmt` has a known ESP-IMGFX equivalent, `*out_fmt` is valid
 *       - false  `v4l2_fmt` is not supported by ESP-IMGFX color conversion
 */
bool camera_convert_v4l2_to_imgfx_fmt(uint32_t v4l2_fmt, esp_imgfx_pixel_fmt_t *out_fmt);

/**
 * @brief  Get the tightly-packed (no row padding) stride for a V4L2 pixel format.
 *
 * @param[in]   v4l2_fmt  V4L2 pixel format FourCC
 * @param[in]   width     Frame width in pixels
 * @param[out]  stride    Resulting stride in bytes
 *
 * @return
 *       - true   `v4l2_fmt` is a supported packed format, `*stride` is valid
 *       - false  `v4l2_fmt` is not supported
 */
bool camera_convert_get_tight_stride(uint32_t v4l2_fmt, uint32_t width, uint32_t *stride);

/**
 * @brief  Copy a padded frame buffer row-by-row into a tightly-packed buffer.
 *
 *         V4L2 drivers may report a `bytesperline` larger than the tight
 *         (width * bytes-per-pixel) stride; ESP-IMGFX color conversion requires
 *         tightly-packed input, so padded rows must be repacked first.
 *
 * @param[out]  dst           Destination buffer, at least `tight_stride * height` bytes
 * @param[in]   src           Source buffer
 * @param[in]   src_len       Size of `src` in bytes
 * @param[in]   src_stride    Source row stride in bytes (`bytesperline`)
 * @param[in]   tight_stride  Destination row stride in bytes
 * @param[in]   height        Number of rows to copy
 *
 * @return
 *       - ESP_OK                On success
 *       - ESP_ERR_INVALID_ARG   Invalid arguments
 *       - ESP_ERR_INVALID_SIZE  `src_len` is too small for `src_stride * height`
 */
esp_err_t camera_convert_pack_rows(uint8_t *dst, const uint8_t *src, size_t src_len,
                                   uint32_t src_stride, uint32_t tight_stride, uint32_t height);

/**
 * @brief  One-shot conversion of a single camera frame to tightly-packed RGB565 (little-endian).
 *
 *         Wraps `esp_imgfx_color_convert` open/process/close for callers that only need to
 *         convert a single frame (e.g. before JPEG encoding), as opposed to a long-running
 *         preview pipeline that keeps the conversion handle open across frames.
 *
 * @param[in]   v4l2_fmt      Source V4L2 pixel format FourCC
 * @param[in]   width         Frame width in pixels
 * @param[in]   height        Frame height in pixels
 * @param[in]   bytesperline  Source row stride in bytes as reported by the driver (0 if tightly packed)
 * @param[in]   src           Source frame buffer
 * @param[in]   src_len       Size of `src` in bytes
 * @param[out]  out_buf       Newly allocated output buffer (caller must `heap_caps_free()`)
 * @param[out]  out_len       Size of `*out_buf` in bytes
 *
 * @return
 *       - ESP_OK                 On success
 *       - ESP_ERR_INVALID_ARG    Invalid arguments
 *       - ESP_ERR_NOT_SUPPORTED  `v4l2_fmt` has no ESP-IMGFX equivalent
 *       - ESP_ERR_NO_MEM         Failed to allocate the output (or intermediate packing) buffer
 *       - ESP_FAIL               Color conversion failed
 */
esp_err_t camera_convert_to_rgb565(uint32_t v4l2_fmt, uint32_t width, uint32_t height, uint32_t bytesperline,
                                   const uint8_t *src, size_t src_len,
                                   uint8_t **out_buf, size_t *out_len);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
