# 完整便携更新与发布

Renderer 设置和 Player 的“设置 → 基本设置”都显示当前版本与检查更新按钮。检查读取公开数据集 `ARXChem/Software-List` 的 `VS-GUI` 目录，支持分页，按数字版本选最新的 `<版本>.7z`。例如 `1.0.10` 大于 `1.0.2`，`1.0.2.0` 等于 `1.0.2`；同版本与旧包不安装。

下载使用随包 `runtime/tools/aria2-next.exe`（2.8.3），八路分段、证书验证、有限重试；哈希按小块异步验证，下载和解压也不阻塞界面。更新包需要目录接口提供有效大小与 SHA-256，下载地址使用文件 Revision 固定版本。关闭更新窗取消准备，取消或校验失败不会修改原程序目录。

归档结构为单个 `VS-Renderer-GUI-<版本>-windows-x64` 目录，包含两个正式 EXE、匹配的原生 DLL、完整 VS / Python / 解码器 / 着色器 / Qt 运行时、语言包、更新工具与 `release.json`。`release.json` 示例：

```json
{"schema": 1, "version": "1.0.2", "platform": "windows-x64"}
```

包路径验证拒绝绝对路径、驱动器/ADS、父目录跳转、重复路径、链接、Windows 设备名及不合法尾字符。解压后检查必需组件和内部版本。下载并验证成功才出现“安装并重启”；点击后关闭本程序，独立 PowerShell 安装器等待该目录内其他程序退出（最多 120 秒），不强杀另一个正在播放的窗口。

完整更新先在原目录同级构建新目录，再把原目录移到 `.previous-<标识>` 备份，将新目录替换为原位置。保留顶层 INI、自定义 VPY、预设元数据、用户项目与截图；内置 VPY 随新版替换，旧内置和缓存仍可在完整备份查找。新目录不沿用寻帧/着色器缓存。复制失败不触碰旧目录，交换失败在原目录缺失时恢复备份；结果写入 `update.log`，安装失败会显示提示。备份不自动删除。

维护者先构建并运行回归，再运行 `tools/stage-update-tools.ps1` 部署更新工具及许可（SHA-256 固定验证 aria2-next 2.8.3）。`tools/stage-1.0.2.ps1` 也会调用它。`tools/package-1.0.2.ps1` 从活跃目录复制到全新暂存目录，排除缓存、个人 INI / VPY、测试产物和历史脚本，只携带 Anime / Realistic 内置 VPY。移除 madVR 的安装/卸载/重置/调试开启脚本和本机 settings.bin，保留便携运行时与许可。活跃目录的用户文件不删除。

压缩命令使用本机 7-Zip Zstandard：`-m0=zstd -mx=22 -ms=on -mmt=2`。这是 [上游文档](https://github.com/mcmilk/7-Zip-zstd#usage-and-features-of-the-full-installation) 的 Zstandard Ultra 等级；随包解压工具支持该编码。输出 `dist/1.0.2.7z` 与 SHA-256 文件。包内 `local-resource-sha256.json` 记录各文件（不包含其自身）的校验值，清理后生成，避免旧资源清单失真。

本轮发布授权：用本机 ModelScope-Manager Python SDK 上传 `VS-GUI/1.0.2.7z` 及校验文件，核对公开目录中的大小与 SHA-256；GitHub 只推送源码、创建无附件 Release 草稿。GitHub 压缩包由用户手动上传，草稿不发布。
