# VS Renderer / VS Player

## 1.0.2 完整更新与便携发布（2026-10-01）

播放器滚轮缩放后，左上角“还原画面”恢复默认适配并清除拖动偏移，普通窗口 / 全屏都可用。Renderer 设置和 Player 基本设置增加版本与检查更新，读取 ModelScope `ARXChem/Software-List/VS-GUI` 最新完整包；随附已核验的 aria2-next 2.8.3 与 7-Zip Zstandard。下载、哈希与解压异步执行，验证后完整替换目录，保留配置、自定义 VPY 和旧目录备份，不降级安装旧版本。

包由独立干净目录生成，排除缓存、测试程序、截图、个人配置、历史 VPY 与未使用的 madVR 注册/重置脚本；活跃程序目录的用户文件保留。输出 `dist/1.0.2.7z`（Zstandard Ultra 22）与 SHA-256，使用本机 ModelScope-Manager SDK 上传指定目录。GitHub 源码更新及无附件 Release 草稿；压缩包不自动传 GitHub。规则与结构见 [完整更新说明](docs/portable-updates.md)。

本轮 Release 构建、完整 CTest **8/8**（含更新专项）、缩放还原与全屏测试通过。构建后验证真实便携目录的程序与更新工具，再进行归档完整性和远程大小/哈希核对。语言包同步新增字符串。此前各轮“不发 Git / 不打包”属于历史阶段，本轮以最新授权发布源码、上传 ModelScope 和创建草稿。

从源码复现更新专项测试前运行 `powershell -File tools/stage-update-tools.ps1` 部署对应工具；二进制和原生播放运行时仍按依赖文档准备。

本轮验证：CTest **7/7**、交付 Player **26/26**（含两个真实链接）。本地版本继续保持 **1.0.2**，输出在 `dist/VS-Renderer-GUI-1.0.2-windows-x64`，包含 `VSRenderer.exe` 与 `vs-player.exe`。不推送 Git、不生成压缩包；本轮记录见 [播放器性能与验证](docs/player-performance-1.0.2.md)。

HTTP / HTTPS 自动走 **3FPlayer 原生直通，不执行 VS / VPY**，无需完整下载或建立 VS 索引；保留 Jinc 等显示缩放。AList 文件 API 解析和 ModelScope 重定向仍使用独立 Qt 网络线程与有界 HTTP Range 代理。播放器的本地 FFMS2 / L-SMASH 寻帧索引默认保存在软件 `cache/indexes`，可选自定义位置、查看大小及清除；不在视频旁新建索引。Renderer 自身源配置不受播放器缓存设置影响。

开发者内置 **Anime / Realistic** 位于 `vpy/builtin`。Anime 低于 2160p 的源先 GPU 细节增强再 A/M Fast，直接输出到实际画布尺寸（最大 3840×2160，保留宽高比）。持续丢帧超过 5% 按增强 → A/Fast → Jinc 直通 → D3D11 Native 降载；Realistic 和 4K 源从 Jinc 原生直通开始。暂停、跳转、改速、调整窗口或分辨率之后重新统计，恢复 2 秒后再观察 5 秒，不把操作停顿用于降载。“先 Jinc 再增强”修复 10-bit 输入报错；已有带内置标记的已知脚本仅补位深转换，先留 `.before-bitdepth-fix` 备份，其他用户脚本不改写。

播放列表进入右侧整个范围即可悬停打开；右键“播放列表”可永久停靠并扩宽窗口，拖动分隔栏调整宽度，× 关闭；全屏改为悬停，退出恢复停靠。长文件名随宽度中间截断，悬停可见完整路径。右键滚轮切换音量（默认）/ 视频缩放；Tab 尺寸按当前画布物理像素与缩放更新，菜单箭头增加间距。

本机标准功耗模式下，真实 3840×2160 / 47.95 fps AV1 的原生路径 8 秒样本呈现 383 帧，丢弃 / 合并为 0；不代表长片或其他硬件保证。MyGO 的 96 帧配对暖态：A/VL 到 2160p 24.18 fps，A/M Fast 到 2160p 31.09 fps，直接到 1264×710 画布 97.17 fps。用户图中的 Fast 5 ms 未标明版本 / 测法；早期非 CNN 与当前 v4.x A/M 也不能混为一谈，见 [性能记录](docs/player-performance-1.0.2.md) 和 [Fast 来源](docs/anime4k-fast-source.md)。

