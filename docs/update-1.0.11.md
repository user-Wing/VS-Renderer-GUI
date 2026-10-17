# 1.0.11 本地更新与验证（2026-10-17）

本轮只修改本地代码、编译和更新运行目录；不推送 GitHub、不创建或修改 Release、不制作发布压缩包。追加要求生成独立单文件 `dist/VS-Player.exe`，首次运行自动创建同目录 `player.ini`。Full 固定目录为 `dist/VS-GUI-Full`，独立播放器目录为 `dist/1.0.11-PlayerLite`（原 1.0.10 目录整体改名，保留配置）。现存旧压缩包不修改。

## BD 菜单与章节

MyGO 下卷 `BRMM_10775` 的 authored FirstPlay 是约 9.009 秒版权片；Top Menu 是另一入口。首次打开光盘保留原始 FirstPlay；显式点击“BD 菜单”时，改用 Top Menu 索引对象建立独立导航缓存，避免重新播放版权片。仅修改缓存入口，原盘 index/MovieObject/素材不变。libVLC 切换 title 的同步调用移至后台，Qt 继续处理其原生视频窗口消息，关闭时等待切换完成后再释放播放器。

普通文件的上游章节字段是 `startTime100ns`，GUI 原先读取 `start100ns`，所有标记因此重叠于开头。现于 3FP GUI 适配层统一字段，BD/外部 MKS 章节仍沿用统一格式。

实盘回归：MyGO 正常首播后选择第 9 集通过；显式菜单入口以及从第 9 集返回菜单后再次选择第 9 集通过，正片位置约 1421.5 秒。三章节文件的 0 / 2 / 4 秒坐标通过。日志 `build/audit-20261017/player-entry-final.txt`：5 passed（包含 init/cleanup）。

## Renderer、预设与公开社区

- 右侧控件顺序：视频路径 / 浏览 / 前一帧 / 播放 / 后一帧 / 并排 / 缩放。路径框最大 320 逻辑像素；缩放框按完整文本分配宽度，D3D11 不截断。当前时间、缩短后的进度条、总时长同排。
- 预设保存全部时间组、名称、整秒边界及滤镜链；全片单组预设可导入任意当前组，保留目标组和其它组的边界。多组预设不再静默只取第一组覆盖当前组，应使用“导入所有时间组”。
- 内置播放器 VPY 的额外处理逻辑以脚本随时间组保存，避免空 nodes 元数据导致实际处理逻辑丢失；组内脚本使用独立 Python 命名空间运行。可视化滤镜修改后使用更新后的可视化链。没有 VSR 元数据的任意外部 VPY 保留“导入全部”方式。
- 社区默认配置和显示链接始终是公开页面：<https://www.modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI/PresetCommunity>。内部目录请求与对用户显示的源地址分离。递归扫描子目录，树状展开/收起，下载保留相对路径，支持选中目录或下载全部；继续校验大小和 SHA256。
- 本地预设管理也递归扫描 `vpy` 子目录并显示目录树。

Renderer 回归：布局、时间组脚本实际帧请求（边界 0/23/24/47/48/100）、当前/全部组导入、脚本组保存恢复、公开社区 Builtin 下载及导入、预设管理、启动时预设渲染，共 9 passed（包含 init/cleanup，并补测子目录内重命名保持目录）。日志 `build/audit-20261017/renderer-complete.txt`。

## VapourSynth 后端

从用户提供的 `VapourSynth64-Portable-R81.zip` 提取并校验官方 ABI3 wheel，更新 Python 3.15 运行环境中的核心，保留原有滤镜。加入 BM3D 11.0、BM3Dvk2 1.0、BestSource 22；版本与 SHA256 固定在 `tools/update-vapoursynth-runtime.py`，运行目录内保留 `runtime-components.json` 和许可证。构建及 staging 流程同步更新，后续编译不会重新落回 R80。

