# VS Renderer / VS Player：项目开发地图

## 1. 发心与边界

为需要可视化滤镜、画面对比和高精度媒体播放的 Windows 用户提供桌面工具。Renderer 编排处理链和导出，Player 负责播放，VSCap 负责捕获与标注，PhotoCraft 提供独立编辑工作区。

成功标准是处理路径可检查、时间轴一致、便携依赖可复现、问题可定位到模块。非目标包括通用 Python IDE、任意 FFmpeg 命令设计器、私有 NGU 算法仿制和全部设备/格式兼容保证。

## 2. 快速开始

开发需要 C++20、CMake 3.25+、Ninja、Qt 6.8+ 的 MinGW 套件；Native 使用独立 MSVC 工具链，通过 C ABI 与 GUI 通信。完整命令见 [依赖与构建](docs/dependencies.md) 和 [API20 构建](docs/native-api20-build.md)。

已有依赖时：配置 `QT_ROOT` 和工具链 PATH → `cmake --preset windows-mingw-release` → `cmake --build --preset windows-mingw-release` → 按改动模块运行 CTest/QtTest。

三个常见坑：① GUI EXE、Native 与 FFmpeg shared ABI 必须匹配；② 只复制 EXE 不构成完整部署；③ 首次索引/着色器编译与稳定播放性能分开测。

## 3. 架构决策

