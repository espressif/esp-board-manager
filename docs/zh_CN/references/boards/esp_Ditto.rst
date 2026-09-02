ESP-Ditto
=========

基于 ESP32-C5 的多传感器交互开发板。本页面区分板载硬件与 BMGR 当前已配置的设备能力。

板载硬件
--------

* ILI9341 并口 LCD、CST816S 触摸、SPI 摄像头、音频输入输出
* BMI270、BMM150、SI12T 和 BQ27220 确实在板上，但当前尚未接入 BMGR 标准设备配置。

BMGR 已适配设备
---------------

* ILI9341 并口 LCD（parlio）
* CST816S 触摸（I2C）
* SPI 摄像头
* 音频输入和输出（内部 codec）

BMGR 配置
---------

板级配置目录为 ``esp_friends_boards/esp_Ditto``。当前仓库未找到该开发板对应的官方 RST 或 GitHub Markdown 硬件指南，以上信息根据组件 README 和板级配置整理。上面的 BMGR 设备列表与当前 ``board_devices.yaml`` 一致；其他板载传感器和电量计芯片仍需单独的设备适配器。
