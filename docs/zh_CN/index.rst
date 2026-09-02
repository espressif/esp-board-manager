乐鑫板卡管理指南
==================================

:link_to_translation:`en:[English]`

.. image:: ../_static/bmgr_banner.svg
   :alt: ESP Board Manager
   :align: center
   :width: 100%

什么是 Board Manager
----------------------------------

ESP Board Manager（以下简称 BMGR）是乐鑫开发板生态的基础架构组件，致力于在乐鑫芯片平台上构建从开发板硬件到应用软件的标准化链路。BMGR 将每一块开发板定义为独立的标准开发板组件：基于 BMGR 完成适配的开发板，可以在任意 BMGR 工程中直接运行，开发者亦可在社区中共享和复用已有的板级配置与工程模板。

BMGR 的核心设计在于将开发板与应用工程解耦。板级维护者通过配置文件将硬件适配成果沉淀为独立组件；应用开发者只需引入目标开发板并调用统一 API，无需关心底层硬件差异；随着社区中开发板组件的持续积累，新工程可以免去大量重复的板级适配工作，直接聚焦业务逻辑。

在实现层面，BMGR 以 YAML 配置文件描述硬件外设与功能设备，代码生成器据此一键输出标准化的初始化代码，并向应用层提供统一的运行时 API 用于设备管理。

BMGR 的优势：

- **一次适配，跨工程复用**\ ：基于 BMGR 完成适配的开发板，可直接在任意 BMGR 工程中引入使用，无需为每个工程重写板级初始化逻辑。
- **开发板与工程解耦**\ ：应用工程通过统一 API 获取设备句柄与配置，切换目标开发板时不修改业务逻辑。
- **消除板级样板代码**\ ：YAML 配置文件结合代码生成器，一键输出标准化的初始化代码，替代逐行编写驱动的重复工作。
- **开发板组件的共享与复用**\ ：开发板配置以标准组件形式发布，可在社区中分享与迭代，新工程直接引用已有组件。

支持的 ESP-IDF 版本
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- ESP-IDF ``release/v5.4``，最低 ``v5.4.3``。
- ESP-IDF ``release/v5.5``，最低 ``v5.5.2``。
- ESP-IDF ``master``。

.. toctree::
   :maxdepth: 3
   :hidden:

   overview/index
   tools/index
   create-board/index
   references/boards/index
   references/index
   programming-guide/index
   migration/index
   faq
