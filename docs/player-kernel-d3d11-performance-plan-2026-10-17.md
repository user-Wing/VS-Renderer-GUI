# VS Player 播放内核性能优化设计与验证计划（2026-10-17）

> 状态：**设计与测试计划，未实施、未验证新方案**。基线以 AgentDock-PC 的 `VS-Renderer-GUI` 项目、提交 `d00741e` 和本地 1.0.12 发布目录为准。本文提供可交给下一轮开发会话实施的最小改动路线；不把历史成绩算作本轮成绩。
>
> 作用范围：`src/backend`、`src/player`、`build/native-api20/FFF.Native/3FP`（构建工作树）及对应的原生补丁/构建入口。最终实现必须更新**可重建的补丁或权威源码**，不能只修改临时 `build` 文件。本文本身不修改程序、运行目录、用户配置或任何媒体。

## 1. 目标、约束和成功定义

### 1.1 总目标

在不降低源视频位深/色度、不改变默认色彩算法、不丢帧作弊的前提下，提高高码率 2160p/4320p 软件解码播放吞吐和帧时间稳定性。优先解决 AV1 4:4:4 12-bit、HEVC 4:4:4 10-bit、大尺寸 YUV CPU→D3D11 上传时的反压。D3D11VA 硬解和普通 SDR/HDR 原生直通是必须保留的性能及质量基线。

**不要假定**增加缓冲、更多 FFmpeg 线程、CPU 亲和性或进程优先级必然提速；同一实验只改变一个变量。停止条件优先于性能收益。

### 1.2 全局硬性验收

1. 正确性：完整解码或预期有界跳帧；不重复呈现旧帧冒充新帧；视频帧时间戳单调、音画时钟连续；暂停、seek、换轨、EOF、重开、设备丢失恢复不得死锁或泄漏。
2. 精度：`yuv444p12le` 仍按 12-bit/4:4:4 解码，不能降至 4:2:0 或 8-bit；VS 输出的 range/matrix/primaries/transfer/chroma 不得被原流元数据重新覆盖。HDR scRGB FP16 与 SDR 输出保持既有格式及颜色。
3. 性能：以**相同硬件、同一时间窗口、同一渲染与音频设置**比较基线/候选，每配置至少三轮。对未触及瓶颈的 1080p、4K 硬解路径不接受稳定段吞吐退化；p95/p99 帧延迟、丢帧和音频欠载不能恶化。
4. 内存：必须记录 RSS/Private Bytes、FFmpeg 解码器在飞帧、应用队列帧和 GPU 专用/共享显存。512 MiB 是现有软件帧预算，**不是整个播放器内存的硬上限**；FFmpeg 内部参考帧/流水线、GPU 纹理、交接帧都应纳入实验账本。
5. 部署：先独立候选 DLL 和测试目录，测试通过后才能考虑替换 `dist/VS-GUI-Full`、`dist/1.0.12-PlayerLite` 和单文件版本。保留旧二进制回滚，INI/VPY/ICC/LUT 逐文件哈希不变。
6. 任何声称“8K48 通过”的结论，必须是固定完整播放链路（含音频/字幕说明）的**稳态实际提交及显示节奏**；仅解码输出、测试函数返回成功或零新增 dropped 都不够。

### 1.3 范围外

不在本轮更换 FFmpeg/dav1d/libplacebo/Qt/VS 版本；不改 HDR 算法、插帧/Anime4K 模型；不添加通用 CPU 任务调度框架、驱动、内核优先级或未经验证的 Vulkan/CUDA 零拷贝链。CPU 优先级与 CCD 调度仅作为可关闭的后续实验。

## 2. 当前已实现的能力与定位

