idf_component_register(
    SRC_DIRS "."
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_timer esp_system
)

if(CONFIG_USB_HS_CONSOLE_USB_CDC_AUTO_INIT)
    target_link_libraries(${COMPONENT_LIB} INTERFACE
        "-Wl,--wrap=app_main"
        "-u __wrap_app_main"
    )
endif()
