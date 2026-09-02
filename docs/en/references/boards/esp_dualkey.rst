============
ESP-DualKey
============

:link_to_translation:`zh_CN:[中文]`

.. note::

  Please check the silkscreen version number on the main board to confirm your development board version.

This user guide will help you get started with ESP-DualKey and will also provide more in-depth information.

ESP-DualKey is an intelligent dual-key controller development board developed by Espressif based on the ESP32-S3 chip, integrating smart home control, Bluetooth keyboard, USB keyboard and other functions. It can switch between different working modes through physical switches. The development board is equipped with dual buttons, RGB lighting effects, battery management, power monitoring and other functions, providing developers with a complete intelligent button solution.

The ESP-DualKey main controller uses the Espressif ESP32-S3 chip, supporting 2.4 GHz Wi-Fi and Bluetooth 5 (LE) wireless connectivity. In terms of storage, the entire device has 8 MB flash storage space to meet the needs of intelligent button applications. It is equipped with a dual-button matrix and WS2812 RGB lighting effects, providing an intuitive and rich interactive experience.

In terms of power supply, ESP-DualKey supports USB-C power supply and built-in lithium battery power supply, integrating battery management chips and power monitoring functions. In addition, it also integrates a USB-C interface that supports power supply and programming download, while providing multiple working mode switching functions.

.. figure:: ../../../_static/boards/esp_dualkey/esp-dualkey-front.jpg
   :alt: ESP-DualKey Front View (Click to enlarge)
   :scale: 30%
   :figclass: align-center

   ESP-DualKey Front View (Click to enlarge)

The document consists of the following major sections:

- `Getting Started`_: Overview of the board and hardware/software setup instructions to get started.
- `Hardware Reference`_: More detailed information about the board's hardware.
- `Hardware Revision Details`_: Hardware revision history and known issues (if any) of the board.
- `Related Documents`_: Links to related documentation.
- `Disclaimer and Copyright Notice`_: Link to the disclaimer and copyright notice.

.. note::

  For instructions on using the factory firmware, please refer to `ESP-DualKey User Guide <https://espressif.craft.me/DiTh53ddFXD94L>`_.


Getting Started
===============

This section provides a brief introduction of ESP-DualKey, instructions on how to do the initial hardware setup and how to flash firmware onto it.


Feature List
------------

The main features of the board are listed below:

**Button Matrix Configuration:**

* 2×1 matrix configuration
* Supports independent button detection
* Supports combination button functions

**RGB LED Features:**

* WS2812 programmable RGB LED
* Supports multiple lighting effect modes
* Supports brightness, hue, and saturation adjustment

**Mode Switch:**

* **Left Position**: Bluetooth keyboard mode
* **Middle Position**: USB keyboard mode
* **Right Position**: Smart home mode (ESP RainMaker)


Description of Components
-------------------------

The key components of the board are described below. For user convenience, the components or interfaces are also labeled on the housing of ESP-DualKey.

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Key Component
     - Description
   * - :strong:`Main Controller`
     -
   * - ESP32-S3
     - Main controller chip supporting 2.4 GHz Wi-Fi and Bluetooth 5 (LE) connectivity, and featuring 8 MB of integrated flash storage.
   * - :strong:`Interaction`
     -
   * - Dual Buttons
     - 2×1 button matrix, supports independent key detection and combination key functions.
   * - Mode Switch
     - Three-position switch for toggling between operating modes (Bluetooth keyboard/Wi-Fi Mode/USB keyboard).
   * - :strong:`RGB Lighting`
     -
   * - WS2812 RGB LED
     - Programmable RGB LED supporting diverse lighting effects, e.g., heat map, breathing effect, flowing effect.
   * - :strong:`Power Management`
     -
   * - Lithium Battery Charging Chip
     - TP4057 lithium battery charging management, supports USB-C charging.
   * - :strong:`Interfaces`
     -
   * - USB-C Port
     - USB-C for power, programming/download, debugging, and battery charging.
   * - Two HY2.0-4P Ports
     - Support auxiliary power output for connecting external sensors.
   * - :strong:`Other Components`
     -
   * - Battery Voltage Monitoring
     - Real-time battery voltage and charging status monitoring.
   * - VBUS Monitoring
     - Monitors USB power status.
   * - Deep-sleep
     - Supports Deep-sleep mode for energy saving.


