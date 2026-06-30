Create Board Guide
==================

:link_to_translation:`zh_CN:[中文]`

This chapter introduces several ways to create a new development board. Choose a starting point based on your situation:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Method
     - Use Case
   * - :doc:`Copy an Existing Board <copy-existing>`
     - The new board is very similar in hardware to an existing board; use it as a base and modify
   * - :doc:`Create Using the Web Configurator <web-create>`
     - Select devices and peripherals through a graphical interface in the browser and export board-level YAML
   * - :doc:`Use idf.py bmgr -n <generate-skeleton>`
     - Start from scratch; create an annotated board template based on your choices
   * - :doc:`Create Manually <manual-create>`
     - No suitable reference board to copy; copy configuration blocks from the device reference manual and modify according to the schematic
   * - :doc:`Use -a/--amend <amend>`
     - Modify configuration on an existing board, or add extra devices

- For field specifications of each device and peripheral, see :doc:`References </references/index>`

- For board directory structure, scan paths, and YAML annotation semantics, see the :doc:`Design Principles </programming-guide/index>` chapter

- For configuring board-level devices, consulting device reference documentation, or handling hardware not yet natively supported by BMGR, see :doc:`Board Device Adaption <device-adaption>`

.. toctree::
   :maxdepth: 1
   :hidden:

   copy-existing
   web-create
   generate-skeleton
   manual-create
   amend
   device-adaption
