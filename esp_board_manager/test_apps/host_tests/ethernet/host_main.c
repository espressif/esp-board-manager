/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "esp_eth.h"
#include "esp_idf_version.h"
#if BMGR_TEST_YT8531
#include "esp_eth_phy_yt8531.h"
#endif  /* BMGR_TEST_YT8531 */
#include "dev_ethernet.h"
#include "esp_board_device.h"
#include "esp_board_entry.h"

static eth_esp32_emac_config_t captured;
static const dev_ethernet_config_t *expected_config;
static esp_eth_mac_t mac;
static esp_eth_phy_t phy;
static int mac_created, phy_created, installed, mac_deleted, phy_deleted, hook_called;
static int ioctl_count, host_phy_created;
static const uint32_t host_vendor_config = 0x12345678;
static bool uninstall_fail, hook_fail, phy_create_fail;
#if BMGR_TEST_YT8531
static bool last_phy_is_yt8531;
static int yt8531_created;
#endif  /* BMGR_TEST_YT8531 */

const char *esp_err_to_name(esp_err_t err)
{
    (void)err;
    return "host";
}
static esp_err_t mac_del(esp_eth_mac_t *value)
{
    assert(value == &mac);
    ++mac_deleted;
    return ESP_OK;
}
static esp_err_t phy_del(esp_eth_phy_t *value)
{
    assert(value == &phy);
    ++phy_deleted;
    return ESP_OK;
}

esp_eth_mac_t *esp_eth_mac_new_esp32(const eth_esp32_emac_config_t *config, const eth_mac_config_t *common)
{
    assert(expected_config != NULL);
    assert(common->sw_reset_timeout_ms == expected_config->mac_config.sw_reset_timeout_ms);
    assert(common->rx_task_stack_size == expected_config->mac_config.rx_task_stack_size);
    assert(common->rx_task_prio == expected_config->mac_config.rx_task_prio);
    assert(common->flags == expected_config->mac_config.flags);
    captured = *config;
    ++mac_created;
    mac.del = mac_del;
    return &mac;
}

esp_eth_phy_t *esp_eth_phy_new_generic(const eth_phy_config_t *config)
{
    assert(expected_config != NULL);
    assert(config->phy_addr == expected_config->phy_config.phy_addr);
    assert(config->reset_gpio_num == expected_config->phy_config.reset_gpio_num);
    assert(config->reset_timeout_ms == expected_config->phy_config.reset_timeout_ms);
    assert(config->autonego_timeout_ms == expected_config->phy_config.autonego_timeout_ms);
    assert(config->hw_reset_assert_time_us == expected_config->phy_config.hw_reset_assert_time_us);
    assert(config->post_hw_reset_delay_ms == expected_config->phy_config.post_hw_reset_delay_ms);
    if (phy_create_fail) {
        return NULL;
    }
#if BMGR_TEST_YT8531
    last_phy_is_yt8531 = false;
#endif  /* BMGR_TEST_YT8531 */
    ++phy_created;
    phy.del = phy_del;
    return &phy;
}

#if BMGR_TEST_YT8531
esp_eth_phy_t *esp_eth_phy_new_yt8531(const eth_phy_config_t *config)
{
    esp_eth_phy_t *result = esp_eth_phy_new_generic(config);
    if (result != NULL) {
        last_phy_is_yt8531 = true;
        ++yt8531_created;
    }
    return result;
}
#endif  /* BMGR_TEST_YT8531 */

esp_err_t esp_eth_driver_install(const esp_eth_config_t *config, esp_eth_handle_t *handle)
{
    assert(config->mac == &mac && config->phy == &phy);
    assert(config->check_link_period_ms == expected_config->check_link_period_ms);
#if BMGR_TEST_YT8531
    assert(last_phy_is_yt8531 == (strcmp(expected_config->chip, "yt8531") == 0));
#endif  /* BMGR_TEST_YT8531 */
    *handle = &installed;
    ++installed;
    return ESP_OK;
}

esp_err_t esp_eth_driver_uninstall(esp_eth_handle_t handle)
{
    assert(handle == &installed);
    if (uninstall_fail) {
        return ESP_FAIL;
    }
    --installed;
    return ESP_OK;
}

