# VS Renderer / VS Player：项目开发地图

2026-11-14 两轮色彩审查修复见 [实施与上游草稿记录](development/color-review-fix-20261114.md)：修复Gray强制转换范围、VPY future声明、未知矩阵CPU入口及旧Native metadata降级，补齐ABI断言和HDR10+帧序重置。FFmpeg SDK通过`VSR_FFMPEG_ROOT`明确选择；此前审查段落保留当时状态，以此记录为后续进展。

2026-11-14 VSCap偶数选区：`src/cap/main.cpp` 的 `selectionPixels_` 保存实际像素选区，界面坐标仅用于显示和命中；框选/调整/移动以2像素为步长，AVIF去掉额外确认框，编码入口保留偶数补边。回归入口 `TestCapWorkflow::selectionEvenPixels`、`oddAvifSaveHasNoConfirmation`，行为见[截图指南](docs/capture-images.md)。

2026-11-14 色彩更新后审查见 [审查报告](development/color-review-20261114.md)：确认有限范围Gray强制转换、用户VPY future声明及CPU路径未知矩阵检查3项P2问题；本轮仅审查，尚未修复。上游db94021相对固定248cb9b无Native差异；现有色彩套件第二轮45项通过不覆盖新增复现边界。

2026-11-14 缩放入口续修：工作台 `MainWindow::applyPreviewScaling` 和比对页 `AnalysisPage::applyScaling` 均加载 OpenNGU HLSL；新增处理路继承当前选择。只用于 BT.709 SDR 放大，缩小默认 D3D11，缺失资源或不兼容色彩回退 Jinc；测试范围见台账。

2026-11-14 色彩修复见 [记录](development/color-fix-20261114.md)：更新Native汇总补丁修正基础转换；`FrameColorMetadata.h`定义外部帧v2尾部，`NativeVsSource.cpp`生成`vsr-source.dll`，`VapourSynthFrameServer`验证元数据来源。旧v1前缀保留，浮点/HDR新能力需配套Native；原生源首开索引与支持边界见记录，历史审计失败数字不代表本轮产物。

2026-11-14 Full精简入口：`tools/trim-full-runtime.py` 在打包暂存内处理工具7z归档、旧AWJ源码、无运行入口的模型和开发辅助文件，剥离前后核对PE接口与代码；不裁剪VS指令集变体或现有功能。兼容旧ZIP与组件更新，见 [打包指南](docs/portable-updates.md)。

2026-11-14 色彩复核见 [更新报告](development/color-audit-20261114.md)：官方最新main为`db94021`，HLG OOTF已修正；当前Full DLL复测线性整数RGB、P3→SDR、PQ显式709及CL仍未关闭。本轮仅审计，未更新固定构建基线或部署。

2026-11-13 上游/OpenNGU：Native 当前固定 `248cb9b`，可复现补丁与脚本见 [Native 构建](docs/native-api20-build.md)。API20 / snapshot23（608字节）镜像在 `backend/ThreeFpApi.h`。VSP 主上采样顺序 OpenNGU→Jinc→D3D11，第二上采样顺序 Jinc→D3D11→OpenNGU；默认第二上采样仍为 Jinc。路由在 `PlayerProfiles.cpp`，菜单在 `PlayerMenus.cpp`；VSR 在 `FilterCatalog.cpp` 的 shader 首项及 `HlslPreview.h` 接入，导出仍走 GLSL。用户提供的私有参数仅置于忽略目录 `.deps/openngu`，`stage-shaders.ps1` 在本地存在时复制到运行目录；缺失时播放器回退 Jinc。验收见 [本轮记录](development/upstream-openngu-20261113.md)。

界面文字手改入口见 [文案位置索引](docs/ui-text-map.md)。导航图标在 `ui/NavigationIcons.h`，统一尺寸与展开绘制在 `app/MainWindow.cpp::NavigationButton`；标准弹窗按钮翻译在 `player/PlayerLanguage.cpp`。Player 首帧过渡由 `PlayerIdleGuide.h` 默认隐藏、`PlayerWindow.cpp` 打开时隐藏与 `PreviewPane.cpp` 首次原生绘制清底协作。

2026-11-13 UI验收续修：Renderer构造完成后通过 `selectPage` 应用当前主页背景；Player空闲引导 `PlayerIdleGuide.h` 使用透明底色，避免遮挡已加载主题。精确帧说明在 `MainWindowTimeGroups.cpp`，GLSL预览/导出提示位于 `ParameterEditor.cpp` 参数底部；Tab括号由 `PlayerInfo.cpp` 与语言包统一。首次显示、参数位置和Tab定向测试见本轮验收报告。

Full 发行策略（2026-11-13）：`tools/stage-ffmpeg.ps1` 使用 `tools/ffmpeg-source.json` 固定的 full09 / dav1d 构建；`package-portable.ps1` 排除 LAV、madVR 和扩展 shader，保留在线组件接口并剥离发行程序符号。`stage-shaders.ps1` 默认只部署完整 Anime4K，MoeSR由内置资源提供；`package-optional-shaders.ps1` 生成完整扩展库，不自动上传；上传到 `ARXChem/Software-List` 的 `mpv-shaders/2026.11.13/VS-GUI-mpv-shaders-optional.7z`，组件页按版本目录发现、校验、安装到 `shaders` 并重启。可选资源不要从源码目录删除，也不要让后续构建重新填满默认包。

