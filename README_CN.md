![ESP Board Manager](docs/_static/bmgr_banner.svg)

# ESP Board Manager

[English](README.md)

ESP Board Manager 是乐鑫推出的开发板管理组件。它使用 YAML 文件描述主控、外设和外部功能器件，并自动生成开发板配置代码，从而简化开发板适配、初始化和应用移植流程。更多信息请参阅 [组件说明文档](esp_board_manager/README_CN.md)。

该组件已发布到 [IDF Component Registry](https://components.espressif.com/components/espressif/esp_board_manager)。

Board Manager 网页工具：[https://board-manager.espressif.com](https://board-manager.espressif.com)

最新版本的在线文档：[https://docs.espressif.com/projects/esp-board-manager/zh_CN](https://docs.espressif.com/projects/esp-board-manager/zh_CN)

## 支持的板级

- 乐鑫官方开发板 `espressif/esp_boards`
  - GitHub 链接：[espressif/esp-board-manager/tree/main/esp_boards](https://github.com/espressif/esp-board-manager/tree/main/esp_boards)
  - ESP Component Registry 链接：[espressif/esp_boards](https://components.espressif.com/components/espressif/esp_boards)

`esp_board_manager` 默认依赖该组件，因此用户正常添加并使用 ESP Board Manager 时，可以直接通过 `idf.py bmgr -l` 查看和选择这些官方开发板。

其他开发板也通过独立的板级组件提供支持。用户可以根据实际需求，将以下组件添加到工程依赖中：

- `espressif/esp_friends_boards`
  - GitHub 链接：[espressif/esp-board-manager/tree/main/esp_friends_boards](https://github.com/espressif/esp-board-manager/tree/main/esp_friends_boards)
  - ESP Component Registry 链接：[espressif/esp_friends_boards](https://components.espressif.com/components/espressif/esp_friends_boards)

- `espressif/m5stack_boards`
  - GitHub 链接：[espressif/esp-board-manager/tree/main/m5stack_boards](https://github.com/espressif/esp-board-manager/tree/main/m5stack_boards)
  - ESP Component Registry 链接：[espressif/m5stack_boards](https://components.espressif.com/components/espressif/m5stack_boards)

- `waveshare/waveshare_boards`
  - GitHub 链接：[waveshareteam/waveshare_boards](https://github.com/waveshareteam/waveshare_boards)
  - ESP Component Registry 链接：[waveshare/waveshare_boards](https://components.espressif.com/components/waveshare/waveshare_boards/versions/0.1.2/readme)

- `guition-jc/guition_boards`
  - GitHub 链接：[Guition-JC/Guition](https://github.com/Guition-JC/Guition)
  - ESP Component Registry 链接：[guition-jc/guition_boards](https://components.espressif.com/components/guition-jc/guition_boards/versions/0.1.3/readme)
