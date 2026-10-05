# VS Renderer / VS Player 项目地图

本文件用于接手开发、定位模块和确认当前边界。用户操作见 [README.md](README.md)，版本变更见 [changelog.md](changelog.md)，逐轮测试与交付记录见 [专项文档](docs/) 和 [历史原文归档](docs/history/development-records-through-2026-10-03.md)。

核对日期：2026-10-05。源码 `CMakeLists.txt` 与便携 `release.json` 为 **1.0.6**。本轮制作完整包与精简包、上传源码并准备无附件 Release Draft；附件上传与正式发布由维护者操作。精简包仅排除 LAV、madVR、MKVToolNix，并同步移除对应本地组件版本记录；BD 原盘菜单的 libVLC 保留。打包脚本为 `tools/package-portable.ps1`，使用 `-Edition Full` 或 `-Edition Lite`，清单包含隐藏文件并排除个人状态。发行说明见 [1.0.6](docs/release-1.0.6.md)。1.0.5 已正式发布，既有版本与归档保留。

本轮根因修复位于 PlayerWindow 的目标呈现确认、PlayerPlaylist 的章节稳定和 QDialog 模式布局，以及 `patches/3fp-concat-seek.patch` 的全局定位。Player、Renderer 与原生 DLL 重编译交付，真实鼠标及原盘六集回归见 [BD 与进度条验收](docs/player-bd-seek-acceptance-1.0.6.md)。原盘菜单字幕语言交接由 `BlurayCatalog::playbackInput` 从 CLPI 保留传输流 PID，解决拼接流 ID 为零导致回退日语；五种语言图像与独立原盘提取结果一致，见 [字幕语言验收](docs/bd-menu-subtitle-languages-1.0.6.md)。

最新修复覆盖 BD 菜单节目接回滤镜、分段定位、底栏与停靠播放列表、歌词原生显示、目录图片循环，以及编辑器标尺/文字交互。真实 BD、kit A–P 和图像回归结果见 [修复验收记录](docs/player-bd-lyrics-editor-fixes-1.0.6.md)。

1.0.6 音频封面/歌词入口位于 `PlayerAudioMetadata`、`PlayerAudio`；BD 完整节目和音频树位于 `PlayerPlaylist`，MKS 轨道和章节由 `PlayerMenus` 接入，Tab 布局位于 `PlayerInfoPanel`。真实文件测试、操作与边界见 [音频 / BD / MKS 记录](docs/audio-bd-mks-1.0.6.md)。

1.0.6 后续 BD 原盘交互由 `PlayerDiscMenu` 动态加载 `runtime/vlc`，`BlurayCatalog` 准备菜单/短节目缓存；PGS 协议和时间零点修复由 `patches/3fp-bd-subtitles-probe.patch` 重建。稳定视频视口在 `PlayerPlaylist::layoutChrome`，默认开启。首次菜单入口显式激活 PreviewPane 视频层；PlayerDiscMenu 切换时后台释放旧播放器，并保留旧 HWND 到释放完成。底部菜单导航按钮已移除。真实 MyGO 菜单、鼠标操作、HW/SW 切换、字幕、短片以及 Tab/右键回归见 [本轮记录](docs/bd-menu-viewport-1.0.6.md)。

最新输入修复位于 PlayerDiscMenu 的 Qt 逻辑坐标转发、PlayerPlaylist 的模式对话框/工具栏几何，以及 ChapterTimeline/PlayerControls 的点击直跳与预览去重；记录见 [输入与布局](docs/player-input-layout-1.0.6.md)。

当前修复以返回的 1.0.5 源码为基线；2026-10-04 已重新编译并覆盖 `dist/VS-Renderer-GUI-windows-x64` 的 Renderer、Player、3FP 和版本清单，保留配置及预设。Jinc 8K 原生稳定段、大图启动与编辑操作的结果及未验收项集中于 [本机验收记录](docs/local-1.0.5-performance.md)。此次没有更新其它 PortableSoft 安装目录或发布线上版本。

3FP 色彩管理新增可选 libplacebo D3D11 显示分支，默认保留原生路径。配置入口在 `PlayerColor`，独立 C ABI 在 `src/color/`，3FP 变更以 `patches/3fp-color-management.patch` 重建；高级显示处理位于解码 / VS 输出之后。依赖、专项验证和尚未实现的 HDR 扩展见 [色彩管理记录](docs/color-management-1.0.4.md)。

