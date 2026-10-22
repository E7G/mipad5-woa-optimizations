# GRAPHICS 单项优化（实验性）

仅针对已验证的 nabu ARM64 / Qualcomm GPU 27.20.2140.0：

```text
HKLM\SOFTWARE\QCOM\Drivers\DxKMD\GRAPHICS
ForceActive = REG_DWORD 0
```

**GRAPHICS 是实际引擎设置名。GRAPHICS_POWER_STATES 是电源组件名，不是配置节名。** 当前 ACPI、匹配二进制的解析路径及实际 ETW 均支持该区分。

不改全局 `DxKMD/ForceActive`、INTERNAL1、EXTERNAL3/4、VIDEO、ROTATOR、CRYPTO，不刷写 boot 或替换 ACPI。全局 ForceActive=0 的历史试验发生黑屏，已明确拒绝作为通用修复。

## 工具

`tools/Set-NabuGraphicsIdle.ps1` 默认 Inspect。Enable/Restore 需要管理员和显式确认实验性质。它是公开版安全封装，经过语法/静态检查；现场验证的是等价私有单值流程，**公开封装未再次上机应用**。

```powershell
# 只读
.\tools\Set-NabuGraphicsIdle.ps1
# 预演（不写注册表/备份）
.\tools\Set-NabuGraphicsIdle.ps1 -Mode Enable -AcknowledgeExperimental -WhatIf
# 显式启用，保存终端给出的备份路径
.\tools\Set-NabuGraphicsIdle.ps1 -Mode Enable -AcknowledgeExperimental
# 只撤销自己新增的 DWORD 0；不导入整个注册表 hive
.\tools\Set-NabuGraphicsIdle.ps1 -Mode Restore -BackupPath '<备份 JSON 路径>' -AcknowledgeExperimental
```

不会自动重启/熄屏。运行中驱动可能缓存配置；改注册表不证明即时生效/即时恢复。需要正常重启验证时，应另行安排 USB/FASTBOOT 恢复条件。原有值不符合预期时拒绝覆盖。更换 GPU 驱动后必须重新验证，不跳过哈希匹配强行使用。


## 2026-10-22 保护补强

显示配置查询有 `DISPLAY\<显示枚举名>` 层级。公共工具现在同时检查该层级、通用 DISPLAY 节及遗留裸路径；任何显示覆盖存在时拒绝 Enable，不会自动删除或改写它们。仅加强检查，不提供显示写入。详见 [只读排查补充](diagnostics-20261022.md)。