| 决策 | 原因与适用范围 | 不采用的方案 / 重估条件 |
| --- | --- | --- |
| Qt Widgets + C++20 | 适配 Windows 控件、HWND 和 D3D11 | 不引入 QML；跨平台或场景图需求改变再评估 |
| 社区、教程使用 Qt 界面和离线 Markdown | Qt 处理站点 API、目录和登录，正文与 Vditor 编辑预览复用 WebView2 中的本地渲染资源 | 注册和第三方登录沿用站点验证。缺少运行时或资源时使用基础 Markdown |
| Agent Qt 核心 + 独立 MCP Node 桥 | Chat Completions/SSE、按需Skills、摘要投影、子会话；Qt异步本地文件/命令/图像；MCP用官方client v2 | 不引入.NET/LakeUI或网页壳；环境工具由模式及父权限统一拦截，原生非兼容模型协议需单独适配 |
| MinGW GUI → Native 动态 C ABI | 隔离 MinGW/MSVC C++ ABI | 不跨工具链传 C++ 对象；升级必须核对结构版本和 DLL |
| FilterGraph → VPY → VSScript → 3FP | 脚本可审查，预览与导出共用处理描述 | 不以离线 vspipe/Y4M 管道充当实时帧桥 |
| 直通与 VS 路径分开 | 普通播放不等待 VS 索引 | 不无条件初始化全部增强依赖 |
| 3FP原生色彩默认 | SDR/HDR使用现有3FP处理；旧自动色彩档迁回原生，libplacebo仅由Full手动启用以对照mpv | 不根据HDR标记自动进入libplacebo；手动档与ICC/LUT参数保留 |
| 时间组独立 FilterGraph | 同片不同段落可使用不同链 | 拼接要求各组输出规格一致，不静默改变格式 |
| Player 区间交接 | `PlayerLoop.cpp` 用源时间传给新 Renderer，`MainWindowTimeGroups.cpp` 按源 FPS 建组并选中 A–B | 不覆盖已有 Renderer 工作区；区间按恒定帧率源帧对齐 |
| Explorer 适配器与解码进程分离 | `src/shell/` 的轻量 COM 适配器启动有时限的 worker，媒体解码在独立进程 | 不在 Explorer 中载入媒体解码器；当前用户注册由 `PlayerThumbnails.cpp` 备份和恢复 |
| 专用时间轴与请求代次 | 管理 seek、补帧和源/输出 FPS | 不使用 seek 后局部帧号直接推算全局源帧 |
| 直通精确定位 | 时间跳转由 Native 合并相邻请求，参考帧仅用于解码。可定位的网络盘 MKV 复用已打开的读取器及索引；HD HEVC 软解从暂停重开时，在既有线程和内存限额内增加解码并行度。按帧号跳转时，本地 Matroska/WebM AV1 按需解析视频包并缓存显示时间戳 | 时间戳或包结构有歧义时沿用解码计数路径，保留可变帧率定位精度 |
| 关键帧导航 | 相对关键帧命令按点击顺序执行，保留播放状态，外部音轨按实际落点同步。随后点击的秒数偏移从新关键帧计算 | 逐帧检查仍会暂停播放 |
| 直通相对逐帧 | `PlayerWindow::stepFrame` 调用内核当前PTS逐帧；Native重新解码局部GOP确认相邻帧 | 不用绝对帧号逐帧，也不把稀疏缓存中的近邻当成真实相邻帧；VS仍按输出帧号定位 |
| 查看与编辑解耦 | PhotoCraft 独立进程保留未保存工作 | 旧 Qt 编辑器仍有源码/测试，不作为生产编辑入口 |
| 截图事实与颜色计划 | `color/ScreenshotColor.h` 区分显示模式、源存储/有效精度、绝对 scRGB 和相对白位；像素与标准声明共用计划 | 不按 HDR 开关或输出10位猜测源精度；WGC helper 缺失时尝试 DXGI，再以明确原因退回传统8位 |
| 固定上游 + 完整 Native 补丁 | 保留本地扩展并可重新构建 | 不拼装 API14/18/20 不同版本 DLL 或补丁 |
| 比对源像素与显示值分开 | `AnalysisPage::updatePixelReadouts` 按源坐标联动；`SourcePixelProbe.h` 和 Native 比对扩展提供转换前码值 | YUV 不由渲染 RGB 反算；交互重绘不继承静态图片媒体帧率；缩放移除固定倍率上限，Native 裁剪视口并映射 UV |
| 启动期库预读 | `src/player/main.cpp` 用128KiB缓冲在低优先级线程读取FFmpeg；单文件解包后的Qt子进程共用 | 不照搬.NET图标和LakeUI设备预热，不额外提高进程优先级；首次解包单独计时 |
| Player 待机 | 首次打开建立本次登录的待机，`PlayerStandbyWin.h` 转交请求，窗口被接管后立即补建，单文件复用解包目录 | 49 MB 工作集预算，`tools/killvsp.c` 按路径关闭待机。Full 随包提供工具，单文件启动器将工具从 PE 资源释放到同目录 |
| 软件播放同步与内存预算 | 上游线程/帧池/CPU 域按负载逐级调整；音频从60ms起按告警升至240ms，呈现允许最多250ms的平滑节奏容差；保留VSP的8K关键包参考帧追赶 | 不把显示器限帧记为解码丢帧；CPU预缩放仍默认关闭，8K逐帧实时按素材验收 |
| AI 字幕外置推理 | 固定 main + #36/#37 Native 音频 tap；Qt 有界传 PCM 到外置 worker，跳转按 generation 隔离 | 模型独立打包并由组件下载器安装；保留旧 GGUF 配置兼容，不移植上游 VB 页面 |
| Shader 平面输出 | `third_party/vsr-placebo` 直接回读 YUV 平面、合并增强 hooks，独立命名空间与 color 依赖 | 保留标准 placebo 插件作为缺少组件时的兼容路径；不把 VS 吞吐当作 Native 播放帧率 |
| Full/Lite 原生 Anime4K | `PlayerBuiltinProfiles.h` 定义六档；AFast CNN 使用 `Anime4KFast.h`，A CNN/AFast 1X 使用内置 HLSL 包；1X 按源尺寸处理后交给第二上采样，旧 no CNN 配置一次迁移 | 内置字节码更新仍须重建并验证像素；Full/Lite 内置档位不回退 GLSL；不兼容色彩或模型失败时明确提示并退回 D3D11，保留播放控制 |
| 通用 GLSL/HLSL 导入 | `ShaderPackage` 编译并缓存，`CustomShader.h` 执行多 pass；兼容文件无需重建 DLL，播放不引入 VS/Python 依赖 | 当前支持 LUMA/MAIN/PREKERNEL 像素 pass、参数默认值与 gather；NATIVE 只提供尺寸，计算/外部纹理等明确拒绝，详见 Player 指南 |
| Full/Renderer 提取末尾 shader | 核对本项目图元数据与实际 Python 正文，再将兼容的末尾节点交给原生执行器 | 前序滤镜仍由 VS 执行；不兼容时回退原 GLSL，编码导出保留完整 VPY，不实现任意 VPY→HLSL |