Windows 上用图形化滤镜链生成 VapourSynth 脚本，并以双路画面对照源视频与处理结果的 Qt 6 桌面工具。

> 当前版本：`1.0.2`（继续开发中）。包含 VS Renderer 与 VS Player；本轮仅更新本地程序目录。


## 1.0.2 本地程序与预设

本轮输出到 `dist/VS-Renderer-GUI-1.0.2-windows-x64`，包含更新后的 `VSRenderer.exe` 和独立播放器 `vs-player.exe`。只生成便携目录，不推送 GitHub、不生成压缩包。旧 1.0.1 目录保留。

Renderer 第一页顶部新增 **预设管理** 和 **加载 VPY**。管理窗采用列表 / 参数总览 / 脚本预览三栏，支持保存当前处理链、读取、导入、导出、重命名、备注、拖动排序、删除到回收站与重置处理链。VPY 保存到 exe 旁的 `vpy` 文件夹，内嵌可恢复的图元数据；脚本本体可直接执行。读取原始 VPY 后播放/导出直接使用其脚本，修改图参数则切回图生成模式。保存的是脚本快照，不会因之后修改处理链而变化。

本程序生成的预设使用 `_vsr_source` 输入覆盖机制，播放器/Renderer 可把同一滤镜用于当前视频；单独运行 VPY 使用保存时的源路径。外部 VPY 可定义 `_vsr_source`，并输出视频到 0（可选源输出 1）；其自写死的路径不会被盲目改写。没有视频时也可保存滤镜预设，使用前选择视频。内置 Anime4K 着色器位于 `shaders`，下拉列表涵盖本机已有的 14 个 GLSL；自定义模式可选择文件路径，不要求另外下载。

**VS Player**：拖入视频或 Ctrl+O 打开，Ctrl+P 加载 VPY，Space 播放/暂停，Enter / F11 切换无边框全屏、Esc 退出，Tab 切换真实媒体信息。第一排时间轴显示章节标记及静音/音量；第二排提供播放、±帧/±秒/±关键帧、前后文件、可输入时间与输出帧号、0.10–16.00× 倍速和配置弹窗，右侧显示编码、实际内核、HDR/SDR 与当前渲染器。倍速弹窗支持连续点击，点击外部或再次点下拉关闭。单文件打开后前后文件按所在目录名称排序；多文件拖入按拖入顺序。输出帧号从 0 开始，暂停时可精确显示补帧后的中间帧。

右键视频区域提供三级菜单及蓝色悬停：开发者 / 用户 VPY、主次字幕、PNG 截图、放大 / 缩小算法、全屏和多页设置。默认 Jinc + relaxed 抗振铃，同尺寸直接采样；抗振铃采用强度 0.5 的开放局部范围约束，不是 madVR 专有实现的复刻。源截图为 VS 原分辨率 16-bit RGB PNG，实画面截图包含字幕及显示效果；HDR 实画面截图目前为截取显示层的 8-bit RGB，未实现 HDR PNG 色彩管理。

字幕支持 MKV 内置 ASS/SRT/PGS 与外部 ASS/SSA/SRT/SUP；挂载外部字幕时停用内置选择，主字幕在底部、次字幕在顶部，可交换及关闭覆盖层。ASS 保留原始样式与特效，SUP 保留位图，样式窗仅控制 SRT。字幕使用原生 D3D11 独立覆盖层；madVR 模式使用其 OSD，不烧入 VS 输出，也未切换到 EVR 内核。

