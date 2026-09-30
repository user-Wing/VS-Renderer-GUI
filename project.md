# VS Renderer 项目地图

## 1. 发心

面向了解视频处理概念、但不愿反复编写脚本且需要快速 A/B 验证参数的用户。GUI 负责组织模块、顺序与参数，VapourSynth 负责处理，3FP 负责双路呈现和帧级检查。成功标准是用户能从“打开源”走到“生成并验证脚本、同步比较结果”，并把当前处理链交给熟悉的 3FUI FFmpeg 参数导出。非目标：首阶段不实现任意 Python IDE、矩阵表达式可视化编辑器、完整编码参数面板。

## 2. 快速开始

- 环境：Windows 10 22H2+、Qt 6.8+、CMake 3.25+、C++20；运行时需 API 14 补丁版 3FP 与项目固定的 VapourSynth。
- 配置：设置 `QT_ROOT` 后运行 `cmake --preset windows-mingw-release`。
- 构建：`cmake --build --preset windows-mingw-release`。
- 测试：`ctest --preset windows-mingw-release`。
- 已知坑：3FP 必须连同匹配的 FFmpeg DLL；MinGW 通过动态 C ABI 使用 MSVC 构建的 `FFF.Native.dll`；L-SMASH 首次打开媒体会建立 `.lwi` 索引，长视频可能等待数秒。

## 3. 架构决策

| 决策 | 理由 | Anti-choice | 重估条件 |
| --- | --- | --- | --- |
| Qt Widgets + C++20 | 原生 HWND、紧凑桌面布局、3FP C ABI 接入直接 | 不采用 QML：首阶段不需要动画/触控场景图 | UI 需要高度动画化或跨端 |
| 3FP 运行时动态加载 | 不绑定 MSVC/MinGW import library，缺失可诊断 | 不静态链接 fork：构建和许可分发成本过高 | 内核提供稳定 CMake 包 |
| 滤镜定义在 C++ catalog，图仅存 ID+参数 | MVP 类型安全且容易测试 | 不先造通用 DSL/插件系统 | 外部作者需要无编译扩展 catalog |
| 生成纯 VPY，由专用线程动态调用 VSScript | 脚本透明；随机帧请求无需重启管道；GUI 不阻塞在 VS 计算 | 不用 vspipe/Y4M 作为实时桥：seek 和积压控制较差 | VS 提供稳定的跨进程随机帧协议 |
| 左路 3FP 为唯一时钟，按源/输出 clip 时间轴换算绝对帧 | 3FP 时间 seek 后的 `frameIndex` 是局部 PTS 索引，不能代表源绝对帧；按呈现时间和两端 FPS 映射也兼容倍帧/抽帧 | 不让两个播放器各自计时，不直接用 3FP 局部帧号驱动 VS | 需要完整 VFR timecode 映射或独立处理后音频预听 |
| Anime4K 通过 `vs-placebo` 执行 GLSL | 官方插件能直接执行 mpv/libplacebo shader，生成 VPY 可审阅且不需要扩展 3FP 私有 shader ABI | 不把 GLSL 名称伪映射成普通 VS resize，也不重写 shader | 需要零 CPU copy 的 GPU texture 互操作 |
| 3FUI 导出使用“源输入 0 + VS Y4M 输入 1” | 保留源音频/字幕/章节映射，同时保证视频必然来自当前 VS 处理链 | 不解析和重建完整 FFmpeg AST；复杂图明确拒绝 | 需要支持多视频输入或 `filter_complex` 图 |
| A/B 滑块通过不重叠的原生父窗口裁剪现有两个 pane | 不增加第三路解码、帧请求或 GPU 处理 | 不在 CPU 合成两张帧图 | 需要离屏录制比较画面 |

## 4. 目录地图

```text
src/app/       主窗口与应用样式
src/backend/   3FP 动态 C ABI、双路播放协调、运行时探测
src/graph/     滤镜 catalog、图状态、VPY 生成
src/ui/        预览表面、可折叠侧栏与参数编辑器
tests/         无 UI 的图与脚本生成测试
docs/          UI 规格、滤镜覆盖和后端桥设计
tools/         依赖/构建诊断脚本
patches/       可应用到 FFF Project 的 3FP C ABI 扩展
third_party/   官方 VapourSynth Git 工作树；其他二进制不入库
```