## 4. 目录地图与改动入口

| 目录 | 职责与关键入口 |
| --- | --- |
| `src/app/` | Renderer：`MainWindow.cpp`、`MainWindowTimeGroups.cpp`、`MainWindowPresets.cpp` |
| `src/agent/` | `AgentSession` 管工具循环、权限与待接收输入，`AgentContext` 管用量与摘要；`AgentLocalFiles`/`AgentLocalProcess`/`AgentLocalImages` 管本机能力，`AgentImageMessages` 管图像校验/投影；`AgentProvider`/`AgentModelCapabilities` 管来源/模型；`AgentInput` 路由命令/引用；`AgentInstructions` 管指令/Skills；`AgentMcp`/`AgentNodeRuntime` 管MCP进程；`AgentTaskQueue` 管子会话，`AgentToolLibrary` 管组合 |
| `src/app/AgentVideoFrames.cpp` | 原片/处理后 VS 帧按完整原分辨率发送，仅显式裁剪取局部；图像工具由 `AgentModelCapabilities` 的端点模型元数据启用，含 Harness 的 `input` 声明 |
| `src/ui/AgentPage.cpp` | 会话列表绑定独立 `AgentSession`，切换不取消后台请求；`MainWindowAgentFeatures.cpp` 为各会话注册工具与独立任务队列，模型列表逐项显示视觉能力 |
| `tools/agent-runtime/` | 官方 MCP client 锁文件与 Node JSONL 桥；测试服务器只供开发，`tools/stage-agent-runtime.ps1` 安装便携生产运行时与许可 |
| `src/graph/` | 滤镜定义/图/预设：`FilterCatalog.cpp`、`VpyScriptBuilder.cpp`、`PresetStore.cpp` |
| `src/backend/` | `VapourSynthFrameServer`、`ThreeFpPlayer`、`ThreeFpApi`、`FrameTimeline.h`、导出与进程 |
| `src/ui/` | `PreviewPane`、`CompareView`、`AnalysisPage`、参数、预设社区及导出窗口 |
| `src/player/` | `PlayerWindow`、`PlayerControls`、`PlayerProfiles`、字幕/音频/设置/图片；`PhotoCraftEditor.*` 连接编辑器 |
| `src/shell/` | `ThumbnailProvider.cpp` 对接现代缩略图及旧式 IExtractImage；文件/流入口共用 `ThumbnailWorker.cpp` / `ThumbnailDecode.h` 隔离解码 |
| `src/cap/` | `main.cpp` 负责截图/标注/贴图与 Qt chrome 合成；`DesktopCapture` 查询模式并冻结 WGC/DXGI 帧；`WgcBridge` 隔离 MSVC C++/WinRT；`UiAccess` 启动 |
| `src/bluray/` | `BlurayCatalog` 解析节目，`BlurayWidget` 提供共用界面 |
| `src/color/`、`src/image/` | 色彩桥、高精度图片、PSD/HDR/JXL 与保留的 Qt 编辑实现 |
| `src/render/` | `Anime4KFast.h` 运行内置增强；`ShaderPackage.*` 负责自定义源解析、转换与缓存，`CustomShader.h` 负责原生多 pass；补丁及构建见 API20 指南 |
| `src/update/` | `PortableUpdater`、`ComponentDownloads` |
| `assets/`、`third_party/` | 图标/语言/滤镜资源与第三方来源、运行时说明和许可 |
| `patches/`、`tools/` | 固定上游补丁；构建、部署、打包和测量脚本 |
| `tools/sr-pareto/` | 独立超分算法 Pareto HTML 模板；JSON 的 `preview: true` 点不计入前沿、可隐藏，使用见目录内 README |
| `tests/` | 可执行回归测试源码与小型 fixtures，保留按模块划分 |
| `docs/` | 按任务组织的现行操作指南，不放逐版本测试/发布流水 |
| `development/` | 源码工作区测试台账、合并历史原文与证据；不随运行包复制 |
| `.deps/`、`build/`、`dist/` | 忽略的依赖、构建/原始日志、本地运行产物；不等于已发布附件 |