1.0.5 后续交互更新集中在 `PlayerControls/Window/Info`、`PreviewPane` 和 `ImageEditorCanvas`；补帧八组合与双自动入口在 `PlayerProfiles/Menus`，Player VS 音频时钟专用接口由 `patches/3fp-vs-audio-clock.patch` 提供。2026-10-05 已与 BD 会话统一更新本地 dist，保留配置与预设。大图并行预测、缓存视图与实时性能的验收/未达标项见 [交互与调度记录](docs/player-interaction-performance-1.0.5.md)，不据短时稳态结果宣称所有补帧模式完成。

图像编辑器本轮补充整体图层交互、独立文字像素层、共享层控件和标尺跟踪；入口仍为播放器“详细编辑”。图层变换/合并接口位于 `ImageDocument`，工具选层/文字位于 `ImageEditorTools`，交互在 `ImageEditorCanvas`，菜单和面板在 `ImageEditorWindow`。操作与验证边界见 [图层交互更新](docs/image-editor-interaction-1.0.4.md)。

后续本轮补充平滑显示及视口缓存、滚轮平移、背景锁定、同窗口多文档与图层右键混合选项。混合参数和合并组接口在 `ImageDocument`，标准PSD混合参数在 `ImagePsd`，播放器入口复用窗口在 `PlayerImageTools`；行为、验证与精度边界见 [图像文档与混合选项](docs/image-editor-workflows-1.0.4.md)。

## 1. 项目定位与成功标准

2026-10-05 Full 组件页面在共享 `src/update/ComponentDownloads.*`，迁移 Player 设置中的本体更新入口；安装辅助脚本为 `tools/apply-components.ps1`，MKVToolNix 固定到 `runtime/mkvtoolnix`。Player BD 入口在 `PlayerPlaylist` 后台读取后直接播放，Renderer 保留 Remux GUI；信息面板自绘透明背景并随播放器激活状态避让设置窗口。验收与发布边界见 [本轮记录](docs/components-player-1.0.5.md)。

2026-10-05 原生软件回退优化位于 `PlayerSession/VideoRenderer`，由两个独立补丁重建；保留 444/10-bit 精度及硬解路径。780M 8K48 软解尚未达标，本地硬解到 8K 短时回归通过。当前部署和实验边界见 [AV1 软件回退记录](docs/av1-software-decode-1.0.5.md)。硬解预取预算由 `patches/3fp-hardware-video-queue.patch` 在 PlayerSession 区分硬件表面 768 MiB 与软件帧 128 MiB，保留八帧队列上限与 150 ms 预取目标；构建脚本自动应用，测量工具增加队列计数。RX 6600 的 1.0.5 报告、原理及本机回归见 [8K 队列记录](docs/hardware-8k-queue-1.0.6.md)。

2026-10-05 新增共享 `src/bluray/`：BD 元数据解析、节目/模板匹配、章节切片、反馈与 Remux GUI；Renderer 和 Player 复用。原生 `bluray:` 协议用 `patches/3fp-bluray-input.patch` 接入，下载桥接复用 ModelScope Manager。实现、路径和已验收/未验收范围见 [BD 记录](docs/bd-remux-1.0.5.md)。

面向理解视频处理参数、希望用 GUI 组织滤镜而无需反复写脚本，并能快速比较效果的用户。Renderer 是处理与比较工作台；Player 是共享后端的独立视频、音频、图片查看器。

核心流程：打开素材 → 选择模块、顺序和参数 → 生成可审阅的 VPY → 同步比较源与输出 → 保存预设或导出。Player 可直接复用 VPY，也提供原生直通、字幕、音频和图片工具。

成功标准是处理链真正执行、两路结果与时间轴对应、错误能定位到具体后端或滤镜、便携包解压即可使用。GUI 保持紧凑清晰，避免控件遮挡和装饰性大卡片。

非目标：任意 Python IDE、矩阵表达式可视化编辑器、完整 FFmpeg 参数设计器；尚未实现的 NGU 与完整 madVR 私有算法不能用近似实现冒名。

## 2. 快速开始

运行环境为 Windows 10 22H2+ x64、D3D11 显卡；Vulkan 滤镜 / RIFE 需对应驱动。当前增强版 FFmpeg 为 x86-64-v3，AWJ 图片转换后端需 AVX2，不能据基础 Windows 要求推断所有 CPU 都可执行全部功能。