## 5. UI 地图

主工作台：顶部命令栏 → 左侧三段可折叠 dashboard → 中央源/处理后双路视图 → 底部共享时间轴与播放控制。入口为 `src/app/MainWindow.cpp`，尺寸和遮挡约束见 `docs/ui-layout-spec.md`。

## 6. 外部依赖

| 依赖 | 版本/来源 | 调用协议 | 缺失时降级 |
| --- | --- | --- | --- |
| Qt | 6.8+；本机 6.10.2 | Widgets/Win32 HWND | 无法配置构建 |
| VapourSynth | 官方 Git `master`；当前 `5b2d556` / R80RC2 | `QLibrary` 加载 VSScript 4.4，VS API 4.3 按帧读取 | 仍可编辑/导出脚本 |
| 3FP / FFF.Native | FFF Project + `patches/3fp-vsrenderer-extensions.patch` | API 14 `FFF3FP_SubmitExternalVideoFrame` | 显示诊断占位，不崩溃 |
| FFmpeg | 与 3FP ABI 匹配的 shared build | 由 3FP 内部加载；桥接进程使用 CLI | 禁用播放/桥接 |
| VS 插件 | LSMASHSource、FFMS2、fmtc、RemoveGrain、AddGrain、VSZip、nlm-ispc、CAS、Zsmooth、Deblock、ZNEDI3、EEDI3、SangNom、Bwdif、VIVTC、vs-placebo 2.0.4 | 随便携运行时部署的 API 4 插件 | 启动自检失败并阻止预览 |

## 7. 核心数据

`FilterGraph` 保存有序 `FilterNode{id, enabled, parameters}`。参数值使用 Qt 基础类型，写入项目 JSON 时保留 catalog schema 版本。生成脚本不覆盖用户文件，预览脚本写入应用缓存目录；用户显式“导出 VPY”才写目标路径。迁移规则：未知节点原样保留但禁用，参数新增取 catalog 默认值，删除参数不输出。

## 8. 代码拆解

依赖方向固定为 `ui/app -> graph/backend`，`graph` 不依赖 UI，`backend` 不依赖 graph。单文件接近 500 行时检查职责；3FP ABI 声明虽可能偏长，但只承担协议镜像，不按行数硬拆。高风险区是实时帧桥、3FP ABI 版本变化和 seek 重建，不与 MainWindow 混写。

## 9. 更新与发布

使用 CMake Presets 构建，测试先行覆盖 VPY 转义、顺序与参数边界。结构、依赖 commit 或关键决策变更当天同步本文件。发布前运行 CTest、启动冒烟、`windeployqt`，再打包与 3FP 对应的 FFmpeg DLL；不得混用其他 major ABI。

## 10. 当前状态

- 版本：`1.0.1`。
- 已完成：项目基线、UI 规格、VapourSynth Git 源码固定、内置便携 Python/VS/API 4 插件、滤镜图/VPY 生成、教程向插件扩展、Anime4K GLSL、API 14 外部帧桥、左右同帧预览、A/B 滑块、3FUI 参数兼容导出、源像素密度感知缩放、缩放拖拽与全局空格播放。
- 下一步：扩充 VCB 全量 catalog、插件管理与 NGU 等独立高级缩放后端。
- 限制：当前帧桥支持常用整数 planar 格式，不支持 VS float/GPU-resident frame；Anime4K 在 VS 内使用 GPU，但输出仍经 CPU plane copy 提交给 3FP，4K 重 shader 未必能实时；NGU 属于独立 GPU 推理实现；处理后音频未接入，左路始终是音频时钟；3FUI 兼容导出拒绝 `filter_complex`/`lavfi`，仅自动重写常规视频 `-map` 与单路 `-filter:v`。

## 11. 故障排查