工作区清理：优先删除历史打包暂存、部署副本和编译中间文件，保留当前 `build/mingw-release`、`dist`、待打包的最新运行时及依赖源码。`build/native-deps-20261115` 被 Native 依赖目录链接引用，不能整体删除。`build/retained-user-files/` 保存清理前提取的配置、VPY、色彩文件、媒体及记录，含 SHA256 清单；它属于保留资料，不是可再生成缓存。

1.1.0 改动入口：`src/app/MainWindowRoutes.cpp` / `RenderRoute.h` 管理处理路；`PlayerDecodeBuffer.cpp` 管理按需软解缓冲（常规8帧、尖峰最多48帧/4GiB，包含解码流水线，预算仅是上限）；`PlayerShortcuts.h` 定义快捷键；`ui/PageTheme.h` 复用页面背景；`assets/ncnn-filter.py` 仅处理 Renderer 单帧，实时 Player 注入跳过标志。AI 音频桥仍为 `PlayerAiSubtitles.cpp`，包说明见 [组件指南](assets/ai-subtitles/README.md)。

## 5. UI 地图

Renderer手动预览入口为`MainWindow::validateScript`；加路、换源和载入预设不触发预览，`RenderRoute::reset`清理旧输出。`CompareView`按处理路数维护标题。Player的`setMinimalMode`使用待恢复时间戳重新打开解码器，保留播放/暂停意图；底栏提示区仅占一行。

| 入口与导航 | 文件 |
| --- | --- |
| Renderer：左栏展开导航 → 工作台导入 → 时间组/处理链 → 参数/脚本 → 源与输出预览 | `src/app/MainWindow*.cpp`、`src/ui/NavigationIcons.h`、`ParameterEditor.cpp`、`CompareView.cpp` |
| 社区：公开教程三栏、论坛阅读与发布 | `src/ui/CommunityPage.cpp` 管 API、教程树、章节树和账号窗口，`CommunityMarkdownEditor.cpp` 管草稿与上传，`CommunityWebView.cpp` 管 Vditor 本地消息及站点会话，背景键 `VSR-Community` |
| 3FUI 导出：参数面板 → VS Renderer 导出插件 → Renderer 导出设置 | `plugins/3fui-export-bridge` 的底栏保持在宿主 LakeUI 面板的绘制层，`src/main.cpp` 在当前用户会话接收命令，`ExportWindow.cpp` 填入命令并保存历史 |
| Renderer 设置：基本、主题、FFmpeg、论坛、快捷键、右键、快照和恢复、版本与组件 | `MainWindow::buildSettingsPage` 组装；`PageTheme.h` 管主题；`MainWindowPreviewActions.cpp` 管按键和右键；`ProjectProtection.inc` 管自动保存与手动恢复；`ShortcutTable.h` 与 `PlayerMenu.cpp` 供两端复用 |
| Agent：发送键旁来源/模型/思考 → 对话；功能 → Skills/AGENTS、MCP/npm；自动队列 → 子会话；Token → 上下文整理 | `AgentPage.*`/`AgentPageDialogs.cpp`组装页面；`AgentComposer`管输入预览/补全，`AgentChoicePopup`管三类圆角选择浮层，`AgentMarkdownView`管消息排版与轮次过程折叠；`MainWindowAgent.cpp`注册工具，`MainWindowAgentFeatures.cpp`连接服务，`MainWindowAgentInteractions.cpp`路由命令/引用/能力；索引4，背景键`VSR-Agent` |
| 图像分析比对：多源 → 偏移/布局 → 同坐标 RGB/原始 YUV → 对比画布导出 | `src/ui/AnalysisPage.cpp`、`MultiCompareView.cpp`、`AnalysisExport.cpp`；OpenNGU 经 `ShaderPackage` 加载，缩小默认 D3D11；该核暂不支持一致的 VS 画布导出；Native 扩展见 `patches/3fp-api20-analysis-probe.patch` |
| BD Remux：节目识别 → 轨道/输出 → 队列 | `src/bluray/BlurayWidget.cpp` |
| Player：打开媒体 → 底栏/列表 → 右键音频/字幕/设置 → 图片工具 | `src/player/PlayerWindow.cpp`、`PlayerMenus.cpp`、`PlayerSettings.cpp`，`PlayerControls.cpp` 处理选轨弹窗与底栏宽度，`PlayerShortcuts.h` 管理默认绑定与旧配置迁移，`PlayerMenus.cpp` 的菜单动作 ID 同时供快捷键功能搜索和执行入口使用，每次编辑绑定时重读用户着色器和 VPY 库，`src/update/ComponentDownloads.cpp` 在展开组件时生成版本行 |
| Player色彩：解码设置 → 3FP默认 / 手动libplacebo对照；旧自动档迁回原生 | `src/player/PlayerColor.cpp`、`PlayerWindow.cpp` |
| PhotoCraft：图片工具栏 → 独立编辑工作区 | `src/player/PhotoCraftEditor.*`、`tools/build-photocraft.ps1` |
| VSCap：热键 → 选区调整 → 标注 → 最终裁切/复制/保存/贴图 | `src/cap/main.cpp` |

