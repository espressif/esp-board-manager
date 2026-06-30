创建开发板指南
==================================

:link_to_translation:`en:[English]`

本章介绍新建一块开发板的几种方式。根据实际情况选择起点：

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - 方式
     - 适用场景
   * - :doc:`复制现有开发板 <copy-existing>`
     - 新开发板与已有开发板硬件高度相似，以其为基础修改
   * - :doc:`使用网页创建 <web-create>`
     - 在浏览器中用图形界面选择设备和外设，导出板级 YAML
   * - :doc:`使用 idf.py bmgr -n <generate-skeleton>`
     - 从零开始，根据您的选择创建带注释的开发板模板
   * - :doc:`手动创建 <manual-create>`
     - 无合适参考板可复制；从设备参考手册的配置示例复制块并按原理图修改
   * - :doc:`使用 -a/--amend <amend>`
     - 在已有开发板上修改配置，或添加额外的设备

- 各设备与外设的字段规范见 :doc:`参考手册 </references/index>`

- 开发板目录结构、扫描路径与 YAML 标记语义见 :doc:`设计原理 </programming-guide/index>` 章节

- 配置板级设备、查阅设备参考文档或处理 BMGR 尚未内置支持的硬件时，参见 :doc:`开发板设备适配 <device-adaption>`

.. toctree::
   :maxdepth: 1
   :hidden:

   copy-existing
   web-create
   generate-skeleton
   manual-create
   amend
   device-adaption