| 症状 | 原因 / 处理 |
| --- | --- |
| CMake 找不到 Qt | 设置 `CMAKE_PREFIX_PATH` 或使用仓库 preset 中的本机 Qt 路径 |
| UI 启动但预览为诊断页 | 将补丁版 `FFF.Native.dll` 与匹配 FFmpeg DLL 放到 exe 旁，或设置 `VSR_3FP_DLL` |
| `Create` 返回失败 | 检查 3FP API 版本与 FFmpeg DLL major，查看状态栏的原生错误 |
| `VapourSynth 未加载` | 当前程序包不完整；确认 exe 旁存在完整 `runtime/python`，开发构建可重跑 `tools/build-vapoursynth.ps1` |
| VPY 执行缺插件 | 当前程序包不完整或混入了旧 API 3 插件；重新 staging 内置运行时，不要求终端用户手动安装 |
| 3FP 报缺少外部帧 ABI | 对 FFF Project 应用仓库 patch 并重建；程序要求 API 14 |
| 处理顺序不对 | 在“处理链”中拖动节点；脚本严格按列表自上而下生成 |
| Anime4K 节点报 shader/namespace 错误 | 确认 `.glsl` 路径存在，并确认便携运行时含 `plugins/libvs_placebo.dll`；缩放 shader 必须给宽高，GUI 的“输出倍率”会同时计算两者 |

## 12. 1.0.1 更新记录（2026-09-26）

- 变更：新增 Anime4K GLSL 文件参数与 `placebo.Shader` 映射，构建/部署固定 `vs-placebo==2.0.4`；新增窗口拖放与源首帧预取。
- 变更：ZNEDI3/EEDI3 默认改为不改变尺寸的双倍帧率模式；纵向 2× 和 SangNom `dh` 自动同步放大宽度，维持原始宽高比。
- 变更：每次载入新 VPY 前重建右侧 3FP 会话，`DeviceFailure` 时再进行一次设备重建并重提当前帧；VS 请求改为一次只处理一帧，完成后跳到最新目标。
- 变更：缩放倍率与 VS 状态使用独立标签；native surface resize 后延迟重绘，父预览区也接收滚轮。
- 变更：新增 A/B 滑块比较；最大化时合并 resize 重绘请求并按当前算法/倍率重绘两路 surface。
- 变更：新增 3FUI 参数兼容导出，`vspipe` 使用 R80 `--container y4m` 与多帧请求，源媒体保留为输入 0，VS 视频改接输入 1；发布包隔离携带 FFmpeg CLI。
- 变更：首帧把 VS 线程数提升至全部逻辑核心并预取 6 帧，稳态仍保持单目标请求和落后帧跳过。
- 验证：MinGW Release 构建成功；CTest 4/4 通过；真实 `vspipe → FFmpeg` 双进程管道产出带源音频的 FFV1 MKV；真实 `anime4k-v4-a.glsl` 以 `placebo.Shader` 输出 30 帧成功；不同尺寸/格式处理链重建提交测试通过。
- 风险：本轮环境未开放原生 Windows GUI 自动控制，拖放与最大化手势仅有 Qt 事件级覆盖，仍需人工做一次可见交互确认；Anime4K 实时帧率取决于实际分辨率、shader 与 GPU。

## 13. 导出队列与原生预览修复（2026-09-26）

- 文件：`src/ui/ExportWindow.*`、`src/backend/ExportPipeline.*`、`src/app/MainWindow.*`、`src/ui/CompareView.*`、`src/ui/PreviewPane.cpp`、`patches/3fp-resize-flags.patch`、`tools/build-3fp.ps1`、`CMakeLists.txt`、相关测试与 README。
- 导出：独立、非模态且无主窗口 owner 的窗口；顶部设置/队列页签；任务顺序执行、独立脚本快照、FFmpeg 机器进度、处理帧数、估算剩余时间、停止。只允许 MKV 并固定 Matroska 封装；拒绝输出覆盖源视频或重复占用队列输出路径。
- 停止：关闭 vspipe 产生 EOF，让 FFmpeg 排空并封装尾部，保留输出；等待两进程退出才开始下一任务。关闭导出窗口仅隐藏；退出主界面先停止队列并等待活动任务收尾。
- 预览：移除 AB 重叠 QWidget 遮罩，改为不重叠 HWND 父容器裁剪；视频 surface 禁止 Qt backing-store 背景绘制。3FP 三处 ResizeBuffers 保留 ALLOW_TEARING，与创建标志一致；构建脚本应用额外补丁并重新编译 DLL。
- 验证：Release GUI 与原生 DLL 构建通过；CTest 4/4。真实 FFmpeg/vspipe 完成与中途停止 MKV 均可完整解码，队列双任务、取消待执行任务、导出窗口隐藏/重开通过。真实 3FP 双 surface 测试覆盖 AB 切换与多位置分界、三组尺寸、滚轮缩放、全屏往返；旧发布 DLL 在同一测试停止呈现，修复 DLL 通过。测试记录位于 `build/export-verification.txt`、`build/resize-old-runtime.txt`、`build/resize-new-runtime.txt`。
- 风险：进度时间依据源时长估算，显式裁切/变速参数会影响估算准确度；停止前零帧或编码器/磁盘故障不能保证可播放内容。原生测试使用 VS 合成帧和程序化事件，真实媒体长时间播放、实际手势与多显示器 DPI 切换仍需人工确认。