便携开发目录：`dist/VS-Renderer-GUI-windows-x64/`，入口是 `VSRenderer.exe` 和 `vs-player.exe`。普通用户不需要另装 Qt、Python 或 VapourSynth。

当前程序文件约596 MiB（625 MB），现有用户缓存另计约34 MiB。FFmpeg在根目录共享DLL，GLSL库按需读取压缩资源；轨道metadata与Default选择（无Default时均选同类首条）、部署和验证边界见 [轨道与体积](docs/tracks-size-1.0.4.md)。主目录 `Clean-Workspace.bat`供手动清理，`--preview`只盘点；保留当前Release/便携目录和配置。

源码工具链：C++20、CMake 3.25+、Ninja、Qt 6.8+（本机 6.10.2 MinGW）；VS / 3FP 原生部分使用 Visual Studio 工具链。依赖准备见 [构建指南](docs/dependencies.md)，具体参数以当前 `tools/` 脚本为准，其中部分指南仍保留早期版本示例。

已准备依赖后，在 PowerShell 设置 Qt 与配套 MinGW / Ninja 环境，例如：

~~~powershell
$env:QT_ROOT = "C:/Qt/6.10.2/mingw_64"
$env:PATH = "C:/Qt/Tools/mingw1310_64/bin;C:/Qt/Tools/Ninja;" + $env:PATH
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release
~~~

三个常见坑：原生 DLL 与 FFmpeg major ABI 必须配套；只复制 EXE 不能替代完整运行时部署；冷启动 shader 编译和首次媒体索引耗时不同于稳态播放速度。

## 3. 架构与关键决策

| 决策 | 目的与适用边界 | 不采用的路径 / 重估条件 |
| --- | --- | --- |
| Qt Widgets + C++20 | 紧凑桌面布局、原生 HWND 与 D3D11 表面 | 当前不需要 QML 场景图；高度动画化或跨端时再重估 |
| 动态加载 3FP C ABI | MinGW GUI 调用 MSVC 构建的 FFF.Native.dll | 不绑定跨编译器 C++ ABI；必须与当前补丁和运行时配套 |
| C++ catalog + 有序 FilterGraph | 参数和执行顺序可验证，VPY 保持透明 | 不先建立通用插件 DSL；外部作者无编译扩展时再重估 |
| 工作线程调用 VSScript，帧送至 3FP | 实时预览支持随机帧、seek、合并最新目标 | vspipe/Y4M 用于离线导出，不作为实时预览桥 |
| 源 / 音频时钟驱动 VS 输出帧 | 按时间与源/输出 FPS 映射，支持补帧 | 不直接使用 seek 后局部帧号；任意剪辑不会自动重建音频 |
| GPU GLSL 通过 vs-placebo | 使用已有 mpv/libplacebo shader 并写入 VPY | 不将 shader 名称伪映射成普通 resize；仍有 CPU 平面读回与上传 |
| 视频、图片分开解码和显示 | 图片不走视频时间轴、VS 或相邻图片预解码 | 图片离线 Anime4K 调整尺寸是独立例外，查看仍不建 VS 图 |
| 比较滑块裁剪原生父窗口 | 各 pane 保持共同画布坐标，不重复解码或逐帧 CPU 拼图 | 比较导出使用独立离线合成，不能宣称与预览逐像素一致 |

**处理路径：** Renderer 实时链为 GUI → FilterGraph → VPY → VSScript 帧服务 → 3FP 外部帧呈现，源路提供播放时钟。Player 本地视频可走 VS 链或 3FP 原生直通；HTTP/HTTPS 固定原生直通，madVR 使用 LAV/DirectShow，二者不执行 VPY。图片由 PlayerImage 解码后交 PreviewPane 绘制，另存工具调用重采样或 AWJ 后端。详细编辑使用独立 ImageDocument：256px稀疏块、LRU磁盘换出、RGBA8/16/32F及局部历史；PSD走独立层组/蒙版读取、延迟通道展开和区域编辑，快照支持后台预览与PSD/TIFF保存。普通图片仍复用整图解码；查看器后台建立受内存上限约束的显示层级，编辑器保留整图概览与可见区域高质量缓存，均不替代原精度像素。接口与边界见 [图像编辑方案](docs/image-editor-plan.md)。

## 4. 目录与改动入口