| 模块 | 本地事实 | 对应证据 |
| --- | --- | --- |
| 解码队列 | 最多 8 帧；软件应用队列预算 512 MiB，硬解相关常量 128 MiB；目标约 150 ms 预取；当前实现还根据 decoder 在飞帧扣减队列空间，最低队列保护值为 2 | `build/native-api20/FFF.Native/3FP/Core/PlayerSession.cpp`：`VideoFrameQueueLimit`、`TargetVideoLookAhead100ns` |
| 软件解码线程 | GUI CPU 解码模式默认允许自适应，`VSR_ADAPTIVE_THREADS=0` 可关闭；普通策略初始目标 16，检测缺帧且无余量时向 24/32 攀升，重新打开解码器/seek 有代价 | `src/backend/ThreeFpPlayer.cpp`；`PlayerSession.cpp`：`SelectSoftwareDecoderThreads`、`EvaluateDecoderLoad` |
| dav1d 特化 | 重负载 4K+ 444p12、8K420p10 按不同上限设置 `thread_count` 和 `max_frame_delay`；dav1d 不参加通用重开线程阶梯 | `PlayerSession.cpp`：`OpenDecoder` |
| 独立解码 | 现有 `SoftwareVideoDecodeWorker` 及有界包输入、单个 `softwareVideoOutput_` 交接槽；正常播放下 `QueueSoftwareVideoDecode` 的入口限 `libdav1d` | `PlayerSession.cpp`：约 3485–3612 |
| 大图上传 | 大于等于 4K 的 planar/semi-planar 使用 DEFAULT texture + `UpdateSubresource`；较小平面使用 DYNAMIC + `Map(WRITE_DISCARD)` | `build/native-api20/FFF.Native/3FP/Render/VideoRenderer.cpp`：约 3595–3627、6085–6150 |
| 硬解直通 | VideoProcessor 可使用 D3D11 解码 slice；Shader/Jinc/高级颜色时保留必要 GPU 复制 | `VideoRenderer.cpp`；`docs/nuxbox-performance-session-2026-10-04.md` |
| DXGI | `FLIP_DISCARD`、`BufferCount=3`、`IDXGIDevice1::SetMaximumFrameLatency(2)`，默认 `Present(1,0)`；VRR/tearing 为可选分支 | `VideoRenderer.cpp`：约 3120–3185、7330–7355 |
| 色彩 | Full 默认 `engine=2`：普通 SDR 与可用 HDR→HDR 保留原生路径，HDR→SDR 需要时用 libplacebo；ICC/LUT、BT.2020 CL/NCL、PQ/HLG、HDR10+ 部分元数据和 FP16 scRGB 已有专用处理 | `src/player/PlayerColor.cpp`、`src/color/ColorBridge.c`、`NativeColorEngine.h` |
| CPU 拓扑/优先级 | 未找到主解码链 P/E 级别调度、先单 CCD 再跨 CCD、进程整体提权；只发现后台封面线程主动降至 `THREAD_PRIORITY_BELOW_NORMAL` | `VideoRenderer.cpp` 的 `CoverBackdropThread`；对原生播放目录的亲和性/优先级 API 搜索 |
| 构建对应 | 先前核验 Full/Lite/Release 三处 `FFF.Native.dll` 的 SHA256 相同，标识前缀 `666E5EC4C5A0E377`；实施前仍应重新核对当前时间戳/哈希 | 本轮现场对照（非持续不变的承诺） |

**特别注意**：`.deps/fff-upstream-api20` 的上游源码默认有 128 MiB/8 线程等旧常量；它不等同于本地已部署的 `build/native-api20` 合并后代码。实现分析以实际构建 DLL 的源码、补丁和二进制核对为准。

## 3. 历史实测：必须保留的正反证据

### 3.1 HEVC 444p10、4K24、高码率尖峰

`docs/hevc444-peak-diagnosis-1.0.6.md` 在 7940HX、4K 输出、强制 CPU 解码、D3D11 原生链下对 `D:/RebellionHEVC444p10-700MBPS-SoftwareLast.mkv` 记录了约 812 Mbps 峰值：

- 8 线程 + 128 MiB：峰区曾新增 5–7 个丢帧。
- 8 线程 + 512 MiB：仍出现丢帧。
- 16 线程 + 128 MiB：相同样本两次测试达到零丢帧。

因此不能只通过提高缓存预算推断能解决卡顿；FFmpeg 内部并行度和主 Pump 阻塞更关键。随后 1.0.7 的 `docs/software-decode-subtitles-1.0.7.md` 记录了改进后完整 2252 帧无丢失的同机测试，但**不能直接外推到所有 4:4:4 视频**。