## 14. 启动预热与批量编码队列（2026-09-26）

- 文件：新增 `src/backend/StartupWarmup.*`、`assets/startup.qrc`、自生成的 `assets/startup-warmup.mkv`、`src/ui/PreparedFiles.*`；修改 `MainWindow.*`、`ExportWindow.*`、`ExportPipeline.*`、CMake、帧桥/导出测试与 README。
- 启动：内置 128×72 H.264/AAC 黑场短片提前走通原生解码、LSMASH/FFMS2 源滤镜、VS 取帧与两路实际 GPU 呈现。预热保持原生会话供首次导入复用，首个 VS 预览也复用已预热的输出设备。启动中拖入的最新文件排到预热完成后打开；预热媒体不写入用户源路径或处理链。临时文件生命周期覆盖解码器与 VS 节点。
- 导出：三个顶部页签分别为导出设置、准备文件、编码队列；单文件导出只针对主界面当前源，按源去重。批量拖入/文件选择支持去重、移除、排序；加入时逐源生成相同设置的独立脚本快照，不依赖主界面后续修改。
- 列表：队列使用可拖动的平面列表，列顺序/列宽可调整；任务按稳定 ID 跟踪，执行顺序取列表当前顺序。显示文件名、状态、百分比、已处理帧数、预计剩余时间，完整路径位于提示中。
- 控制：暂停时挂起本程序启动的 FFmpeg/vspipe 子进程并挂起后续调度，恢复后继续，ETA 排除暂停时间。停止时先恢复进程再通过 EOF 收尾，保留 MKV；允许删除尚未开始的任务、让停止/失败任务使用原快照从零重做。FFmpeg 因 shortest/帧数限制正常退出时，不再把供帧进程的被动终止误报为失败。
- 命名：原始目录/浏览指定目录；默认毫秒时间戳，额外序号消除同名输出。不加时间戳标注高风险；阻止覆盖批次输入、重复源、与已有队列输入/输出冲突。VS 输出 clip 提供时长，批量任务无需提前载入主预览也可估算进度。
- 验证：Release 构建成功。完整原生帧桥测试 10/10，导出测试 8/8，图/VPY 与缩放交互测试通过。真实进程验证暂停无增长、恢复继续、暂停后停止保留可解码 MKV，以及停止后从头输出完整 120 帧；验证批量拖放、去重、两种命名、禁止覆盖输入、顺序调整、删除等待任务、连续执行与单文件重复点击。按应用样式查看了队列/准备页截图。
- 性能记录：当前机器测试短片启动预热 14.741 秒，其后首次加载 0.555 秒（单次合成短片测试，不代表任意媒体时延）。日志：`build/startup-frame-verification.txt`、`build/batch-queue-verification.txt`。
- 风险：预热将一次性初始化移到启动期间，不能消除具体源文件的磁盘读取/索引或未使用过的滤镜编译开销；暂停不会释放已占用的解码/编码资源。任意真实媒体、大批量长时间运行与实际鼠标拖动仍需用户场景验证；测试覆盖 Qt 拖放事件和列表顺序调整。


## 15. 页面导航与双路直接分析（2026-09-26）

