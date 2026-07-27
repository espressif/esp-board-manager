#include <stdio.h>
#include "driver/gpio.h"
#include "esp_board_manager.h"
#include "esp_board_manager_defs.h"
#include "esp_board_periph.h"
#include "bmgr_test_names.h"

void test_periph_gpio(void)
{
    /* Initialize GPIO peripherals */
    esp_err_t ret = esp_board_periph_init(BMGR_TEST_NAME_GPIO_PA_CONTROL);
    if (ret != ESP_OK) {
        printf("Failed to initialize GPIO power amp\n");
        return;
    }
    /* Get GPIO handles */
    void *gpio_power_amp_handle = NULL;
    ret = esp_board_manager_get_periph_handle(BMGR_TEST_NAME_GPIO_PA_CONTROL, &gpio_power_amp_handle);
    if (ret != ESP_OK || !gpio_power_amp_handle) {
        printf("Failed to get GPIO power amp handle\n");
        goto cleanup_power_amp;
    }
    printf("GPIO power amp handle: %p\n", gpio_power_amp_handle);

    /* Show peripheral information */
    esp_board_periph_show(BMGR_TEST_NAME_GPIO_PA_CONTROL);

cleanup_power_amp:
    esp_board_periph_deinit(BMGR_TEST_NAME_GPIO_PA_CONTROL);
    printf("GPIO power amp deinitialized\n");
}