### 3.2 8K48 420p10 软件解码仍失败

`docs/update-1.0.11.md`：7940HX、8K AV1 47.952fps、CPU 解码、4K 显示，完整 GUI 的 60s 窗口约 **41.03 解码 fps / 31.03 提交 fps / 599 丢帧 / 233 音频欠载**；纯 FFmpeg/libdav1d 不呈现约 **46.79 fps**。不能把完整播放链约 31 fps 与纯解码差额全部归咎于 GPU，也不能凭补丁通过推断已达到实时 8K48。

### 3.3 4K444p12 Anime 现有通过项

`docs/player-anime-performance-1.0.12.md`：4K 444p12 AV1 约 47.95fps → 1440p CNN/CNN+，指定窗口约 30s 可实现源速率零新增丢帧；**Y412→4K48 原型尚未通过**。增强链必须与普通直通分开计算。

### 3.4 旧上传基准

当前 `VideoRenderer.cpp` 注释记录 10-bit 平面上传对照：1080p Map/Update 0.582/0.546ms、4K 3.293/2.886ms、8K 22.266/15.213ms。这些数据是在旧实验条件下测得，不是当前 A/B 结果。禁止将“8K Update 较快”误写成“GPU 已足够快”。

## 4. 分阶段实现设计

### 4.1 P0-A：测量先行，不动算法

**目的**：将问题区分为读取/解复用、FFmpeg send/receive、CPU 在飞帧等待、应用单槽交接、主 Pump/音频/字幕、D3D11 提交、GPU 着色、DXGI Present 与真实上屏。

增量埋点（debug/perf build，默认关闭）：

1. `av_read_frame`、`avcodec_send_packet`、`avcodec_receive_frame` 的墙钟耗时；另记 FFmpeg 内部 frame delay、线程数和输入包 PTS。调用等待时间不得描述成“纯 CPU 解码耗时”。
2. `softwareVideoPackets_`、`softwareVideoOutput_`、`videoFrameQueue_` 的长度、帧寿命、生产者/消费者等待纳秒；增加 `frame_generation` 以区分 seek 前后；记录丢弃原因。
3. `UpdateSubresource`/`Map` 的 CPU 提交耗时、GPU timestamp 的异步查询结果；对 query 使用延迟收集，**不能为了测 GPU 在热路径 `GetData` 自旋等待**。
4. 逐秒：decoded/accepted/dropped/coalesced、实际显示 cadence（DXGI frame statistics 可用时）、audio underruns、media-clock error、CPU 核当量、工作集、VRAM、输出色深、实际 swap/dest 尺寸。
5. 从诊断日志区分首帧、seek 预热、稳定段、峰值段、EOF；每帧可按 frame-id/PTS 关联，不允许只有平均 fps。

输出建议：`build/perf-kernel-20261017/baseline/<case>/<run>.csv`、`stage.csv`、`system.csv`、`summary.json`。采集开销需用 profiling off/on A/B 核对；正式性能比较关闭重度逐帧日志，仅使用低干扰计数器。

**验收**：确认测试本身不破坏帧序/音频时钟；统计可区分“解码快、提交慢”“GPU 慢”和“时钟/音频停顿”；日志附实际 DLL、FFmpeg DLL、媒体哈希/时长、OS/驱动、电源计划、刷新率、色彩设置。未做到前不开始声称性能提升。

### 4.2 P0-B：软件解码 → 呈现的有界解耦

**现状**：`SoftwareVideoDecodeWorker` 已存在，但仅限正常 libdav1d；其 `softwareVideoOutput_` 是单槽，主会话取帧慢时解码工作者仍会阻塞。

**最小候选**（依次单独验证）：

- B1：保持原有 FFmpeg 解码器、线程数/帧预算和帧顺序，仅将单槽改为**可选的 2 帧或 3 帧有界交接环**，用引用计数持有 `AVFrame`，确保不会复制整个 4:4:4 12-bit 平面；生产者达到上限阻塞，不能无限入队。
- B2：对 HEVC CPU 解码（以及其它确实使用 `AVCodecContext` 的 FFmpeg CPU 解码器）复用同一独立工作线程机制，但禁止会话线程和工作线程无互斥地同时 `send/receive/flush/free`；seek/flush/切流/关闭先停工作者并清除旧代际。
- B3：分离读取/解码与提交的锁范围，避免持有会话或 D3D11 设备锁等待输入；避免因为字幕/音频更新造成循环性的帧交接反压。

