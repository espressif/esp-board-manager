ESP Board Manager Guide
=======================

:link_to_translation:`zh_CN:[中文]`

.. image:: ../_static/bmgr_banner.svg
   :alt: ESP Board Manager
   :align: center
   :width: 100%

What Is Board Manager
---------------------

ESP Board Manager (BMGR) is a foundational component of Espressif's board development framework. It provides a standardized path from board hardware to application software on Espressif chips. BMGR represents each board as a self-contained, standard board component: a board adapted for BMGR runs directly in any BMGR project, and developers can share and reuse existing board configurations and project templates within the community.

BMGR is built around a single idea: decoupling the board from the application project. Board maintainers capture their hardware adaptation work as standalone components described by configuration files. Application developers only need to select a target board and call the unified API, without dealing with the underlying hardware differences. As board components accumulate in the community, new projects can skip repetitive board adaptation and focus directly on application logic.

Under the hood, BMGR describes hardware peripherals and functional devices in YAML configuration files. A code generator turns them into standardized initialization code with a single command and exposes a unified runtime API for device management.

Key benefits:

- **Adapt once, reuse across projects**: A board adapted for BMGR can be used directly in any BMGR project, with no need to rewrite board initialization logic each time.
- **Decoupled board and project**: Application code obtains device handles and configurations through a unified API, so switching the target board requires no changes to application logic.
- **No board boilerplate**: A YAML configuration plus the code generator produces standardized initialization code with a single command, replacing repetitive driver code.
- **Shareable board components**: Board configurations are published as standard components that can be shared and iterated on within the community, and new projects reference them directly.

Supported ESP-IDF Versions
~~~~~~~~~~~~~~~~~~~~~~~~~~

- ESP-IDF ``release/v5.4``, minimum ``v5.4.3``.
- ESP-IDF ``release/v5.5``, minimum ``v5.5.2``.
- ESP-IDF ``master``.

.. toctree::
   :maxdepth: 2
   :hidden:

   overview/index
   tools/index
   create-board/index
   references/index
   programming-guide/index
   migration/index
   faq