2026-11-13 合并另一设备源码：以 `39b07df` 三方合并并保留本机 Agent；重生成可叠加的音频/UNC补丁，本机完整重建 Native。`src/app/AgentVideoFrames.*` 为视觉 Agent 提供隔离 VS 按帧/秒读取、无损PNG与局部裁剪，不依赖屏幕截图；`MainWindowAgentFeatures.cpp` 约束先汇报缺陷再建议滤镜，`tests/TestAgentVideoFrames.cpp` 验证真实VS及模型图像往返。性能仅对照1080p48→2160p48，记录见 [合并验收](development/merge-20261113.md)。

2026-11-11 HLSL续作：MoeSR末11 pass FP32访问/累加重排、1X整数读取包、`ShaderPackage::enhance`受控同尺寸邻域Load已落地；五轮GPU分别约节时5%/9%，共享A+增强整链安装环境三轮约4.37%。Full/Lite共用1X路由并保留原生回退，Fast/noCNN其余候选未通过严格检查而保留原执行器。固定SHA生成器为`tools/optimize-moesr-tail.py`、`tools/package-anime4k-1x.py`，实际十档播放回归为`tests/HlslOptimizationAcceptance.inc`。Full安装EXE已备份更新，Native源码/DLL与用户INI不变，完整Native SDK仍未齐备。数值误差、基准自比较差异、部署和恢复证据见`docs/hlsl-chain-performance-20261111.md`与`development/evidence/hlsl-chain-20261111/`。

2026-11-11 780M实测：本轮用户已授权性能测试，覆盖此前临时禁测要求。旗舰A使用FP32重建/像素重排融合，GPU约15%节时；D3D12/SM6/FP16未胜出，不迁移。见`docs/780m-hlsl-performance-20261111.md`；生成器`tools/optimize-anime4k-mode-a.py`、回归`tests/TestAnime4KFusion.cpp`。Full安装EXE已备份更新，Native/设置不变，用户FFmpeg已恢复。

2026-11-10 晚间工作流改进：`app/ProjectProtection.inc`负责项目状态/恢复/Undo；`backend/DetachedExport.h`与`export-worker.cpp`负责独立持久编码队列，UI仅提交清单和控制命令、轮询状态（本机文件消息，不是网络RPC）。`FilterTimeGroup::startFrame`增加0基源帧边界，schema3。`cap/CaptureEncoding.h`定义AVIF格式/偶数边缘补齐；`TimelineThumbnail.h`独立单帧导航解码。Tab使用VS帧属性追踪，不改Native。D3D12仅研究见docs/d3d12-research.md。3FP上游色彩/性能及当前补丁本轮原样保留，禁止在NuxBox压制时跑性能基准。

2026-11-10 本地音频/UNC修复：`PlayerSettings` 在视频渲染器下一行加入默认3FP音频输出；`ThreeFpApi/ThreeFpPlayer` 复用端点/独占 ABI；`AudioEndpoints.h` 枚举稳定端点ID。`3fp-vsp-audio-unc.patch` 在原两层补丁后应用，源率策略和路径策略分别位于 `AudioFormatPolicy.h` / `LocalMediaPath.h`。测试及未完成的完整Native DLL/AVR/NAS验收见 `development/validation.md`，本轮交付源码而非已部署发行包。


2026-11-08 D3D12/8K48 AV1 可行性见[实测分析](development/d3d12-av1-feasibility-20261108.md)：纯硬解D3D11VA/D3D12VA约109/108 fps，软件路径仍未满足52 fps余量；未迁移实现，8K48软件逐帧实时问题保持未关闭。

色彩审计后续结论与实施边界见 [两轮讨论报告](development/color-audit-followup-20261107.md)：显示端元数据改列策略边界，VS桥实际影响仍待闭环，DV按用户要求强制回退，PQ原色结论限定为已测输入；目前未实施修复。

2026-11-07 色彩审计见 [覆盖报告](development/color-audit-20261107.md)：当前 DLL 已复现线性整数RGB、P3→SDR和PQ显式原色处理缺口，尚未修复。官方最新main已独立拉取并核实仍为`72af915`；`.deps/fff-vsp110`的本地整合提交`c8feda7`已包含它，另有未提交修改。七组关键色彩代码与最新上游一致；产物核验以报告中的DLL哈希为准。

核对日期：2026-11-14。源码版本 **1.1.3（Release 草稿，附件由维护者上传）**。Native 固定上游 main `248cb9bb0cef91922ab8f88ea7f6ef38ec0e311c`，使用 `patches/3fp-vsp-upstream-20261113.patch` 保留 VSP 扩展。使用步骤见 [docs](docs/README.md)，历史变化见 [changelog](changelog.md)，当前失败和验收边界见 [台账](development/validation.md)。