Player 无预设原画、固定 Jinc / D3D11 为 3FP 原生路径，不建立 VS 源索引；字幕流由 `patches/3fp-streaming-text-subtitles.patch` 为 3FP 增加限量读取和 seek 支持，`PlayerSubtitles` 消费 MoreData 状态。构建补丁、机械硬盘素材与尚未完成的远端验证见 [直通加载说明](docs/player-direct-open-1.0.4.md)。

~~~text
src/app/       Renderer 工作台、设置、预设入口与样式
src/graph/     FilterCatalog、FilterGraph、VpyScriptBuilder、PresetStore
src/backend/   3FP 动态接口、VS 帧服务、LAV/madVR、预热与导出进程
src/ui/        PreviewPane、比较布局、分析页、参数、预设与导出窗口
src/player/    独立播放器；控制/预设/图片/音频/字幕/网络/设置分模块
src/image/     分块图层文档、高精度像素/蒙版/选区、局部历史与原生编辑工具
src/update/    PortableUpdater 检查、下载和更新包验证
assets/        Qt 资源、图标、语言、内置 helper、GLSL 与来源清单
patches/       FFF Project 的外部帧、缩放、字幕、音频等原生补丁
tools/         依赖构建、运行时部署、便携打包、更新安装与验证
tests/         图脚本、参数 UI、原生帧桥、比较、导出、Player、更新测试
third_party/   VapourSynth 源码及第三方许可/模型来源资料
docs/          当前规格、专项验证、发布说明；history/ 保存原始记录
.deps/         本机构建依赖、源码/二进制缓存，不是发布内容
build/         构建、专项日志和审计；主要目录为 mingw-release/
dist/          本地便携目录及按版本命名的归档
~~~

常见改动位置：新增滤镜同时检查 `FilterCatalog.cpp`、`VpyScriptBuilder.cpp` 和插件 staging；实时帧/seek 查 `VapourSynthFrameServer.cpp`、`FrameTimeline.h` 与对应窗口；图片操作查 `PlayerImageTools.cpp` / `PlayerImageResample.cpp`；更新查 `PortableUpdater.cpp` / `tools/apply-update.ps1`。

## 5. UI 地图

| 页面 / 窗口 | 当前职责 | 主要文件 |
| --- | --- | --- |
| Renderer：VS 实时渲染 | 顶部源与预设；处理链、参数、视频三栏；源/处理后比较和共享播放控制 | `src/app/MainWindow.cpp`、`src/ui/ParameterEditor.cpp`、`CompareView.cpp` |
| Renderer：图像分析比对 | 最多九路原生视频，源交换/卸载、独立时间偏移、并排/网格/滑块 | `src/ui/AnalysisPage.cpp`、`MultiCompareView.cpp` |
| Renderer：设置 / 预设 / 导出 | 导航内嵌设置；独立预设管理；非模态导出设置、准备文件、编码队列 | `MainWindowPresets.cpp`、`PresetDialog.cpp`、`ExportWindow.cpp` |
| Player 主窗口 | 播放表面、两排控制、悬停/停靠播放列表、信息面板和分级右键菜单 | `src/player/PlayerWindow.cpp`、`PlayerControls.cpp`、`PlayerPlaylist.cpp`、`PlayerMenus.cpp` |
| Player 设置 / 图片工具 | INI 加载保存和取消/确定/应用；图片顶栏、尺寸、裁剪、转换 | `src/player/PlayerSettings.cpp`、`PlayerImageTools.cpp` |
| 详细图像编辑 | Photoshop系列工具工作区；原精度像素/组/蒙版与历史；PSD、颜色、HDR及导出 | `src/image/ImageDocument.*`、`ImagePsd.*`、`ImageEditorWindow/Canvas/Tools.*`、`ImageAdjustments.*`、`ImageHdr/Surface.*` |

Renderer 默认窗口为1800×900逻辑像素，参数栏独立占用可用高度。基础设计见 [UI 规格](docs/ui-layout-spec.md)，其中1440×900与早期侧栏布局是旧基线；当前布局看 [三栏说明](docs/renderer-panels-1.0.3.md)。切 Renderer 页面会暂停离开的播放页；分析页按需初始化。全局空格用于播放/暂停，缩放后支持左键拖动。

## 6. 外部依赖与部署边界

