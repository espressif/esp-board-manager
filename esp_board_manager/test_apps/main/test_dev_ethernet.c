/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/inet.h"
#include "ping/ping_sock.h"
#include "sdkconfig.h"
#include "dev_ethernet.h"
#include "esp_board_manager.h"
#include "bmgr_test_names.h"
#include "test_dev_ethernet.h"

static const char *TAG = "TEST_ETHERNET";

#define TEST_DEV_ETHERNET_LINK_TIMEOUT_MS  (20000)
#define TEST_DEV_ETHERNET_CONNECTED_BIT    (1U << 0)
#define TEST_DEV_ETHERNET_GOT_IP_BIT       (1U << 1)
#define TEST_DEV_ETHERNET_PING_BIT         (1U << 2)

static void eth_net_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                                  void *event_data)
{
    EventGroupHandle_t events = (EventGroupHandle_t)arg;
    if (events == NULL) {
        return;
    }
    if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Ethernet IP " IPSTR ", gateway " IPSTR,
                 IP2STR(&got_ip->ip_info.ip), IP2STR(&got_ip->ip_info.gw));
        xEventGroupSetBits(events, TEST_DEV_ETHERNET_GOT_IP_BIT);
    }
}

static void eth_ping_end(esp_ping_handle_t ping, void *arg)
{
    (void)ping;
    xEventGroupSetBits((EventGroupHandle_t)arg, TEST_DEV_ETHERNET_PING_BIT);
}

static void eth_link_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                                   void *event_data)
{
    (void)event_base;
    (void)event_data;
    EventGroupHandle_t events = (EventGroupHandle_t)arg;
    if (event_id == ETHERNET_EVENT_CONNECTED && events != NULL) {
        xEventGroupSetBits(events, TEST_DEV_ETHERNET_CONNECTED_BIT);
    }
}

esp_err_t test_dev_ethernet_handle(void)
{
    ESP_LOGI(TAG, "=== Ethernet handle test ===");

    dev_ethernet_handles_t *handles = NULL;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_ETHERNET, (void **)&handles);
    if (ret != ESP_OK || handles == NULL || handles->eth_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get ethernet device '%s': %s", BMGR_TEST_NAME_ETHERNET,
                 esp_err_to_name(ret));
        return ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
    }

    dev_ethernet_config_t *config = NULL;
    ret = esp_board_manager_get_device_config(BMGR_TEST_NAME_ETHERNET, (void **)&config);
    if (ret != ESP_OK || config == NULL) {
        ESP_LOGE(TAG, "Failed to get ethernet config '%s': %s", BMGR_TEST_NAME_ETHERNET,
                 esp_err_to_name(ret));
        return ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
    }

    ESP_LOGI(TAG, "chip=%s sub_type=%s",
             config->chip ? config->chip : "(null)",
             config->sub_type ? config->sub_type : "(null)");

    uint8_t mac[ETH_ADDR_LEN] = {0};
    ret = esp_eth_ioctl(handles->eth_handle, ETH_CMD_G_MAC_ADDR, mac);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get MAC address: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "MAC %02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return ESP_OK;
}

esp_err_t test_dev_ethernet_link(void)
{
    ESP_LOGI(TAG, "=== Ethernet link test ===");

    dev_ethernet_handles_t *handles = NULL;
    esp_netif_t *eth_netif = NULL;
    esp_eth_netif_glue_handle_t glue = NULL;
    EventGroupHandle_t events = NULL;
    bool started = false;
    bool handler_registered = false;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_ETHERNET, (void **)&handles);
    if (ret != ESP_OK || handles == NULL || handles->eth_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get ethernet device '%s': %s", BMGR_TEST_NAME_ETHERNET,
                 esp_err_to_name(ret));
        return ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
    }

    events = xEventGroupCreate();
    if (events == NULL) {
        ESP_LOGE(TAG, "Failed to create link event group");
        return ESP_ERR_NO_MEM;
    }

    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    ret = ESP_OK;

    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&cfg);
    if (eth_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create ethernet netif");
        ret = ESP_FAIL;
        goto cleanup;
    }

    glue = esp_eth_new_netif_glue(handles->eth_handle);
    if (glue == NULL) {
        ESP_LOGE(TAG, "Failed to create ethernet netif glue");
        ret = ESP_FAIL;
        goto cleanup;
    }

    ret = esp_netif_attach(eth_netif, glue);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to attach ethernet netif: %s", esp_err_to_name(ret));
        goto cleanup;
    }

    ret = esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_CONNECTED, eth_link_event_handler, events);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register ethernet event handler: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    handler_registered = true;

    ret = esp_eth_start(handles->eth_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start ethernet: %s", esp_err_to_name(ret));
        goto cleanup;
    }
    started = true;

    EventBits_t bits = xEventGroupWaitBits(events, TEST_DEV_ETHERNET_CONNECTED_BIT, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(TEST_DEV_ETHERNET_LINK_TIMEOUT_MS));
    if ((bits & TEST_DEV_ETHERNET_CONNECTED_BIT) == 0) {
        ESP_LOGE(TAG, "Timed out waiting for Ethernet link (%d ms)", TEST_DEV_ETHERNET_LINK_TIMEOUT_MS);
        ret = ESP_ERR_TIMEOUT;
        goto cleanup;
    }

    ESP_LOGI(TAG, "Ethernet link up");
    ret = ESP_OK;