1.1.1 本地预览入口：`build/preview-1.1.1/launch-preview.cmd`；持久验证证据：`development/evidence/player-1.1.1/`。本次聊天工作文件已迁入 `build/session-20261107/`（本机归档，不参与发行）。

高 DPI 全屏底边控制栏定位于 `PlayerPlaylist.cpp::updateChrome`；Qt 全局坐标在 300% 缩放下可能把物理最后一像素舍入到逻辑边界外，仅全屏容许右/下各一个逻辑像素。Full/Lite 的 `fullscreenPhysicalBottomEdge` 使用 Win32 真实屏幕坐标覆盖左右端和中点；48F 专项入口为 `remote48FrameCadence`，由环境变量指定设备素材，不内置媒体路径。NuxBox 的 Tab 24F 报告未复现，用户复测已恢复。Full/Lite 内置 MoeSR 与 Anime4K 由 `PlayerProfiles.cpp::nativeBuiltinCompatible` 按内置档位识别，统一走原生 HLSL，不执行旧 VPY 正文，也不依赖其中的绝对路径或自定义 VPY 开关。需要执行编辑后的 Python 时，应另存到用户自定义预设。

本轮续修：`ShaderPackage.cpp`和`CustomShader.h`统一使用D3D11的16个采样器槽；包格式及旧缓存语义不变，Native须同步重编。`apply-update.ps1`同盘移动已准备的payload，异盘复制；`PlayerInfo`仅在Tab显示时查询GPU/功耗，`PlayerWindow`提前设置深色底色。单文件默认说明随包保存，首次启动写到配置旁，之后不覆盖手工修改。

历史续修记录：`src/render/ShaderPackage.cpp`增加单文件多pass HLSL导入；含`//!HOOK`的HLSL复用现有MAIN/PREKERNEL指令解析，每段独立编译并保留资源反射检查。普通单pass HLSL契约与Native二进制包格式不变，缓存语义键升级v7。`tests/TestShaderPackage.cpp`覆盖重复main的多段输入；本地独立产物`dist/VS-Player-MoeSR.exe`保留原`dist/VS-Player.exe`，未改版本或发布。

空置教程由`PlayerIdleGuide.h`绘制，默认数据为`assets/player-guide.json`；控件ID在`PlayerControls.cpp`定义，布局刷新仍由`PlayerPlaylist.cpp::updateChrome`驱动。外部配置复用滤镜说明JSON的`playerGuide`字段，Full在`vpy/`下，Single在启动器旁。文件监听支持保存后刷新，单文件携带说明默认配置，首次写出后已有外部文件优先。

滤镜菜单说明由`PlayerMenus.cpp`读取同一JSON：Jinc/D3D11沿用既有builtin键，其他算法共用`scaling/`键，分类标题用`menu/`键，补帧自动档独立使用`interpolation/auto/`键。`assets/player-menu-descriptions.json`提供缺失键默认值，由`ensureProfiles`补齐而不覆盖用户内容；菜单重新打开时读取最新说明。

父分类的实际悬停触发由`PlayerMenu.cpp`管理：`PlayerMenus`将JSON文字绑定到分类动作的`playerDescription`属性，悬停时主动刷新提示，进入子菜单、离开分类或关闭菜单后停止；不依赖系统生成延迟ToolTip事件。

本轮入口：`PlayerImageTools`负责比例尺寸、位深与Full GLSL/内置HLSL图片导出；Lite通过FFmpeg编码且关闭质量搜索。`PageTheme.h`使用独立底层QWidget绘图并即时刷新，跳过PreviewPane子树；`ThemePackage.h`管理主题包。`WrappedFileCombo.h`处理文件名换行；`PortableChart.h`将配置旁Pareto图保存为相对路径，并兼容单文件内嵌图。`tools/jxl-runtime.cmake`保留SIMD并修补Win64的向量跨函数传参对齐；版本未改变。字幕、滑块、功耗入口仍分别为`PlayerSubtitles`/Native `AssSubtitleRenderer`、`CompareView`和`PowerSensors`，功耗权限边界见 [研究记录](development/pawnio-power.md)。

发版前审查处置见 [审查复核](development/code-review-1.1.0.md)，实际功能回归及未关闭项见 [测试台账](development/validation.md)。外部 AI 包需确认执行指纹；网络链接只允许用户指定的 LAN 目标，跨源私有地址跳转拒绝。

HLSL 播放控制修复：所有内置 Anime4K/MoeSR 档位独立于自定义 VPY 的 HLSL 开关；不兼容时恢复 D3D11 控制，原生 HLSL 切换等待跳转完成并接纳内核已在播放的状态。复现入口为 `builtinHlslPlaybackControls`，覆盖拖入视频自动播放。

