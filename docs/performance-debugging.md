# 性能测量与故障复现

## 固定复现条件

先记录 GUI 版本/提交、Native 与 avcodec 哈希、Full/PlayerLite、CPU/GPU/驱动、Windows HDR 开关、显示器物理分辨率/刷新率/DPI。用 ffprobe 确认编码、分辨率、位深、色度、帧率和色彩标记，不按文件名推断。

区分三个路径：D3D11/Jinc 原生直通、真正的 VS/VPY 增强、图片查看；分别记录硬件/CPU 解码、实际输出尺寸、音频/字幕与色彩引擎。

## 测量步骤

1. 冷启动首帧单独计时；稳定吞吐使用同一片段、相同 seek 起点，预热后固定窗口采样。
2. 同条件多轮对照，记录窗口内新增解码帧、实际新帧呈现、丢帧、合并与音频欠载，以及墙钟与媒体进度。建议稳定段 60 秒、至少三轮；未做到时明确实际次数和窗口，不能写成标准矩阵已完成。
3. 核对真实 HWND/surface 尺寸和模式。`Present` 次数可能包含字幕/OSD 重绘，不能直接当视频 fps；Native 新帧计数与面板采样需保持一致。
4. 修改线程/上传/队列时一次对照一个变量，保留旧 DLL、参数、CSV 和哈希。最终 DLL 若又修改，注明性能采样是否重跑。
5. 同时检查播放、暂停、双向 seek、帧定位、EOF、关闭重开、音轨/字幕与色彩像素，避免性能改善掩盖功能回退。

与 mpv 比较时固定同设备、素材、解码方式、输出 surface、缩放质量、HDR 映射、字幕和音频；`--no-config` 排除用户配置。条件不相同只可当参考，不计算严格提升倍数。

## 工具与测试入口

`tools/perf-3fp.cpp`、`perf-system.ps1` 和 `measure-nuxbox-*.ps1` 提供原生/系统/mpv 采样入口。先读参数和路径，历史脚本引用的设备/素材不代表当前存在。干净交接包的操作入口是 `tools/package-dev-handoff.ps1`，随包 START-HERE 指向本指南；原设备特定任务与旧工具链快照在源码工作区 `development/history.md` 中，仅作历史依据。

`tests/TestPlayer.cpp` 中 `animeCadenceMeasurement` 使用 `VSR_ANIME_PERF_SOURCE`、`VSR_PERF_STAGE`、`VSR_PERF_WIDTH`、`VSR_PERF_SECONDS`、`VSR_PERF_FULLSCREEN`、`VSR_PERF_CSV` 设置输入；缺素材会跳过。用 `-functions` 核对当前测试函数后执行，见 [测试步骤](dependencies.md)。

## 内核诊断开关

设置后重启播放器，正常使用不需要手工设置。

| 环境变量 | 当前默认 | 用途 |
| --- | --- | --- |
| `VSR_ADAPTIVE_THREADS` | 软件解码开启 | 0 固定线程数；默认按负载递增，重开媒体重新评估 |
| `FFF_PLACEMENT` | 自动 | `off` / `prefer` / `spill` / `boost` 作为CPU域调度对照 |
| `FFF_LADDER_TRACE` / `FFF_PLACEMENT_TRACE` | 未设置 | 输出线程、帧池、呈现节奏和CPU落点诊断 |
| `FFF3FP_UPLOAD_FORCE_STAGING` | 0 | 1 在大帧和显存允许时切回双STAGING环；默认DYNAMIC + WRITE_DISCARD |
| `FFF3FP_HWCAP_TRIAL` / `FFF3FP_HWCAP_MEMORY` | 1 / 1 | 用首帧试解码确认硬解能力，在会话内记忆流级拒绝 |
| `FFF3FP_HWCAP_PROBE` / `FFF3FP_HWCAP_TRACE` | 0 / 未设置 | 驱动能力预筛选只作可选诊断，默认不替代实际试解码 |
| `FFF_AUDIO_TARGET_TRACE` | 未设置 | 输出60～240ms音频目标缓冲的告警与升档原因 |
| `VSR_SW_REALTIME_CATCHUP` | 1 | 0 禁用8K恒帧率AV1的参考帧追赶，排查用 |
| `VSR_SW_REALTIME_TRACE` | 未设置 | 1 输出追赶切换与解码失败诊断 |
| `VSR_SW_CCD` | 2 | 0 交还系统；2 软提示；1 强绑定，仅作排查 |
| `VSR_MMCSS` | 0 | 1 尝试为关键线程注册 MMCSS |
| `VSR_DXGI_WAITABLE` | 0 | 1 尝试等待对象交换链，不支持时回退 |

