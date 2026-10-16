# VS Cap 多贴图、AWJ AVIF、HDR PNG 和 VS Player 静态图片颜色映射

> 第1节为功能说明；第2节之后的网页阶段构建/部署记录属于历史。当前交付与验收以文末“本地续修”和 [续修记录](update-20261016.md) 为准。

日期：2026-10-16。工程：`C:\Users\ARXChem\Documents\Open Source Projects\VS-Renderer-GUI`。

## 1. 本轮实现

### 1.1 多贴图与编辑工具栏

- 原有贴图窗口本来就使用 `QList<QPointer<PinWindow>>`，但剪贴板导入仅展开第一张文件。现每个本地图片 URL 独立建立贴图窗口，多张贴图错位展示、独立缩放与销毁；从截图工具直接连续置顶也支持多个窗口。
- 贴图启用工具栏时默认编辑画面；新增「拖动」单独工具（`CaptureOverlay::Pan`），只有选中后才能在编辑状态下移动贴图。隐藏工具栏时仍可拖动。
- 矩形和椭圆左侧增加「●」粗细控制（对其滚轮调节 1～512）和「■」纯色填充开关。维持原 Ctrl 正方形/正圆与撤销逻辑。
- 测试对象名 `capShapeWidth`、`capShapeFill`、`capPinView`。具体实现位于 `src/cap/main.cpp`。

### 1.2 窗口选区与托盘

- 左键单击托盘图标（`QSystemTrayIcon::Trigger`）触发截图；原右键托盘菜单不变。
- 截图启动前通过 Windows `EnumWindows` 获取可见窗口，优先 `DWMWA_EXTENDED_FRAME_BOUNDS`；忽略隐藏/最小化/cloaked 窗口，按当前显示器的物理原点和 DPI 换算至截图选区。未选区时悬停高亮候选窗口，单击自动选择矩形。
- 鼠标按住拖动仍按原算法自由框选；之后仍可移动矩形并通过八个控制点调整。
- 局限：窗口列表是截图启动时的快照，部分带特殊材质/权限的窗口无法精确获得可见边界；DPI、跨显示器及特殊屏幕需要在真实桌面确认。

### 1.3 AWJ AVIF

- 设置中新加保存格式 PNG/AVIF 选项、`capAvifQuality`（默认质量 70）。
- 选择工具栏「保存」时按设置选定格式输出，原来的显式 PNG / AVIF 按钮保留。
- 不再用 FFmpeg 进行 AVIF 编码；调用发布目录内 `runtime/awj/AWJ.exe`，其次是同目录 AWJ.exe，开发构建时回退 `.deps/awj/1.1.0/AWJ.exe`。符合项目现有 PlayerImageTools 对 AWJ 1.1.0 的参数形式。
- 普通图默认 AVIF 10bit 4:2:0（奇数尺寸自动 4:4:4）；HDR 带 BT.2020/PQ CICP 信息默认 AVIF 12bit。编码器 AOM，速度 6，质量 70；结果安全写入目标并清理临时图。
- 已用 `AWJ.exe` 对项目真实 PNG 做命令行试编码，得到 36.8 KiB 的 `result.avif`，成功 1 / 失败 0。参数级验证不等于编辑器真实全流程已通过。

### 1.4 PNG 16bit×4 和 HDR/SDR 双版本

- 所有正常 PNG 均通过 `Format_RGBA64` 输出，即 RGBA 每通道 16 bit，总共 64 bit。
- SDR 输出 1 张 PNG；HDR 保存 1 张原色 BT.2100 PQ 64bit PNG + 一张带 `_SDR.png` 的 sRGB 色调映射 64bit PNG。原生 HDR 全屏采集也沿用此行为。
- HDR 同时保留历史 `.scrgb-f32` 与 `.json` 科学/编辑工作流 sidecar（两张 PNG 以外的原始数据，不视为额外预览图）；FP16 捕获中转换至整数 PQ 必然有量化，原始浮点副本未丢弃。
- 现有 PNG 用 Qt `QImageWriter` `setCompression(9)` 和标准 zlib/DEFLATE 无损压缩。**未使用 LZMA2**：W3C PNG 标准规定默认 IDAT 必须使用 zlib/DEFLATE；直接改为 LZMA2，通用解码器不能打开。参见 https://www.w3.org/TR/png-3/#compression。

### 1.5 VS Player 的 HDR 静态图片 SDR 映射

- PNG 静态图片入口 `src/player/PlayerImage.cpp` 读取侧车 `.json` 的 `pngTransfer` / `pngColorSpace` 并恢复 `Bt2100Pq` 或 HLG 标记（作为插件忽略 ICC 的兜底）。
- `src/ui/PreviewPane.cpp` 保留原始 QImage 用于用户高精度导出，另为 QWidget 画面生成 SDR 映射图；应用 PQ/HLG EOTF、203 nit SDR 参考、BT.2020 或 P3 → sRGB、亮度映射及 sRGB OETF。避免把 PQ 码值当 sRGB 导致过曝、过暗或偏色。
- **边界**：这轮是 QWidget 的 SDR 兼容显示路径，即使在 HDR 显示器上也不宣称原生 HDR 高亮度显示；精确色度、HDR 全局亮度、SDR 首帧像素需要真实显示器验收。