| 依赖 | 当前来源 / 协议 | 准备与缺失行为 |
| --- | --- | --- |
| Qt | 6.8+，本机6.10.2；Core/Gui/Widgets/Network | 配置阶段需完整开发包，运行阶段需匹配 Qt DLL/plugins |
| VapourSynth / Python | 官方源码当前 `5b2d556`（R80RC2），VS API4；便携 Python/VS | `tools/build-vapoursynth.ps1` 默认 Python3.15，staging 至 `runtime/python`；缺失时不能 VS 预览，原生/图片路径独立 |
| 3FP / FFF.Native | 本地扩展 ABI16，动态 C ABI；选择兼容官方更新，保留本地 DV | `tools/build-3fp.ps1` 应用性能及 `3fp-jinc-upstream-1.0.5.patch` 等补丁；不是官方完整功能镜像 |
| FFmpeg | 自编译增强版，2026-10-03，匹配 shared major ABI | 根目录供3FP；`runtime/ffmpeg` 为CLI，源插件/LAV自带解码库不自动更新；见 [运行时说明](docs/ffmpeg-enhanced-1.0.4.md) |
| VS 插件 / RIFE 模型 | API4插件，vs-placebo2.0.4、MVTools29、NCNN param/bin | 安装与stage脚本固定来源；R80不加载旧API3，缺 namespace 时报告具体错误；PKL不能代替NCNN模型 |
| GLSL 库 | 随包mpv着色器及上游许可/哈希 | `assets/mpv-shaders` 与便携 `shaders`；目录/搜索/分类入口见 [GLSL说明](docs/mpv-shaders-1.0.4.md) |
| LAV / madVR | LAV0.83.0 x64，随包AX/DLL，由类工厂加载 | 不要求全局COM注册；缺失不能启用相应后端，madVR模式绕过VS |
| 图片解码 / 转换 | libavif1.4.2、dav1d1.5.3、libjpeg-turbo3.2.0；独立AWJimage1.1.0 | CMake脚本准备固定依赖；AWJ只用于转换，AGPL许可/NOTICE/对应源码随包，缺失不影响基础图片查看 |
| 更新工具 | aria2-next2.8.3、7-Zip | `tools/stage-update-tools.ps1` 校验后部署；缺失不能完成相应下载/安装步骤 |

运行时依赖和许可必须整体交付，不能将本机绝对资源路径当作便携包已包含。详细来源见 [第三方声明](THIRD_PARTY_NOTICES.md)。

## 7. 核心数据与兼容规则

| 数据 | 位置与负责模块 | 规则 |
| --- | --- | --- |
| 滤镜图 | 内存 `FilterGraph` / `FilterNode` | 节点含instanceId、definitionId、enabled、parameters；严格按列表顺序生成 |
| Renderer 项目 | 用户选定 `*.vsr.json`，MainWindow读写 | schemaVersion=1、sourcePath/sourceFilter/nodes；QSaveFile提交 |
| VPY 预设 | EXE旁 `vpy/`、`vpy/builtin/`，PresetStore | `# VSR_PRESET` 中Base64 JSON schema=1，保存节点/源/备注；外部VPY无元数据仍可直接运行 |
| Player 配置与位置 | EXE旁 `player.ini`，PlayerSettings/PlayerWindow | INI导入导出；已知内置预设迁移留备份，用户脚本不整体重写 |
| Renderer 设置 / 临时脚本 | QSettings默认存储；QStandardPaths缓存下 `preview.vpy` | Renderer设置不是player.ini；脚本缓存与播放器便携索引缓存分开 |
| 索引、输出与更新记录 | 默认 `cache/indexes` 或自定义路径；用户输出路径；`release.json` | 索引按媒体身份复用；导出任务持有脚本/画布快照；更新manifest版本与构建一致 |

当前项目/预设图加载会**跳过未知滤镜ID、忽略未定义参数**，新增已知参数取catalog默认值；没有未知节点无损往返保证。加载VPY可覆盖输入源，时间剪辑和处理后音频需要用户明确管理。

## 8. 分层与高风险改动

`graph` 不依赖 UI；`backend` 提供帧、播放和进程接口；`app/ui/player` 负责交互与协调。新逻辑先放入已有职责模块，避免把处理算法、原生协议或后台编码塞进窗口事件处理。