- 文件：新增 `src/ui/AnalysisPage.*`、`tests/TestAnalysis.cpp`；修改 `MainWindow.*`、`CompareView.*`、CMake 与 README。
- 导航：顶部最左侧三横按钮控制图标栏/文字栏，现有控件顺延；两个页面分别保留 VS 实时渲染与图像分析比对，分析页按需创建，切页暂停离开的播放器。VS 专用工具在分析页隐藏。
- 分析：两路 MKV 原生解码，不经 VS；按钮和拖放导入，复用 CompareView 的并排/AB 原生裁剪及 PreviewPane 缩放平移、像素取色，保留底部模式、全局播放/逐帧和放大算法布局。
- 时间：每路进度条显示本地时间，拖动同步定位两路并保留偏移；±1 帧/±1 秒单独对齐。异步定位串行并合并最新拖动目标；逐帧读取实际解码后时间，不把 seek 后的局部 frameIndex 当作绝对帧号。A 提供音频与播放时钟，B 静音，超过 80ms 的漂移最多每 500ms 校正一次；一路结束暂停全部。
- 验证：真实 24fps/30fps MKV 测试覆盖首帧、正反逐帧对齐、秒级偏移、连续定位、全局逐帧、从 B 进度条定位、播放暂停、片尾、AB、窗口缩放、全屏往返、原生句柄保持；导航测试覆盖折叠/展开及页面切换，保存应用样式截图。新增分析测试 4/4 通过，原有四组 CTest 全部通过。结果记录见 `build/analysis-verification.txt`。
- 限制：不同帧率按时间选择最近可解码帧，两路独立播放时钟不保证音频采样级或硬件锁步同步；长片/VFR、多显示器 DPI 和实际拖放手势尚需用户场景验证。本轮只更新便携目录程序，不生成压缩包。


## 16. 启动性能、多路分析与开源增强（2026-09-26）

- 文件：`VapourSynthFrameServer.*`、`StartupWarmup.*`、`MainWindow.cpp`、`ThreeFpPlayer.*`、`ThreeFpApi.h`、`AnalysisPage.*`、新增 `MultiCompareView.*`、`PreviewPane.*`、滤镜 catalog/VPY builder、相关测试；新增 `patches/3fp-performance-chroma.patch`、`third_party/shaders`，更新构建/打包脚本和文档。
- 性能根因：VS 构造函数通过 BlockingQueuedConnection 等待 Python/API 初始化；每个原生渲染实例重复编译内置 D3D 着色器；GUI 播放状态读取 OutputBitDepth 等待设备锁；VS 外部首帧同步提交可能再次编译。改为异步初始化和分阶段进度，首帧编译先在原生工作线程进行；状态读取/首次重绘不等待编译锁。进程内共享字节码，并按源码哈希从随程序部署的 shader-cache 加载；缺缓存时编译后保存。VS 待处理用户导入仍在预热结束后打开，其他界面功能可用。
- 性能证据：带缓存的新进程预热 392ms，10ms 界面心跳最大间隔 38ms，随后首次导入 110ms；合成 3840×2160 H.264 MP4 首帧 283ms。无缓存的构建预编译耗时 18.398s、最大 UI 间隔 88ms，因此部署必须包含 shader-cache，不能只复制 EXE。时间是本机单次测试，不代表任意真实素材。
- 多路：最多 9 个视频，主流容器交给 FFmpeg/3FP 解码；MKV 限制只保留在导出。默认按数量选择 AB/ABC/ABCD/三列网格；ABC 横排/竖排/左大/右大，ABCD 2×2；2+1 与 2×2 提供中心整体拖动，分割线亦可单独拖。每个视频保持自身原生 HWND，不因槽位交换重建解码器。
- 对齐：文件状态与槽位排列分离，选择已占用源自动交换。无文件时隐藏底部控件；轨道数随模式变化且最多 4 行，全部 9 个视频偏移独立保存。当前显示的源和选定音频源参与全局时间定位，其他源暂停，重新显示时按保留的偏移定位。音频可选任意源/静音；底部加入 VRR 与 Pacing。
- 平移：根据真实视频适配后的显示尺寸计算超出视口的宽高，不再用含黑边的 QWidget 全高估算垂直移动范围。测试确认两个方向相同鼠标位移对应相同画面像素位移。
- 渲染：独立色度 Point/Linear/Catmull-Rom/Softcubic/Mitchell/Lanczos3/Spline36/Jinc2/Bilateral/亮度引导双边重建/Super-XBR 单阶段；色度与亮度缩放各自选择，Cubic、Spline、Reconstruction 使用子菜单。放大新增 Spline36/Super-XBR 单阶段。Super-XBR 是 MIT 开源 pass-0 对角核移植，完整三阶段与 NGU 未实现、不冒用其名称；UI 和 README 明确说明。项目专用 SetScalingAlgorithms 参数低字节为原缩放枚举，上字节为色度核 + 1，需配套本轮 DLL。
- VS：已有 deband/deblock/NLMeans 按对应英文效果归入去伪影类，新增 dering、sharpen_edges、crispen_edges、thin_edges、enhance_detail；使用 VS 原语进行范围限制、卷积与形态学操作，保留可调强度，不宣称与 madVR 私有实现完全一致。
- 验证：真实 MP4 九路导入、四行上限、交换、隐藏偏移、ABC 交点/ABCD 几何与不同帧率双路同步通过；全部八类 VS 增强组成脚本并成功取帧；11 个色度核与新增放大核实际呈现，Nearest/Bilinear 像素不同；异步构造/预热心跳、2160p 首帧计时通过。日志见 `build/multi-analysis-verification.txt`、`build/chroma-verification.txt`、`build/warmup-cached.txt`。
- 最终回归：Release 与原生 DLL 构建通过，CTest 5/5 全部通过（25.69s），包含原有导出、帧桥、缩放/拖拽与新多路分析。
- 风险：9 路 4K 同时播放的吞吐取决于硬件解码会话、显存与 GPU；本轮九路素材为小尺寸合成片，未验证九路 4K 持续播放、所有编码或 VFR 长片。Qt grab 截图仅用于检查控件布局，D3D 视频像素由原生 probe 与呈现计数验证。使用本轮 EXE、DLL 和字节码缓存一起部署；不生成压缩包。