1.1.1 内置档位表位于 `PlayerBuiltinProfiles.h`：保留旧 ID 0–7，8/9 为完整 Mode A 的增强/普通档，10 为 MoeSR-50K；默认自动降档 8→0→6→4→5，MoeSR 可选但默认不参与。资源与预编译字节码位于 `assets/builtin-shaders/`，原始 MoeSR 与整数读取生成器分别保留在其 `source/` 和 `tools/optimize-moesr-hlsl.py`。`HlslPreview.h` 仅将支持的后置锐化/细节增强送到 `ShaderPackage::enhance`；其他后置滤镜继续保留 VS 顺序。Native `CustomShader.h` 必须配套重建。

后续版本化压缩包内部 Full 目录改为 `VS-GUI-Full`，Lite 为 `VS-GUI-Lite`；Full 使用 `tools/package-portable.ps1 -ProgramDirectory dist/VS-GUI-Full`，单文件使用 `tools/package-playerlite-single.ps1`。MoeSR 分类在 Anime4K 之前，Full/Lite 共用菜单与内嵌 HLSL 资源。

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
| Agent Qt 核心 + 独立 MCP Node 桥 | Chat Completions/SSE、按需Skills、摘要投影、子会话；Qt异步本地文件/命令/图像；MCP用官方client v2 | 不引入.NET/LakeUI或网页壳；环境工具由模式及父权限统一拦截，原生非兼容模型协议需单独适配 |
| MinGW GUI → Native 动态 C ABI | 隔离 MinGW/MSVC C++ ABI | 不跨工具链传 C++ 对象；升级必须核对结构版本和 DLL |
| FilterGraph → VPY → VSScript → 3FP | 脚本可审查，预览与导出共用处理描述 | 不以离线 vspipe/Y4M 管道充当实时帧桥 |
| 直通与 VS 路径分开 | 普通播放不等待 VS 索引 | 不无条件初始化全部增强依赖 |
| 3FP原生色彩默认 | SDR/HDR使用现有3FP处理；旧自动色彩档迁回原生，libplacebo仅由Full手动启用以对照mpv | 不根据HDR标记自动进入libplacebo；手动档与ICC/LUT参数保留 |
| 时间组独立 FilterGraph | 同片不同段落可使用不同链 | 拼接要求各组输出规格一致，不静默改变格式 |
| 专用时间轴与请求代次 | 管理 seek、补帧和源/输出 FPS | 不使用 seek 后局部帧号直接推算全局源帧 |
| 直通相对逐帧 | `PlayerWindow::stepFrame` 调用内核当前PTS逐帧；Native重新解码局部GOP确认相邻帧 | 不用绝对帧号逐帧，也不把稀疏缓存中的近邻当成真实相邻帧；VS仍按输出帧号定位 |
| 查看与编辑解耦 | PhotoCraft 独立进程保留未保存工作 | 旧 Qt 编辑器仍有源码/测试，不作为生产编辑入口 |
| 固定上游 + 完整 Native 补丁 | 保留本地扩展并可重新构建 | 不拼装 API14/18/20 不同版本 DLL 或补丁 |
| 比对源像素与显示值分开 | `AnalysisPage::updatePixelReadouts` 按源坐标联动；`SourcePixelProbe.h` 和 Native 比对扩展提供转换前码值 | YUV 不由渲染 RGB 反算；交互重绘不继承静态图片媒体帧率；缩放移除固定倍率上限，Native 裁剪视口并映射 UV |
| 启动期库预读 | `src/player/main.cpp` 用128KiB缓冲在低优先级线程读取FFmpeg；单文件解包后的Qt子进程共用 | 不照搬.NET图标和LakeUI设备预热，不额外提高进程优先级；首次解包单独计时 |
| 软件播放同步与内存预算 | 上游线程/帧池/CPU 域按负载逐级调整；音频从60ms起按告警升至240ms，呈现允许最多250ms的平滑节奏容差；保留VSP的8K关键包参考帧追赶 | 不把显示器限帧记为解码丢帧；CPU预缩放仍默认关闭，8K逐帧实时按素材验收 |
| AI 字幕外置推理 | 固定 main + #36/#37 Native 音频 tap；Qt 有界传 PCM 到外置 worker，跳转按 generation 隔离 | 模型独立打包并由组件下载器安装；保留旧 GGUF 配置兼容，不移植上游 VB 页面 |
| Shader 平面输出 | `third_party/vsr-placebo` 直接回读 YUV 平面、合并增强 hooks，独立命名空间与 color 依赖 | 保留标准 placebo 插件作为缺少组件时的兼容路径；不把 VS 吞吐当作 Native 播放帧率 |
| Full/Lite 原生 Anime4K | `src/render/Anime4KFast.h` 执行 Fast 27、no CNN 16、亮度增强 2 个转换 pass；算法 9～14 对应六种增强变体，1X 按源尺寸处理后复用原生缩放器 | 内置字节码更新仍须重建并验证像素；Full/Lite 内置档位不回退 GLSL；不兼容色彩或模型失败时明确提示并退回 D3D11，保留播放控制 |
| 通用 GLSL/HLSL 导入 | `ShaderPackage` 编译并缓存，`CustomShader.h` 执行多 pass；兼容文件无需重建 DLL，播放不引入 VS/Python 依赖 | 当前支持 LUMA/MAIN/PREKERNEL 像素 pass、参数默认值与 gather；NATIVE 只提供尺寸，计算/外部纹理等明确拒绝，详见 Player 指南 |
| Full/Renderer 提取末尾 shader | 核对本项目图元数据与实际 Python 正文，再将兼容的末尾节点交给原生执行器 | 前序滤镜仍由 VS 执行；不兼容时回退原 GLSL，编码导出保留完整 VPY，不实现任意 VPY→HLSL |

