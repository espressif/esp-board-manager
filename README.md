![ESP Board Manager](docs/_static/bmgr_banner.svg)

# ESP Board Manager

[中文版](README_CN.md)

ESP Board Manager is Espressif's board management component. It uses YAML files to describe the main controller, peripherals, and external devices, then automatically generates board configuration code to simplify board bring-up and application adaptation. Read more in [component's readme](esp_board_manager/README.md).

The component is available on [IDF Component Registry](https://components.espressif.com/components/espressif/esp_board_manager).

Board Manager Web Tool: [https://board-manager.espressif.com](https://board-manager.espressif.com)

Documentation for the latest version: [https://docs.espressif.com/projects/esp-board-manager](https://docs.espressif.com/projects/esp-board-manager)

## Supported Boards

- Official Espressif development boards: `espressif/esp_boards`
  - GitHub: [espressif/esp-board-manager/tree/main/esp_boards](https://github.com/espressif/esp-board-manager/tree/main/esp_boards)
  - ESP Component Registry: [espressif/esp_boards](https://components.espressif.com/components/espressif/esp_boards)

The `esp_board_manager` component depends on this component by default, so users can list and select these official boards directly with `idf.py bmgr -l`.

Other development boards are available through standalone board-pack components. Add the following components to your project dependencies as needed:

- `espressif/esp_friends_boards`
  - GitHub: [espressif/esp-board-manager/tree/main/esp_friends_boards](https://github.com/espressif/esp-board-manager/tree/main/esp_friends_boards)
  - ESP Component Registry: [espressif/esp_friends_boards](https://components.espressif.com/components/espressif/esp_friends_boards)

- `espressif/m5stack_boards`
  - GitHub: [espressif/esp-board-manager/tree/main/m5stack_boards](https://github.com/espressif/esp-board-manager/tree/main/m5stack_boards)
  - ESP Component Registry: [espressif/m5stack_boards](https://components.espressif.com/components/espressif/m5stack_boards)

- `waveshare/waveshare_boards`
  - GitHub: [waveshareteam/waveshare_boards](https://github.com/waveshareteam/waveshare_boards)
  - ESP Component Registry: [waveshare/waveshare_boards](https://components.espressif.com/components/waveshare/waveshare_boards/versions/0.1.2/readme)

- `guition-jc/guition_boards`
  - GitHub: [Guition-JC/Guition](https://github.com/Guition-JC/Guition)
  - ESP Component Registry: [guition-jc/guition_boards](https://components.espressif.com/components/guition-jc/guition_boards/versions/0.1.3/readme)