当前 `MainWindow.cpp` 约1100个非空行，是优先检查职责集中的位置；AnalysisPage、PlayerWindow、ExportWindow和VS帧服务各约400–460个非空行。行数仅作信号，后续若需拆分，可按工作台控件、源/帧协调、项目读写分开；本轮不实施重构。

高风险区：3FP ABI和成套DLL、源/输出时间映射、seek与失效请求取消、字幕生命周期、超大图片内存与Alpha/ICC、导出停止收尾、更新目录交换。改动应针对相应测试组验证；Qt控件截图不能证明D3D视频像素正确。

## 9. 构建、验证、部署与文档维护

780M 原生直通性能后续在 NuxBox 本地开发；素材、mpv 对照、工具链和验收见 [远端性能交接](docs/nuxbox-performance-handoff.md)。`tools/package-dev-handoff.ps1` 制作独立干净源码/SDK快照，不删除当前工作区、不携带个人配置和整套运行时。远端 v143 可通过 `build-3fp.ps1 -PlatformToolset v143` 显式选择；未据用户丢帧报告宣称性能已修复。

此设备本轮改为先升级 V145；Qt 6.10.2 MinGW、MinGW 13.1、CMake、Ninja 已安装，VSPlayer 与 Player 专项已构建并验证。`C:\BuildTools\2026` 的 MSVC 14.51 / v145 已完成原生 Release 构建；新增帧呈现等待补丁和异步阶段计时，独立 HWND 与 GUI 的重复测量及实测边界见 [2026-10-04 设备记录](docs/nuxbox-performance-session-2026-10-04.md)。

开发流程：明确行为与边界 → 改对应模块 → 构建目标和相关测试 → 验证所需原生呈现/真实输出 → 按任务范围部署。当前CTest包括图脚本、shader参数、VS帧桥、缩放平移、导出、比较、Player、语言包、更新；图片相关为核心、PSD、颜色/HDR、基础工具、高级工具和工作区布局六组。只按变更运行相关专项，测试组通过与子用例通过分开记录。

`tools/stage-portable.ps1` 更新本地便携目录；`tools/package-portable.ps1` 从已验证目录构造干净LZMA2归档；`-BuiltinDirectory` 可指定由当前构建生成的干净内置预设，不改动本机用户VPY。旧 `package-release.ps1` 仍存在，不应套用其中历史版本参数。归档使用版本根目录以兼容1.0.2更新器，本地目录保持固定名。包含必需运行时与许可，排除个人INI、用户VPY、缓存、日志和测试程序。

发布需另行确认源码推送、归档完整性/内部哈希、完整解压启动以及远端附件/更新源状态。更新流程见 [便携更新](docs/portable-updates.md)；历史包修正见 [兼容记录](docs/update-compatibility-1.0.3.md)。本次文档整理不代表打包或发布授权。

维护分工：

- `README.md`：安装与常用操作，不逐轮叠加更新。
- `project.md`：结构、决策、数据、当前边界；变化时修改对应段落，不在顶部插入日期流水账。
- `changelog.md`：按版本倒序，在已有版本的新增/修复/变更分类下合并条目。
- `docs/`：专项实现、测量条件、测试日志和交付证据；历史原文只追加归档。

## 10. 当前能力、下一步与限制

1.0.4本地开发已包含：Renderer模块滤镜/VPY、源与输出同步预览、九路分析与受限画布导出；Player视频/音频/图片、字幕、原生/VS/madVR路径、自动/固定Anime与补帧；图片旋转/镜像/裁剪/调整尺寸/AWJ转换；GLSL分类与宽高联动、直通拖动预览、Lanczos4和增强FFmpeg。

当前 1.0.5 交付范围为源码推送和本地完整便携压缩包，不创建 Release 或 Draft，不上传发行附件。原 1.0.4 发布资料保留，具体交付与测试查 [更新日志](changelog.md)。

图片“详细编辑”入口已开放，使用Photoshop系列布局和B–D工具/颜色菜单；RGB PSD/PSB图层、组和蒙版可编辑、另存，提供浮点TIFF及HDR能力检测/SDR回退。真实520MiB、9层PSD专项和最终交付见 [本轮记录](docs/image-editor-round2-1.0.4.md)。后续方向仍包括按需求扩充VCB教程向catalog、完善插件部署与诊断、评估帧搬运性能及NGU独立后端；这些是方向，不是已实现能力。

当前限制：