**内存与调度约束**：

- 应用解码队列现有 512 MiB 预算、八帧上限、150ms 预取仍保留；**新交接槽必须与主队列及在飞帧一起核算**，不可变相扩成 512 MiB + 3 大帧 + 32 参考帧而不记账。
- 4K444p12 一帧约 47.46 MiB，8K444p12 一帧约 189.84 MiB。大帧模式应优先减小交接槽数而不是继续增大总缓冲。
- `av_frame_clone` 只增引用并不保证引用生命周期零成本；测试必须涵盖硬解纹理/软件 AVFrame 的引用释放时机。
- 不能因为队列更深而悄悄提高 A/V 延迟；非实时文件播放可容许少量预取，但 seek/pause 需立即失效旧代际。

**回滚**：保持原单槽/原 HEVC 线程方式可由实验开关恢复；如果 p99 延迟、内存、seek 或音频欠载恶化，即便平均 fps 提升也不默认启用。

### 4.3 P0-C：D3D11 大帧上传流水化

**现状**：4K/8K planar 当前使用 `D3D11_USAGE_DEFAULT` + `UpdateSubresource`；硬解直通在无需 Shader 时已可使用解码 surface slice，不应强制增加拷贝。

候选顺序：

1. C1：针对软件 YUV 平面，在保持输入格式和每一平面 R16/R8 texture 映射不变的前提下，对 `DEFAULT` 上传纹理做 2/3 组复用 A/B，允许 CPU 准备下帧、GPU 消费上帧。复用前需要明确的资源仍在 GPU 使用时的驱动行为；使用 fence/query 的无阻塞收割，不在热路径等待完成。
2. C2：只有 C1 的 ETW/GPU timestamps 证实同步等待时，才探索 CPU 侧紧凑排列、减少多余 `memcpy`、行对齐/上传批次以及可复用 staging 路径。不能把 `Map(WRITE_DISCARD)` 套到 `STAGING` 纹理，不能认为 `ID3D11DeviceContext` 的 deferred context 必然提高 GPU 实际吞吐。
3. C3：高级颜色路径仅在存在额外 `NativeColorEngine::draw` 的动态纹理→DEFAULT `CopyResource` 且其确实成为瓶颈时研究零重复复制或资源常驻；确保同步、安全的 resource usage、HDR metadata 完整，不要将 GPU→CPU 回传引入显示热路径。

对照 A/B：原单纹理、2 组、3 组；分别在 4K444p10、4K444p12、8K420p10、8K444p12（存在样本时）进行，记录 CPU Update 延迟 p50/p95/p99、GPU 拷贝/Shader、GPU 内存峰值、实际显示帧。若帧数未提高却额外占用数百 MiB 显存，应回退。

### 4.4 P1-A：CPU 拓扑感知，先采样后设置亲和性

**用户提出的策略**：“优先 P/E 核、先单 CCD 后双 CCD”。当前项目并未实现此策略；7940HX 不是 Intel P/E 混合核心，因此不要把 P/E 优化强行应用到它身上。

建议按 `GetSystemCpuSetInformation`、`GetLogicalProcessorInformationEx` 和 processor group/NUMA/topology 查询建立**实验用**映射，先核验物理核心/SMT/LLC/CCD 对应关系，不能直接假设 CPU 编号 0–15 属于同一个 CCD。

候选 A/B：

- A0 Windows 默认调度（强制保留）；A1 只给音频/时钟/呈现关键线程设置合适的软 CPU Sets；A2 解码工作者尽量同一 CCD；A3 在持续发生解码吞吐不足时扩展到另一 CCD/所有核。
- 混合 P/E 机器单独运行 P 核优先、系统默认、部分 E 核参与三组。FFmpeg 内部线程创建由第三方库管理；**若未可靠控制其线程集合，不得宣称已经绑定所有解码线程**。
- 单 CCD 可能丧失一半物理核心吞吐，跨 CCD 可能增加缓存/内存访问；只按实测 p99/完成帧数选择，不能因“局部性理论更好”而默认收窄亲和性。
- 禁止运行中频繁更换亲和性和逐帧迁移；固定窗口阶段调整后需稳定观察数秒再计量。