Application Examples
--------------------

The following application examples are available for ESP-DualKey:

- `ESP-DualKey Smart Button Controller Example <https://github.com/espressif/esp-dev-kits/tree/master/examples/esp-dualkey/examples/factory_demo>`__ - Demonstrates the use of the ESP-DualKey development board as a smart button controller with smart home control, Bluetooth keyboard, and USB keyboard functionalities, allowing for mode switching through physical switches.
- `test_apps <https://github.com/espressif/esp-dev-kits/tree/master/examples/esp-dualkey/components/esp_hid/test_apps>`__ - Provides unit tests for the esp_hid component on ESP32 and ESP32-C3 chips, allowing developers to validate the functionality of their code and add new test cases as needed.

For more examples and the latest updates, please refer to the `examples <https://github.com/espressif/esp-dev-kits/tree/master/examples/esp-dualkey>`__ folder.

To explore the application examples or to develop your own, please follow the steps outlined in the `Start Developing Applications`_ section.


Start Developing Applications
-----------------------------

Before powering up your board, please make sure that it is in good condition with no obvious signs of damage.


Required Hardware
^^^^^^^^^^^^^^^^^

- ESP-DualKey
- USB 2.0 cable (Standard-A to Type-C)
- Computer running Windows, Linux, or macOS

.. note::

  Please ensure you use an appropriate USB data cable. Some cables can only be used for charging and cannot be used for data transmission and programming.


Hardware Setup
^^^^^^^^^^^^^^

Connect ESP-DualKey with the computer using a USB data cable through the ``USB-C interface`` for firmware flashing, debugging and power supply.


Software Setup
^^^^^^^^^^^^^^

Please proceed to `ESP-IDF Get Started <https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/index.html>`__ to set up the development environment and flash an application example onto your board.

.. note::

  In most cases USB drivers required to operate the board are already included in Windows, Linux, and macOS operating systems. Some additional port access or security configuration may be required depending on your OS. In case of issues please check documentation on `how to establish serial connection <https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/establish-serial-connection.html>`__ with the board. The documentation also includes links to USB drivers applicable to boards produced by Espressif.

Espressif provides Board Support Packages (BSPs) for various Espressif boards that help you initialize and use key onboard peripherals, such as LCD displays, audio chips, buttons, and LEDs, more easily and efficiently. For a complete list of supported boards, please visit `esp-bsp <https://github.com/espressif/esp-bsp>`__.


Hardware Reference
==================


Block Diagram
-------------

The block diagram below shows the components of ESP-DualKey and their interconnections.

.. figure:: ../../../_static/boards/esp_dualkey/esp-dualkey-block-diagram.png
   :alt: ESP-DualKey Functional Block Diagram (Click to enlarge)
   :scale: 60%
   :figclass: align-center

   ESP-DualKey Functional Block Diagram (Click to enlarge)


Power Supply Options
^^^^^^^^^^^^^^^^^^^^

There are two mutually exclusive ways to provide power to the board:

1. Through ``USB-C interface`` power supply

   When using this method for power supply, use a USB Type-C data cable to connect to the USB-C interface on the device.

2. Through ``built-in lithium battery`` power supply

   The device has an integrated lithium battery inside, supporting independent power supply operation.


Mode Switch Circuit
^^^^^^^^^^^^^^^^^^^

The mode switch circuit diagram of the board is shown below.

.. figure:: ../../../_static/boards/esp_dualkey/esp-dualkey-switch-schematic.png
   :alt: Mode Switch Circuit Diagram (Click to enlarge)
   :width: 80%
   :figclass: align-center

   Mode Switch Circuit Diagram (Click to enlarge)


Hardware Revision Details
=========================

This is the first revision of this board released.


Related Documents
=================

-  `ESP-DualKey Schematic`_ (PDF)
-  `User Guide`_ (HTML)

.. _ESP-DualKey Schematic: https://dl.espressif.com/AE/esp-dev-kits/SCH-ESP-Dualkey-MainBoard-V1_2.pdf
.. _User Guide: https://espressif.craft.me/DiTh53ddFXD94L

Disclaimer and Copyright Notice
===============================

See `Disclaimer and Copyright Notice <https://docs.espressif.com/projects/esp-adf/en/latest/disclaimer-and-copyright.html>`__.