## 4. 目录地图与改动入口

| 目录 | 职责与关键入口 |
| --- | --- |
| `src/app/` | Renderer：`MainWindow.cpp`、`MainWindowTimeGroups.cpp`、`MainWindowPresets.cpp` |
| `src/agent/` | `AgentSession` 管工具循环、权限与待接收输入，`AgentContext` 管用量与摘要；`AgentLocalFiles`/`AgentLocalProcess`/`AgentLocalImages` 管本机能力，`AgentImageMessages` 管图像校验/投影；`AgentProvider`/`AgentModelCapabilities` 管来源/模型；`AgentInput` 路由命令/引用；`AgentInstructions` 管指令/Skills；`AgentMcp`/`AgentNodeRuntime` 管MCP进程；`AgentTaskQueue` 管子会话，`AgentToolLibrary` 管组合 |
| `tools/agent-runtime/` | 官方 MCP client 锁文件与 Node JSONL 桥；测试服务器只供开发，`tools/stage-agent-runtime.ps1` 安装便携生产运行时与许可 |
| `src/graph/` | 滤镜定义/图/预设：`FilterCatalog.cpp`、`VpyScriptBuilder.cpp`、`PresetStore.cpp` |
| `src/backend/` | `VapourSynthFrameServer`、`ThreeFpPlayer`、`ThreeFpApi`、`FrameTimeline.h`、导出与进程 |
| `src/ui/` | `PreviewPane`、`CompareView`、`AnalysisPage`、参数、预设社区及导出窗口 |
| `src/player/` | `PlayerWindow`、`PlayerControls`、`PlayerProfiles`、字幕/音频/设置/图片；`PhotoCraftEditor.*` 连接编辑器 |
| `src/cap/` | `main.cpp` 负责截图/标注/贴图，`DesktopCapture` 捕获，`UiAccess` 启动 |
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

1.1.0 改动入口：`src/app/MainWindowRoutes.cpp` / `RenderRoute.h` 管理处理路；`PlayerDecodeBuffer.cpp` 管理按需软解缓冲（常规8帧、尖峰最多48帧/4GiB，包含解码流水线，预算仅是上限）；`PlayerShortcuts.h` 定义快捷键；`ui/PageTheme.h` 复用页面背景；`assets/ncnn-filter.py` 仅处理 Renderer 单帧，实时 Player 注入跳过标志。AI 音频桥仍为 `PlayerAiSubtitles.cpp`，包说明见 [组件指南](assets/ai-subtitles/README.md)。

## 5. UI 地图

Renderer手动预览入口为`MainWindow::validateScript`；加路、换源和载入预设不触发预览，`RenderRoute::reset`清理旧输出。`CompareView`按处理路数维护标题。Player的`setMinimalMode`使用待恢复时间戳重新打开解码器，保留播放/暂停意图；底栏提示区仅占一行。

| 入口与导航 | 文件 |
| --- | --- |
| Renderer：导入 → 时间组/处理链 → 参数/脚本 → 源与输出预览 → 预设/导出 | `src/app/MainWindow*.cpp`、`src/ui/ParameterEditor.cpp`、`CompareView.cpp` |
| Agent：发送键旁来源/模型/思考 → 对话；功能 → Skills/AGENTS、MCP/npm；自动队列 → 子会话；Token → 上下文整理 | `AgentPage.*`/`AgentPageDialogs.cpp`组装页面；`AgentComposer`管输入预览/补全，`AgentChoicePopup`管三类圆角选择浮层，`AgentMarkdownView`管消息排版与轮次过程折叠；`MainWindowAgent.cpp`注册工具，`MainWindowAgentFeatures.cpp`连接服务，`MainWindowAgentInteractions.cpp`路由命令/引用/能力；索引4，背景键`VSR-Agent` |
| 图像分析比对：多源 → 偏移/布局 → 同坐标 RGB/原始 YUV → 对比画布导出 | `src/ui/AnalysisPage.cpp`、`MultiCompareView.cpp`、`AnalysisExport.cpp`；OpenNGU 经 `ShaderPackage` 加载，缩小默认 D3D11；该核暂不支持一致的 VS 画布导出；Native 扩展见 `patches/3fp-api20-analysis-probe.patch` |
| BD Remux：节目识别 → 轨道/输出 → 队列 | `src/bluray/BlurayWidget.cpp` |
| Player：打开媒体 → 底栏/列表 → 右键音频/字幕/设置 → 图片工具 | `src/player/PlayerWindow.cpp`、`PlayerMenus.cpp`、`PlayerSettings.cpp` |
| Player色彩：解码设置 → 3FP默认 / 手动libplacebo对照；旧自动档迁回原生 | `src/player/PlayerColor.cpp`、`PlayerWindow.cpp` |
| PhotoCraft：图片工具栏 → 独立编辑工作区 | `src/player/PhotoCraftEditor.*`、`tools/build-photocraft.ps1` |
| VSCap：热键 → 选区调整 → 标注 → 最终裁切/复制/保存/贴图 | `src/cap/main.cpp` |

