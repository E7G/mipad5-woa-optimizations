# 触摸驱动

## 内容

`drivers/nt36xxx` 为固定上游源码加本地休眠补丁。修改集中在：

- 显示状态回调与 WDF D0 生命周期分开跟踪。
- `PowerTransitionLock -> ControllerLock -> SpbLock` 锁顺序。
- 熄屏抑制报告并排空连续模拟定时器，释放触点，再下发控制器休眠命令。
- 唤醒重传固件，错误继续向上传递，不在任意电源回调中直接开关 WDF 中断。
- 固件读取/分区解析边界检查、逐步 I/O 返回值和资源回收。
- INF 最低系统定向到 build 26100；这不是降低到更旧 Windows 的兼容声明。

`patches/nt36523-sleep.patch` 为相对上游固定提交的文本差异；完整源码也已附上。发布只规范了文本编码/行尾，没有为开源再修改运行逻辑。`evidence/source-provenance.json` 记录输入与发布文件哈希。

## 构建

使用有 ARM64 C++ 工具、WindowsKernelModeDriver10.0 和匹配 WDK/SDK 的 Visual Studio 开发环境。公开源码移除了本地绝对路径 `Directory.Build.props`，没有附带私有 WDK 缓存。标准 WDK 环境下示例：

```powershell
msbuild drivers/nt36xxx/contrib/nt36xxx.sln /t:Build /m:1 /nodeReuse:false /p:Configuration=Release /p:Platform=ARM64 /p:WindowsTargetPlatformVersion=10.0.28000.0 /p:KMDF_VERSION_MINOR=31 /p:SignMode=Off
```

本地历史构建采用 SDK/WDK NuGet 10.0.28000.2526、单 worker、禁用并行编译；公共标准 WDK 安装构建路线尚未独立复现，可能需要匹配安装的 SDK 版本。CI 只做主机模拟测试，不声称产出已签名 ARM64 驱动。

上游工程还引用下列外部文件；请从已授权的上游/设备提取来源取得，放入 `drivers/nt36xxx/firmware/`，本仓库不提供：

|文件|历史测试使用的 SHA256|
|---|---|
|novatek_nt36523_fw01.bin|72364eca0a91855d59e043371aca774354e5ad0885df5024edcec8f976543ad2|
|novatek_nt36523_fw02.bin|343af7c62fc195118d992a34bd1d1f73fe195c8e9bd45bf01c6d83a3483c8479|

安装需要自己的签名/信任安排和事先可用的恢复路径。先导出原驱动包，再验证无休眠改动的编译基线与回滚；不要直接覆盖系统目录。本仓库不自动关闭安全设置或安装内核驱动。

## 测试边界

主机 mock 可以覆盖锁顺序、时序请求和注入失败，不能模拟真实 WDF 并发、中断、控制器电流或物理断电。现场看见 SPI 回调减少不是全机 DRIPS 修复证明。
