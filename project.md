# VS Renderer / VS Player：项目开发地图

核对日期：2026-11-07。源码版本 **1.1.0（本地开发，未发布）**。Native 固定上游 main `72af915f8bf2567155c1f87851a6fa263f73b14f`，使用 `patches/3fp-vsp-1.1.0.patch` 保留 VSP 扩展。使用步骤见 [docs](docs/README.md)，历史变化见 [changelog](changelog.md)，当前失败和验收边界见 [台账](development/validation.md)。

历史续修记录：`src/render/ShaderPackage.cpp`增加单文件多pass HLSL导入；含`//!HOOK`的HLSL复用现有MAIN/PREKERNEL指令解析，每段独立编译并保留资源反射检查。普通单pass HLSL契约与Native二进制包格式不变，缓存语义键升级v7。`tests/TestShaderPackage.cpp`覆盖重复main的多段输入；本地独立产物`dist/VS-Player-MoeSR.exe`保留原`dist/VS-Player.exe`，未改版本或发布。

本轮入口：`PlayerImageTools`负责比例尺寸、位深与Full GLSL/内置HLSL图片导出；Lite通过FFmpeg编码且关闭质量搜索。`PageTheme.h`使用独立底层QWidget绘图并即时刷新，跳过PreviewPane子树；`ThemePackage.h`管理主题包。`WrappedFileCombo.h`处理文件名换行；`PortableChart.h`将配置旁Pareto图保存为相对路径，并兼容单文件内嵌图。`tools/jxl-runtime.cmake`保留SIMD并修补Win64的向量跨函数传参对齐；版本未改变。字幕、滑块、功耗入口仍分别为`PlayerSubtitles`/Native `AssSubtitleRenderer`、`CompareView`和`PowerSensors`，功耗权限边界见 [研究记录](development/pawnio-power.md)。

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
| Full/Lite 原生 Anime4K | `src/render/Anime4KFast.h` 执行 Fast 27、no CNN 16、亮度增强 2 个转换 pass；算法 9～14 对应六种增强变体，1X 按源尺寸处理后复用原生缩放器 | 内置字节码更新仍须重建并验证像素；Full 对修改过正文或不兼容的内置 VPY 保留原执行路径 |
| 通用 GLSL/HLSL 导入 | `ShaderPackage` 编译并缓存，`CustomShader.h` 执行多 pass；兼容文件无需重建 DLL，播放不引入 VS/Python 依赖 | 当前支持 LUMA/MAIN/PREKERNEL 像素 pass、参数默认值与 gather；NATIVE 只提供尺寸，计算/外部纹理等明确拒绝，详见 Player 指南 |
| Full/Renderer 提取末尾 shader | 核对本项目图元数据与实际 Python 正文，再将兼容的末尾节点交给原生执行器 | 前序滤镜仍由 VS 执行；不兼容时回退原 GLSL，编码导出保留完整 VPY，不实现任意 VPY→HLSL |

## 4. 目录地图与改动入口

| 目录 | 职责与关键入口 |
| --- | --- |
| `src/app/` | Renderer：`MainWindow.cpp`、`MainWindowTimeGroups.cpp`、`MainWindowPresets.cpp` |
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
| `tests/` | 可执行回归测试源码与小型 fixtures，保留按模块划分 |
| `docs/` | 按任务组织的现行操作指南，不放逐版本测试/发布流水 |
| `development/` | 源码工作区测试台账、合并历史原文与证据；不随运行包复制 |
| `.deps/`、`build/`、`dist/` | 忽略的依赖、构建/原始日志、本地运行产物；不等于已发布附件 |

1.1.0 改动入口：`src/app/MainWindowRoutes.cpp` / `RenderRoute.h` 管理处理路；`PlayerDecodeBuffer.cpp` 管理按需软解缓冲（常规8帧、尖峰最多48帧/4GiB，包含解码流水线，预算仅是上限）；`PlayerShortcuts.h` 定义快捷键；`ui/PageTheme.h` 复用页面背景；`assets/ncnn-filter.py` 仅处理 Renderer 单帧，实时 Player 注入跳过标志。AI 音频桥仍为 `PlayerAiSubtitles.cpp`，包说明见 [组件指南](assets/ai-subtitles/README.md)。

## 5. UI 地图

| 入口与导航 | 文件 |
| --- | --- |
| Renderer：导入 → 时间组/处理链 → 参数/脚本 → 源与输出预览 → 预设/导出 | `src/app/MainWindow*.cpp`、`src/ui/ParameterEditor.cpp`、`CompareView.cpp` |
| 图像分析比对：多源 → 偏移/布局 → 同坐标 RGB/原始 YUV → 对比画布导出 | `src/ui/AnalysisPage.cpp`、`MultiCompareView.cpp`、`AnalysisExport.cpp`；Native 扩展见 `patches/3fp-api20-analysis-probe.patch` |
| BD Remux：节目识别 → 轨道/输出 → 队列 | `src/bluray/BlurayWidget.cpp` |
| Player：打开媒体 → 底栏/列表 → 右键音频/字幕/设置 → 图片工具 | `src/player/PlayerWindow.cpp`、`PlayerMenus.cpp`、`PlayerSettings.cpp` |
| Player色彩：解码设置 → 3FP默认 / 手动libplacebo对照；旧自动档迁回原生 | `src/player/PlayerColor.cpp`、`PlayerWindow.cpp` |
| PhotoCraft：图片工具栏 → 独立编辑工作区 | `src/player/PhotoCraftEditor.*`、`tools/build-photocraft.ps1` |
| VSCap：热键 → 选区调整 → 标注 → 最终裁切/复制/保存/贴图 | `src/cap/main.cpp` |

