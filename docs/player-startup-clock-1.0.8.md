# 1.0.8 无音轨视频启动时钟修复

日期：2026-10-06。本轮仅本地修改、构建与部署；版本保持 1.0.8，保留维护者更新的 FFmpeg，不推送、不打包、不改动在线内容。

## 原因

两个指定文件标称帧率均为 `7001/146`（约 47.952 fps），时间基为 `1/1000`，起点为 0。`OP-Auto33-321.mkv` 只有 AV1 视频流；`Girls.Band.Cry.04.AV1.FLAC.1080p48F_2026.10.05-22.31.36.mkv` 带 FLAC 音轨及 ASS 字幕，视频实际为 3840×2160、10-bit 4:4:4 AV1。

`PlayerSession::ArmAudioUntilVideoFrame` 曾在没有音频输出时也设置 `audioBlockedUntilVideoFrame_`。`PumpVideoPresentation` 为尽快显示音频同步首帧，允许该门控状态绕过 PTS 等待；但无音频输出不会记录和释放音频边界，后续帧也一直走这个分支。画面因此受 Present / 显示器刷新速度限制，而媒体时钟本身仍按 1 倍速推进。跳转、启动期间内部速率设置或视口调整均可能重新设定门控；暂停续播可清除无音频门控，造成交互后的表现不一致。

修复只改门控条件：存在 `audioRenderer_` 时才启用首帧音频同步。无音轨或音频输出不可用时，视频按原有 QPC 媒体时钟及 PTS 等待呈现。有音频输出的首帧同步行为保留；自动关键帧、帧率元数据和显示器设置未修改。

补丁为 `patches/3fp-silent-video-startup-clock.patch`，已纳入 `tools/build-3fp.ps1`，在此前软件解码、音频电平、HDR 读回补丁后应用。完整既有补丁源码的构建前后仅 `PlayerSession.cpp` 的门控条件及说明改变。

## 真实文件验证

新增 `TestPlayer::directStartupClock`，从首次可见帧开始记录墙钟、媒体位置、帧 PTS、接收计数与真实 Present 计数，并核对首次打开、跳转、暂停续播的播放速度。首帧等待另记毫秒数，不将冷启动初始化时间混入播放帧率。测试使用隔离的测试配置，不修改运行目录 `player.ini`。

旧 DLL 在 OP 的 D3D11 / Jinc 路径复现失败：约 3.04 秒内呈现约 500 帧，帧 PTS 已到约 10.5 秒，接近 165 fps；当前显示器为 165 Hz。记录：`build/startup-clock-1.0.8/op-baseline.txt`。

修复后 OP 的原画、D3D11、Jinc 三种直通模式：

| 解码 | 首次打开 / 跳转 / 续播实测接收帧率 | 帧 PTS 与时钟偏差 | 结果 |
| --- | --- | --- | --- |
| D3D11 硬解 | 47.80–48.17 fps | 小于 27 ms | 三种模式全部通过；测量期间无渲染丢弃或合并 |
| CPU 软解 | 46.75–48.10 fps | 小于 34 ms | 三种模式全部通过；首次打开 D3D11 缩放丢弃 4 帧、Jinc 丢弃 1 帧，跳转 / 续播无丢弃 |

硬解首帧 271–449 ms，软解首帧 218–239 ms。原画首次测量曾从 Playing 状态开始时尚未出现首帧，改为独立记录首帧时间后完成最终验收，没有为此增加生产代码改动。最终日志为 `op-hardware-final.txt` 和 `op-software-final.txt`。

带音轨的 4K 对照文件在本机回退为软解。旧 DLL 与新 DLL 都没有 165 fps 快进，但都发现启动时的软件解码丢帧 / 延迟；严格的完整实时播放用例未通过，不能据此声称该文件整体播放性能已通过验收。日志为 `audio-control-baseline.txt`、`audio-control-fixed.txt`，该项属于后续性能排查边界。

## 回归与本地部署

回归及部署记录位于 `build/startup-clock-1.0.8/`。新原生 DLL SHA256 为 `2a8d6171ea657e2a34b0ad93d9ed8693e6ce5dbec914f16778cf118fc699efce`；部署保留配置、VPY、色彩资源、语言文件及用户更新的 FFmpeg / FFprobe，并更新本地资源哈希清单。

`regression-final.txt` 中 14 项通过，涵盖合成无音轨 47.952 fps 启动、HEVC / AV1 软解及音频提前结束后的尾帧、独立音频时钟、硬软解播放 / 暂停时点击及拖动寻帧、输入电平和 HDR 浮点读回。额外两条浮层用例在鼠标命中检查处失败：`QApplication::widgetAt(QCursor::pos())` 返回空。分别用旧、新 DLL 单独复查仍同样失败（`overlays-baseline.txt`、`overlays-isolated.txt`），因此本轮不能声称浮层回归全部通过；未改动此前浮层实现或放宽这些断言。

已部署到 `dist/VS-Renderer-GUI-windows-x64`，54 个受保护文件哈希一致。随后使用运行目录的 DLL 再验收 OP 的 D3D11 直通：首次打开、跳转、续播均通过，首帧 269 ms，三段媒体时间与墙钟的差异小于 1 ms，测量期间无渲染丢弃或合并。记录为 `deployed-op-smoke.txt`；烟测后再次核对 54 个受保护文件全部未变。