### 4.5 P1-B：调度优先级和 MMCSS

当前未见主播放线程统一设置高优先级，只有封面后台线程降低优先级。候选为：音频和视频呈现的**关键实时性线程**试用适当的 `AvSetMmThreadCharacteristics` / `AvSetMmThreadPriority`，保持 FFmpeg 解码工作者普通优先级为基线。进程级 `HIGH_PRIORITY_CLASS` 仅作为另一个独立可撤销实验，绝不使用 `REALTIME_PRIORITY_CLASS`。

验证必须在后台 CPU 压力 0/50/90%、窗口前后台状态、HAGS/VSync 状态固定时逐项对照，检查音频欠载、UI 输入延迟、前台其他进程响应和是否引入优先级反转。任何系统交互退化或音频欠载增加即撤销，不因微幅 fps 改善继续启用。

### 4.6 P2：交换链与色彩成本的隔离对照

**当前三缓冲并非缺失项**：`FLIP_DISCARD`、3 buffers、最大帧延迟 2 已实现，盲目扩大到 4–6 不在推荐范围。

- D1：保持现有模式；D2：实验性创建 `FRAME_LATENCY_WAITABLE_OBJECT` 交换链并用对应 waitable object 进行节奏控制；注意当前仅用 device-level `SetMaximumFrameLatency(2)`，不能在未打开相关 swap-chain flag 时认为 swapchain2 latency API 有效。
- D3：普通 SDR、HDR 原生、HDR→SDR libplacebo 三套处理分开计时；比较是否存在重复 GPU Copy 或额外 ICC/LUT 资源生成。处理 HDR→SDR 时不得通过关闭色调/色域映射换吞吐。
- D4：Shader/常量与 LUT 重用以现有缓存为基线，已经分开 t3 HDR 曲线、t4 transform LUT、t5 Jinc 权重；不能再引入同槽资源污染。

颜色回归同时比对 GPU 离屏高精度读回和真实显示器（如有色度仪另测 ΔE），前者证明代码路径数值一致，后者才能声称绝对色准。没有测量不宣称“比 mpv 色准高”。

## 5. 具体测试矩阵与验收办法

### 5.1 运行条件（所有测试共用）

- 优先 AgentDock-PC 上的 Ryzen 9 7940HX / RTX 4070 Laptop。CPU/GPU、供电计划、温度、系统负载、显卡驱动、Windows HDR、输出色深、刷新率、VS/字幕和解码模式都写入 JSON。硬件解码必须显式显示 D3D11VA，软件解码必须显式显示 CPU，不能依靠肉眼判断。
- 固定 3840×2160 输出测试 4K/8K 源；另对 8K 原生输出单独成组。变更窗口尺寸即作废该轮。
- 同素材取三段：简单稳定段、压缩复杂尖峰、字幕密集段；先热缓存重复三轮，再单独冷启动。峰段不可用视频总平均码率代替分秒码率。
- 每轮启动 seek 后至少预热 5 秒；稳态至少 60 秒，HEVC 尖峰样本优先覆盖完整约 94 秒；8K48 困难样本至少覆盖 60 秒并单独记录失败。
- 记录帧级帧龄/PTS，在帧率场景中核对重复帧、丢帧、合并帧、真正上屏、音频欠载、媒体时间/真实时间比例；`presentedVideoFrames` 仅代表接受帧，`swapChainPresents` 包括 OSD 重绘，**不能把二者任一个单独当作无丢帧证明**。
- 三轮结果报告中位数及最差值；改善未超过测量噪声时写“未证实收益”。优先同一 DLL 多次使用实验 gate 的 interleaved A/B，减少环境漂移。

### 5.2 测试用例