esp_err_t esp_eth_get_mac_instance(esp_eth_handle_t handle, esp_eth_mac_t **out)
{
    assert(handle == &installed);
    *out = &mac;
    return ESP_OK;
}

esp_err_t esp_eth_get_phy_instance(esp_eth_handle_t handle, esp_eth_phy_t **out)
{
    assert(handle == &installed);
    *out = &phy;
    return ESP_OK;
}

esp_err_t esp_eth_ioctl(esp_eth_handle_t handle, esp_eth_io_cmd_t cmd, void *data)
{
    assert(handle == &installed);
#if BMGR_TEST_YT8531
    assert((int)cmd == YT8531_ETH_CMD_S_RGMII_CLK_DELAY);
    const yt8531_rgmii_clk_delay_config_t *delay = data;
    const yt8531_rgmii_clk_delay_config_t *expected = expected_config->vendor_config;
    assert(delay != NULL && expected != NULL);
    assert(expected_config->vendor_config_size == sizeof(*expected));
    assert(delay->rxc_dly_en == expected->rxc_dly_en);
    assert(delay->rx_delay_sel == expected->rx_delay_sel);
    assert(delay->tx_delay_sel == expected->tx_delay_sel);
    assert(delay->tx_delay_sel_fe == expected->tx_delay_sel_fe);
#else
    (void)cmd;
    (void)data;
    assert(false);
#endif  /* BMGR_TEST_YT8531 */
    ++ioctl_count;
    return hook_fail ? ESP_FAIL : ESP_OK;
}

static esp_err_t chip_hook(esp_eth_handle_t handle, const dev_ethernet_config_t *config)
{
    assert(handle == &installed && config == expected_config);
    ++hook_called;
    return hook_fail ? ESP_FAIL : ESP_OK;
}

static esp_eth_phy_t *host_phy_create(const eth_phy_config_t *config)
{
    esp_eth_phy_t *result = esp_eth_phy_new_generic(config);
    if (result != NULL) {
        ++host_phy_created;
    }
    return result;
}

ESP_BOARD_ETHERNET_PHY_REGISTER(host_phy, host_phy_create, chip_hook);
ESP_BOARD_ETHERNET_PHY_REGISTER(host_no_post, host_phy_create, NULL);
ESP_BOARD_ETHERNET_PHY_REGISTER(host_post_only, NULL, chip_hook);

static void configure_common(dev_ethernet_config_t *config, bool custom)
{
    config->mac_config = (eth_mac_config_t)ETH_MAC_DEFAULT_CONFIG();
    config->phy_config = (eth_phy_config_t)ETH_PHY_DEFAULT_CONFIG();
    config->phy_config.phy_addr = 1;
    config->phy_config.reset_gpio_num = 7;
    config->check_link_period_ms = 2000;
    config->vendor_config = NULL;
    config->vendor_config_size = 0;
    if (custom) {
        config->mac_config.sw_reset_timeout_ms = 230;
        config->mac_config.rx_task_stack_size = 6144;
        config->mac_config.rx_task_prio = 9;
        config->mac_config.flags = ETH_MAC_FLAG_WORK_WITH_CACHE_DISABLE | ETH_MAC_FLAG_PIN_TO_CORE;
        config->phy_config.phy_addr = -1;
        config->phy_config.reset_gpio_num = -1;
        config->phy_config.reset_timeout_ms = 320;
        config->phy_config.autonego_timeout_ms = 5300;
        config->phy_config.hw_reset_assert_time_us = 23000;
        config->phy_config.post_hw_reset_delay_ms = -1;
        config->check_link_period_ms = 700;
    }
}