## 17. 修正 AB/ABC/ABCD 滑块语义与拖动（2026-09-26）

- 文件：`MultiCompareView.*`、`PreviewPane.*`、`AnalysisPage.cpp`、`TestAnalysis.cpp`、README。
- 根因：多路布局在每次 MouseMove 中隐藏再显示所有拖柄，使 Windows/Qt 释放正在拖动的鼠标捕获；ABC/ABCD 错把裁切拼接实现成了可变大小的完整视频分格。
- 行为：常规三路是等大 3×1 完整视频，常规四路是等大 2×2 完整视频；ABC/ABCD 滑块使用完整画布上的同源坐标裁切。三段、半幅加两个四分之一区域、四象限分别由对应视频的局部组成；交点控制两个方向，单线可独立拖。支持 ABC 镜像与纵向三段。
- 渲染：滑块各路保持同一尺寸的视频 HWND、同一个画布原点，仅改变非重叠原生父容器裁剪和子窗口偏移；隐藏 pane 标题，源选择器固定在画布上方。复用原有缩放渲染，不增加 CPU 拼图、逐帧请求或解码器。进入滑块时统一到第一格的缩放/平移，强制同步，离开后恢复同步选项。
- 拖动：保留手柄可见状态并显式保持鼠标捕获，不在移动中 Hide/Show；拖柄和视频表面均保持原生句柄，拖动不会产生视频 Resize 事件。
- 验证：六种裁切布局各连续拖动 100 次，鼠标捕获无丢失、视频表面零 Resize、共同原点不变、裁切区互不重叠且完整覆盖画布。两路真实 2160p H.264 合成视频下 100 次拖动合计 303ms，无隐藏、无尺寸变化、帧时间戳不变；这不是所有机器或高负载播放下的帧率保证。日志 `build/wipe-geometry.txt`、`build/wipe-2160p.txt`。
- 回归：Release 构建成功；CTest 5/5 通过（31.62s），覆盖原有导出、VS 帧桥、平移缩放和多路分析。
- 风险：多显示器 DPI 和真实长片播放期间的实际手势仍需用户场景确认。本轮只更新程序与说明，不打压缩包。