设置分基本、主题、播放、性能、解码、渲染、缓存、文件关联八页，自动保存到便携目录 `player.ini`，可导入 / 导出单个完整 INI（VPY 文件仍单独保存，LAV / madVR 原生属性由各组件管理）。预解码默认开启，常规 / 极致分别设置四项阈值为 50% / 100%，可自定义 CPU / GPU / 内存 / 进程显存预算及提前帧数。阈值控制后台追加，CPU 同时控制 VS 线程数、内存控制缓存目标；插件和其他程序的瞬时占用不受硬限制。Tab 顶部显示进程 CPU、系统最忙 GPU 引擎占用、进程内存；丢帧拆分为 VS 跳过、渲染丢弃及合并，seek 不算跳过帧。

基本设置新增中文 / English 外置语言包和自动播放（默认开）；关闭自动播放时准备首帧并暂停。主题可改中 / 西文字体、背景（默认 `#202124`）和不透明度（默认 100%）。播放设置默认记忆视频 / 音频位置、后台多线程打开，按媒体时间恢复并映射到 VS 输出帧率；左右默认 ±1 秒，可选帧 / 关键帧，Ctrl ±10 秒、Ctrl+Alt ±60 秒可调。关闭多线程时 VS 源限制为单解码线程，3FP 保留后台工作线程。

视频 / 音频解码器独立选择 `3FPlayer (FFF.Native.dll)` 或 `LAVVideo.ax` / `LAVAudio.ax`，提供各自原生配置按钮。VS 源仍由 VPY 定义，LAV 视频选择控制本地时钟图；网络及内置原生视频路径固定 3FP。倍速音频使用 3FP / atempo。右键缩放支持 `D3D11 Native`：支持的 SDR 硬解源用 VideoProcessor，HDR / 软件源 / 交互放大等情况轻量 Bilinear 回退以保持原有颜色处理。Anti-ringing relaxed 当前仅作用于 Jinc（强度 0.5），不适用于菜单全部算法。

文件关联支持常用视频 / 音频格式及全选视频、全选音频、全选所有、取消所有。点击“注册所选格式”在当前用户注册候选播放器，再通过 Windows 默认应用完成默认选择；不改写系统保护的 UserChoice，也不抢占已有默认应用。语言文件为 `languages/zh_CN.json` / `en_US.json`，新增界面文字时同步两包并运行 `tools/check-player-languages.py`；该检查已加入 CTest。

3FP 倍速使用 FFmpeg atempo 分段链保留音调，时钟和画面请求同步改变速度；重滤镜来不及处理时跳过落后请求。LAV 随目录携带并直接实例化，无需注册系统滤镜；设置可选 3FP / LAV。LAV 原速提供播放时钟和音频，最终画面仍由 VPY 的源滤镜和 VS 处理。不变调变速时自动改用 3FP / atempo，显示实际内核，LAV 设置保留。硬件/软件按钮控制 3FP 播放源的解码方式，不改变 VPY 自身源滤镜。选择新的内核或文件会重新打开；不宣称 LAV 代替了 VPY 内部的解码源。

HDR 自动请求由 3FP 检查显示输出能力；HDR 源在 SDR 显示器映射，底部仍标 HDR。未在真实 HDR 显示器上验证；VPY 需要保留相应颜色/传递属性。会改变视频时间轴的任意脚本（例如剪辑、倒放或任意 FrameEval）不会自动重建音频时间轴。Tab 没有可取得的数据明确显示未知/未提供。

madVR `madVR09217` 现可实际加载随附 `madVR64.ax`，无需系统注册，任务栏控制器由随附 `madHcCtrl.exe` 提供。此模式使用 LAV DirectShow 视频输入，绕过 VS；Tab 显示 `madshi video renderer`，尝试切换 VPY 时顶部提示不生效。原速音频使用 LAV，非原速用 3FP / atempo 保持音调。madVR 不接收 VS 处理后的帧；其画质、HDR 和缩放配置交给原生控制器。启动参数可传入视频与 VPY（含空格路径需引用），例如 `vs-player.exe "video.mkv" "vpy/preset.vpy"`。

性能实测及 3840→4096 尺寸问题的分析见 [播放器性能记录](docs/player-performance-1.0.2.md)。三个边缘 / 细节锐化滤镜的 YUV 路径现由 vs-placebo Vulkan GPU 执行，相邻锐化合并调用，色度保留；Gray/RGB 仍用 CPU。Renderer / Player 加载已有预设时，仅精确识别的旧内置锐化块转为 GPU，不覆盖用户 VPY；新保存的预设自带 helper。

