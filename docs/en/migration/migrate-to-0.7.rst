Migrate to 0.7.1
================

:link_to_translation:`zh_CN:[中文]`

Version 0.7.1 includes the 0.7.0 migration of ``audio_codec`` to ``espressif/esp_codec_dev`` 2.0, and introduces semantic peripheral bindings and the ``display_lcd`` frame format. Regenerate code and validate the target board after updating board YAML.

Component Dependency
--------------------

``esp_board_manager`` requires ``espressif/esp_codec_dev`` ``^2.0.0-beta1`` when audio codec support is enabled. This constraint permits compatible ``2.x`` versions. The resolved version is recorded in ``dependencies.lock``.

``dev_audio_codec`` YAML Migration
----------------------------------

The ``audio_codec`` configuration template is:

.. code-block:: yaml

   config:
     adc_enabled: true
     dac_enabled: false
     sys_cfg:
       is_master: false
       no_mclk: true
     adc_cfg:
       digital_mic: false
       label: [FL, FR, RE, NA]
     dac_cfg:
       ref_enable: false
       ref_dac_ch: 0
       real_adc_data_ch: 0
   peripherals:
     - pa_name: gpio_power_amp
       gain: 0.0
       active_level: 1
     - reset_name: gpio_codec_reset
       active_level: 0
     - i2s_name: i2s_audio_out
       clk_src: 0
       tx_aux_out_io: -1
       tx_aux_out_line: 0
       tx_aux_out_invert: false
     - i2c_name: i2c_master
       address: 0x30
       frequency: 100000

``adc_cfg.label`` uses codec-dev 2.0 channel order from LSB to MSB. Reference PA and reset GPIOs from ``peripherals`` with ``pa_name`` or ``reset_name`` and set ``active_level`` in the same entry.

Legacy Field Compatibility
--------------------------

Existing board YAML remains parseable with warnings:

- ``mclk_enabled`` maps to the inverse ``sys_cfg.no_mclk`` value.
- ``adc_channel_labels`` is copied without reordering. Review its legacy MSB-to-LSB order before replacing it with ``adc_cfg.label``.
- ``adc_channel_mask``, ``dac_channel_mask``, ``adc_max_channel``, ``dac_max_channel``, ``adc_init_gain``, ``dac_init_gain``, ``aec``, ``eq``, and ``alc`` do not configure codec initialization.

Semantic Peripheral Bindings
----------------------------

Devices should reference peripherals with the ``*_name`` field for the required semantic role, instead of depending on a peripheral-name prefix or list ordering. For example, migrate an SPI LCD binding from a mapping containing only ``name`` to ``spi_name``:

.. code-block:: yaml

   # Legacy form
   devices:
     - name: display_lcd
       type: display_lcd
       sub_type: spi
       peripherals:
         - name: spi_lcd

   # Version 0.7.1 form
   devices:
     - name: display_lcd
       type: display_lcd
       sub_type: spi
       peripherals:
         - spi_name: spi_lcd

The selector is determined by the device type and sub-type. Common selectors include ``i2c_name``, ``spi_name``, ``gpio_name``, ``pa_name``, ``reset_name``, ``dsi_name``, and ``ldo_name``. Each peripheral reference entry can contain only one role-specific selector, and the referenced peripheral type must match that role.