## 6. 外部依赖

| 依赖 | 固定版本 / 准备与调用 | 缺失行为 |
| --- | --- | --- |
| Qt | 6.8+；本地记录 6.10.2，MinGW；CMake 查找 | 编译或 GUI 启动失败 |
| Agent Node/npm/MCP | Node 24.19.0、npm 11.17.0、MCP client/core 2.4.0；`stage-agent-runtime.ps1`固定生产包到`runtime/agent`，JSONL桥提供stdio/Streamable HTTP | 缺少组件时MCP设置可安装/修复；模型对话与内置软件工具独立，许可与完整性见运行时`components.json` |
| VapourSynth / Python | VS API4 / R81；构建和 stage 脚本，`update-vapoursynth-runtime.py` 锁定哈希 | VS 链不可用；直通路径独立 |
| VS 插件 | BM3D 11.0、BM3Dvk2 1.0、BestSource 22；其余随运行时保留 | 对应滤镜不可用；不能用 API3 DLL 替代 |
| FFF.Native | API20 / 固定 `248cb9b`；标准脚本应用 `3fp-vsp-upstream-20261113.patch` 并复制本库头文件；snapshot v23 / 608 字节、外部帧v2 / 240字节，动态 C ABI | 原生播放/呈现不可用；旧内核无法保留新增帧信息时明确返回不支持 |
| FFmpeg | full09（2026-11-13），AVX2；shared ABI：63/63/61/7/10/12，来源哈希见 `tools/ffmpeg-source.json`；编译原生VS源需设置 `VSR_FFMPEG_ROOT` | 相应解码/导出失败，单文件启动器先检查组件；SDK缺失时CMake明确失败 |
| GLSL / RIFE | 随包 shader 与 NCNN 模型，`stage-shaders.ps1` / `install-analysis-plugins.ps1` | 相应增强不可用；RIFE 需要 Vulkan |
| GLSL 转换组件 | `runtime/shader-tools/` 中的 glslc、SPIRV-Cross 及配套依赖，由 `tools/stage-shader-tools.py` 整理；HLSL 用 D3DCompiler 编译 | 缺失时不能导入 GLSL；原生 HLSL 不调用 GLSL 转换工具，逐帧播放仅执行已编译字节码 |
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
| Player 配置 | `player.ini` / `PlayerSettings` | 保留用户显式选择；单文件放启动器旁 |
| 自定义 shader | 配置旁 `shaders/custom` 或 `render/shaderDirectory`，所选源为 `render/customShader`；子目录 `cache` 存 `.vsrhlsl` | 格式 1、语义版本 v8（参数默认值与 gather；兼容已验证的 v7 字节码）；源内容、扩展名、转换器及 D3DCompiler 标识参与缓存键，结构损坏重建；导入失败保持旧选择 |
| Renderer / 社区设置 | QSettings；`community/sourceUrl`、`appearance/dark`、`appearance/language` | 源/主题/语言在同一滚动页；主题即时生效 |
| Agent 来源/会话/工具组合 | `QStandardPaths::AppDataLocation/agent/` 中的`providers.json`、`conversations/`、`tools.json`；选择与禁用工具用QSettings | JSON schemaVersion=1；来源密钥DPAPI加密，导出来源省略密钥；会话导入检查工具配对 |
| Agent 指令/插件/队列 | 同目录`AGENTS.md`、`skills/`、`skills.json`、`mcp.json`、`tasks.json` | 技能先索引后读取；MCP凭证DPAPI、导入默认断开；队列导入/重启后手动恢复；摘要只改变发送投影，不删除原始消息 |
| Agent 待接收输入/模型能力 | 会话JSON中的`pendingMessages`保留原文/资料与来源，导入暂停；能力仅内存缓存 | API投影过滤本地显示字段；工具配对完成后接收输入/图像；能力绑定端点+模型，仅决定思考字段与视觉许可，不能覆盖模型或消息 |
| Agent 环境权限/图像 | QSettings `agent/accessMode`、`agent/workingDirectory`；图像保留在会话`content`规范块中 | 默认software，full由用户选择；导入会话/队列不提升权限或恢复进程；图像来源需能力声明，摘要剥离编码数据、请求保留近期图像 |
| 说明 / 命令 / AI | `vpy/preset-descriptions.json`；QSettings `export/commandHistory`；`player.ini` 的 `ai/package` | 说明只补空键；命令历史最多 50 条；模型路径指向外置清单 |
| 临时脚本/索引 | QStandardPaths、媒体索引缓存 | 可再生；导出冻结脚本、源和参数，不删除用户 VPY |
| 分发元数据 | `release.json`、资源/组件清单 | 版本、根目录和文件哈希须对应实际产物 |