论坛格式扩展入口为 `assets/community-markdown/document.js` 的 `vsrRegisterFence(language, render)`。注册脚本接收围栏原文和文档基址，返回 DOM 节点。`editor.js` 管 Vditor 工具栏及左右实时预览，教程和论坛共用 `document.js` 与 `enhancements.js`。

Cube 通过 Renderer 的普通滤镜节点和 Player 右键菜单使用。`CubeLut.h` 管理程序旁的 `luts/` 与自动工作色域识别。Full 共用此目录，单文件 Player 以启动器为基准。`VpyScriptBuilder.cpp` 生成 SDR 色域转换和调色图，`NativeVsCube.cpp` 通过 FFmpeg 的 `lut3d` 处理 VapourSynth RGB 浮点帧，`PlayerImageTools.cpp` 调用 FFmpeg 处理静态图。

## 6. 外部依赖

| 依赖 | 固定版本 / 准备与调用 | 缺失行为 |
| --- | --- | --- |
| Qt | 6.8+；本地记录 6.10.2，MinGW；CMake 查找 | 编译或 GUI 启动失败 |
| WebView2 / Markdown | NuGet SDK `Microsoft.Web.WebView2` 1.0.3719.77，可用 `VSR_WEBVIEW2_SDK` 指定。Evergreen Runtime 加载 `community-markdown/`，内含 Vditor、Lute、KaTeX、DOMPurify、Highlight.js 与按需 PDF.js | 密码登录、原生列表和基础 Markdown 仍可用 |
| Agent Node/npm/MCP | Node 24.19.0、npm 11.17.0、MCP client/core 2.4.0；`stage-agent-runtime.ps1`固定生产包到`tools/agent`，JSONL桥提供stdio/Streamable HTTP | 缺少组件时MCP设置可安装/修复；模型对话与内置软件工具独立，许可与完整性见运行时`components.json` |
| VapourSynth / Python | VS API4 / R81；构建和 stage 脚本，`update-vapoursynth-runtime.py` 锁定哈希 | VS 链不可用；直通路径独立 |
| VS 插件 | BM3D 11.0、BM3Dvk2 1.0、BestSource 22；其余随运行时保留 | 对应滤镜不可用；不能用 API3 DLL 替代 |
| FFF.Native | API20 / 固定 `85f514b`；标准脚本应用 `3fp-vsp-upstream-20261115.patch` 和 `3fp-vsr-preview.patch` 并复制本库头文件；snapshot v23 / 608 字节、外部帧v2 / 240字节，动态 C ABI | 原生播放/呈现不可用；旧内核缺少整帧布局时无法复制该画面 |
| FFmpeg | full09＋SVT420修正发行版（2026-11-15），AVX2；shared ABI：63/63/61/7/10/12，来源哈希见 `tools/ffmpeg-source.json`；编译原生VS源需设置 `VSR_FFMPEG_ROOT` | 相应解码/导出失败，单文件启动器先检查组件；SDK缺失时CMake明确失败 |
| GLSL / RIFE | 随包 shader 与 NCNN 模型，`stage-shaders.ps1` / `install-analysis-plugins.ps1` | 相应增强不可用；RIFE 需要 Vulkan |
| GLSL 转换组件 | `tools/shader-tools/` 中的 glslc、SPIRV-Cross 及配套依赖，由 `tools/stage-shader-tools.py` 整理；HLSL 用 D3DCompiler 编译 | 缺失时不能导入 GLSL；原生 HLSL 不调用 GLSL 转换工具，逐帧播放仅执行已编译字节码 |
| libVLC / LAV / madVR | Full 可选后端，stage 脚本及各自许可 | 对应后端不可用，PlayerLite 不包含 |
| PhotoCraft | 固定源与补丁见 `tools/photocraft-source.json`、`build-photocraft.ps1` | 详细编辑不可用，基础查看独立 |
| JXL / AWJ / 图片库 | `tools/jxl-runtime.cmake`、`awj-runtime.cmake`、`image-runtime.cmake` | 对应格式/转换不可用；AWJ 需 AVX2 |
| aria2 / 7-Zip / MKVToolNix | `stage-update-tools.ps1`、`stage-mkvtoolnix.ps1` | 更新下载/解压或 Remux 不可用 |