## 6. 外部依赖

| 依赖 | 固定版本 / 准备与调用 | 缺失行为 |
| --- | --- | --- |
| Qt | 6.8+；本地记录 6.10.2，MinGW；CMake 查找 | 编译或 GUI 启动失败 |
| VapourSynth / Python | VS API4 / R81；构建和 stage 脚本，`update-vapoursynth-runtime.py` 锁定哈希 | VS 链不可用；直通路径独立 |
| VS 插件 | BM3D 11.0、BM3Dvk2 1.0、BestSource 22；其余随运行时保留 | 对应滤镜不可用；不能用 API3 DLL 替代 |
| FFF.Native | API20 / main `72af915`；标准脚本应用 1.1.0 汇总补丁并复制本库头文件；配置 112 字节、snapshot v22 / 592 字节，动态 C ABI | 原生播放/呈现不可用；缺少可选扩展的旧库不能提供原始 YUV 或自定义 shader |
| FFmpeg | 当前 shared ABI：63/63/61/7/10/12；Native SDK 与运行 DLL 配套 | 相应解码/导出失败，单文件启动器先检查组件 |
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
| 说明 / 命令 / AI | `vpy/preset-descriptions.json`；QSettings `export/commandHistory`；`player.ini` 的 `ai/package` | 说明只补空键；命令历史最多 50 条；模型路径指向外置清单 |
| 临时脚本/索引 | QStandardPaths、媒体索引缓存 | 可再生；导出冻结脚本、源和参数，不删除用户 VPY |
| 分发元数据 | `release.json`、资源/组件清单 | 版本、根目录和文件哈希须对应实际产物 |

## 8. 分层与高风险区

依赖方向：graph 不依赖 UI；backend 管帧、原生和进程；app/ui/player/cap 管交互。不要把算法直接塞入按钮回调。

2026-10-17 抽样：`MainWindow.cpp` 1193 行 / 60 KB，`src/cap/main.cpp` 888 行 / 59 KB，`PlayerWindow.cpp` 645 行 / 62 KB，旧 `ImageEditorWindow.cpp` 377 行 / 97 KB。行数与字节数只是职责审查信号；确需拆分时按交互状态、媒体操作、持久化和独立算法划分，不按行数机械拆分。

高风险包括 DLL/FFmpeg ABI、HDR 白位/色域/位深、seek 时钟和交接线程生命周期、超大图内存/ICC/Alpha、更新目录交换。涉及这些内容时需要实际媒体/GPU 像素或交互验证，编译成功与离屏布局不能代替实机结果。

## 9. 更新、测试与发布

1. 明确变更范围，复现问题；修改相关模块，构建并运行对应测试。测试选择与结果写入 [版本测试台账](development/validation.md)，步骤见 [构建](docs/dependencies.md) / [测量](docs/performance-debugging.md)。
2. Native 用 API20 固定脚本和补丁；历史 API14/18 脚本只用于对应基线。GUI 使用 CMake preset。
3. 本轮直接覆盖现有 `dist/VS-GUI-Full` 和 `dist/VS-Player.exe`，保留用户预设与色彩文件；仅按任务修改主题及Pareto配置，不创建新的发行目录。精简播放器暂存在`build/vsp110-playerlite-runtime`，不再生成PlayerLite发行目录。`tools/deploy-local-runtime.py`增量部署并核对INI/VPY/ICC/LUT哈希；它会修改运行目录，文档整理时不运行。
4. 打包前明确 Full、历史 Lite 或当前 PlayerLite；`package-portable.ps1 -Edition Lite` 是历史 Full 删减版，不能充当当前 PlayerLite。单文件从构建暂存用 `package-playerlite-single.ps1` 生成；完整流程见 [便携更新](docs/portable-updates.md)。
5. 解压、依赖、启动、配置和清单验证后才可报告包完成；源码推送、草稿、附件上传、正式发布分别记录，需要相应任务授权。

文档纪律：结构/决策变化当天更新本地图；操作变化只改对应 docs 指南；用户可见变化写 changelog；测试按版本与轮次更新同一台账，不新建 `docs/update-版本.md`、`validation-版本.md`。原始日志留 `build/`，必要证据放 `development/evidence/`；不携带用户媒体。README、本地测试数据和开发记录不得混入上游 PR。

## 10. 当前状态

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