已实测：R81 加载成功；CPU BM3D 时域 VBasic → VAggregate → VFinal → VAggregate 成功；BM3Dvk2 的基本/最终估计、radius=1 时域处理成功，输入及中间节点保持 GPU 常驻；BestSource `gpu=True, gpufallback=False` → Vulkan resize RGBS → BM3Dvk2 → GPUDownload 成功，未静默回退软件解码。设备为 RTX 4070 Laptop GPU，Vulkan 1.4.351。日志 `test-r81.txt` / `test-r81-decode.txt` 位于本轮 audit 目录。

R81 官方说明是导出给 CUDA/Vulkan 的输入帧每个源帧至多复制一次并共享请求，并不等于已经证明整个 Vulkan 解码 → 降噪 → CUDA 链路只拷贝一次。此次没有测量 CUDA 导出次数，也没有把 GUI 的 CPU 帧桥改成 CUDA 零拷贝桥；现有默认生成的 FFMS2/L-SMASH 源保持原有行为。新 GPU 源/滤镜可用于 VPY。

来源：[R81](https://github.com/vapoursynth/vapoursynth/releases/tag/R81)、[BM3Dvk2](https://github.com/HolyWu/VapourSynth-bm3dvk2)、[BM3D](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-BM3D)、[BestSource](https://github.com/vapoursynth/bestsource)。

## 上游性能策略核对

截至本轮读取 GitHub API 和源码，上游主线仍是 `0fe651ddab8a2bb6ddb729f314f9f4c5117fb565`（API20、PR22）；最新打开的 PR23 是颜色命名调整，不是 CPU 调度优化。本地已包含 PR22。公开源码可确认线程数阶梯、按内存约束的视频队列和帧对象复用；本地软件队列预算 512 MiB，上游主线仍为 128 MiB，均还受队列帧数上限约束。AV1 本地使用 dav1d 匹配工作线程和帧上下文深度。

未在公开主线找到 P/E 核分阶段、单 CCD 优先再扩双 CCD 的亲和性/CPU Sets 调度或提高进程优先级实现。因此不能声称这些策略已经全部集成，也无法把截图 31.20 → 46.74 fps 当作本机同条件 +50% 的测量证据。此次未凭描述自行添加亲和性策略。

## 指定 8K 软件解码复测

素材：`D:/Video Compress Test/PlayerTest/ChouKaguya432048-Part.mkv`，7680×4320 AV1、YUV420P10、7001/146 ≈ 47.952 fps，带 AAC 6 声道；Ryzen 9 7940HX，16 核 32 线程。

隔离探针 CPU=1，3840×2160 输出，SDR 映射、音频解码/播放启用并静音，从 10 秒附近开始，剔除开头 5 秒，统计约 60 秒：

| 项目 | 结果 |
| --- | --- |
| 解码输出 | 42.17 fps |
| 提交视频帧 | 32.23 fps |
| 丢弃视频帧 | 596 |
| 音频欠载增量 | 204 |
| 播放时钟推进 / 实际时间 | 0.892 |
| 冷首帧 | 5.95 秒 |

同一片段纯 FFmpeg / libdav1d、16 线程、max_frame_delay=16、无音频/字幕/呈现：2877 帧 / 61.483 秒 = **46.79 fps**，约 0.976×。单纯解码距离 47.95 fps 仍差约 2.4% 吞吐；完整播放提交帧距离目标约少 32.8%，若按所需提升计算约需 48.8%。**当前不能判定 8K 软件播放通过。**

完整 VS Player 界面另行统计 60.011 秒（CPU=1、原始直通、4K 输出、AAC 启用）：解码 **41.03 fps**、提交 **31.03 fps**、丢弃 **599** 帧、音频欠载 **233** 次，播放时钟推进比 **0.868**，末帧落后音频时钟约 **1.71 秒**。距离片源少约 35.3% 的提交帧吞吐，达到目标需要约 54.5% 提升。采样过程正常结束（诊断测试 3 passed 仅代表采样有效，**不代表流畅播放通过**）。日志为 `8k-player-software-test.txt` / `8k-player-software.csv` / `8k-player-summary.json`。隔离探针详情 `8k-summary.json` / `8k-software-api20.csv`；纯解码日志 `8k-decode-only.txt`。最初误用 GPU 模式的探针文件已明确命名 `8k-hardware-invalid.*`，排除于所有软件性能结论。

## 最终构建与本地部署

Release 构建完成，Full 的 Renderer / Player / VSCap 及独立 PlayerLite 均标记 1.0.11。聚焦验证：Renderer 9 passed、Full Player 5 passed、Blu-ray 解析及缓存入口 7 passed、Lite 首播及无 VS 软件播放 4 passed（均包含 QtTest init/cleanup）。R81 的 CPU/GPU 滤镜与 GPU 解码验证另见上述日志。没有据此宣称全量测试套件通过。

部署使用 `tools/deploy-local-runtime.py`，对 EXE/DLL/运行时文件采用临时文件原子替换并备份，校验 INI、VPY、ICC/ICM 和 LUT 哈希不变；更新 `release.json` / `resource-manifest.json`，旧压缩包不修改。实际部署审计位于本轮 `build/deployment-*/deployment.json`。

## 仍需人工验收 / 旧失败记录

旧综合测试 `bdMenuFromEmptyWindow` 的系统光标/弹出菜单位置断言在当前不可交互桌面失败；旧 `frameEditTakesPriorityAndPrefetchKeepsGpuBusy` 的 frameEditPending 超时也仍保留失败记录，不能把本轮聚焦回归当作全量套件通过。前轮长光盘跨段 seek、UIAccess 置顶覆盖与真实 HDR 的待验收项没有在本轮宣称解决。

## PR23、单文件播放器与菜单入口续修

合入 [PR23](https://github.com/Lake1059/FFF_Project/pull/23) 的固定提交 `ce195ebd06e9cbb8352a443f0c874cf569a60733`，保留本项目 Native 扩展并重新生成可复现补丁，`git apply --check` 通过。对 PR22 基线的 Native 差异均为注释/命名措辞，ABI 和执行语句没有变化；上游 VB Host 的外部色彩注入模块属于其宿主界面，本项目 Qt 宿主没有因此自动获得该 VB 功能。不把本次合入宣称为新增性能优化。完整 IAMF 构建维持启用。

实际 computer-use 从右键“打开 BD 文件夹”选择 Downloads 中的 MyGO 下卷目录，首播约 9 秒版权片后能显示 PLAY ALL、第 8～13 集和字幕设置。复现并修正遗漏的右键“BD 主菜单 / 弹出菜单”入口：先前从 3FP 正片回菜单仍调用默认 FirstPlay，导致再播版权片；现在与播放列表“BD 菜单”共用直接返回方法，暂停时也恢复播放。新版实际画面验证两次返回均直接显示选集页，点击第 9 集可播放并交回 3FP。原盘文件不改，首次开盘仍遵循原始 FirstPlay。

聚焦测试：Full 返回/选集 4 passed；Lite 全部 7 passed；Blu-ray 解析 8 passed / 3 skipped（未设置对应实盘测试环境变量）。这不代替尚未解决的历史长光盘 seek 或全量测试套件。日志位于 `build/audit-pr23-20261017/`。

独立版固定输出 `VS-Player.exe`，同时嵌入 Qt、FFF.Native、FFmpeg shared DLL/CLI、libass、原生 BD 导航及 Lite 自身依赖，不再要求用户另外放 FFmpeg DLL。运行时首次解到 `%LOCALAPPDATA%/VSPlayerLite/<payload-hash>/` 并复用缓存；这是单文件分发，不是完全不解压的静态链接程序。配置固定在启动器旁 `player.ini`，现有配置不覆盖。仍保持 Lite 功能边界，不加入 VapourSynth、libVLC、LAV、madVR 或高级色彩运行时。

## 音频 / ASS 为什么影响视频：本轮受控对照

不能把 AAC/ASS 的低压缩解码成本等同于整条播放管线零成本。Native 的主 Pump 同时处理解复用、音频补给、音画时钟和视频提交；软件视频解码虽另有线程，输出仍是单槽交接，队列满或主 Pump 不能及时收帧时会反压解码线程。ASS 则另外打开容器、读取字幕包/字体，libass 排版栅格化后还要合成/上传图层。这里的“decoded fps”也是整条管线约束后的输出计数，不能当独立解码器满速基准。这些是源码中的耦合点，尚未做逐阶段耗时追踪，不能把其中某个点说成唯一根因。

本轮以新版 Full 实际播放器、CPU 解码模式 1、原始 D3D11、固定 3840×2160 输出、媒体 10 秒附近跳转后预热 5 秒、采样 20 秒进行对照。AAC 保持解码及音频输出，字幕开关仅改变是否呈现；均不改原片。两份 8K 原片抽取的视频压缩包 SHA256 相同（`3a267183...d381360a`），排除了不同视频编码内容的影响。

| 同条件样本 | 解码输出 fps | 视频提交 fps | 丢帧增量 | 音频欠载 | 平均 CPU 核当量 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 8K 纯视频 | 41.34 | 31.99 | 187 | 0 | 16.14 |
| 同码流 8K + AAC 6ch | 40.91 | 30.95 | 200 | 87 | 16.14 |
| 4K 444p12 + FLAC，ASS 隐藏 | 47.95 | 47.95 | 0 | 0 | 7.08 |
| 同一 4K 文件，ASS 显示 | 47.93 | 47.93 | 0 | 0 | 8.15 |

8K 的 AAC 对照提交差约 1.04 fps（3.3%），纯视频本身已大量丢帧，不能把此前约 46.79 fps 的无呈现 FFmpeg 和约 31 fps 的完整播放器之差都算成 AAC 开销。有音频时墙钟推进比 0.884、无音频 1.000，欠载说明供给/时钟发生停顿。此次普通 AAC 的 `audioBinaural=0`，不能归因于 OBR/HRTF。

4K ASS 在采样内有 292 次图层更新，CPU 多约 1.07 核，但两组均跟上 47.95 fps、零丢帧；**仅此 20 秒片段**没有复现用户所述高压片段降速。当前文件缓存已热，打开到首次观察到超过两帧约 0.57～0.87 秒，这不是磁盘/着色器全冷启动数据；也不能据此宣称其它字幕字体、片段或首次加载问题全部修复。未凭这些有限对照盲改线程/时钟策略。原始 CSV、日志及摘要为 `build/audit-pr23-20261017/av-subtitle-summary.json`。

补测将原 OP 的 ASS 无重编码封装进同一 8K + AAC 视频，20 秒输出 39.58 / 提交 29.79 fps、丢帧 196、音频欠载 93、字幕更新 144 次、CPU 16.51 核当量。此字幕时间轴沿用 OP，不是 8K 影片的对白，仅用于同类 ASS 渲染负载对照；样本保留在 build/audit 中，不修改原片。与 AAC 单独组相比未出现“大幅降速”，8K 完整软件播放仍未达 47.95 fps。

## 窗口底栏与 VSCap 模态输入续修

Player 窗口模式的原生视频区域从底部扣除控制栏高度，初始窗口高度同时增加同样的高度，底栏位于视频下方。缩放窗口也维持不重叠，进出全屏不累计增高；全屏保留原有覆盖式自动隐藏底栏。信息面板仅在全屏预留浮层底部间距，避免窗口模式重复扣除。

VSCap 设置使用 `QDialog::exec()` 的应用模态输入。此前新建的无父级置顶截图层没有接管模态输入，因此设置阻断截图层，而截图层又覆盖设置，表现为整屏无法操作。现在仅在已经存在模态窗口时，将截图层设为新的应用模态窗口；Esc/完成截图后恢复原设置窗口，未保存的输入仍保留。保存文件对话框以当前模态窗口为父级，避免落到被截图层遮挡的位置。普通截图与贴图的既有模式保持。

点按钮可点击选择边框模式，默认选中；与填充按钮组成互斥组。滚轮事件仍调节线宽且不会取消当前模式，矩形/圆形共用。填充方块使用独立的 20px 图标，其中实心方块为 16px；按钮固定为相邻矩形按钮的高度，不通过增大字体撑高控件。

验证使用自动回归，不调用 computer-use：新增模态输入复现用例在修复前失败（设置占用 activeModalWidget），修复后可框选、按 Esc 并返回设置；边框按钮修复前不支持选中，修复后模式与像素验证通过。透明度/全屏专项的控件 alpha 与布局检查通过，桌面像素合成检查因前台被占用跳过（2 passed / 1 skipped），未为此操纵用户桌面。Windows VSCap 工作流 16 passed；播放器窗口/全屏/底栏回归 5 passed；Lite 模式和软件播放 4 passed（均包含 init/cleanup）。日志位于 `build/audit-controls-modal-20261017/`。离屏复现和 Windows 平台测试分开记录，不以离屏测试替代实际 Windows Qt 事件路由。没有将这些聚焦检查宣称为全量播放器测试通过。

本轮保持 1.0.11，编译并更新 Full、目录式 Lite 和 `dist/VS-Player.exe`；不上传、不制作发布压缩包、不改 Release。部署继续备份替换并校验 INI、VPY、LUT 和色彩配置哈希。

## 单文件 FFmpeg 外置（后续要求，覆盖此前内置方案）

`VS-Player.exe` 移除 FFmpeg CLI、shared DLL 和资源清单中标注的 FFmpeg 专用依赖，保留 Qt、Native、字幕/光盘组件及共同依赖。Full 和目录式 Lite 的 FFmpeg 文件不删除。启动器先查自身目录，再查进程 PATH；缺少 ffmpeg.exe、ffprobe.exe 或匹配 ABI 的 shared x64 DLL 时明确提示并退出，不静默启动一个无法播放的视频后端。静态单文件 ffmpeg.exe 不能替代 Native 所需共享库；当前 ABI 是 avcodec63、avformat63、avutil61、swresample7、swscale10、avfilter12。

启动后的子进程 PATH 包含启动器目录和原 PATH；音频元数据/封面、图片回退解码及截图转换在缓存目录没有 CLI 时改用外部 PATH 工具。INI 仍放启动器旁，更新时不覆盖。运行时缓存仍按 payload 哈希隔离，不复用旧的内置 FFmpeg 缓存。

外置后单文件约 30.18 MiB，此前 97.86 MiB，减少约 69.2%；内嵌 ZIP 扫描确认 FFmpeg CLI、shared DLL 及清单中的专用依赖均未包含。启动器测试通过“完全缺失、仅有 CLI、同目录组件齐全、PATH 组件齐全”四种情况，以及已有 INI 的重开保留。为遵守不使用 computer-use 的要求，提示/进程派发测试直接编译实际启动器源码，仅用测试包装器记录 MessageBoxW 文本和最终 CreateProcessW 参数；没有声称做了弹窗画面验收。真实解包缓存副本中删除全部 FFmpeg，采用外部 PATH 后运行 Lite 的 ffprobe 探测与软件视频播放，两项及 init/cleanup 共 4 passed。启动器的临时测试包装器和原始报告在 `build/audit-external-ffmpeg-20261017/`。

VSCap 填充按钮后续修正：内部 20px 图标绘制 16px 实心方块，点/方块控件高度固定为邻近矩形工具的高度；不使用大字体增加按钮或工具条的 Y 尺寸。相关模式、滚轮、模态截图和工具条回归 5 passed（含 init/cleanup），已重新编译 VSCap。

## 图标与发行准备（最终追加授权）

单文件启动器在链接阶段通过 windres 编译并嵌入 `assets/icons/player.ico`，资源组 ID 101；图标属于外层 EXE，不只存在于缓存中的播放器。FFmpeg 外置策略、同目录 INI 与版本 1.0.11 保持。最终授权为推送本轮源码并准备 1.0.11 无附件 Release Draft，覆盖前述本地阶段的“不上传/不改 Release”约束；Full 由维护者自行打包，所有二进制附件由维护者上传。

重新生成的单文件为 31,717,490 bytes（30.25 MiB）。通过 Windows 资源 API 读取外层 EXE 的图标组及 10 个图标资源，与原 ICO 的各尺寸图像字节逐项比对一致；ZIP CRC、payload 哈希及 Native 与目录式 Lite 同源校验通过，未内嵌 FFmpeg 或用户 INI。审计记录位于 `build/audit-icon-release-20261017/result.json`。本次部署仍确认原设置哈希不变。