来源、安装和验证入口见 [构建指南](docs/dependencies.md)；分发许可见 [第三方声明](THIRD_PARTY_NOTICES.md)。

## 7. 核心数据

| 数据 | 位置 / 所属模块 | 兼容原则 |
| --- | --- | --- |
| 处理链/时间组 | `FilterGraph`、`FilterTimeGroup` | 整秒起点分段；每组独立链，输出规格一致 |
| Renderer 项目 | `*.vsr.json` / `MainWindow` | `schemaVersion=2`，保存各处理路、可见性与当前编辑路，兼容旧单路；未知节点不保证无损往返 |
| VPY 预设 | 程序旁 `vpy/` / `PresetStore` | `# VSR_PRESET` Base64 JSON；时间组 metadata `schema=2`，兼容旧单链；不能与项目 schema 混淆 |
| Player 配置 | `config/player.ini` / `PlayerSettings` | 保留用户显式选择；单文件以启动器目录为根，历史和索引分别写入 config/history 与 config/cache |
| Player 待机 | `config/player.ini` 的 `basic/standbyInSession` 控制会话驻留。`PlayerLoginStartup.h` 首次正常打开时登记当前用户登录启动项 | 两版分别记录初始化状态，用户移除启动项后再次打开不会重建 |
| 论坛与导出设置 | Renderer 的 QSettings 中 `community/rememberLogin`、`community/sessionDpapi`、`community/sourceUrl`、`export/ffmpegExecutable` | 论坛会话仅发送到固定源站，记住登录使用当前用户 DPAPI，WebView2 使用 InPrivate。导出为每个新任务解析 FFmpeg 路径 |
| 论坛草稿 | `AppLocalDataLocation/community-post-draft.json` | 新话题草稿由 Qt 原子替换。管理员回收站会话仅保留在当前进程 |
| 自定义 shader | 配置旁 `shaders/custom` 或 `render/shaderDirectory`，所选源为 `render/customShader`；子目录 `cache` 存 `.vsrhlsl` | 格式 1、语义版本 v8（参数默认值与 gather；兼容已验证的 v7 字节码）；源内容、扩展名、转换器及 D3DCompiler 标识参与缓存键，结构损坏重建；导入失败保持旧选择 |
| Renderer / 社区设置 | QSettings；`community/sourceUrl`、`appearance/dark`、`appearance/language` | 源/主题/语言在同一滚动页；主题即时生效 |
| Agent 来源/会话/工具组合 | `QStandardPaths::AppDataLocation/agent/` 中的`providers.json`、`conversations/`、`tools.json`；选择与禁用工具用QSettings | JSON schemaVersion=1；会话本地记录保存自定义名称与用户指定的分组，折叠状态存于 QSettings；工作目录不决定分组，旧记录归入未分组；显式新建只保留一条可复用的空白会话；启动或删除后自动准备的空白草稿不进入左侧历史；来源密钥DPAPI加密，导出来源省略密钥；会话导入检查工具配对 |
| Agent 指令/插件/队列 | 同目录`AGENTS.md`、`skills/`、`skills.json`、`mcp.json`、`conversation-tasks/<会话ID>.json` | 技能先索引后读取；MCP凭证DPAPI、导入默认断开；旧tasks.json首次复制迁入当前会话，原文件保留；队列重启后手动恢复；摘要不删除原始消息 |
| Agent 待接收输入/模型能力 | 会话JSON中的`pendingMessages`保留原文/资料与来源，导入暂停；能力仅内存缓存 | API投影过滤本地显示字段；工具配对完成后接收输入/图像；能力绑定端点+模型，仅决定思考字段与视觉许可，不能覆盖模型或消息 |
| Agent 环境权限/图像 | QSettings `agent/accessMode`、`agent/workingDirectory`；图像保留在会话`content`规范块中 | 默认software，full由用户选择；导入会话/队列不提升权限或恢复进程；图像来源需能力声明，摘要剥离编码数据、请求保留近期图像 |
| 说明 / 命令 / AI | `vpy/preset-descriptions.json`；QSettings `export/commandHistory`；`config/player.ini` 的 `ai/package` | 说明只补空键；命令历史最多 50 条；模型路径指向外置清单 |
| 临时脚本/索引 | QStandardPaths、媒体索引缓存 | 可再生；导出冻结脚本、源和参数，不删除用户 VPY |
| 分发元数据 | `release.json`、资源/组件清单 | 版本、根目录和文件哈希须对应实际产物 |