Enter 双向切换无边框全屏，底栏离开鼠标 2 秒隐藏，鼠标移到底部显示。鼠标进入右侧面板所在范围显示播放列表；右键即使没有视频也可打开文件、文件夹或链接，子目录可展开。支持 `vs-player.exe "https://…/video.mkv"`、file URI 和拖放 URL；HTTP/HTTPS 原生直通通过 Range 代理；网络始终不执行 VPY，所选预设保留给下一本地视频。目录显示所有文件，上一 / 下一视频按媒体文件列表切换。

构建两个目标后运行 `tools/stage-1.0.2.ps1`，首次复用上一便携目录的 Qt/FFmpeg 和缓存，重复执行时保留现有用户 VPY / INI，再更新 VS 运行时与原生 DLL并复制本机 Shader/LAV/madVR。本机原生 DLL 在独立 `.deps/fff-player` 副本构建，新增速度/章节补丁位于 `patches/3fp-player-rate.patch`，输出格式、抗振铃和字幕覆盖补丁位于 `patches/3fp-player-output.patch`，没有改写另一项目的原始 checkout。


## 核心能力

- 将常用预处理步骤组织为可排序、可开关的模块化滤镜链。
- 参数修改立即反映到可审阅的 `.vpy` 脚本，不要求用户手写 Python。
- 独立导出窗口提供顶部“导出设置 / 准备文件 / 编码队列”页签，可排队执行多个 MKV 任务，显示进度、处理帧数、预计剩余时间并停止；停止后保留已封装的输出，关闭导出窗口后任务继续执行。
- “导出当前处理结果”接受 3FUI 复制出的 FFmpeg 命令：VS 处理后的画面由 `vspipe` 送入 FFmpeg，源文件中的音频、字幕、附件、章节与元数据仍可按原参数映射。
- 启动时用内置短片预热解码、LSMASH/FFMS2 与双路 GPU 渲染；预热期间接收的源视频会在完成后自动加载。
- 支持把视频直接拖入窗口；导入后立即预取左侧首帧，不再等第一次点击播放才启动解码/索引。
- 左路 3FP 作为播放时钟；程序从 VPY 同时读取源与处理后 clip 的帧数/FPS，把左侧当前呈现时间换算成各自绝对帧号，再请求对应 VapourSynth 帧。
- 内置 `vs-placebo` 的 Anime4K GLSL 节点，可直接浏览并加载 mpv/libplacebo `.glsl`；默认指向 `C:\PortableSoft\FFmpegFreeUI ReadyToRun x64\libplacebo\anime4k-v4-a.glsl`，倍率同时作用于宽高以保持画面比例。
- 两路同步滚轮缩放、放大后左键拖拽平移与像素取色；缩放缓存随倍率恢复到源像素密度，默认 Nearest 放大、Lanczos 3 缩小，并可切换 Bilinear/Bicubic/Lanczos/Jinc；空格始终控制播放/暂停。
- 比较栏可在并排和 A/B 滑块间切换；滑块左侧显示源视频、右侧显示处理结果，共用现有两路 surface，以不重叠的原生父窗口裁剪，不重复计算 VS。
- 手动调整窗口、最大化和全屏时保留 DXGI 交换链的 tearing 创建标志，确保尺寸更新后呈现、缩放继续工作；原生视频表面不参与 Qt 背景绘制。
- 支持 3FCompare 扩展的 VRR tearing present 与 media-rate pacing 开关。
- VapourSynth、Python、源滤镜和当前 catalog 插件随程序部署，终端用户无需手动安装 VS 或 Python。
- catalog 内置 Zsmooth、Deblock、ZNEDI3、EEDI3、SangNom、Bwdif 与 VIVTC；参数面板直接说明各滤镜的收益、速度与画质代价。
- VS 性能落后时不积压连续帧：当前请求完成后直接跳到左侧最新目标帧；右上角状态与缩放倍率分开显示，不再逐帧在“同步中/实时”之间闪烁。
- 首帧载入显式使用全部逻辑核心，并并发预取后续 6 帧；导出时 `vspipe` 同样启用多帧请求，以计算资源换取冷启动和吞吐。
- 内置运行时缺文件或插件时在启动和脚本载入阶段明确诊断，不静默生成不可运行的流程。