| ID | 素材与场景 | 对照变量 | 主指标及通过条件 |
| --- | --- | --- | --- |
| T01 | `D:/RebellionHEVC444p10-700MBPS-SoftwareLast.mkv`，4K24 / 约 812 Mbps 峰段 | 基线 vs HEVC 异步/交接槽 1/2/3；线程数保持一致 | 完整帧时序、无新增死锁；峰值丢帧 ≤ 基线且 p99 阻塞下降；若原基线已零丢帧，以等待时延改善为目标 |
| T02 | `D:/Video Compress Test/PlayerTest/ChouKaguya432048-Part.mkv`，8K420p10 47.95fps 软件解码→4K | baseline / B / C / B+C（最终才测交互） | decoded/accepted/displayed、drop、underrun、clock；量化当前约 31fps 不足。48fps 达标需稳定贴近源帧率且零异常，而不是单测秒数显示 0 drops |
| T03 | `D:/Video Compress Test/PlayerTest/ChouKaguya432048-NoAudio.mkv`，8K SDR 无音轨 | A/B / vs clock-only | 无音轨 QPC 时钟按墙钟推进、seek 正常、EOF 不提前；Jinc SDR 灰阶无明显偏暗 |
| T04 | `D:/Video Compress Test/Large-Test/1080p-2D.mkv`，标签 unspecified | Baseline / new upload/presenter | 不自动标 PQ/HLG，不出现 gamma/色域变化；1080p 性能无退化 |
| T05 | 实测存在时的 4K444p12 AV1 + FLAC + ASS，1440p Anime CNN/CNN+ | 当前已通过链路 / 任一 P0 调整 | 30s 连续窗口源 47.95fps、零额外 VS 跳过/渲染丢帧/合并，输出持续 444 16-bit |
| T06 | 4K/8K D3D11VA 硬解 420 8/10bit → 4K | 基线 / 纹理池/帧时延变更 | 60s 零新增 dropped/coalesced/underrun；不得增加 CPU 回传；显存不能无必要增长 |
| T07 | SDR 709 full/limited、2020 CL/NCL、PQ/HLG、HDR→SDR、Jinc+LUT | Baseline / B / C / 色彩引擎 | GPU 读回数值偏差在现有回归容差内，scRGB 203 nit 白位、t3/t4/t5 资源不冲突，浮点超白不被截断 |
| T08 | seek × 20、暂停/恢复 × 20、换轨 × 10、EOF/连续播放、设备重建 | B1/B2 | 没有死锁/旧代际帧、不访问释放的 AVFrame；在场线程退出、内存峰值与稳态资源正常 |
| T09 | 单 CCD/跨 CCD/默认，4/8/16/24/32 解码工作者 | A0/A1/A2/A3 | 以 p95/p99 与 60s 被呈现帧、音频欠载判断；不能只看 CPU 占用/频率 |
| T10 | 负载 0%、50%、90%，窗口/全屏，后台字幕和音频 | Normal/MMCSS/High | 前台可操作，系统调度不倒退，p99 音画延迟不变坏 |
| T11 | 同一显示器 VSync/VRR（如支持），60Hz/144Hz（如支持） | 3-buffer current / waitable prototype | 比较 actual display cadence、Present wait、尾延迟，不以 Present 被丢弃数量冒充视频解码丢帧 |
| T12 | mpv/MPC/PotPlayer 同素材同配置的外部参照 | 只做实际可比条件 | 固定软件解码、D3D11 输出、缩放、HDR/ICC、字幕、刷新率；记录各播放器版本和统计定义，不混合默认画质链比较 |

**样本路径仅来自项目既有记录；开始跑前必须用 `Test-Path` 与 ffprobe 检查存在性、时长、`pix_fmt`、fps 和色彩标签。缺失素材标记未执行，不能替换一个相似文件后沿用旧结论。**

### 5.3 功能回归（现有 + 拟补充）

