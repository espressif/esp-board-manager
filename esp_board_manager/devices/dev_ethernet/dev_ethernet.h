/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include "esp_eth.h"
#include "esp_eth_mac.h"
#include "esp_eth_mac_esp.h"
#include "esp_eth_phy.h"
#include "esp_idf_version.h"
#include "soc/soc_caps.h"
#include "esp_board_device.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RMII   "emac_rmii"   /*!< Internal EMAC over RMII */
#define ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RGMII  "emac_rgmii"  /*!< Internal EMAC over RGMII */

/**
 * @brief  Ethernet device handles.
 */
typedef struct {
    esp_eth_handle_t  eth_handle;  /*!< Native ESP-IDF Ethernet driver handle */
} dev_ethernet_handles_t;

#if CONFIG_ETH_USE_ESP32_EMAC
/**
 * @brief  Internal EMAC subtype configuration.
 *
 *         Pin and clock layout follow IDF `eth_esp32_emac_config_t`.
 *         `dataif_gpio` is only present on SoCs that mux the data plane
 *         through GPIO (`SOC_EMAC_USE_MULTI_IO_MUX` or
 *         `SOC_EMAC_MII_USE_GPIO_MATRIX`).
 */
typedef struct {
    emac_esp_smi_gpio_config_t  smi_gpio;       /*!< SMI MDC/MDIO GPIO numbers */
    eth_mac_clock_config_t      clock_config;   /*!< RMII or RGMII clock configuration */
    eth_mac_dma_burst_len_t     dma_burst_len;  /*!< DMA burst length for TX and RX */
    int                         intr_priority;  /*!< Interrupt priority; non-positive selects the IDF default */
    int32_t                     mdc_freq_hz;    /*!< MDC frequency limit; non-positive selects the IDF default */
#if SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX
    eth_mac_dataif_gpio_config_t  dataif_gpio;  /*!< MII/RMII/RGMII data-plane GPIOs */
#endif                                          /* SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX */
#if !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK
    eth_mac_clock_config_t  clock_config_out_in;  /*!< External RMII output-clock return input */
#endif                                            /* !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK */
} dev_ethernet_emac_config_t;
#endif  /* CONFIG_ETH_USE_ESP32_EMAC */

/**
 * @brief  Ethernet device configuration.
 *
 *         Vendor configuration remains owned by its provider and must stay
 *         valid until device deinitialization. Generated vendor data is static.
 */
typedef struct {
    const char       *name;                  /*!< Device name */
    const char       *chip;                  /*!< PHY chip name selecting its registered operations, or a generic PHY */
    const char       *sub_type;              /*!< EMAC data interface: emac_rmii or emac_rgmii */
    eth_mac_config_t  mac_config;            /*!< Common MAC creation parameters */
    eth_phy_config_t  phy_config;            /*!< PHY creation parameters, including reset timing */
    uint32_t          check_link_period_ms;  /*!< Link-status polling interval in milliseconds */
    const void       *vendor_config;         /*!< Chip-specific immutable configuration; NULL when absent */
    size_t            vendor_config_size;    /*!< Size of vendor_config in bytes, or zero when absent */
    union {
#if CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT && CONFIG_ETH_USE_ESP32_EMAC
        dev_ethernet_emac_config_t  emac_rmii;  /*!< RMII clock/GPIO union members */
#endif                                          /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT && CONFIG_ETH_USE_ESP32_EMAC */
#if CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT && CONFIG_ETH_USE_ESP32_EMAC &&  \
    ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && SOC_EMAC_SUPPORT_1000M
        dev_ethernet_emac_config_t  emac_rgmii;  /*!< RGMII clock/GPIO union members */
#endif                                           /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT && CONFIG_ETH_USE_ESP32_EMAC &&  \ */
    } sub_cfg;
} dev_ethernet_config_t;

/**
 * @brief  Create a PHY instance from common ESP-IDF configuration.
 *
 * @param[in]  config  Common PHY creation parameters.
 *
 * @return
 *       - Non-NULL  Created PHY instance, owned by the Ethernet device.
 *       - NULL      PHY creation failed.
 */
typedef esp_eth_phy_t *(*dev_ethernet_phy_create_t)(const eth_phy_config_t *config);

/**
 * @brief  Apply PHY-specific configuration after driver installation.
 *
 *         Runs before the application starts the Ethernet driver. The callback
 *         validates vendor_config_size before reading vendor_config. It must
 *         not retain the configuration pointer beyond device deinitialization.
 *
 * @param[in]  eth_handle  Installed Ethernet driver handle.
 * @param[in]  config      Effective device configuration.
 *
 * @return
 *       - ESP_OK  Configuration applied, or no vendor configuration supplied.
 *       - Other   PHY-specific failure; the device initialization is rolled back.
 */
typedef esp_err_t (*dev_ethernet_phy_post_init_t)(esp_eth_handle_t eth_handle, const dev_ethernet_config_t *config);

/**
 * @brief  Optional chip-specific PHY creation and configuration operations.
 */
typedef struct {
    dev_ethernet_phy_create_t     create;     /*!< Optional constructor; NULL uses the generic PHY */
    dev_ethernet_phy_post_init_t  post_init;  /*!< Required when vendor_config is supplied */
} dev_ethernet_phy_ops_t;

/**
 * @brief  Register immutable PHY operations under a chip identifier.
 *
 *         Place one registration per chip in a compiled source file. Chip names
 *         must be valid C identifiers and match the YAML chip string exactly.
 *         The registration uses the namespaced key ethernet_phy_<chip>.
 *
 * @param[in]  chip            Chip identifier token, for example yt8531.
 * @param[in]  create_func     Constructor with dev_ethernet_phy_create_t signature, or NULL.
 * @param[in]  post_init_func  Callback with dev_ethernet_phy_post_init_t signature, or NULL.
 */
#define ESP_BOARD_ETHERNET_PHY_REGISTER(chip, create_func, post_init_func)  static const dev_ethernet_phy_ops_t esp_board_ethernet_phy_ops_##chip = {  \
    .create    = (create_func),                                                                                                                        \
    .post_init = (post_init_func),                                                                                                                     \
    };                                                                                                                                                 \
    DEVICE_EXTRA_FUNC_REGISTER(ethernet_phy_##chip, &esp_board_ethernet_phy_ops_##chip)

/**
 * @brief  Initialize an ethernet device.
 *
 * @param[in]   cfg            Pointer to dev_ethernet_config_t.
 * @param[in]   cfg_size       Size of @p cfg.
 * @param[out]  device_handle  Pointer to receive dev_ethernet_handles_t.
 *
 * @return
 *       - 0   Success.
 *       - -1  Invalid arguments or ethernet initialization failure.
 */
int dev_ethernet_init(void *cfg, int cfg_size, void **device_handle);

/**
 * @brief  Deinitialize an ethernet device.
 *
 * @param[in]  device_handle  Pointer to dev_ethernet_handles_t.
 *
 * @return
 *       - 0   Success.
 *       - -1  Invalid arguments or ethernet deinitialization failure.
 */
int dev_ethernet_deinit(void *device_handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