## 图像分析比对

左上角三横按钮展开或收起导航，切换“VS 实时渲染”和“图像分析比对”。切页暂停离开的播放器，保留文件、画面设置和每个视频独立的时间偏移。导航最底部“设置”可切换 VS 源滤镜 L-SMASH Works / FFMS2，并保存选择；该选择不再占用“输入与环境”栏目。

顶部“打开视频”可多选，也可直接拖入多个视频，最多 9 个。对比页接受 MP4、MKV、MOV、AVI、WebM、TS 等主流格式（具体编码由随附 FFmpeg/3FP 支持），不经过 VS；**MKV 限制仅用于第一页导出**。

- 2 路：AB 并排或 A/B 滑块。
- 3 路：常规模式为三个等大完整画面横排（3×1）。ABC 滑块把同一画布分为三段，分别显示三个视频的对应局部；可选横向/纵向三段，或半幅加两个四分之一区域（支持左右镜像），交点同时控制横竖切割位置。
- 4 路及以上：可选 AB、ABC、ABCD；四路常规模式为 2×2 完整画面，ABCD 滑块则用同一坐标系下的四个局部拼成一张画面，中心点可整体拖动，横竖线也可独立调整。
- 5–9 路：导入后自动使用三列网格，依次为 3+2、3+3、3+3+1、3+3+2、3+3+3，也可切回 AB/ABC/ABCD。

所有滑块模式共用完整画布、缩放和移动坐标，复用原有局部区域缩放渲染。拖动仅改变原生父窗口的裁切区域，不改变视频表面尺寸，也不重新定位或解码帧；源下拉框固定放在画布上方，避免混入拼接区域。常规模式可独立控制缩放，滑块模式强制同步以保持拼接对齐。

每格顶部使用紧凑的视频源下拉框，右侧 × 可卸载该视频，不删除源文件；其余视频保留解码会话和对齐偏移，腾出的名额可继续导入。每格与每条对齐轨道都有视频源下拉框。选择已占用的视频会自动交换位置，隐藏视频的偏移不会重置。未导入时不显示底部控件；导入后按当前模式显示轨道，最多 4 行，网格中其余视频可通过下拉框换到对齐轨道。

进度条显示各路本地时间，拖动任一条同步移动当前参与比较的视频并保持偏移。每行 ±1 帧、±1 秒只调整本视频；全局逐帧以第一格实际帧时间为准，其他视频按时间定位。音频下拉可选择任意已导入视频或静音；VRR 低延迟与 VRR Pacing 位于播放控制右侧。上下平移按实际视频显示区域计算，与水平拖动保持相同像素响应。

右下角“颜色处理”控制独立色度上采样：Nearest、Bilinear、Bicubic、Softcubic、Mitchell、Lanczos、Spline36、Jinc、Bilateral、亮度引导双边重建及 Super-XBR 单阶段。悬停显示当前选中算法的完整名称，菜单用勾选标记当前项；颜色处理控件已加宽，两个 VRR 开关有边框和选中高亮。颜色处理与放大算法均采用一致的下拉列表和蓝色悬停提示。放大算法新增 Spline36 和 Super-XBR 单阶段；后者是 Hyllian 开源 pass-0 对角核的移植，**并非完整三阶段 Super-XBR**。不包含 NGU 或其他 madVR 专有实现。

## 导出对比画布

顶部模式选择右侧“导出对比画布…”复用独立导出窗口与编码队列，粘贴 3FUI FFmpeg 命令后输出 MKV。保存的是点击按钮时的布局快照：当前显示的源、常规并排/网格或 AB/ABC/ABCD 切割位置、各路缩放平移、时间偏移及音频来源。画布内容合成为单路视频，不包含控件、取色提示和拖柄；任务开始后修改页面不会改变已保存的快照。