## 18. 视频卸载、取色提示与 1.0.1 交付（2026-09-26）

- 本轮：紧凑源选择器及 × 卸载按钮；卸载释放目标解码器，重排剩余会话与 pane，保留原生句柄和各自偏移，支持继续导入。回调按 pane 查找当前索引，避免移除中间源后访问旧编号。
- RGB 根因：提示固定在完整画布左下，被其他象限的父窗口裁剪。改为按所有祖先裁切范围计算可见区域，在其左下显示。
- 颜色处理加宽至至少 240 逻辑像素，当前算法完整悬停提示、菜单互斥标记；两页 VRR 控件增加边框和选中状态。
- 验证：真实九路导入后卸载、偏移和 HWND 保留、重导入、全部移除及恢复；ABCD 四区域真实像素探针和提示几何检查通过。全套 CTest 五组通过。Qt 截图仅核查控件，不包含原生视频画面。
- 交付：根目录 changelog.md 记录本轮及版本累计更新，docs/release-1.0.1-draft.md 保存本地发布说明。代码推送 GitHub，生成含 shader-cache 的 7z 和校验文件，不创建或上传 GitHub Release。


## 19. 对比画布导出、参数布局与 RIFE（2026-09-30）

- 文件：AnalysisExport.cpp 与内嵌 comparison-export.py 承担画布快照和合成，AnalysisPage 保留播放职责；ExportWindow 复用现有队列，保护快照所有输入，音频输入应用负偏移/静音。按归一化区域、真实视频视口、缩放和平移计算裁切，RGB float 拼图后输出 YUV444P16。分段堆叠代替九张满尺寸空白画布。
- 语义：最大源按像素总数选原始宽高；所有导入源参与尺寸选择，仅可见源合成。全局零时刻至可见源/音频最早结束，负偏移前段保持首帧。静态布局快照；VFR 按源时间基近似。Jinc/Super-XBR/双边色度拒绝不一致导出，不静默降级；实时 D3D 与 VS 非逐像素复刻。
- UI：自适配文件名宽度、最小 160px 进度条、8px 间距；底部模式/音频加宽，颜色处理采用标准组合框。SVG 勾号及显式复选框。设置按钮移动源滤镜选择并保存。根因修复：侧栏裸 border-right QSS 继承到子控件，限定为 #processingSidebar 后消失；参数使用行间距与长标签换行、展开菜单宽度自适配。
- 滤镜：新增 14 个节点，Zsmooth 0.20.0 / VSZip 22.1.0 已有 API 4 算法直接复用，Descale r11 与 RIFE r9_mod_v33 实际部署。批量下载发现旧 DFTTest/CTMF/AWarp/Retinex 等为 API 3，未加入便携 staging。build-vapoursynth 调用 install-analysis-plugins，RIFE 模型来自用户现有转换目录，SHA-256 清单与许可证在 third_party/rife。最终用户无需单独安装，源码构建需提供已验证模型目录。
- RIFE：PKL 是 PyTorch 权重，NCNN 使用 param/bin。本机两个 NCNN 模型实际插值与 2× 帧数通过，AMD 610M 小图验证成功；RTX 4070 Laptop 单线程 1080p 暖态 12.88 / 10.97 张插值帧每秒，不能保证 24→48 实时。VS 高于源 FPS 时提升状态轮询，验证真实 GUI 提交奇数插值帧；CPU plane copy 和 GPU 吞吐限制仍存在。
- 验证：所有 AB/ABC/ABCD 子模式及九路网格在缩放/平移后实际取帧；四颜色 MKV 实际编码并逐象限检查、最大尺寸包含隐藏源，长名称与最小进度条、禁止覆盖非主源、设置入口验证。14 新滤镜和两个 RIFE 模型实际输出。

- 最终回归：Release 构建，CTest 5/5 通过；RIFE 实际主页面 700ms 测试提交 29 帧，其中 13 帧为补出来的奇数中间帧。日志 build/rife-preview.txt、build/new-filters.txt、build/composition-final.txt、build/rife-1080p.txt、build/rife-amd.txt。