性能结果按实际开关记录，不能把开关开启时的测量写成默认模式表现。P/E 或多 CCD 策略需在对应设备实测。

`FFF_DAV1D_WORKERS`、`FFF_DAV1D_DELAY`、`VSR_SW_UPLOAD_PIPELINE` 是旧补丁的实验开关，现行上游策略已替代其决策；不要套用旧轮次命令推断当前上传路线或线程数，读取实际快照和日志。

## 上游内核优化

当前固定 main `922e4b7`，API20；[构建与隔离边界](native-api20-build.md)给出完整SHA。以下描述代码机制，上游PR中的机器测量不能当作本机收益。本轮实测单独放在[验证台账](../development/validation.md)。

| 上游更新 | 怎么减少开销或改善稳定性 | 当前VSP中的边界 |
| --- | --- | --- |
| [#27](https://github.com/Lake1059/FFF_Project/pull/27)、[#31](https://github.com/Lake1059/FFF_Project/pull/31)：线程、CPU域与帧池阶梯 | 根据实际饥饿和上传饱和情况调整资源；先尝试较合适的CPU域，再扩展或提高优先级。普通软解从低线程数起步，帧池按帧字节选择起点、必要时升档，避免所有片源都付出最大线程/内存成本 | 高负载dav1d保留单独线程策略；帧池是准入预算，不是进程RSS硬上限。P/E机器尚需实测 |
| [#25](https://github.com/Lake1059/FFF_Project/pull/25)、[#27](https://github.com/Lake1059/FFF_Project/pull/27)、[#31](https://github.com/Lake1059/FFF_Project/pull/31)：上传与计量 | 解码线程与会话交接有界；默认直接Map动态纹理，减少额外GPU拷贝和纹理占用。可选双STAGING环让CPU填充与GPU消费交错，分配失败可回落；记录实际次数、字节、Map等待和复制耗时，区分内存带宽与同步等待 | 默认采用上游后续选择的动态纹理，不能把“有暂存环”理解为所有机器都更快；CPU预缩放默认仍关闭 |
| [#28](https://github.com/Lake1059/FFF_Project/pull/28)：七种格式直传 | NV16、NV20LE、NV24、P410LE、P416LE、YUV410P、YUV440P直接描述纹理布局，省去整帧CPU sws_scale；按各格式的LSB/MSB对齐缩放采样值 | 不改采样算法；新增半平面格式仍需具体解码器/GPU输出才能覆盖，不用其他格式测试替代 |
| [#28](https://github.com/Lake1059/FFF_Project/pull/28)、[#33](https://github.com/Lake1059/FFF_Project/pull/33)：高位深输出与明确色彩选择 | >8-bit SDR在支持Advanced Color时优先16-bit scRGB，失败回落10-bit；HDR源明确选择MapToSdr或RawHdrAsSdr时保持该选择，避免下一帧被自动策略反转 | 属于精度和行为修正，16-bit链也可能增加带宽，并非性能免费提升；普通8-bit SDR不提升 |
| [#32](https://github.com/Lake1059/FFF_Project/pull/32)：GPU优先与首帧能力门 | 独立读入媒体并实际解出首帧，再确认D3D11VA或CUDA路线；硬解拒绝后切CPU，记住流级不支持，避免seek/重开/设备恢复时反复尝试。暂时打开失败不永久缓存 | 用户显式CPU选择保留；不以“4:4:4一律不支持”提前排除GPU；硬解试探本身有首开成本 |
| [#27](https://github.com/Lake1059/FFF_Project/pull/27)、[#33](https://github.com/Lake1059/FFF_Project/pull/33)：呈现节奏和丢帧归因 | 负载不足时可在250ms容差内降低呈现节奏，超过容差恢复追赶；动态图层刷新独立于片源帧率，仍受面板上限约束。面板本身显示不了的帧单独记账，不再错误触发缩小画面 | VSP没有照搬上游VB弹幕泵；保留Native图层能力。节奏容差不能保证超载解码一直实时或全程音画误差≤250ms |
| [#34](https://github.com/Lake1059/FFF_Project/pull/34)、[#35](https://github.com/Lake1059/FFF_Project/pull/35)：自适应音频池 | 从60ms或设备实际缓冲时长起步，出现欠载、时间戳抖动、断续、补静音或丢重叠时每批增加30ms，最高240ms；同轮不反复缩回，换文件/音轨重新起算 | 正常轻负载更低延迟，异常片源换取缓冲余量；不等于能补足持续不足的解码算力 |
| [#34](https://github.com/Lake1059/FFF_Project/pull/34)、[#35](https://github.com/Lake1059/FFF_Project/pull/35)：HDR流级状态 | 分类和输出链选择读取流声明，显示器/目标亮度改变时重算；只有ST2094诊断继续逐帧更新，避免没有SEI的单帧改变HDR分类或输出档 | 实际动态色调映射仍按帧工作；Qt没有照搬VB信息行的打开时锁存逻辑 |
| [#29](https://github.com/Lake1059/FFF_Project/pull/29)、[#31](https://github.com/Lake1059/FFF_Project/pull/31)、[#32](https://github.com/Lake1059/FFF_Project/pull/32)：附加Native能力 | 流级side-data独立缓存、外部声床渲染插槽、可选辅助视频层、RTX自动HDR，以及AVIF多extent/网格合成均随内核保留 | Qt已增加RTX VSR/TrueHDR入口，外部声床没有新增设置页；VSP图片由自己的图像模块解码，不能把Native AVIF修复直接当成VSP图片模块升级。IAMF/OBR继续保留，普通声床旧代码也未实际接入内置双耳渲染器 |

[#24](https://github.com/Lake1059/FFF_Project/pull/24)主要是中性命名和注释，不能计为性能收益。1.0.14 已迁入 AI 字幕[#36](https://github.com/Lake1059/FFF_Project/pull/36)、[#37](https://github.com/Lake1059/FFF_Project/pull/37) 的音频 tap 与 Native 字幕更新；外置 worker 的配置步骤见 Player 指南。采样与识别队列有上限，设备速度由真实推理 RTF 判断，未装模型时不作识别性能结论。

## 内存与同步采样注意

`software8kPlaybackMeasurement` 同时记录墙钟、媒体位置、画面PTS、CPU时间、私有内存、工作集及进程DXGI本地／非本地显存用量。字幕信号只计数，不保留每次更新的QImage；用QSignalSpy收集图像会让测试本身持续占内存。以同一片段的墙钟和画面进度判断同步，不能只看媒体时钟或Present次数。

软件路径按帧字节估算在途解码、交接、保留帧和显示队列；默认只保留一组上传纹理。预算不等同于整个进程RSS硬上限，解码器参考帧、驱动、字幕和VS缓存需另测。新追赶仅用于大于等于8K的CPU AV1恒帧率持续落后场景；在关键包边界切换，仅省略非参考帧，暂停定位恢复完整解码。该8K路径的压缩视频包上限为32 MiB、64包，其余保持16 MiB，避免一个慢GOP长期堵住后续音频读取。静音填补和丢帧都应记入结果，不能将其表述为无损逐帧实时。

## 结果记录

测试结论写到源码工作区的 `development/validation.md` 对应版本/轮次：输入与环境 → 运行范围 → passed/failed/skipped → 性能是否达标 → 最终产物对应关系 → 未测项和证据位置。日志留 `build/`，精选证据放 `development/evidence/`，不复制原视频。

QtTest PASS 只表示其断言满足；没有实时阈值断言的测量即使 PASS 也可能低于源帧率。低 fps 同时少量丢帧仍可能是慢放/欠载；纯 FFmpeg 解码速度也不能替代完整 GUI 播放。历史结果查询台账，不在 docs 新建逐轮报告。