### 1.6 UIAccess 高层级置顶

- `general/uiAccess` 默认关闭；开启后下次启动通过 UAC 管理员授权重启。本轮新增 `src/cap/UiAccess.cpp`，先检查 TokenUIAccess，已获权限时不重复重启。
- 管理员桥接仅在创建自身用户的 UIAccess token 时，使用同一桌面会话 winlogon 的 SeTcb impersonation 权限；随后 RevertToSelf、关闭所有句柄，通过 CreateProcessAsUser 启动自身。没有驱动、证书安装或修改系统进程。
- 设置开关不会立即重启而丢失贴图。失败时显示原因并继续正常置顶运行。已有实例通过 mutex 检查避免重复 UAC。
- 设计参考 [uiaccess 工具](https://github.com/killtimer0/uiaccess)，代码为本项目自行实现。标准签名清单方案的系统约束见 [微软说明](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-securityoverview)。此替代启动路径仍需用户批准 Windows UAC。
- 编译已通过；**本轮未实际发起 UAC，也未验收覆盖任务管理器始终置顶**，不将该项记作实机通过。

## 2. 代码影响与编译

直接修改：
- `CMakeLists.txt`（新增 `dwmapi`、`advapi32` 链接）
- `src/cap/main.cpp`
- `src/cap/DesktopCapture.cpp`
- `src/player/PlayerImage.cpp`
- `src/ui/PreviewPane.cpp`
- `tests/TestCap.cpp`
- `tests/TestCapWorkflow.cpp`

原工程还有此前未提交的播放器 3FP 改动和清理脚本，均没有调整。

完成 Windows/MinGW Release 编译：
- `build/mingw-release/vs-cap.exe`
- `build/mingw-release/vs-player.exe`
- `build/mingw-release/lite/vs-player.exe`
- `build/mingw-release/vsr_cap_tests.exe`
- `build/mingw-release/vsr_cap_workflow_tests.exe`

`git diff --check` 为 0（仅 CRLF 提示）。

由于 `dist/VS-GUI-Full/vs-cap.exe` 当时正在运行（旧实例可能有贴图未保存），**没有强行结束旧进程或覆盖该程序**。已经安全预置：
- `dist/VS-GUI-Full/vs-cap-vscap-next.exe`
- `dist/VS-GUI-Full/vs-player-hdr-next.exe`

两者位于原发行目录，可直接使用原有 Qt DLL 和 AWJ；但托盘截图软件采用单实例 Mutex，因此使用新版本前，应正常从旧 VSCap 托盘菜单退出，避免丢失旧贴图。待旧截图工具及播放器都退出后，可在项目根目录执行 `powershell -ExecutionPolicy Bypass -File .\tools\activate-vscap-20261016.ps1` 将这两个待激活版本复制为正式运行文件；脚本检测目标 EXE 仍在运行则拒绝覆盖。之前的单文件 Lite 也已用新的 HDR 图片映射构建重打包至 `dist/VSPlayerLite.exe`。

## 3. 验证状态与尚存边界

- 完成：Windows 编译通过、AWJ CLI 真实压缩成功、Git diff check 通过；测试代码根据 RGBA64 与 AWJ 改动完成编译。
- **未通过自动化实机 GUI 验证**：从 AgentDock 非交互会话启动 `vsr_cap_tests.exe` 与 `vs-cap.exe --self-test` 都发生阻塞超时。测试未取得断言结果，不能记为通过。实际窗口识别、图像贴图拖动与工具栏需要可交互桌面验收。
- 需要真实 HDR 显示器才能验证 HDR 截图动态范围、双版本显示的一致性和 VS Player SDR 映射校准。
- UIAccess 因缺签名正式授权，只有已授予 UIAccess 的设备才会开启新增高层级行为；其余情况下回退普通置顶提示。


## 本地续修与回归

修复 HDR preview 转成 RGBA64 后仍保留浮点 `capHdr` 标记的问题，避免保存 SDR 伴随图时错读像素和生成无效 sidecar。SDR 伴随图验证实际像素及 sRGB 色彩空间，HDR 双图测试验证 2 PNG + 原始浮点/JSON 共 4 文件。

完整 Windows 工作流 15 passed、精度测试 5 passed（均含初始化/清理），覆盖取消保存重试、多贴图、拖动、形状填充、撤销、HDR 双图和真实 AWJ AVIF 输出。正式部署和剩余验收边界见 [续修记录](update-20261016.md)。本文之前章节中的网页端编译/测试记录保留为历史，不能替代续修结果。
