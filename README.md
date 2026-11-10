<a href="https://github.com/espressif/esp-board-manager/blob/ecosystem-stats/README.md">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="https://raw.githubusercontent.com/espressif/esp-board-manager/ecosystem-stats/ecosystem-banner-en-dark.svg">
    <source media="(prefers-color-scheme: light)" srcset="https://raw.githubusercontent.com/espressif/esp-board-manager/ecosystem-stats/ecosystem-banner-en-light.svg">
    <img alt="ESP Board Manager: supported boards and projects using Board Manager" src="https://raw.githubusercontent.com/espressif/esp-board-manager/ecosystem-stats/ecosystem-banner-en-light.svg">
  </picture>
</a>

# ESP Board Manager

[中文版](README_CN.md)

ESP Board Manager is Espressif's board management component. It uses YAML files to describe the main controller, peripherals, and external devices, then automatically generates board configuration code to simplify board bring-up and application adaptation. Read more in [component's readme](esp_board_manager/README.md).

The component is available on [IDF Component Registry](https://components.espressif.com/components/espressif/esp_board_manager).

Board Manager Web Tool: [https://board-manager.espressif.com](https://board-manager.espressif.com)

Documentation for the latest version: [https://docs.espressif.com/projects/esp-board-manager](https://docs.espressif.com/projects/esp-board-manager)

## Supported Boards

ESP Board Manager supports development boards from different vendors through the following board-pack components:

| Vendor | Board Component | Resources |
| :---: | --- | --- |
| [<img src="docs/_static/logos/espressif.svg" alt="Espressif" width="120">](https://www.espressif.com/) | `espressif/esp_boards`<br>Official Espressif development boards | [GitHub](https://github.com/espressif/esp-board-manager/tree/main/esp_boards) · [Component Registry](https://components.espressif.com/components/espressif/esp_boards) |
| [<img src="docs/_static/logos/espressif.svg" alt="Espressif" width="120">](https://www.espressif.com/) | `espressif/esp_friends_boards`<br>Third-party development boards | [GitHub](https://github.com/espressif/esp-board-manager/tree/main/esp_friends_boards) · [Component Registry](https://components.espressif.com/components/espressif/esp_friends_boards) |
| [<img src="docs/_static/logos/m5stack.svg" alt="M5Stack" width="120">](https://m5stack.com/) | `espressif/m5stack_boards` | [GitHub](https://github.com/espressif/esp-board-manager/tree/main/m5stack_boards) · [Component Registry](https://components.espressif.com/components/espressif/m5stack_boards) |
| [<img src="docs/_static/logos/waveshare.png" alt="Waveshare" width="120">](https://www.waveshare.com/) | `waveshare/waveshare_boards` | [GitHub](https://github.com/waveshareteam/waveshare_boards) · [Component Registry](https://components.espressif.com/components/waveshare/waveshare_boards) |
| [<img src="docs/_static/logos/guition.webp" alt="Guition" width="120">](https://www.guition.com/model-selection) | `guition/guition_boards` | [GitHub](https://github.com/Guition-JC/Guition) · [Component Registry](https://components.espressif.com/components/guition/guition_boards) |

The `esp_board_manager` component depends on `espressif/esp_boards` by default, so users can list and select these official boards directly with `idf.py bmgr -l`. To use other board-pack components, add the corresponding components to your project dependencies.

To create your own board-pack component, refer to the [board-pack component template](https://github.com/LiuCodee/demo_boards).

## Community and Support

Scan the QR code below with WeChat to join the ESP Board Manager community’s official discussion and support group.

<img src="docs/_static/esp_board_manager_community_qr.png" alt="ESP Board Manager community WeChat QR code" width="240" height="240">
