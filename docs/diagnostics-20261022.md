# 2026-10-22 只读排查补充

## 状态，不是完整待机修复

- 私有单值流程已分别验证并保留 GRAPHICS、VIDEO、ROTATOR。VIDEO 播放/唤醒与 ROTATOR 横竖屏/唤醒检查通过；不能据此宣称所有硬件路径或长期稳定性均已验证。
- ROTATOR 本轮实际连续熄屏约 8.34 分钟，观测区间约 99.95% 为空闲 F2；GRAPHICS 约 99.91% 为空闲 F1。SW/HW DRIPS 仍为 0。
- 短期容量上报不变不能证明零耗电或量化省电；无匹配控制，不给出节电百分比。
- 公共写入封装仍只支持 GRAPHICS。不要从此记录自行推导批量关闭显示、MMU 或其他引擎的脚本。

## 显示设置的命名空间纠正

匹配 SHA256 `8C62AFDE3009041BF05BCC5213582F3914C4A0AA3FAB6EC242CCE61665470B45`
的 GPU 核心中，配置查询例程先构造 `DISPLAY`，再用反斜杠拼接显示枚举名称。

例程中的配置节示例：

```text
DISPLAY
DISPLAY\INTERNAL1
DISPLAY\EXTERNAL3
DISPLAY\EXTERNAL4
```

仅检查裸 `INTERNAL1` 不能覆盖这条查询路径。`Set-NabuGraphicsIdle.ps1` 已补强只读检查和写入前拒绝条件，同时继续拒绝遗留裸路径覆盖。此改动**不提供显示设置的写入入口**。

静态查询路径不等于证明某个显示覆盖值安全有效；本阶段没有对这些路径写值。之前全局 ForceActive=0 的黑屏结果仍然有效，不将本次发现解释成可直接开启完整显示断电。

## 依赖含义要区分

当前 ACPI 可见 GPU 对 MMU0/MMU1 等设备的 `_DEP`，MMU0 对 MMU1 的 `_DEP`。
这些设备依赖不能直接当作 GPU 每个 PoFx 组件的完整运行时依赖/活动引用图。

- [Microsoft：设备命名空间及设备依赖](https://learn.microsoft.com/en-us/windows-hardware/drivers/bringup/device-management-namespace-objects)
- [Microsoft：DXGK_POWER_RUNTIME_COMPONENT 的 ProviderCount/Providers](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmddi/ns-d3dkmddi-_dxgk_power_runtime_component)

现有熄屏记录仍显示主显示、外部显示逻辑组件和 MMU0 为 active/F0；这不是物理功率测量，也尚不能证明 MMU0 是独立故障。CRYPTO 声明为 HW_BLOCK_NONE，不因其逻辑 F0 就强制断电。

## 上游核对范围

2026-10-22 核对了当前使用的两个同名核心文件，分别与对应上游 Git blob 一致：

- [qcdxkm8150.sys](https://github.com/Project-Aloha/windows_oem_xiaomi_nabu/blob/master/Platform/Drivers/Graphics/DXKM/qcdxkm8150.sys)：`e8a0d3a195ae39b855e8280c0c0873c3016b07cd`。
- [qcsmmu8150.sys](https://github.com/WOA-Project/windows_silicon_qcom_hana/blob/main/Drivers/SOC/SMMU/qcsmmu8150.sys)：`b9e30db071107c4e4e74fafb8896ffba32ccb5d3`。

这不等于整个上游包所有变体都相同，也不等于其他 rs* 二进制适合本机；本轮未下载或替换新驱动。

只读保护测试：`powershell -NoProfile -File tools/Test-NabuGraphicsGuard.ps1`。
测试仅抽取保护函数并模拟读数，不执行公共写入封装、不打开测试机注册表。