## 8. 分层与高风险区

依赖方向：graph 不依赖 UI；backend 管帧、原生和进程；app/ui/player/cap 管交互。不要把算法直接塞入按钮回调。

2026-10-17 抽样：`MainWindow.cpp` 1193 行 / 60 KB，`src/cap/main.cpp` 888 行 / 59 KB，`PlayerWindow.cpp` 645 行 / 62 KB，旧 `ImageEditorWindow.cpp` 377 行 / 97 KB。行数与字节数只是职责审查信号；确需拆分时按交互状态、媒体操作、持久化和独立算法划分，不按行数机械拆分。

2026-11-08 Agent：Session约610行，LocalFiles约310行、LocalProcess约390行；权限派发仍属于Session，本地I/O与图片协议已独立。继续扩展时优先保持API/工具/I/O方向，不向主窗按钮填入文件或进程实现。

高风险包括 DLL/FFmpeg ABI、HDR 白位/色域/位深、seek 时钟和交接线程生命周期、超大图内存/ICC/Alpha、更新目录交换。涉及这些内容时需要实际媒体/GPU 像素或交互验证，编译成功与离屏布局不能代替实机结果。

Native 高风险集中于 `.deps/fff-vsp-120/FFF.Native/3FP/Core/PlayerSession.cpp` 与 `Render/VideoRenderer.cpp`（均超过一万行）；本轮仅局部修复。后续按解码生命周期、预算准入和图集职责分别提取，避免把 GUI 预测器搬入 Native。安全回归随标准补丁归档于 `FFF.Native.Tests/SecurityRegression.cpp`。

## 9. 更新、测试与发布

1. 明确变更范围，复现问题；修改相关模块，构建并运行对应测试。测试选择与结果写入 [版本测试台账](development/validation.md)，步骤见 [构建](docs/dependencies.md) / [测量](docs/performance-debugging.md)。
2. Native 用 API20 固定脚本和补丁；历史 API14/18 脚本只用于对应基线。GUI 使用 CMake preset。
3. 1.2.0 本机 Full 目录为 `dist/VS-GUI-Full`。单文件从 `build/playerlite-runtime-1.2.0` 暂存生成，成品为 `dist/VS-Player.exe`；Full 发布附件为 `dist/1.2.0.7z`。两者的可写配置都在各自程序旁的 `config/`。
4. 使用 `package-portable.ps1` 生成 Full。单文件脚本 `package-playerlite-single.ps1` 从当前版本暂存生成 Fast LZMA2 压缩的 7z 载荷，`playerlite-launcher.c` 调用 Windows `tar.exe` 解包。两种发行均排除用户主题和配置。完整流程见 [便携更新](docs/portable-updates.md)。
5. 解压、依赖、启动、配置和清单验证后才可报告包完成；源码推送、草稿、附件上传、正式发布分别记录，需要相应任务授权。

文档纪律：结构/决策变化当天更新本地图；操作变化只改对应 docs 指南；用户可见变化写 changelog；测试按版本与轮次更新同一台账，不新建 `docs/update-版本.md`、`validation-版本.md`。原始日志留 `build/`，必要证据放 `development/evidence/`；不携带用户媒体。README、本地测试数据和开发记录不得混入上游 PR。

