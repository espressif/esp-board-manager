ESP-Ditto
=========

ESP32-C5 multi-sensor interaction board. This page separates the physical hardware from the devices currently configured by BMGR.

Onboard Hardware
----------------

* ILI9341 parallel LCD, CST816S touch, SPI camera, audio input/output
* BMI270, BMM150, SI12T, and BQ27220 are present on the board but are not currently integrated as BMGR standard devices.

BMGR-supported Devices
----------------------

* ILI9341 parallel LCD (parlio)
* CST816S touch (I2C)
* SPI camera
* Audio input and output (internal codec)

BMGR Configuration
------------------

The board definition is in ``esp_friends_boards/esp_Ditto``. No matching official RST or GitHub Markdown hardware guide was found in the checked sources; this page is based on the component README and board definition. The BMGR device list above reflects the current ``board_devices.yaml``; the additional onboard sensors and power monitor require separate device adapters.
