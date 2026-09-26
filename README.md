# VS Renderer

Windows 上用图形化滤镜链生成 VapourSynth 脚本，并以双路画面对照源视频与处理结果的 Qt 6 桌面工具。

> 当前版本：`1.0.1`。已具备紧凑型双路 UI、滤镜链编辑、参数面板、VPY 生成，以及 VapourSynth → 3FP 同帧实时预览。

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

左上角三横按钮展开或收起导航，切换“VS 实时渲染”和“图像分析比对”。切页暂停离开的播放器，保留文件、画面设置和每个视频独立的时间偏移。

顶部“打开视频”可多选，也可直接拖入多个视频，最多 9 个。对比页接受 MP4、MKV、MOV、AVI、WebM、TS 等主流格式（具体编码由随附 FFmpeg/3FP 支持），不经过 VS；**MKV 限制仅用于第一页导出**。

- 2 路：AB 并排或 A/B 滑块。
- 3 路：常规模式为三个等大完整画面横排（3×1）。ABC 滑块把同一画布分为三段，分别显示三个视频的对应局部；可选横向/纵向三段，或半幅加两个四分之一区域（支持左右镜像），交点同时控制横竖切割位置。
- 4 路及以上：可选 AB、ABC、ABCD；四路常规模式为 2×2 完整画面，ABCD 滑块则用同一坐标系下的四个局部拼成一张画面，中心点可整体拖动，横竖线也可独立调整。
- 5–9 路：导入后自动使用三列网格，依次为 3+2、3+3、3+3+1、3+3+2、3+3+3，也可切回 AB/ABC/ABCD。

所有滑块模式共用完整画布、缩放和移动坐标，复用原有局部区域缩放渲染。拖动仅改变原生父窗口的裁切区域，不改变视频表面尺寸，也不重新定位或解码帧；源下拉框固定放在画布上方，避免混入拼接区域。常规模式可独立控制缩放，滑块模式强制同步以保持拼接对齐。

每格顶部使用紧凑的视频源下拉框，右侧 × 可卸载该视频，不删除源文件；其余视频保留解码会话和对齐偏移，腾出的名额可继续导入。每格与每条对齐轨道都有视频源下拉框。选择已占用的视频会自动交换位置，隐藏视频的偏移不会重置。未导入时不显示底部控件；导入后按当前模式显示轨道，最多 4 行，网格中其余视频可通过下拉框换到对齐轨道。

进度条显示各路本地时间，拖动任一条同步移动当前参与比较的视频并保持偏移。每行 ±1 帧、±1 秒只调整本视频；全局逐帧以第一格实际帧时间为准，其他视频按时间定位。音频下拉可选择任意已导入视频或静音；VRR 低延迟与 VRR Pacing 位于播放控制右侧。上下平移按实际视频显示区域计算，与水平拖动保持相同像素响应。

右下角“颜色处理”控制独立色度上采样：Nearest、Bilinear、Bicubic、Softcubic、Mitchell、Lanczos、Spline36、Jinc、Bilateral、亮度引导双边重建及 Super-XBR 单阶段。悬停显示当前选中算法的完整名称，菜单用勾选标记当前项；颜色处理控件已加宽，两个 VRR 开关有边框和选中高亮。Cubic 与 Reconstruction 使用二级菜单。放大算法新增 Spline36 和 Super-XBR 单阶段；后者是 Hyllian 开源 pass-0 对角核的移植，**并非完整三阶段 Super-XBR**。不包含 NGU 或其他 madVR 专有实现。

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
