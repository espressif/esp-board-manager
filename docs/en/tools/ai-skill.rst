AI Skill
========

:link_to_translation:`zh_CN:[中文]`

The repository provides task descriptions (Skills) for AI coding assistants under ``esp_board_manager/tools/AI_SKILLS/``. Each Skill is a fixed-step procedure description that allows AI assistants to understand Board Manager configuration semantics according to a consistent set of rules, and to assist with board adaptation, migration, and troubleshooting following an established workflow.

Skills are not a required step for using Board Manager, and they do not replace the commands, YAML templates, or reference pages in the repository. It is recommended to first read the relevant documentation to clarify your goal and how to verify it, then provide the corresponding Skill to an AI tool to assist with execution or review. AI output involving hardware-related content such as IO numbers, addresses, and timing must still be manually verified against the schematic.

Available Skills
----------------

.. list-table::
   :header-rows: 1
   :widths: 25 55 20

   * - Skill
     - Applicable Scenario
     - Entry Point
   * - ``lcd-touch-i2c-migration``
     - Migrate the legacy ``dev_lcd_touch_i2c`` and ``type: lcd_touch_i2c`` to the generic ``dev_lcd_touch`` with ``type: lcd_touch`` and ``sub_type: i2c``, covering YAML field mapping, updates to the touch factory function in ``setup_device.c``, and compatibility checks.
     - ``tools/AI_SKILLS/lcd_touch_i2c_migration/SKILL.md``

Migration guide document: :doc:`/migration/migrate-to-0.6`.

How to Use
----------

Provide the corresponding Skill file as input to an AI assistant that supports the Skill protocol (for example, Claude Code, Cursor, etc.). The Skill describes the task prerequisites, decision branches, items that must be confirmed, and content that must not be changed automatically. The AI will provide modification suggestions according to the Skill; all final changes still require human review before merging.