## 8. 分层与高风险区

依赖方向：graph 不依赖 UI；backend 管帧、原生和进程；app/ui/player/cap 管交互。不要把算法直接塞入按钮回调。

2026-10-17 抽样：`MainWindow.cpp` 1193 行 / 60 KB，`src/cap/main.cpp` 888 行 / 59 KB，`PlayerWindow.cpp` 645 行 / 62 KB，旧 `ImageEditorWindow.cpp` 377 行 / 97 KB。行数与字节数只是职责审查信号；确需拆分时按交互状态、媒体操作、持久化和独立算法划分，不按行数机械拆分。

2026-11-08 Agent：Session约610行，LocalFiles约310行、LocalProcess约390行；权限派发仍属于Session，本地I/O与图片协议已独立。继续扩展时优先保持API/工具/I/O方向，不向主窗按钮填入文件或进程实现。

高风险包括 DLL/FFmpeg ABI、HDR 白位/色域/位深、seek 时钟和交接线程生命周期、超大图内存/ICC/Alpha、更新目录交换。涉及这些内容时需要实际媒体/GPU 像素或交互验证，编译成功与离屏布局不能代替实机结果。

## 9. 更新、测试与发布

1. 明确变更范围，复现问题；修改相关模块，构建并运行对应测试。测试选择与结果写入 [版本测试台账](development/validation.md)，步骤见 [构建](docs/dependencies.md) / [测量](docs/performance-debugging.md)。
2. Native 用 API20 固定脚本和补丁；历史 API14/18 脚本只用于对应基线。GUI 使用 CMake preset。
3. 本轮更新现有 `dist/VS-GUI-Full` 和 `dist/VS-Player.exe`，保留用户预设与色彩文件；不创建新的发行目录。精简播放器暂存在`build/playerlite-runtime-1.1.3`，不再生成PlayerLite发行目录。`tools/deploy-local-runtime.py`增量部署并核对INI/VPY/ICC/LUT哈希；它会修改运行目录，文档整理时不运行。
4. 打包前明确Full、历史Lite或当前PlayerLite；`package-portable.ps1 -Edition Lite`是历史Full删减版，不能充当当前PlayerLite。单文件从当前版本暂存（本轮`build/playerlite-runtime-1.1.3`）用`package-playerlite-single.ps1`生成tar.gz level 1负载，启动后临时解包、退出清理；Full/Single均排除themes目录及用户主题INI。Full压缩包按当前CMake版本生成（本轮`dist/1.1.3.7z`）；只创建Release草稿，附件由维护者上传。完整流程见 [便携更新](docs/portable-updates.md)。
5. 解压、依赖、启动、配置和清单验证后才可报告包完成；源码推送、草稿、附件上传、正式发布分别记录，需要相应任务授权。

文档纪律：结构/决策变化当天更新本地图；操作变化只改对应 docs 指南；用户可见变化写 changelog；测试按版本与轮次更新同一台账，不新建 `docs/update-版本.md`、`validation-版本.md`。原始日志留 `build/`，必要证据放 `development/evidence/`；不携带用户媒体。README、本地测试数据和开发记录不得混入上游 PR。

## 10. 当前状态

1.1.3 汇总两轮色彩审查修复、原生VS源、OpenNGU预览入口、VSCap偶数选区与Full精简。Full和单文件需配套新Native；发行包为`dist/1.1.3.7z`与`dist/VS-Player.exe`。本轮只准备Release草稿，维护者手动上传附件；验收见台账。

2026-11-08：Agent 第五轮加入软件内操作/环境完全访问、工作目录、异步文件搜索编辑/补丁、命令与后台进程、真实图像消息；蓝色用户消息采用真实圆角绘制。子会话继承模式、目录和父禁用工具，只读任务禁止命令/写入；降权或离开会话清理进程。此前Markdown、能力匹配、过程折叠、运行中追加、`/`/`@`及自动协作保留。16组专项均取得通过结果，真实DeepSeek完成本地文件→Node校验→只读子会话复核；初次HTTP/2断连和原二进制重测均留证。Full隔离PATH下文件/进程/图像/界面已验证。模型协议仍限Chat Completions，视觉需端点明确声明；使用与边界见[指南](docs/agent.md)，验证及本地部署见[台账](development/validation.md)。

下一轮按用户要求将Lite配置封装进入压缩包；本轮只补齐Full菜单说明并覆盖本地Full，未实施该打包变化。

1.1.0 已落地三段缩放、多路输出、单帧NCNN、主题、音量/快捷键和动态缓冲；本轮追加图片导出与主题可见性修复，同步现有Full和单文件。指定背景图已在Renderer四页及Player主界面/设置实窗验收，图片位深和Lite FFmpeg路径专项通过；JPEGli后端仅8位。此前GBC指定尖峰已完成一次0丢帧复测，CPU功耗仍因驱动权限显示N/A；全量回归与长期播放没有通过结论，详见台账。

