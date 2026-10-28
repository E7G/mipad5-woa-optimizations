# nabu 原驱动与 Mi 9 移植优化：来源及本机状态

本页是**可核验的来源/哈希记录，不是可安装驱动包**。第三方 Qualcomm、微软、小米及 WOA 二进制均不在本仓库再分发。完整机器备份、签名密钥、证书和原始日志也不公开。

## 原版来源

- nabu 驱动包：[MiPad5-Drivers-v2608.03-Desktop.7z](https://github.com/remtrik-stuff/MiPad5-Windows-Releases/releases/download/2608.03/MiPad5-Drivers-v2608.03-Desktop.7z)，上游 [2608.03 release](https://github.com/remtrik-stuff/MiPad5-Windows-Releases/releases/tag/2608.03)。SHA256 `39a4eba96ed5e28aab9099d79995c3b30a1436f3465682fbbeef09c6a77ddd39`，与 GitHub release 的 digest 及本地下载一致。
- 平台项目：[Project-Aloha/windows_oem_xiaomi_nabu](https://github.com/Project-Aloha/windows_oem_xiaomi_nabu)。原版与 UEFI/ACPI 版本须匹配；本页不建议将四个 Mi 9 文件无条件覆盖到其他 nabu 构建。
- 精确文件哈希见 [`evidence/nabu-mi9-driver-sources-20261028.json`](../evidence/nabu-mi9-driver-sources-20261028.json)。其中 `originalBspSha256` 来自本地解包后逐文件 SHA256，不是发行包的总哈希。

## 曾移植的 Mi 9 组件

来源：[qaz6750/XiaoMi9-Drivers](https://github.com/qaz6750/XiaoMi9-Drivers)，固定到 JSON 中列明的提交。表中哈希为原版 BSP → 米 9 上游 → 本机定制（如有），避免把上游成果称为本项目原创。

| 组件 | nabu 原版 SHA256 前缀 | Mi 9 上游 SHA256 前缀 | 本机处理 / 验证 |
|---|---|---|---|
| `qcpep8150.sys` | `39fafcd6` | `dee271ed` | Mi 9 CPU7 改动之上加单个空指针崩溃防护；本机签名成品 `2542f508…35e8b0`，2026-10-28 CPU7 受控基准仍有效。该成品**不是**上游原件，未公开。 |
| `qcpmicext8150.sys` | `533c3a55` | `ecf85b7c` | Mi 9 PMIC Extension；2026-10-13 设备绑定核对，未见新问题码。 |
| `qcbattmngr8150.sys` | `3659a20d` | `83f9ea34` | Mi 9 BatteryManager；2026-10-13 设备绑定核对，未见新问题码。 |
| `qcpmictcc8150.sys` | `be9850ce` | `d5b54181` | Mi 9 Type-C Controller；2026-10-28 UsbNabu 对照后恢复原绑定，设备问题码 0。 |

只针对这四项记录**确有来源和本机试验的移植**；其它驱动仍以 nabu 原 BSP 或各自上游为准，不能统称为“全套 Mi 9 驱动”。2026-10-28 与 NabuPrimeCore/UsbNabu 的受控对照表明：CPU7 高速档没有可测加速，现有 Type-C 与候选均可单独 PD 充电和 OTG 读盘；边充边 OTG、PD 合约和实际充电功率未测。因此维持原方案，而非叠加第二套 CPU governor 或 USB-C 接管驱动。

## PEP 定制补丁边界

本机 PEP 在历史转储所指的一个空上下文解引用点加保护，保留非空路径和既有错误返回；它**没有修复**全部 PEP 待机、电池耗电、RTC/ACPI 或虚拟化问题。补丁来源是对已有二进制的本机修复，不提供源码、通用差分、签名成品或一键安装包。SHA256 只用于识别，不代表签名许可或跨机兼容。

## 核验与限制

使用 [`tools/verify_driver_sources.py`](../tools/verify_driver_sources.py) 核对**自行合法取得**的文件哈希；脚本只读，不联网、不安装驱动。可对整个原版 7z，或从原版/米 9 上游取得的四个 `.sys` 文件分别核对。文件名可能相同，需指定 `--set original` 或 `--set mi9`。

本机底层日志审计的原始事件、设备序列号、私有路径与备份不公开。此处的 `lastVerified` 是对应组件**最后有证据的验证日期**，不能推断每个驱动在 2026-10-28 都再次逐字节读回。
