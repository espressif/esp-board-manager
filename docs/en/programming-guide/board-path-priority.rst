Board Source Directories and Scan Paths
========================================

:link_to_translation:`zh_CN:[中文]`

Before generating code, ``idf.py bmgr`` aggregates the boards available to the current project from the following sources.

Automatic Scan Sources
----------------------

By default, BMGR scans in the following order. **For duplicate board names, the later source overrides the earlier one**:

1. **Components under ``managed_components/`` with names containing** ``boards``: Board collections downloaded via the component manager, for example ``espressif__esp_boards/``, with up to 3 levels of recursion.
2. **Project components/ directory** (``components/``): Board components maintained independently within the project, for example ``components/my_board/``, with up to 3 levels of recursion. Suitable for custom boards or modified copies of official boards. For duplicate names, boards under project ``components/`` take priority over boards under ``managed_components/``.
3. **Board dependencies with ``override_path:`` or ``path:`` overrides in ``main/idf_component.yml``**: Board dependencies pointed to local directories via the main component manifest, with up to 3 levels of recursion; commonly used for local debugging.
4. **Board root directory specified by** ``-c`` (``-c <path>``): An additional board collection root directory passed on the command line, with up to 3 levels of recursion. It is scanned last and has the highest priority for duplicate names.

.. note::

   Recursive N levels deep: starting from the given root directory, at most N levels of subdirectories are expanded; directories beyond that are not traversed.

Run ``idf.py bmgr -l`` to list all boards recognized in the current project along with their source labels.

.. note::

   The output order of ``idf.py bmgr -l`` depends on the board sources visible to the current project, dependency versions, and custom board paths. Board lists and indexes shown in documentation or examples are for reference only. Use the actual output from the current project when selecting a board. In CI scripts, use the board name directly instead of a list index.

Selecting a Board by Name: ``-b <name>``
-----------------------------------------

When a board name is passed to ``-b``, BMGR first aggregates the scan pool in the order described above, then searches it for a match:

.. code-block:: bash

   idf.py bmgr -b esp32_s3_korvo_2_3

If the same board name exists in multiple sources, scan order determines the override order (the later match takes effect). ``-b`` also accepts an index number corresponding to the order shown by ``idf.py bmgr -l``.

Specifying a Board by Path: ``-b <path>``
------------------------------------------

If the value of ``-b`` is a board directory that exists on disk (absolute path or path relative to the project root), BMGR uses that directory directly and overrides any existing entry with the same name in the scan pool; this takes priority over all automatic scan sources:

.. code-block:: bash

   idf.py bmgr -b /abs/path/to/my_board

Suitable for temporary verification, pointing to a repository path in CI, or debugging an external directory without copying the board into the project.

Combining ``-c`` and ``-b``
----------------------------

``-c <path>`` adds the specified directory to the scan pool (source 4). Typical combinations:

**Selecting a board by name from a custom directory**: If the target board exists only in the ``-c`` path, it can be selected by name from the expanded scan pool:

.. code-block:: bash

   idf.py bmgr -b my_custom_board -c /path/to/custom_boards

Version Notes
-------------

- **0.5.x**: Official boards are built into the BMGR component and are available without declaring additional dependencies.
- **0.6 and later**: Official boards are removed from the BMGR component and split into multiple independent board components:

  - ``espressif/esp_boards`` provides official Espressif development boards（BMGR 0.6 declares this dependency by default）.
  - ``espressif/esp_friends_boards`` provides partner and community development boards.
  - ``espressif/m5stack_boards`` provides M5Stack series development boards.

  Declare the required board component dependency in the project's main component manifest (``idf_component.yml``). After the component is downloaded, BMGR recognizes its boards automatically via source 1. For the component each board belongs to, see :doc:`/references/boards/index`.

Recommended Practices
----------------------

- **Using official boards**: Declare the corresponding board component dependency in the project's main component manifest, for example ``espressif/esp_boards``, ``espressif/esp_friends_boards``, or ``espressif/m5stack_boards``. See :doc:`/references/boards/index` for the component each board belongs to.
- **Custom boards**: Place in the project's ``components/<board_name>/``, or publish the board as a standalone board component and declare it as a project dependency. Running ``idf.py bmgr -x`` to clean generated code does not affect custom board directories.
- **Temporary debugging**: Use ``idf.py bmgr -b /abs/path/to/board``; no need to copy the board into the project.
- **Publishing a board component**: Publish the board directory as a standalone component for easy reference by other projects.