1.0.15 同步近期3FP内核和可迁移的启动优化，保留多轨视频硬解试探修复；单文件使用更高压缩率。Full输出至 `dist/VS-GUI-Full`，单文件为 `dist/VS-Player.exe`；具体测试范围、启动采样及限制见测试台账。运行中的VSCap原文件保留，发行压缩包与附件由维护者手动制作、上传。

1.0.14 在固定 API20 主线补丁上迁入 AI 音频 tap 与字幕 HDR，Qt 接入 RTX VSR/TrueHDR、外置 AI 字幕、八档与组合处理、Renderer 主题设置、3FUI 命令桥及 Cap 设置改进。Player 设置新增独立字幕页；渲染页完整列出 Anime 档位，Jinc/D3D11 补帧分别保存参与列表；其他缩放算法限纯 D3D11 直通，Tab/底栏读取实际路径。ParameterEditor 跟随全局主题。PlayerControls 显示音量百分比和状态图标，均衡器由 PlayerAudio 创建与倍速互斥的 Popup，并在展开时保持控制栏可见。入口分别为 `PlayerAiSubtitles`、`PlayerProfiles`、`MainWindow`、`ExportWindow`、`src/cap/main.cpp`；插件源码在 `plugins/3fui-export-bridge`，单文件更新在 `SinglePlayerUpdater`。保留 VS 帧桥、BD 定位、音频效果和色彩桥；普通声床没有新增内置双耳渲染器。原理见[性能排查](docs/performance-debugging.md#上游内核优化)，逐项状态及实测见[台账](development/validation.md)。本轮更新本地产物并同步源码，单文件 `dist/VS-Player.exe` 同步到 1.0.14；准备 Release 草稿，发行压缩包与附件由维护者手动制作、上传。

1.0.13 增加安装进程清理、压缩运行时按需展开与单文件BD定位显示保持；列表/右键共用返回菜单入口，原生菜单页禁用时间轴，同盘重开复用盘信息，原生会话同步销毁后再替换窗口。VSCap续修将颜色窗口归属到真实工具栏窗口，避免贴图拖动模式拦截颜色交互；已发布的1.0.13附件不自动包含续修，重新分发须核对哈希。具体测试见台账。此前1.0.12 已有 BD 全卷定位/按段索引、Jinc 色彩、无音轨 VS 时钟、Anime 预取与软件内核的聚焦测试记录；**未运行全量 Full 播放器套件**。后轮结果只覆盖其实际测试范围，不能把同版多个 passed 数字相加为一次总回归。

当前仍未完成：

1. 在7940HX上的8K48 软件解码逐帧实时播放（本轮以丢帧追赶墙钟为目标，不视为逐帧实时通过；新一轮结果见测试台账）

2. 在7840HS/780M核显上的Anime4K CNN 24F 1080p--2160p（NO CNN模式已经通过）

3. 14700K P/E 调度分支

4. IAMF/OBR 真实空间音频听感

1.0.14 语言检查通过。新增验收边界：AI 未配置真实模型、3FUI 未操作真实宿主按钮、管理员计划任务未实际启用；i5-9500/1650 Super 无远程入口。BT.2020 CL 原生路径的既有断言失败在旧 DLL 同样复现，尚未关闭。逐项依据及跨版本变化见 [台账](development/validation.md)。

下一步按用户指定设备和素材复测上述未完成项。不要把 1.0.10/11 的旧失败继续写成 1.0.12 已修复路径的当前结论，也不要从工作区版本推断外部 Release 状态。

## 11. 故障排查

| 症状 | 先检查 |
| --- | --- |
| Qt 配置或构建失败 | `QT_ROOT`、MinGW PATH、CMake preset；[构建指南](docs/dependencies.md) |
| Native 黑屏/ABI 错误 | API20 补丁、FFmpeg shared DLL、`VSR_3FP_DLL`；[原生构建](docs/native-api20-build.md) |
| VS 缺插件/预览失败 | API4 运行时、模型和 `VSR_VSSCRIPT_DLL`；[Renderer](docs/renderer.md) |
| GLSL/HLSL 导入或执行失败 | `runtime/shader-tools`、Native 自定义扩展、完整编译错误及 hook/色彩/尺寸边界；[Player](docs/player.md#当前支持边界) |
| seek/音画时间不同步 | `FrameTimeline.h`、`PlayerSession`、请求代次；[性能排查](docs/performance-debugging.md) |
| HDR 色偏/截图异常 | `src/color/`、`HdrPreview.h`、源屏幕 SDR 白位；[色彩](docs/color-management.md) |
| VSCap 模态/文字/贴图异常 | `src/cap/main.cpp`；[截图](docs/capture-images.md) |
| BD 菜单或跨段失败 | `BlurayCatalog`、concat 定位、片段索引；[蓝光](docs/bluray.md) |
| 包缺 DLL/升级失败 | 分发形式、必需清单、包根目录与备份；[更新](docs/portable-updates.md) |
