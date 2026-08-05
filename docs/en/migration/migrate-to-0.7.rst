Migrate to 0.7.0
================

:link_to_translation:`zh_CN:[中文]`

Version 0.7.0 migrates the ``audio_codec`` device to ``espressif/esp_codec_dev`` 2.0. Board YAML must use the codec-dev 2.0 initialization layout for new configurations.

Component Dependency
--------------------

``esp_board_manager`` requires ``espressif/esp_codec_dev`` ``^2.0.0-beta1`` when audio codec support is enabled. This constraint permits compatible ``2.x`` versions. The resolved version is recorded in ``dependencies.lock``.

This migration supports ESP-IDF release/v5.4 ``>=5.4.4`` and release/v5.5 ``>=5.5.3``.

YAML Migration
--------------

New codec initialization fields are grouped under ``config``:

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

``adc_cfg.label`` uses codec-dev 2.0 channel order from LSB to MSB. PA and reset GPIOs are not ``config`` groups. Reference them from ``peripherals`` with ``pa_name`` or ``reset_name`` and set their ``active_level`` in the same entry.

Legacy Field Compatibility
--------------------------

Existing board YAML remains parseable with warnings:

- ``mclk_enabled`` maps to the inverse ``sys_cfg.no_mclk`` value.
- ``adc_channel_labels`` is copied without reordering. Review its legacy MSB-to-LSB order before replacing it with ``adc_cfg.label``.
- ``adc_channel_mask``, ``dac_channel_mask``, ``adc_max_channel``, ``dac_max_channel``, ``adc_init_gain``, ``dac_init_gain``, ``aec``, ``eq``, and ``alc`` do not configure codec initialization.

Runtime Settings
----------------

Set channel masks and stream-specific settings through ``esp_codec_dev_open()`` sample information. Set input gain and output volume after opening the device through ``esp_codec_dev_set_in_gain()`` and ``esp_codec_dev_set_out_vol()``. AEC, EQ, and ALC require codec-specific runtime handling and are outside the ``audio_codec`` initialization scope.

Validation
----------

Regenerate board code with ``idf.py bmgr -b <board>`` after updating board YAML. Then run a full build and verify playback or recording on the target board.
