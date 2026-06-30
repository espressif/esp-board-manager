开发板参考
============

:link_to_translation:`en:[English]`

**自 BMGR 0.6 起**：开发板从 BMGR 组件内移除，拆分为多个独立板卡组件，具体如下：

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - 组件
     - 说明
   * - ``espressif/esp_boards``
     - 乐鑫官方开发板。BMGR 0.6 默认声明此依赖，引入 BMGR 即自动可用，无需工程额外配置。`在线查阅 <https://github.com/espressif/esp-board-manager/tree/main/esp_boards>`__
   * - ``espressif/esp_friends_boards``
     - 合作伙伴与社区开发板。需在工程主组件清单（``idf_component.yml``）中手动声明依赖。`在线查阅 <https://github.com/espressif/esp-board-manager/tree/main/esp_friends_boards>`__
   * - ``espressif/m5stack_boards``
     - M5Stack 系列开发板。需在工程主组件清单中手动声明依赖。`在线查阅 <https://github.com/espressif/esp-board-manager/tree/main/m5stack_boards>`__
