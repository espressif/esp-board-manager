#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/sdmmc_host.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "esp_board_periph.h"
#include "sdmmc_cmd.h"
#include "bmgr_test_names.h"

#define I2C_PROBE_TIMEOUT_MS  50
#define I2C_PROBE_FIRST_ADDR  0x03
#define I2C_PROBE_LAST_ADDR   0x77

void test_periph_i2c(void)
{
    /* Initialize I2C peripheral */
    esp_err_t ret = esp_board_periph_init(BMGR_TEST_NAME_I2C_MASTER);
    if (ret != ESP_OK) {
        printf("Failed to initialize I2C master peripheral\n");
        return;
    }

    /* Get I2C handle */
    void *i2c_handle = NULL;
    ret = esp_board_manager_get_periph_handle(BMGR_TEST_NAME_I2C_MASTER, &i2c_handle);
    if (ret != ESP_OK || !i2c_handle) {
        printf("Failed to get I2C master handle\n");
        goto cleanup;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x30 >> 1,
        .scl_speed_hz = 400000,
    };
    i2c_master_dev_handle_t dev_handle = NULL;
    ret = i2c_master_bus_add_device(i2c_handle, &dev_config, &dev_handle);
    if (ret != ESP_OK) {
        printf("Failed to add I2C device\n");
        return;
    }

    uint8_t write_buf[2] = {0, 0x97};
    i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), 300 / portTICK_PERIOD_MS);

    uint8_t reg_addr = 0x00;
    uint8_t data[1] = {0};
    i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, sizeof(data), 300 / portTICK_PERIOD_MS);
    printf("reg: %x, data: %x\n", reg_addr, data[0]);
    if (data[0] == 0x97) {
        printf("I2C master peripheral write read success\r\n");
    } else {
        printf("I2C master peripheral write read failed\r\n");
    }
    i2c_master_bus_rm_device(dev_handle);

    /* Show peripheral information */
    esp_board_periph_show(BMGR_TEST_NAME_I2C_MASTER);

cleanup:
    /* Cleanup */
    esp_board_periph_deinit(BMGR_TEST_NAME_I2C_MASTER);
    printf("I2C master peripheral deinitialized\n");
}

esp_err_t test_periph_i2c_probe(void)
{
    void *bus = NULL;
    esp_err_t ret = esp_board_manager_get_periph_handle(BMGR_TEST_NAME_I2C_MASTER, &bus);
    if (ret != ESP_OK || bus == NULL) {
        printf("Failed to get I2C master handle: %s\n", esp_err_to_name(ret));
        return ret != ESP_OK ? ret : ESP_ERR_INVALID_STATE;
    }

    size_t found_count = 0;
    printf("I2C probe scan (0x%02x..0x%02x), timeout=%d ms\n",
           I2C_PROBE_FIRST_ADDR, I2C_PROBE_LAST_ADDR, I2C_PROBE_TIMEOUT_MS);
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    for (uint16_t row = 0; row < 0x80; row += 0x10) {
        printf("%02x: ", (unsigned)row);
        for (uint16_t offset = 0; offset < 0x10; offset++) {
            uint16_t address = row + offset;
            if (address < I2C_PROBE_FIRST_ADDR || address > I2C_PROBE_LAST_ADDR) {
                printf("   ");
                continue;
            }

            ret = i2c_master_probe((i2c_master_bus_handle_t)bus, address, I2C_PROBE_TIMEOUT_MS);
            if (ret == ESP_OK) {
                printf("%02x ", (unsigned)address);
                found_count++;
            } else if (ret == ESP_ERR_TIMEOUT) {
                printf("UU ");
            } else {
                printf("-- ");
            }
        }
        printf("\n");
    }
    printf("I2C probe found %u device(s)\n", (unsigned)found_count);
    return ESP_OK;
}