- 外部帧桥支持常用整数平面格式，不支持VS float或GPU驻留帧直交；GPU滤镜仍有CPU平面拷贝，重shader/高分辨率不保证实时。
- 自动降档与资源阈值是调度策略，不是整机资源硬上限；固定Anime档位不因负载自动降档。最低补帧档持续丢帧时保留播放并提示，由用户决定关闭。
- 图片查看与常规重采样使用CPU；动图只显示首帧，无整图GPU解码/显存缓存。离线Anime4K尺寸输出受GPU纹理和显存限制，AWJ质量指标用GPU不代表编码使用GPU。
- 图像编辑尚无DNG、Bezier/可回改文字/完整矢量对象、语义AI及完整PSD复杂对象保真；颜色管理为Qt RGB ICC。新建Float32 PSD缺少源Photoshop HDR色彩数据时拒绝保存，应选浮点TIFF。工具工作范围与原生HDR实测边界见本轮记录。
- 单文件3FUI兼容导出仅MKV，使用源输入0和VS/Y4M输入1，拒绝 `filter_complex` / `lavfi`。画布导出仅AB滑块、ABC左右2+1、ABCD常规/滑块2×2，保持最大源尺寸；部分预览核无一致导出实现，会明确拒绝。
- Super-XBR为开源单阶段移植，抗振铃不是madVR私有算法复刻；未实现NGU。任意VFR、真实HDR、多显示器DPI、九路4K长时间播放未获全面验证。
- 工作区清理目前只有盘点与配置保留，未删除约7.79GiB候选文件；详情见 [清理记录](docs/workspace-cleanup.md)，不可写作已释放空间。

## 11. 故障排查与专项入口

| 症状 / 改动目标 | 首查位置与处理 |
| --- | --- |
| CMake找不到Qt或生成器 | `QT_ROOT`、PATH、`CMakePresets.json`；Qt与MinGW版本配套 |
| 预览诊断页 / 缺外部帧ABI | 检查匹配的FFF.Native与根目录FFmpeg；开发可用 `VSR_3FP_DLL`，按完整patch链重建 |
| VS未加载 / 缺namespace | 检查完整 `runtime/python`、API4插件；开发可用 `VSR_VSSCRIPT_DLL`，不要让用户自行混装API3 |
| 换模式、seek后右路无画面 | 帧服务代次、表面启用顺序、源/输出时间映射；见 [帧桥](docs/realtime-bridge.md) |
| Anime/补帧负载、输入位深 | `PlayerProfiles.cpp`、`PlayerResources.cpp`；见 [六档边界](docs/player-six-stage-1.0.3.md) 和 [性能条件](docs/player-performance-1.0.2.md) |
| GLSL尺寸或算法选择异常 | catalog/参数编辑器/VPY；见 [分类与尺寸](docs/mpv-shaders-1.0.4.md) |
| 超大图片、裁剪或转换错误 | PlayerImage/Tools/Resample及AWJ输出；见 [图片工具](docs/player-image-tools-1.0.4.md) |
| AVIF能解码但内容黑块/错色 | 先核对源编码内容，不能只凭可解码认定完整；见 [受控更正](docs/avif-large-yuv444-analysis.md) |
| 文件图标或角标不更新 | Player注册分类与Windows默认应用；旧统一ProgID仍兼容打开，见 [图标接入](docs/icons-integration.md) |
| 更新包拒绝 / 归档兼容 | 包根层级、唯一完整目录、SHA-256和内部版本；见 [便携更新](docs/portable-updates.md) |

其它常查文档：[Renderer三栏](docs/renderer-panels-1.0.3.md)、[比较导出](docs/renderer-ui-export-1.0.3.md)、[图片与音频](docs/player-images-audio-1.0.3.md)、[手动Anime](docs/player-manual-presets-1.0.3.md)、[GLSL与补帧](docs/mpv-shaders-interpolation-1.0.3.md)、[滤镜覆盖](docs/filter-coverage.md)。专项文档中的旧日期和测试结论属于对应轮次，不自动成为当前全量回归结果。

本轮图像编辑B–D/HDR已以40个相关专项验收（含520MiB、9层PSD真实鼠标编辑/撤销、原生完整查看、CICP P3/PQ AVIF、后台PSD保存快照及独立PSD读取）；记录与边界见 [第二轮交付](docs/image-editor-round2-1.0.4.md)。便携仅更新Player与文档，Renderer/native/配置受哈希保护；没有推送或发布压缩包。
