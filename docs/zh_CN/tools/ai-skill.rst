AI Skill
========================================

:link_to_translation:`en:[English]`

仓库在 ``esp_board_manager/tools/AI_SKILLS/`` 下提供面向 AI 编程助手的任务说明（Skill）。每个 Skill 是一份固定步骤的流程描述，便于 AI 助手按统一规则理解 Board Manager 的配置语义，并按既定流程辅助板级适配、迁移与问题排查。

Skill 不是使用 Board Manager 的必需步骤，也不替代仓库内的命令、YAML 模板与参考页。建议先阅读相关文档以明确目标与验证方式，再将对应 Skill 提供给 AI 工具协助执行或复查。AI 输出涉及 IO、地址、时序等硬件相关内容时，仍需对照原理图进行人工确认。

当前提供的 Skill
----------------------

.. list-table::
   :header-rows: 1
   :widths: 25 55 20

   * - Skill
     - 适用场景
     - 入口
   * - ``lcd-touch-i2c-migration``
     - 将旧的 ``dev_lcd_touch_i2c`` 与 ``type: lcd_touch_i2c`` 迁移到通用 ``dev_lcd_touch``、``type: lcd_touch`` 加 ``sub_type: i2c``，涵盖 YAML 字段映射、``setup_device.c`` 中触摸工厂函数的更新以及兼容性检查。
     - ``tools/AI_SKILLS/lcd_touch_i2c_migration/SKILL.md``

迁移说明文档：:doc:`/migration/migrate-to-0.6`。

使用方式
--------------

将对应 Skill 文件作为输入提供给支持 Skill 协议的 AI 助手（例如 Claude Code、Cursor 等）。Skill 描述了任务的前置条件、判断分支、必须确认项以及不应自动改动的内容。AI 会按照 Skill 给出修改建议，最终改动仍需人工核对后再合入。