cleanup:
    if (started) {
        esp_err_t stop_ret = esp_eth_stop(handles->eth_handle);
        if (stop_ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop ethernet: %s", esp_err_to_name(stop_ret));
            if (ret == ESP_OK) {
                ret = stop_ret;
            }
        }
    }
    if (handler_registered) {
        esp_event_handler_unregister(ETH_EVENT, ETHERNET_EVENT_CONNECTED, eth_link_event_handler);
    }
    if (glue != NULL) {
        esp_eth_del_netif_glue(glue);
    }
    if (eth_netif != NULL) {
        esp_netif_destroy(eth_netif);
    }
    if (events != NULL) {
        vEventGroupDelete(events);
    }
    return ret;
}

esp_err_t test_dev_ethernet_net(void)
{
    ESP_LOGI(TAG, "=== Ethernet network test ===");
    dev_ethernet_handles_t *handles = NULL;
    esp_netif_t *eth_netif = NULL;
    esp_eth_netif_glue_handle_t glue = NULL;
    EventGroupHandle_t events = NULL;
    esp_ping_handle_t ping = NULL;
    bool started = false;
    bool eth_handler_registered = false;
    bool ip_handler_registered = false;
    esp_err_t ret = esp_board_manager_get_device_handle(BMGR_TEST_NAME_ETHERNET, (void **)&handles);
    if (ret != ESP_OK || handles == NULL || handles->eth_handle == NULL) {
        return ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
    }
    events = xEventGroupCreate();
    if (events == NULL) {
        return ESP_ERR_NO_MEM;
    }
    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        goto cleanup;
    }
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        goto cleanup;
    }
    eth_netif = esp_netif_new(&(esp_netif_config_t)ESP_NETIF_DEFAULT_ETH());
    if (eth_netif == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    glue = esp_eth_new_netif_glue(handles->eth_handle);
    if (glue == NULL) {
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }
    ret = esp_netif_attach(eth_netif, glue);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_CONNECTED, eth_link_event_handler, events);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    eth_handler_registered = true;
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, eth_net_event_handler, events);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ip_handler_registered = true;
    ret = esp_eth_start(handles->eth_handle);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    started = true;
    EventBits_t bits = xEventGroupWaitBits(events, TEST_DEV_ETHERNET_GOT_IP_BIT, pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(TEST_DEV_ETHERNET_LINK_TIMEOUT_MS));
    if ((bits & TEST_DEV_ETHERNET_GOT_IP_BIT) == 0) {
        ESP_LOGE(TAG, "Timed out waiting for DHCP address (%d ms)", TEST_DEV_ETHERNET_LINK_TIMEOUT_MS);
        ret = ESP_ERR_TIMEOUT;
        goto cleanup;
    }
    esp_netif_ip_info_t ip_info;
    ret = esp_netif_get_ip_info(eth_netif, &ip_info);
    if (ret != ESP_OK || ip_info.gw.addr == 0) {
        ESP_LOGE(TAG, "Ethernet has no default gateway");
        ret = ret == ESP_OK ? ESP_ERR_INVALID_STATE : ret;
        goto cleanup;
    }
    esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
    ping_config.count = 3;
    ping_config.timeout_ms = 1000;
    ping_config.target_addr.type = IPADDR_TYPE_V4;
    ping_config.target_addr.u_addr.ip4.addr = ip_info.gw.addr;
    esp_ping_callbacks_t callbacks = {
        .cb_args = events,
        .on_ping_success = NULL,
        .on_ping_timeout = NULL,
        .on_ping_end = eth_ping_end,
    };
    ret = esp_ping_new_session(&ping_config, &callbacks, &ping);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    ret = esp_ping_start(ping);
    if (ret != ESP_OK) {
        goto cleanup;
    }
    bits = xEventGroupWaitBits(events, TEST_DEV_ETHERNET_PING_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000));
    if ((bits & TEST_DEV_ETHERNET_PING_BIT) == 0) {
        ESP_LOGE(TAG, "Timed out waiting for gateway ping");
        ret = ESP_ERR_TIMEOUT;
        goto cleanup;
    }
    uint32_t replies = 0;
    ret = esp_ping_get_profile(ping, ESP_PING_PROF_REPLY, &replies, sizeof(replies));
    if (ret == ESP_OK && replies == 0) {
        ESP_LOGE(TAG, "Gateway ping received no replies");
        ret = ESP_FAIL;
    }
cleanup:
    if (ping != NULL) {
        esp_ping_stop(ping);
        esp_ping_delete_session(ping);
        /* esp_ping_delete_session() only signals its task to exit. The ping
         * task polls that flag with a one-second timeout, so keep callback
         * state alive until it has definitely exited. */
        vTaskDelay(pdMS_TO_TICKS(1100));
    }
    if (started) {
        esp_err_t stop_ret = esp_eth_stop(handles->eth_handle);
        if (stop_ret != ESP_OK && ret == ESP_OK) {
            ret = stop_ret;
        }
    }
    if (ip_handler_registered) {
        esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, eth_net_event_handler);
    }
    if (eth_handler_registered) {
        esp_event_handler_unregister(ETH_EVENT, ETHERNET_EVENT_CONNECTED, eth_link_event_handler);
    }
    if (glue != NULL) {
        esp_eth_del_netif_glue(glue);
    }
    if (eth_netif != NULL) {
        esp_netif_destroy(eth_netif);
    }
    if (events != NULL) {
        vEventGroupDelete(events);
    }
    return ret;
}