Affected devices should replace their former string references or ``- name: <peripheral>`` mappings with the selectors shown in the following table.

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - Device type
     - Before: ``peripherals`` entries
     - After: ``peripherals`` entries
   * - ``audio_codec``
     - ``name: gpio_power_amp`` + ``pa_active_level: 1``; ``name: gpio_codec_reset``; ``name: i2s_audio_out``; ``name: i2c_master``; ``name: adc_audio_in``
     - ``pa_name: gpio_power_amp`` + ``active_level: 1``; ``reset_name: gpio_codec_reset``; ``i2s_name: i2s_audio_out``; ``i2c_name: i2c_master``; ``adc_name: adc_audio_in``
   * - ``button``
     - ``sub_type: gpio``: ``name: gpio_button``; ``sub_type: adc_single`` / ``adc_multi``: ``name: adc_button``
     - ``sub_type: gpio``: ``gpio_name: gpio_button``; ``sub_type: adc_single`` / ``adc_multi``: ``adc_name: adc_button``
   * - ``camera``
     - ``sub_type: dvp`` / ``spi``: ``name: i2c_master``; ``sub_type: csi``: ``name: i2c_master`` + ``name: ldo_mipi``
     - ``sub_type: dvp`` / ``spi``: ``i2c_name: i2c_master``; ``sub_type: csi``: ``i2c_name: i2c_master`` + ``ldo_name: ldo_mipi``
   * - ``display_lcd``
     - ``sub_type: spi``: ``name: spi_display``; ``sub_type: dsi``: ``name: dsi_display`` + ``name: ldo_mipi``
     - ``sub_type: spi``: ``spi_name: spi_display``; ``sub_type: dsi``: ``dsi_name: dsi_display`` + ``ldo_name: ldo_mipi``
   * - ``lcd_touch``
     - ``name: i2c_master``
     - ``i2c_name: i2c_master``
   * - ``gpio_expander``
     - ``name: i2c_master``
     - ``i2c_name: i2c_master``
   * - ``gpio_ctrl``
     - ``name: gpio``
     - ``gpio_name: gpio``
   * - ``power_ctrl``
     - ``sub_type: gpio``: ``name: gpio_power``; ``sub_type: custom``: ``name: i2c_pmic``
     - ``sub_type: gpio``: ``gpio_name: gpio_power``; ``sub_type: custom``: retain ``name: i2c_pmic``
   * - ``ledc_ctrl``
     - ``name: ledc_backlight``
     - ``ledc_name: ledc_backlight``
   * - ``fs_fat``
     - ``sub_type: spi``: ``name: spi_master``
     - ``sub_type: spi``: ``spi_name: spi_master``
   * - ``littlefs``
     - ``sub_type: spi``: ``name: spi_master``
     - ``sub_type: spi``: ``spi_name: spi_master``

The ``peripherals`` of ``power_ctrl: custom`` are general references that the framework acquires before custom initialization. They do not use a role-specific selector and retain the ``name`` form.

In 0.7.0, PA references used ``name`` to identify the peripheral and ``pa_active_level`` to configure its active level. In 0.7.1, use ``pa_name`` for the PA binding and ``active_level`` for the active level; codec reset GPIO references use ``active_level`` as well.

Legacy string references and mappings that contain only ``name`` remain parseable, but BMGR infers their roles from device context and emits a compatibility warning. An unknown selector, duplicate role, combined ``name`` and ``*_name`` reference, or incompatible peripheral type is rejected during parsing. See :doc:`/programming-guide/yaml-rules` for the complete rules.

LCD Frame Format
----------------

``display_lcd`` now generates ``frame_format`` in its device configuration. Applications should use this field to select pixel buffers, LVGL byte swapping, or image-conversion output instead of relying on panel-driver byte-order assumptions.

When automatic derivation succeeds, ``frame_format`` is ``RGB565_LE``, ``RGB565_BE``, ``BGR888``, or ``RGB888``. An insufficient or unsupported layout generates ``UNKNOWN``. SPI and I80 derive the value from ``data_endian``; DSI and RGB derive it from a pixel or color format. For PARLIO configurations, whose format cannot be derived automatically, set ``config.frame_format`` explicitly:

.. code-block:: yaml

   config:
     frame_format: RGB565_LE

The explicit value must be ``RGB565_LE``, ``RGB565_BE``, ``BGR888``, or ``RGB888``. When it differs from the automatically derived value, BMGR emits a warning and uses the explicit value. See :doc:`/references/devices/display-lcd` for the complete derivation rules.

Runtime Settings
----------------

Set channel masks and stream-specific settings through ``esp_codec_dev_open()`` sample information. Set input gain and output volume after opening the device through ``esp_codec_dev_set_in_gain()`` and ``esp_codec_dev_set_out_vol()``. AEC, EQ, and ALC require codec-specific runtime handling and are outside the ``audio_codec`` initialization scope.

Validation
----------

Regenerate board code with ``idf.py bmgr -b <board>`` after updating board YAML. Then run a full build and verify playback, recording, or display operation on the target board.