输出尺寸取所有已导入源中像素总数最多的视频的原始宽高（相同总像素时取先导入者）；当前未显示的高分辨率源也参与尺寸选择。常规模式拼完整画面，滑块模式拼同一画布上的局部；按当前画面视口保留适配和黑边，导出尺寸与预览宽高比不同会按归一化坐标映射。默认以第一格帧率，从全局时间 0 输出至参与画面/音频的最早结束点；负偏移的前段保持源首帧。VFR 使用源滤镜给出的时间基近似。

支持 Point/Linear/Bicubic/Lanczos/Spline36 与对应 Cubic 色度核。Jinc、Super-XBR、双边色度尚无一致的 VS 导出实现，当前会明确阻止，不静默换成其他核；CPU VS 和实时 D3D 路径不保证逐像素相同。3FUI 命令中额外的裁切、缩放、调色继续作用于合成结果。所选音频源作为 FFmpeg 输入 0，应用同一时间偏移；静音选择强制不输出音频。保护所有输入及已排队任务的输入路径，禁止覆盖。

对齐轨道会按长文件名自适配扩大下拉框、缩短进度条，进度条至少 160 逻辑像素，下拉框最多 600；完整名称可悬停查看，展开列表可更宽。时间与左右控件采用一致的 8 像素间隔。底部布局、音频选择器加宽，VRR 使用显式方框和蓝色勾选标记。

## 新增滤镜与 RIFE

新增 14 个可调节点：TemporalMedian、FluxSmoothT、FluxSmoothST、SmartMedian、InterQuartileMean、DegrainMedian、Cnr4、CCD、DCTFilter、TemporalSoften、VerticalCleaner、CLAHE、Descale、RIFE。前 12 个使用随附 Zsmooth / VSZip 的 API 4 实现，Descale 和 RIFE 原生插件也随便携程序部署；无需用户另外下载插件。时域中值/Cnr4/RIFE 使用基于邻帧亮度差的镜头属性，CCD 和补帧前后明确转换格式。参数说明与调节项直接显示在节点面板。

RIFE 提供 4.26 和 4.26 Heavy，采用 NCNN Vulkan，携带两套转换后的 `.param/.bin` 模型。已在 NVIDIA RTX 4070 Laptop 和 AMD Radeon 610M 实际生成插值帧；Intel 等设备需可用 Vulkan 驱动，未逐机型验证。倍数、GPU 编号、工作线程及镜头切换处理可调。

VS 的 PyTorch 路线支持 `.pkl` 权重，Windows 常见 CUDA/TensorRT 配置以 NVIDIA 为目标，PyTorch 依赖体积大且当前便携 Python 3.15 无对应常用预编译轮子，因此本轮采用 NCNN。`.pkl` 不能直接作为 NCNN 模型传入；本机使用用户指定目录中已有的 4.26 / Heavy 转换模型，记录精确 SHA-256，不临时把 PKL 改扩展名。

本机 1920×1080 合成输入、GPU 0、单工作线程、四张插值帧的暖态测试：4.26 约 12.88 张插值帧/秒，Heavy 约 10.97 张/秒（不含解码、帧桥和显示开销），达不到该配置下 24→48 fps 实时播放；小分辨率预览已验证中间帧确实提交。补帧播放使用更密的时间轮询，慢滤镜仍跳过落后目标，离线导出保持完整帧数。

