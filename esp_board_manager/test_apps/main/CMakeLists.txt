idf_component_register(
    SRCS
        # Main application
        "main.c"

        # Interactive test harness (console, registry, context, commands, resource
        # helpers, and per-group test-case tables)
        "framework/bmgr_test_console.c"
        "framework/bmgr_test_context.c"
        "framework/bmgr_test_registry.c"
        "commands/cmd_bmgr.c"
        "commands/cmd_case.c"
        "support/bmgr_test_json.c"
        "support/bmgr_test_options.c"
        "cases/bmgr_case_audio.c"
        "cases/bmgr_case_button.c"
        "cases/bmgr_case_camera.c"
        "cases/bmgr_case_custom.c"
        "cases/bmgr_case_fs.c"
        "cases/bmgr_case_gpio_expander.c"
        "cases/bmgr_case_lcd.c"
        "cases/bmgr_case_led.c"
        "cases/bmgr_case_periph.c"
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT}>:automation/test_dev_lcd_auto.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT}>:automation/test_dev_audio_auto.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT}>:automation/test_dev_camera_auto.c>

        # Peripheral tests
        "periph/test_periph_i2c.c"
        "periph/test_periph_gpio.c"
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_UART_SUPPORT}>:periph/test_periph_uart.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_ADC_SUPPORT}>:periph/test_periph_adc.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_RMT_SUPPORT}>:periph/test_periph_rmt.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_PCNT_SUPPORT}>:periph/test_periph_pcnt.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_DAC_SUPPORT}>:periph/test_periph_dac.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_SDM_SUPPORT}>:periph/test_periph_sdm.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_MCPWM_SUPPORT}>:periph/test_periph_mcpwm.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_PERIPH_ANACMPR_SUPPORT}>:periph/test_periph_anacmpr.c>

        # Device tests (conditionally included based on Kconfig)
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT}>:test_dev_audio_codec.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_FS_SPIFFS_SUPPORT}>:test_dev_fs_spiffs.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT}>:test_dev_fs_fat.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_LITTLEFS_SUPPORT}>:test_dev_littlefs.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT}>:test_dev_lcd_lvgl.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT}>:test_dev_lcd_init.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_GPIO_CTRL_SUPPORT}>:test_dev_pwr_ctrl.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_LEDC_CTRL_SUPPORT}>:test_dev_ledc.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_LED_STRIP_SUPPORT}>:test_dev_led_strip.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_CUSTOM_SUPPORT}>:test_dev_custom.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_GPIO_EXPANDER_SUPPORT}>:test_dev_gpio_expander.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT}>:test_dev_camera.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT}>:test_dev_camera_convert.c>
        $<$<AND:$<BOOL:${CONFIG_ESP_BOARD_DEV_CAMERA_SUPPORT}>,$<BOOL:${CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT}>>:test_dev_camera_lcd.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_BUTTON_SUPPORT}>:test_dev_button.c>

        # Audio board manager test (conditionally included based on SD card support)
        $<$<AND:$<BOOL:${CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT}>,$<BOOL:${CONFIG_ESP_BOARD_DEV_FS_FAT_SUPPORT}>>:test_board_mgr_audio_sdcard.c>
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT}>:test_board_mgr_audio_embed.c>

        # Utility files (conditionally included)
        $<$<BOOL:${CONFIG_ESP_BOARD_DEV_AUDIO_CODEC_SUPPORT}>:wav_header.c>
    INCLUDE_DIRS "." "./periph" "./framework" "./commands" "./cases" "./support" "./automation"
    REQUIRES esp_board_manager gen_bmgr_codes esp_lcd esp_timer esp_partition spiffs console
    EMBED_TXTFILES audio_files/test.wav audio_files/16k_16bit_1ch.wav
)

# Dynamically link LVGL components when LCD SPI support is enabled
idf_build_get_property(build_components BUILD_COMPONENTS)

# Check if lvgl component is available (using string matching)
foreach(comp ${build_components})
    if(comp MATCHES "lvgl")
        message(STATUS, "Found LVGL component: ${comp}")
        idf_component_get_property(lvgl_lib ${comp} COMPONENT_LIB)
        target_link_libraries(${COMPONENT_LIB} PRIVATE ${lvgl_lib})
        break()
    endif()
endforeach()

# Check if esp_lvgl_port component is available (using string matching)
foreach(comp ${build_components})
    if(comp MATCHES "esp_lvgl_port")
        idf_component_get_property(esp_lvgl_port_lib ${comp} COMPONENT_LIB)
        message(STATUS, "Found esp_lvgl_port component: ${esp_lvgl_port_lib}")
        target_link_libraries(${COMPONENT_LIB} PRIVATE ${esp_lvgl_port_lib})
        break()
    endif()
endforeach()


spiffs_create_partition_image(storage ../spiffs_image FLASH_IN_PROJECT)

if(CONFIG_ESP_BOARD_DEV_LITTLEFS_SUB_FLASH_SUPPORT)
    littlefs_create_partition_image(littlefs ../littlefs_image FLASH_IN_PROJECT)
endif()