## 10. 当前状态

当前源码版本 **1.2.0**。截图颜色入口为 `ScreenshotColor.h`、`DesktopCapture.cpp`、`ScreenshotPng.h` 和 `ImageHdrSurface.cpp`；视频源帧截图入口保持既有行为。WGC 编译需要 MSVC C++ 工具和 Windows SDK C++/WinRT（`tools/build-wgc.ps1`），运行仅依赖系统 Windows API；`VSR_BUILD_WGC_BRIDGE=OFF` 可仅构建 DXGI/传统回退。实际 HDR/ACM/DPI/多 GPU 覆盖需另行实机验收。

本地 1.2.0 Full 已部署至 `dist/VS-GUI-Full/`，这是桌面 Renderer 和 Player 快捷方式使用的目录。公开教程、论坛发布、Cube 和 Explorer 缩略图的验收见 [版本测试台账](development/validation.md)。

本地原生内核以主线 `5efa9fc` 和 IO PR #77 为基线，固定检出、补丁和构建步骤见 [原生内核构建](docs/native-api20-build.md)。RAW 图片通过 LibRaw 开发为 16 位 RGB。

VSP Native 的安全/生命周期修复、可恢复丢帧和预算策略在 `patches/3fp-vsp-upstream-20261115.patch`；GUI 预测与失败重试入口为 `src/player/PlayerDecodeBuffer.cpp`。预算零值、缩容驻留、旧 DLL 降级、可信外部帧契约及隔离回归命令见 [Native 构建指南](docs/native-api20-build.md#native-安全与缓冲契约)。构建候选不自动更新安装目录。

Anime4K 收敛为六档并迁移旧配置。生产 Native 必须关闭 `SecurityRegressionTests` 并检查不存在测试注入导出；Full 打包默认排除私有 OpenNGU 素材，本次 1.2.0 经授权随两个附件分发。实际发布状态以两端远端附件及哈希核验为准，流程见 [便携更新](docs/portable-updates.md)。

## 11. 故障排查

| 症状 | 先检查 |
| --- | --- |
| Qt 配置或构建失败 | `QT_ROOT`、MinGW PATH、CMake preset；[构建指南](docs/dependencies.md) |
| Native 黑屏/ABI 错误 | API20 补丁、FFmpeg shared DLL、`VSR_3FP_DLL`；[原生构建](docs/native-api20-build.md) |
| VS 缺插件/预览失败 | API4 运行时、模型和 `VSR_VSSCRIPT_DLL`；[Renderer](docs/renderer.md) |
| GLSL/HLSL 导入或执行失败 | `tools/shader-tools`、Native 自定义扩展、完整编译错误及 hook/色彩/尺寸边界；[Player](docs/player.md#当前支持边界) |
| seek/音画时间不同步 | `FrameTimeline.h`、`PlayerSession`、请求代次；[性能排查](docs/performance-debugging.md) |
| HDR 色偏/截图异常 | `src/color/`、`HdrPreview.h`、源屏幕 SDR 白位；[色彩](docs/color-management.md) |
| VSCap 模态/文字/贴图异常 | `src/cap/main.cpp`；[截图](docs/capture-images.md) |
| BD 菜单或跨段失败 | `BlurayCatalog`、concat 定位、片段索引；[蓝光](docs/bluray.md) |
| 缩略图接口测试通过但 Explorer 不生成 | 核对独立系统进程实际读取的注册项；开发代理的注册表视图可能不同。安装后仍需文件列表自动生成验收，不能用强制提取代替；[本地诊断记录](development/thumbnail-jxl-20261115.md) |
| 包缺 DLL/升级失败 | 分发形式、必需清单、包根目录与备份；[更新](docs/portable-updates.md) |

安全修复入口：`BlurayCatalog.cpp` 校验 MPLS 头部；`ImageDocument.cpp` 两处合成器限制深度64、结构访问65536，`PlayerImage` 将取消传到 PSD 解码/合成。Native 额外应用 `patches/3fp-avif-budget.patch`，由标准 API20 构建脚本接入；只修源码不代表发行 DLL 已更新。