现有 `tests/TestPlayer.cpp` 中可复用：
- `softwareCodecPipeline`：HEVC/AV1 CPU 解码，暂停、seek、音频/EOF。
- `vsPrefetchSurvivesShortSchedulingDelay`：VS 短暂延迟后的顺序追赶/大延迟重同步。
- `animeCadenceMeasurement`：通过 `VSR_ANIME_PERF_SOURCE`、`VSR_PERF_STAGE` 等环境变量记录真实 GUI 出帧。
- `jincColorMatchesNative`：Jinc/原生色彩一致性、SDR/PQ/HLG 和 16-bit 444。
- `colorSettingsPreserveVsChain` 与 `generatedColorMatrixUsesVsProps`：VS 色彩元数据与 VPY 行为。

后续实施时**新增而不是声称已存在**的测试建议：
- `softwareHandoffQueueBoundedAndOrdered`：输入 1/2/3 槽，高负载+seek generation，多帧交接有限且无乱序。
- `hevcAsyncDecodeShutdownAndFlush`：HEVC 线程阻塞时执行 seek、切轨、关闭、设备重建，确保不死锁。
- `uploadRingPreserves444HighBitDepth`：16-bit 容器中的 10/12-bit 数据与色度平面对照，宽度、stride、行对齐和 R16_UNORM 一致。
- `colorLutJincResourcesIsolated`：动态 HDR t3、颜色 LUT t4、Jinc t5 的实际资源绑定与像素回归。
- `presentModeQueueDepthUnderLoad`：DXGI 深度与真实上屏节奏对照。
- `threadAffinityNeverStarvesAudio`：CPU Sets/优先级在背景压力下不会抢占音频关键处理。

执行测试命令应以实际构建 preset 的 ctest 发现结果为准（先 `ctest --preset windows-mingw-release -N`），不要假定测试 exe 命名和工作目录。该机器本轮 PATH 没检测到 `cmake` 命令，因此这里不声称已构建/运行以上新增测试。

### 5.4 性能命令模板（仅示例，实施前检查路径与产物）

```powershell
Set-Location "C:\Users\ARXChem\Documents\Open Source Projects\VS-Renderer-GUI"
$video = "D:\RebellionHEVC444p10-700MBPS-SoftwareLast.mkv"
$dll = "dist\VS-GUI-Full\FFF.Native.dll"
$probe = "build\perf-player\perf-3fp.exe"
if (!(Test-Path $video) -or !(Test-Path $dll) -or !(Test-Path $probe)) {
    throw "Required video, DLL or probe is unavailable; do not treat as a measured run."
}
# CPU=1；推荐使用当前 Native D3D11 算法 519（7 + 双线性色度 2<<8），
# 强制输出 3840x2160，测量 93 秒。注意旧探针计量从首帧开始；
# 真正 5 秒预热/峰段/帧级埋点需在下一轮实现或分析时显式剔除。
$env:VSR_3FP_DECODE_MODE = "1"
& $probe (Resolve-Path $dll).Path $video 3840 2160 93 519
```

该模板**不是已经运行的测试结果**。候选 DLL 请输出到独立 `build/perf-kernel-20261017/candidate/` 目录，避免直接覆盖正在使用的 Full/Lite DLL；旧 `perf-3fp` 探针不自动启用 GUI 自适应线程配置（其 `FFF3FPConfiguration` 默认为 0），因此 **GUI 与独立 Native 探针的线程策略必须显式核对**。不能把它们的 fps 差算成播放器额外开销，除非配置确实相同。

## 6. 成功标准、自动回退与最终提交

### 6.1 必须通过（硬门槛）

- 原有软件、硬件、无音轨、VS、字幕、HDR/SDR、Jinc 色彩测试全部保持通过。全量回归若有历史既存失败，要出具“基线也失败”的复核证据，不能修改无关测试期待值掩盖问题。
- 关键样本无画面损坏、丢失精度、旧帧回流；连续 seek / 设备恢复无死锁；音频欠载不增加。
- 原本稳定 4K48 和 8K 硬解路径的最终显示速率、p99 与内存不明显恶化。
- 新路径有**运行时关闭开关或可替换旧 DLL**；没有回退能力禁止覆盖 Full。
- 每次改动只触及该阶段必要代码、对应测试、补丁和文档；不要借机会清理工作区旧文件或重构无关 UI。

### 6.2 优化达成（软门槛，不以此降低正确性）

