Board Reference
================

:link_to_translation:`zh_CN:[中文]`

**Starting from BMGR 0.6**: Boards are removed from the BMGR component and split into multiple independent board components:

.. list-table::
   :header-rows: 1
   :widths: 35 65

   * - Component
     - Description
   * - ``espressif/esp_boards``
     - Official Espressif development boards. BMGR 0.6 declares this dependency by default; it is automatically available when BMGR is introduced — no additional project configuration is needed. `View online <https://github.com/espressif/esp-board-manager/tree/main/esp_boards>`__
   * - ``espressif/esp_friends_boards``
     - Partner and community development boards. Requires manually declaring the dependency in the project's main component manifest (``idf_component.yml``). `View online <https://github.com/espressif/esp-board-manager/tree/main/esp_friends_boards>`__
   * - ``espressif/m5stack_boards``
     - M5Stack series development boards. Requires manually declaring the dependency in the project component manifest. `View online <https://github.com/espressif/esp-board-manager/tree/main/m5stack_boards>`__
