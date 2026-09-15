# CI coverage boards

Synthetic compile-coverage boards for ESP Board Manager. They are not hardware
bring-up configs. Pins are valid for the target SoC so generation and
`idf.py build` can succeed; they are not claimed to match a real module.
GitLab `build_test_apps` builds these in addition to the existing product boards.

Each board turns on every **peripheral wrapper** the SoC catalog marks as
supported, plus device wrappers that can compile without a real chip driver:

- GPIO / I2C / I2S / SPI / UART / ADC oneshot+continuous / LEDC / RMT / SDM
- DAC, MCPWM, PCNT, ANA_CMPR, LDO, DSI when the catalog allows them
- `audio_codec`, GPIO / ADC-single / ADC-multi buttons, `gpio_ctrl`, `ledc_ctrl`,
  `led_strip` (RMT, plus SPI when the SoC has SPI3), `power_ctrl` (GPIO),
  SPIFFS, LittleFS flash/SPI (and SDMMC when catalog-supported), `fs_fat`
  SPI (and SDMMC when catalog-supported), `knob` when pins allow, `custom`
- `display_lcd` SPI, `lcd_touch` I2C, and `gpio_expander`, using the local fake
  factories in `setup_device.c` instead of registry panel/touch/IO expander
  components (P4 coverage omits LCD; Function EV already compiles DSI LCD + LVGL)
- CSI `camera` on P4 (reuses the existing LDO peripheral); DSI bus peripheral
  stays for wrapper coverage even without a DSI LCD device

`setup_device.c` is copied into each board directory. It only provides linker
stubs (`lcd_panel_factory_entry_t`, `lcd_dsi_panel_factory_entry_t`,
`lcd_touch_factory_entry_t`, `io_expander_factory_entry_t`). Runtime init of
those devices is expected to return `ESP_ERR_NOT_SUPPORTED`.

Intentionally omitted (pin-heavy buses, unimplemented wrappers, or already
covered by a product board):

- `display_lcd` on P4 (SPI and DSI): Function EV already builds DSI LCD + LVGL;
  keeping LCD on the coverage board overflows internal RAM on IDF v5.4.4
- `display_lcd` RGB / RGB+3-wire SPI / I80 / PARLIO
- `lcd_touch` SPI (wrapper not built yet)
- DVP / SPI camera
- `led_strip` SPI on chips whose only GPSPI host is already used by `spi_master`, or that have no leftover GPIO after `knob`
- `power_ctrl` / `button` `custom` subtypes (need extra `DEVICE_EXTRA_FUNC_REGISTER` ops)

The C3 coverage board overrides `sdkconfig.defaults.esp32c3`'s 4 MB Lyra layout and uses the 8 MB `partitions_test_app.csv` so LCD/LVGL test sources still fit.

The generator intersects every catalog profile that contains the chip
(`idf_5_4.json` through `idf_6_2.json`, S31 from `idf_6_1.json` onward) so the
committed YAML stays valid when CI runs `idf.py bmgr` on v5.4.4 through latest.

Regenerate YAML and per-board `setup_device.c` after catalog or recipe changes:

```bash
python3 generate_coverage_boards.py
```