来源：[Zsmooth](https://github.com/adworacz/zsmooth)、[VSZip](https://github.com/dnjulek/vapoursynth-zip)、[Descale](https://github.com/Irrational-Encoding-Wizardry/descale)、[NCNN RIFE](https://github.com/styler00dollar/VapourSynth-RIFE-ncnn-Vulkan)、[PyTorch RIFE](https://github.com/HolyWu/vs-rife)。

## MVTools 与 2× 补帧优化

新增 `MVTools 2× 实时补帧`，使用随运行时部署的 API 4 MVTools v29。原始时间点直接取输入帧，中间时间点才请求双向运动向量与 `FlowInter(time=50)`；两路通过 VS Interleave 交错，末帧保持，不预生成整段高帧率视频。运动块、pel、重叠、色度搜索与镜头切换混合可调。传统 CPU 运动补偿不依赖 AI GPU，遮挡及快速运动仍可能产生扭曲。[MVTools 上游](https://github.com/dubhatervapoursynth/vapoursynth-mvtools)

RIFE 上游本来就跳过原始时间点的神经网络推理；本轮进一步让原始帧绕过 RGB 往返转换，保留输入格式和像素，只对中间帧进行推理与输出转换。两种补帧均可选择半宽半高或四分之一宽高计算，再把中间帧恢复到原尺寸。默认原分辨率，低分辨率计算会损失中间帧细节，原始帧保持不变。RIFE 3×/4× 同样保留原帧支路。[RIFE 上游实现](https://github.com/styler00dollar/VapourSynth-RIFE-ncnn-Vulkan/blob/r9_mod_v33/RIFE/plugin.cpp)

修复绿色闪烁后，本机 1080p FFV1 testsrc2、预先取源帧、12 张中间帧暖态单次测量：MVTools block=16 原尺寸 28.04、半尺寸 93.29 张/秒；RIFE 4.26（RTX 4070 Laptop，单 GPU 工作线程）原尺寸 13.11、半尺寸 47.14、四分之一尺寸 118.95 张/秒。24→48 fps 需要每秒 24 张中间帧；不含显示开销，不代表所有素材/核显均实时。完整记录 `build/mvtools-green-regression.txt`，复测脚本 `tools/verify-interpolation.py`。

MVTools 的“色度参与运动搜索”只控制 Analyse 搜索，Super 始终保留色度供 FlowInter 重建；否则原帧正常、中间帧 U/V 为零会造成绿色闪烁。修复无需打开色度运动搜索，原有快速亮度搜索继续可用。

补帧专项最初使用独立的 `build/mingw-interpolation`；现已合入 `dist/VS-Renderer-GUI-1.0.2-windows-x64` 的 Renderer 和 Player。交付程序内的补帧代码与当前源码一致，并用随附运行时通过彩色 8/10/16-bit、并发跳帧、真实媒体及两套 RIFE 模型回归。旧保存的 VPY 不会自动改写，需重新生成；旧 MVTools 脚本的 `Super` 应使用 `chroma=True`。旧版压缩包未修改。

## 启动性能与 VS 增强

VS/Python 在后台初始化，状态栏显示分阶段进度，期间可编辑处理链、切页和操作对比页。原生状态轮询与首次重绘不再等待着色器编译锁。发布目录中的 `shader-cache` 保存按源码匹配的预编译字节码，请与程序一起保留；缺失时会后台重新编译，首次耗时更长。

本机合成素材验证：带缓存的新进程预热约 392ms，界面计时器最大停顿 38ms；2160p H.264 首帧约 283ms。不同媒体索引、编码、GPU 和存储速度会影响实际结果。

VS 滤镜库按“去伪影 / 图像增强”提供 Reduce banding、ringing、compression、random noise，以及 Sharpen edges、Crispen edges、Thin edges、Enhance detail。已有去色带/去块/NLMeans 复用随附开源插件，新增效果使用 VS 的卷积、局部范围限制和形态学操作；不声称与 madVR 私有算法逐像素一致。

## 构建

要求 Windows 10 22H2+、Qt 6.8+、CMake 3.25+ 和 C++20 编译器。本机 Qt Creator 套件可直接打开根目录 `CMakeLists.txt`。

```powershell
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug
```

先构建官方 Git 工作树中的 VapourSynth R80。脚本同时安装经过 R80 验证的 API 4 插件，并把便携 Python/VS 运行时自动放入程序输出目录：

```powershell
.\tools\build-vapoursynth.ps1 -SevenZip C:\path\to\7z.exe
```

再给 FFF Project 应用仓库补丁、构建 `FFF.Native.dll` 并复制到 Qt 输出目录：

```powershell
.\tools\build-3fp.ps1 -FffProject C:\path\to\FFF_Project
```

程序固定优先加载 `runtime/python/Lib/site-packages/vapoursynth/vsscript.dll`，不读取终端用户的 VS/Python 安装，也不要求调用 `vspipe.exe` 才能预览。CMake 链接程序后会重新执行运行时 staging，避免生成只有 exe、没有后端的半成品。`VSR_VSSCRIPT_DLL` 仅作为没有 bundle 时的开发覆盖项。补丁版 `FFF.Native.dll` 可放到可执行文件旁，或通过 `VSR_3FP_DLL` 指定；3FP 仍须配套其 FFmpeg shared DLL。

内部架构、依赖来源、当前限制和故障排查见 [project.md](project.md)，精确 UI 尺寸见 [docs/ui-layout-spec.md](docs/ui-layout-spec.md)。

完整依赖安装、3FP API 14 构建和 Release 构建步骤见 [docs/dependencies.md](docs/dependencies.md)。普通用户使用发布页中的 Windows x64 压缩包，无需单独安装 Qt、Python 或 VapourSynth。

## 3FUI 参数兼容导出

命令必须以 `ffmpeg.exe`（或其绝对路径）开头，并保留 `-i <输入文件>` 与末尾 `<输出文件>`。程序把源文件保留为输入 0，把当前 VPY 的 Y4M 输出加入为输入 1，并自动把 `-map 0:v...` 改为 `-map 1:v...`；音频、字幕、附件、元数据和章节仍来自输入 0。使用 `scale_cuda` 时会自动补 `hwupload_cuda`。为避免错误改写任意图，兼容模式暂不接受 `-filter_complex`/`-lavfi`。

仅支持 `.mkv`，实际封装固定为 Matroska。每个排队任务保存独立的 VPY 快照。停止通过关闭 VS 输出管道让 FFmpeg 收尾，不强杀 FFmpeg；关闭主界面时等待正在编码的文件收尾后退出。若尚未编码出任何帧，可能没有可播放内容；编码器或磁盘本身发生错误时，队列会提示检查文件完整性。

“导出设置”底部只导出主界面当前文件，同一源文件在队列中只保留一个任务，不会因重复点击产生多份任务。“准备文件”可拖入多个文件、排序和移除，点击“全部加入编码队列”时，为每个源文件保存当前 VS 处理链与共用的 3FUI 参数快照。已加入的文件从准备列表移走，后续修改主界面设置不会改变队列里的脚本。

批量输出目录可选“原始目录”或浏览指定目录；默认命名为 `原文件名_yyyyMMdd_HHmmss_zzz.mkv`，发生同名冲突时再追加序号。也可选择“不加时间戳（高风险）”，但禁止覆盖本批次输入或与队列中的输入/输出路径冲突；输入本身是 MKV 时，不加时间戳须选择其他目录。

编码队列使用可拖动排序的列表，列宽和列顺序也可调整；下一任务按当前列表顺序执行。支持挂起/暂停整个队列（包含当前编码）、恢复、删除未开始任务、停止选中任务并保留输出。停止或失败的任务可“从头重新处理”，使用原任务快照并覆盖该任务的部分输出，进度从零开始。处理帧数来自 FFmpeg，时长估计来自当前 VS 输出 clip；额外的 FFmpeg 裁切/变速参数可能影响估算。

发布包中的导出 CLI 位于 `runtime\ffmpeg`，与根目录供 3FP 使用的 FFmpeg DLL 隔离，解压后即可导出。

## 插值与画面比例

`ZNEDI3/EEDI3` 的 `dh=True` 只会把高度变成 2 倍，若宽度不变，像素尺寸比例会从 `W:H` 变为 `W:2H`。1.0.1 默认采用不改变尺寸的双倍帧率模式；明确选择“纵向 2×”时，生成脚本会把宽度也扩成 2 倍。SangNom 的 `dh` 采用同样处理。Bwdif 的双倍帧率只增加帧数，不应改变宽高。

## 许可证

本项目源码采用 [MIT License](LICENSE)。打包的第三方运行时遵循各自许可证，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 第三方声明

本仓库中的 `third_party/vapoursynth` 是从官方 Git 仓库拉取的上游源码，其许可见该目录。3FP/FFF.Native 与 FFmpeg 不作为本项目源码提交；仓库只保存可复现的 3FP 扩展补丁，运行时二进制按各自许可提供。