- P0-B 若提升 8K 提交速率，至少应跨三轮稳定重现，p95/p99 单槽等待下降且音频欠载不增加；最佳目标是向 47.95fps 靠近，不强制以预设的“+50%”宣传。
- P0-C 以同画质的实际上屏吞吐和 GPU/CPU 同步等待为收益指标；仅毫秒级单次 Update 优化但帧吞吐不变应报告“局部优化、无端到端收益”。
- P1 CPU/优先级实验在不同负载、冷/热运行中一致有利才考虑默认开启；未证实收益则默认关闭。

### 6.3 每阶段的提交/回滚记录

每次提供：`baseline-sha256`、`candidate-sha256`、`ffmpeg-module-hashes`、改动文件列表与 diff、环境参数、三个以上原始 CSV、summary JSON、逐帧异常、回归日志、未通过项、最终是否部署。分支/补丁与 `tools/build-native-api20.ps1` 应支持从固定上游重建；禁止把仅存在 `build/native-api20` 的临时差异当作可发布修复。

## 7. 推荐实施顺序（可直接按步骤交给开发 AI）

1. **第 0 轮：测量基线。** 验证当前 Full/Lite/Release DLL、FFmpeg 和媒体参数，跑 T01/T02/T05/T06，输出分阶段等待与完整链 fps；成功条件是能指出主要时间耗在何处。不可修改色彩链。
2. **第 1 轮：单槽 → 2 槽可选环。** 仅改 libdav1d 的帧交接；T02/T03/T08 及旧软件回归通过才比较 3 槽；测内存及 seek。无提升就撤销。
3. **第 2 轮：HEVC 异步解码。** 保持同样线程数与预算，只扩展可安全异步的解码入口；测试 T01/T08；稳定后才允许与环形交接合并。
4. **第 3 轮：GPU 上传资源轮转。** 先收集 Update 等待与 GPU 时间；证实为瓶颈后单独实现 2 组 DEFAULT Texture，必要时测 3 组，跑 T02/T04/T06/T07；不触碰硬解 VideoProcessor 直通。
5. **第 4 轮：CPU Sets / MMCSS 实验。** 默认关闭，先调查 7940HX 实际 CCD/core 映射和 Windows 线程归属；再运行 T09/T10，不把不可控的 FFmpeg 内部线程当成已绑定。
6. **第 5 轮：DXGI 等待对象与高级色彩开销。** 只在 P0 后端到端性能仍由 Present/Copy 受限时执行 T07/T11，保留当前三缓冲方案为 A0。
7. **收尾：** 先做至少三次固定窗口对照、完整回归及部署一致性检查；写明每个最优参数的适用范围，禁止把 4K 通过扩大为 8K、把 SDR 准确扩大为全 HDR 面板色准。

## 8. 参考资料（项目内，可核验）

- `docs/hevc444-peak-diagnosis-1.0.6.md`：高码率 HEVC 峰段、8→16 线程与缓存对照。
- `docs/software-decode-subtitles-1.0.7.md`：512 MiB / 32 线程 / 独立软件解码交接和停顿试验。
- `docs/update-1.0.11.md`：8K48 软件解码未通过、AAC/ASS 成本、CPU/GPU 负载对照。
- `docs/player-anime-performance-1.0.12.md`：4K444p12 Anime 成功/失败边界。
- `docs/player-color-vs-1.0.12.md`：Jinc/LUT 资源槽冲突及 SDR/PQ/HLG 回归。
- `docs/color-management-1.0.4.md`：libplacebo、ICC、HDR 元数据、scRGB 203/80nit。
- `docs/nuxbox-performance-session-2026-10-04.md`：D3D11VA/VideoProcessor 及三缓冲在 780M 的历史测试。
- `src/backend/ThreeFpPlayer.cpp`、`src/backend/VapourSynthFrameServer.cpp`、`src/color/ColorBridge.c`。
- `build/native-api20/FFF.Native/3FP/Core/PlayerSession.cpp`、`Render/VideoRenderer.cpp`、`Render/NativeColorEngine.h`。
- `tools/perf-3fp.cpp`、`tools/perf-system.ps1`、`tests/TestPlayer.cpp`。
