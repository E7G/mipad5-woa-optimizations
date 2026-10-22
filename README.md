# Mi Pad 5 WOA 优化与修复 / nabu

小米平板 5（nabu）Windows ARM64 的**实验性源码与可回滚配置**。不是完整驱动包，不是“深睡已修好”的固件。

## 已公开内容

|内容|状态|
|---|---|
|NT36523 触摸休眠补丁|本机 ARM64 构建、手动唤醒/触摸验证通过；SPI 熄屏内部重复回调消失|
|GRAPHICS 单引擎 `ForceActive=0`|本机同次启动下 GPU/MMU1 空闲 F1、绘制读回、约 104 分钟熄屏后唤醒通过；已选择保留|
|配置/回滚脚本|从私有操作流程抽取的公共工具；默认只读，写入需显式选择，不自动重启|
|主机测试与分析记录|仅脱敏摘要，不含原始转储、网络或设备身份信息|
|VIDEO 引擎|私有单项流程已验证播放画面/声音与唤醒，观测空闲 F2，已选择保留；公共写入工具未扩展到此项|
|ROTATOR 引擎|私有单项流程已验证横竖屏、通用绘制与约 8.34 分钟唤醒，观测空闲 F2，已选择保留；DRIPS 仍为 0|

**整机 SW/HW DRIPS 仍为 0%，未证明待机根因已全部解决。** 约 104 分钟熄屏估算掉电 2.7%（约 1.56%/小时），无同条件原配置对照，不能宣称节电幅度。此前全局 `ForceActive=0` 导致无法唤醒，已撤销，不提供该配置。

## 使用

- [触摸驱动变更、构建和固件要求](docs/touch.md)
- [GRAPHICS 单项设置与回滚](docs/graphics.md)
- [测试方法、结果与限制](docs/validation.md)
- [2026-10-22 三引擎现状与显示命名空间检查](docs/diagnostics-20261022.md)
- [来源、署名与再分发范围](THIRD_PARTY_NOTICES.md)

主机模拟测试（不需要平板，不安装驱动）：

```sh
python tools/test_touch.py
python tools/validate_public.py
```

需要 Python 3 与 GCC；默认使用测试程序生成的合成固件字节，不下载或分发真实固件。Windows 驱动构建需另备 Visual Studio、匹配的 ARM64 WDK/SDK 和合法取得的原始触摸固件。仓库不含签名私钥、证书、已签名驱动或一键刷写工具。

## 范围

已保留原有许可证及作者署名。摄像头、PEP/PMIC、Type-C、SMMU、Qualcomm GPU 二进制、Android boot、BCD、双系统界面、SSH/Wi-Fi 配置不随此仓库发布；已安装的第三方驱动不能当作本项目原创成果。

首批验证记录截至 **2026-10-18**，首次公开于 **2026-10-21**。**2026-10-22** 补充 VIDEO/ROTATOR 现场记录、显示命名空间只读核对与公共保护测试；完整待机修复仍未实现。

## English summary

Experimental, source-first nabu Windows ARM64 touch sleep fixes and an opt-in, reversible GRAPHICS-only registry setting. One device resumed successfully; GPU/MMU1 idle F1 was observed. **System DRIPS remains zero; complete standby repair and comparative battery savings are NOT demonstrated.** No proprietary driver/firmware binaries, certificates, private keys or raw diagnostic traces are included. See the linked documents before attempting a build or a configuration change.

The 2026-10-22 follow-up records private VIDEO/ROTATOR trials and corrects display namespace inspection. The public configuration wrapper remains GRAPHICS-only. No full-standby fix or battery-savings percentage is claimed.