static void configure_emac(dev_ethernet_emac_config_t *config, eth_data_interface_t interface, bool custom)
{
    const eth_esp32_emac_config_t defaults = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    memset(config, 0, sizeof(*config));
    config->smi_gpio.mdc_num = 31;
    config->smi_gpio.mdio_num = 52;
    config->dma_burst_len = custom ? ETH_DMA_BURST_LEN_8 : defaults.dma_burst_len;
    config->intr_priority = custom ? 2 : defaults.intr_priority;
    config->mdc_freq_hz = custom ? 1000000 : defaults.mdc_freq_hz;
    if (interface == EMAC_DATA_INTERFACE_RMII) {
        config->clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
        config->clock_config.rmii.clock_gpio = 0;
#if SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX
        config->dataif_gpio.rmii = (eth_mac_rmii_gpio_config_t) {
            .tx_en_num = 49, .txd0_num = 34, .txd1_num = 35,
            .crs_dv_num = 28, .rxd0_num = 29, .rxd1_num = 30,
        };
#endif  /* SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX */
#if !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK
        config->clock_config_out_in.rmii.clock_mode = EMAC_CLK_EXT_IN;
        config->clock_config_out_in.rmii.clock_gpio = custom ? 50 : -1;
#endif  /* !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK */
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && SOC_EMAC_SUPPORT_1000M
    else {
        assert(interface == EMAC_DATA_INTERFACE_RGMII);
        config->clock_config.rgmii.clock_rx_gpio = 14;
        config->clock_config.rgmii.clock_tx_gpio = 13;
        config->clock_config.rgmii.clock_phy_ref_gpio = custom ? 20 : -1;
        config->dataif_gpio.rgmii = (eth_mac_rgmii_gpio_config_t) {
            .tx_ctl_num = 12,
            .txd0_num = 8,
            .txd1_num = 9,
            .txd2_num = 10,
            .txd3_num = 11,
            .rx_ctl_num = 15,
            .rxd0_num = 19,
            .rxd1_num = 18,
            .rxd2_num = 17,
            .rxd3_num = 16,
        };
    }
#endif  /* ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && SOC_EMAC_SUPPORT_1000M */
}

static void assert_emac_config(const dev_ethernet_emac_config_t *config, eth_data_interface_t interface)
{
    assert(captured.interface == interface);
    assert(captured.smi_gpio.mdc_num == config->smi_gpio.mdc_num);
    assert(captured.smi_gpio.mdio_num == config->smi_gpio.mdio_num);
    assert(captured.dma_burst_len == config->dma_burst_len);
    assert(captured.intr_priority == config->intr_priority);
    assert(captured.mdc_freq_hz == config->mdc_freq_hz);
    if (interface == EMAC_DATA_INTERFACE_RMII) {
        assert(captured.clock_config.rmii.clock_mode == config->clock_config.rmii.clock_mode);
        assert(captured.clock_config.rmii.clock_gpio == config->clock_config.rmii.clock_gpio);
#if SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX
        assert(captured.emac_dataif_gpio.rmii.tx_en_num == config->dataif_gpio.rmii.tx_en_num);
        assert(captured.emac_dataif_gpio.rmii.txd0_num == config->dataif_gpio.rmii.txd0_num);
        assert(captured.emac_dataif_gpio.rmii.txd1_num == config->dataif_gpio.rmii.txd1_num);
        assert(captured.emac_dataif_gpio.rmii.crs_dv_num == config->dataif_gpio.rmii.crs_dv_num);
        assert(captured.emac_dataif_gpio.rmii.rxd0_num == config->dataif_gpio.rmii.rxd0_num);
        assert(captured.emac_dataif_gpio.rmii.rxd1_num == config->dataif_gpio.rmii.rxd1_num);
#endif  /* SOC_EMAC_USE_MULTI_IO_MUX || SOC_EMAC_MII_USE_GPIO_MATRIX */
#if !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK
        assert(captured.clock_config_out_in.rmii.clock_mode == config->clock_config_out_in.rmii.clock_mode);
        assert(captured.clock_config_out_in.rmii.clock_gpio == config->clock_config_out_in.rmii.clock_gpio);
#endif  /* !SOC_EMAC_RMII_CLK_OUT_INTERNAL_LOOPBACK */
    }
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && SOC_EMAC_SUPPORT_1000M
    else {
        assert(captured.clock_config.rgmii.clock_rx_gpio == config->clock_config.rgmii.clock_rx_gpio);
        assert(captured.clock_config.rgmii.clock_tx_gpio == config->clock_config.rgmii.clock_tx_gpio);
        assert(captured.clock_config.rgmii.clock_phy_ref_gpio == config->clock_config.rgmii.clock_phy_ref_gpio);
        assert(captured.emac_dataif_gpio.rgmii.tx_ctl_num == config->dataif_gpio.rgmii.tx_ctl_num);
        assert(captured.emac_dataif_gpio.rgmii.txd0_num == config->dataif_gpio.rgmii.txd0_num);
        assert(captured.emac_dataif_gpio.rgmii.txd1_num == config->dataif_gpio.rgmii.txd1_num);
        assert(captured.emac_dataif_gpio.rgmii.txd2_num == config->dataif_gpio.rgmii.txd2_num);
        assert(captured.emac_dataif_gpio.rgmii.txd3_num == config->dataif_gpio.rgmii.txd3_num);
        assert(captured.emac_dataif_gpio.rgmii.rx_ctl_num == config->dataif_gpio.rgmii.rx_ctl_num);
        assert(captured.emac_dataif_gpio.rgmii.rxd0_num == config->dataif_gpio.rgmii.rxd0_num);
        assert(captured.emac_dataif_gpio.rgmii.rxd1_num == config->dataif_gpio.rgmii.rxd1_num);
        assert(captured.emac_dataif_gpio.rgmii.rxd2_num == config->dataif_gpio.rgmii.rxd2_num);
        assert(captured.emac_dataif_gpio.rgmii.rxd3_num == config->dataif_gpio.rgmii.rxd3_num);
    }
#endif  /* ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && SOC_EMAC_SUPPORT_1000M */
}

static void exercise_success(const esp_board_entry_desc_t *entry, dev_ethernet_config_t *config,
                             eth_data_interface_t interface, const dev_ethernet_emac_config_t *emac)
{
    assert(entry != NULL);
    expected_config = config;
    void *handle = NULL;
    assert(entry->init_func(config, sizeof(*config), &handle) == 0);
    assert(handle != NULL && installed == 1);
    assert_emac_config(emac, interface);
    assert(entry->deinit_func(handle) == 0 && installed == 0);
    assert(mac_created == mac_deleted && phy_created == phy_deleted);
}

static void exercise(const esp_board_entry_desc_t *entry, dev_ethernet_config_t *config,
                     eth_data_interface_t interface, const dev_ethernet_emac_config_t *emac)
{
    assert(entry != NULL);
    expected_config = config;
    void *handle = NULL;
    int before_mac = mac_created;
    assert(entry->init_func(NULL, sizeof(*config), &handle) == -1);
    assert(entry->init_func(config, sizeof(*config) - 1, &handle) == -1);
    assert(entry->init_func(config, sizeof(*config), NULL) == -1);
    const char *sub_type = config->sub_type;
    config->sub_type = interface == EMAC_DATA_INTERFACE_RMII ? ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RGMII : ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RMII;
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    config->sub_type = sub_type;
    assert(handle == NULL && mac_created == before_mac);
    exercise_success(entry, config, interface, emac);

    /* Failed chip initialization uses common rollback. */
    hook_fail = true;
    handle = NULL;
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    assert(handle == NULL && installed == 0);

    /* Failed rollback must retain a reachable driver wrapper. */
    uninstall_fail = true;
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    assert(handle != NULL && installed == 1);
    uninstall_fail = false;
    assert(entry->deinit_func(handle) == 0 && installed == 0);
    hook_fail = false;
    assert(mac_created == mac_deleted && phy_created == phy_deleted);

    /* A failed PHY constructor must release the newly created MAC. */
    phy_create_fail = true;
    handle = NULL;
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    assert(handle == NULL && installed == 0);
    phy_create_fail = false;
    assert(mac_created == mac_deleted && phy_created == phy_deleted);
}

static void exercise_vendor_config(const esp_board_entry_desc_t *entry, dev_ethernet_config_t *config,
                                   eth_data_interface_t interface, const dev_ethernet_emac_config_t *emac)
{
    void *handle = NULL;
    int before_host = host_phy_created;
    int before_hook = hook_called;
    config->vendor_config = &host_vendor_config;
    config->vendor_config_size = sizeof(host_vendor_config);
    exercise_success(entry, config, interface, emac);
    assert(host_phy_created == before_host + 1 && hook_called == before_hook + 1);

    /* A post-init-only adapter can retain the generic PHY constructor. */
    config->chip = "host_post_only";
    exercise_success(entry, config, interface, emac);
    assert(host_phy_created == before_host + 1 && hook_called == before_hook + 2);

    /* Unknown chips must not silently discard a vendor payload. */
    config->chip = "unknown_phy";
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    assert(handle == NULL && installed == 0);
    config->chip = "host_no_post";
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    assert(handle == NULL && installed == 0);

    config->chip = "host_phy";
    config->vendor_config_size = 0;
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    assert(handle == NULL && installed == 0);
    config->vendor_config = NULL;
    config->vendor_config_size = sizeof(host_vendor_config);
    assert(entry->init_func(config, sizeof(*config), &handle) == -1);
    assert(handle == NULL && installed == 0);

    /* A chip with no adapter and no payload keeps generic PHY support. */
    config->vendor_config_size = 0;
    config->chip = "unknown_phy";
    exercise_success(entry, config, interface, emac);
    config->chip = "host_no_post";
    exercise_success(entry, config, interface, emac);
    config->chip = "host_phy";
    assert(mac_created == mac_deleted && phy_created == phy_deleted);
}

int main(void)
{
#if CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT && CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT
    const esp_board_entry_desc_t *rmii = esp_board_entry_find_subtype_desc("ethernet", "emac_rmii");
    const esp_board_entry_desc_t *rgmii = esp_board_entry_find_subtype_desc("ethernet", "emac_rgmii");
    assert(rmii != NULL && rgmii != NULL);
    assert(rmii->init_func != rgmii->init_func);
#endif  /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT && CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT */
    dev_ethernet_config_t config = {
        .name = "ethernet",
        .chip = "host_phy",
    };
    configure_common(&config, false);
#if CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT
    config.sub_type = ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RMII;
    configure_emac(&config.sub_cfg.emac_rmii, EMAC_DATA_INTERFACE_RMII, false);
    const esp_board_entry_desc_t *rmii_entry = esp_board_entry_find_subtype_desc("ethernet", "emac_rmii");
    exercise(rmii_entry, &config, EMAC_DATA_INTERFACE_RMII, &config.sub_cfg.emac_rmii);
    configure_common(&config, true);
    configure_emac(&config.sub_cfg.emac_rmii, EMAC_DATA_INTERFACE_RMII, true);
    exercise(rmii_entry, &config, EMAC_DATA_INTERFACE_RMII, &config.sub_cfg.emac_rmii);
    exercise_vendor_config(rmii_entry, &config, EMAC_DATA_INTERFACE_RMII, &config.sub_cfg.emac_rmii);
#endif  /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RMII_SUPPORT */
#if CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT
    config.sub_type = ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RGMII;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && SOC_EMAC_SUPPORT_1000M
    configure_common(&config, false);
    configure_emac(&config.sub_cfg.emac_rgmii, EMAC_DATA_INTERFACE_RGMII, false);
    const esp_board_entry_desc_t *rgmii_entry = esp_board_entry_find_subtype_desc("ethernet", "emac_rgmii");
    exercise(rgmii_entry, &config, EMAC_DATA_INTERFACE_RGMII, &config.sub_cfg.emac_rgmii);
    configure_common(&config, true);
    configure_emac(&config.sub_cfg.emac_rgmii, EMAC_DATA_INTERFACE_RGMII, true);
    exercise(rgmii_entry, &config, EMAC_DATA_INTERFACE_RGMII, &config.sub_cfg.emac_rgmii);
    exercise_vendor_config(rgmii_entry, &config, EMAC_DATA_INTERFACE_RGMII, &config.sub_cfg.emac_rgmii);
#if BMGR_TEST_YT8531
    config.chip = "yt8531";
    int before_ioctl = ioctl_count;
    exercise_success(rgmii_entry, &config, EMAC_DATA_INTERFACE_RGMII, &config.sub_cfg.emac_rgmii);
    assert(ioctl_count == before_ioctl);
    yt8531_rgmii_clk_delay_config_t delay = {0};
    config.vendor_config = &delay;
    config.vendor_config_size = sizeof(delay);
    exercise_success(rgmii_entry, &config, EMAC_DATA_INTERFACE_RGMII, &config.sub_cfg.emac_rgmii);
    assert(ioctl_count == before_ioctl + 1);
    delay.rxc_dly_en = true;
    delay.rx_delay_sel = 7;
    delay.tx_delay_sel = 11;
    delay.tx_delay_sel_fe = 3;
    int before_yt8531 = yt8531_created;
    exercise(rgmii_entry, &config, EMAC_DATA_INTERFACE_RGMII, &config.sub_cfg.emac_rgmii);
    assert(yt8531_created == before_yt8531 + 3);
#else
    config.chip               = "yt8531";
    config.vendor_config      = &host_vendor_config;
    config.vendor_config_size = sizeof(host_vendor_config);
    void *missing_driver_handle = NULL;
    assert(rgmii_entry->init_func(&config, sizeof(config), &missing_driver_handle) == -1);
    assert(missing_driver_handle == NULL && installed == 0);
    assert(mac_created == mac_deleted && phy_created == phy_deleted);
#endif  /* BMGR_TEST_YT8531 */
    config.chip = "host_phy";
#else
    void *handle = NULL;
    int before = mac_created;
    const esp_board_entry_desc_t *entry = esp_board_entry_find_subtype_desc("ethernet", "emac_rgmii");
    assert(entry != NULL);
    assert(entry->init_func(&config, sizeof(config), &handle) == -1);
    assert(handle == NULL && mac_created == before);
#endif  /* ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(6, 1, 0) && SOC_EMAC_SUPPORT_1000M */
#endif  /* CONFIG_ESP_BOARD_DEV_ETHERNET_SUB_EMAC_RGMII_SUPPORT */
    /* The optional driver exposes typed ops with a validated native payload. */
    void *phy_ops = NULL;
#if BMGR_TEST_YT8531
    assert(esp_board_extra_func_get("ethernet_phy_yt8531", &phy_ops) == 0 && phy_ops != NULL);
    const dev_ethernet_phy_ops_t *yt8531 = phy_ops;
    assert(yt8531->create == esp_eth_phy_new_yt8531 && yt8531->post_init != NULL);
    expected_config = &config;
    int before_direct_ioctl = ioctl_count;
    yt8531_rgmii_clk_delay_config_t direct_delay = {0};
    config.vendor_config = NULL;
    config.vendor_config_size = 0;
    config.sub_type = ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RMII;
    assert(yt8531->post_init(&installed, &config) == ESP_OK && ioctl_count == before_direct_ioctl);
    config.vendor_config = &direct_delay;
    config.vendor_config_size = sizeof(direct_delay);
    assert(yt8531->post_init(&installed, &config) == ESP_ERR_INVALID_ARG);
    config.sub_type = ESP_BOARD_DEVICE_ETHERNET_SUB_TYPE_EMAC_RGMII;
    config.vendor_config_size = sizeof(direct_delay) - 1;
    assert(yt8531->post_init(&installed, &config) == ESP_ERR_INVALID_ARG);
    config.vendor_config_size = sizeof(direct_delay);
    assert(yt8531->post_init(&installed, &config) == ESP_OK && ioctl_count == before_direct_ioctl + 1);
    direct_delay.rx_delay_sel = 15;
    direct_delay.tx_delay_sel = 1;
    direct_delay.tx_delay_sel_fe = 8;
    assert(yt8531->post_init(&installed, &config) == ESP_OK && ioctl_count == before_direct_ioctl + 2);
    assert(yt8531->post_init(&installed, NULL) == ESP_ERR_INVALID_ARG);
#else
    assert(esp_board_extra_func_get("ethernet_phy_yt8531", &phy_ops) == -1);
    assert(ioctl_count == 0);
#endif  /* BMGR_TEST_YT8531 */
    assert(esp_board_extra_func_get("ethernet_phy_unknown_phy", &phy_ops) == -1);
    assert(esp_board_entry_find_subtype_desc("ethernet", "unknown") == NULL);
    assert(hook_called > 0);
    return 0;
}
