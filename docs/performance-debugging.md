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
| `FFF_DAV1D_WORKERS` / `FFF_DAV1D_DELAY` | 按帧字节；指定8K为8/6、4K444高位深为24/24 | 1–64 的工作线程/帧上下文覆盖 |
| `VSR_SW_HANDOFF` | 1 | 8K 交接槽 1–3 |
| `VSR_SW_UPLOAD_PIPELINE` | 0 | 1 测试双DEFAULT／双STAGING上传环，默认使用原UpdateSubresource |
| `VSR_SW_REALTIME_CATCHUP` | 1 | 0 禁用8K恒帧率AV1的参考帧追赶，排查用 |
| `VSR_SW_REALTIME_TRACE` | 未设置 | 1 输出追赶切换与解码失败诊断 |
| `VSR_SW_CCD` | 2 | 0 交还系统；2 软提示；1 强绑定，仅作排查 |
| `VSR_MMCSS` | 0 | 1 尝试为关键线程注册 MMCSS |
| `VSR_DXGI_WAITABLE` | 0 | 1 尝试等待对象交换链，不支持时回退 |

性能结果按实际开关记录，不能把开关开启时的测量写成默认模式表现。P/E 或多 CCD 策略需在对应设备实测。

## 内存与同步采样注意

`software8kPlaybackMeasurement` 同时记录墙钟、媒体位置、画面PTS、CPU时间、私有内存、工作集及进程DXGI本地／非本地显存用量。字幕信号只计数，不保留每次更新的QImage；用QSignalSpy收集图像会让测试本身持续占内存。以同一片段的墙钟和画面进度判断同步，不能只看媒体时钟或Present次数。

软件路径按帧字节估算在途解码、交接、保留帧和显示队列；默认只保留一组上传纹理。预算不等同于整个进程RSS硬上限，解码器参考帧、驱动、字幕和VS缓存需另测。新追赶仅用于大于等于8K的CPU AV1恒帧率持续落后场景；在关键包边界切换，仅省略非参考帧，暂停定位恢复完整解码。该8K路径的压缩视频包上限为32 MiB、64包，其余保持16 MiB，避免一个慢GOP长期堵住后续音频读取。静音填补和丢帧都应记入结果，不能将其表述为无损逐帧实时。

## 结果记录

测试结论写到源码工作区的 `development/validation.md` 对应版本/轮次：输入与环境 → 运行范围 → passed/failed/skipped → 性能是否达标 → 最终产物对应关系 → 未测项和证据位置。日志留 `build/`，精选证据放 `development/evidence/`，不复制原视频。

QtTest PASS 只表示其断言满足；没有实时阈值断言的测量即使 PASS 也可能低于源帧率。低 fps 同时少量丢帧仍可能是慢放/欠载；纯 FFmpeg 解码速度也不能替代完整 GUI 播放。历史结果查询台账，不在 docs 新建逐轮报告。
