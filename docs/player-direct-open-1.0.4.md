# Player 直通与大文件加载

## 问题与修复范围

2026-10-03 排查 NuxBox K6（主机名 LAPTOP-DSH3PNDH）机械硬盘素材：

`F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬\Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv`

文件 107,938,080,889 字节；FFprobe 返回 7680×4320 AV1、AAC 音频和内嵌 ASS 字幕，时长 8523.219 秒。这一次 FFprobe 流信息探测耗时 125 ms；此数据不是播放器首帧耗时，也不代表冷缓存磁盘性能。

排查到三处开销：

- 未选预设的原画播放仍创建 FFMS2 的 VS 源，需要帧索引。
- 固定 Jinc / D3D11 打开仍等待 VS 初始化。
- 3FP 内嵌 ASS 初始化一直 `av_read_frame` 到 EOF；内嵌 SRT 在 Qt 层先调用 FFmpeg 全量提取。这两条字幕路径都会读取整部媒体，和播放竞争机械硬盘带宽。

现在无预设原画使用原生 3FP，不创建 VS 源；固定直通档位不等待 VS 初始化。ASS / SRT 保留容器与解码器，按播放位置读取，单次最多 64 包或累计约 4 MiB（最后一个包可超过限制），时间窗口前读约 1 秒。跨位置跳转重定位，向前预留 30 秒字幕 preroll；MoreData 允许暂停时继续补齐当前窗口，避免相同位置缓存阻止后续读取。该窗口不能保证恢复在更早位置开始、持续超过 30 秒的跨越式长字幕；不等价于完整全文字幕索引。

增强、补帧、自定义 VPY 仍走 VS，首次 FFMS2 / L-SMASH 索引是这些模式的独立开销。此修改不保证所有编码、磁盘和 GPU 均能秒开或实时播放 8K48。

## 构建与验证

`tools/build-3fp.ps1` 在已有原生补丁之后应用 `patches/3fp-streaming-text-subtitles.patch`，更新后的 EXE 与 FFF.Native.dll 需配套。补丁逆向检查用于确认已应用；没有更改媒体文件。

专项测试：

本地专项通过 13/13（包含初始化与清理）：小素材约 0.96 MB，原画 / Jinc / D3D11 首帧分别 221 / 189 / 281 ms；测试日志 `build/mingw-release/fast-open-tests.txt`。这些数字仅是本地小素材回归，不是 NuxBox 108 GB 素材成绩。`indexCacheAndBitdepthResize` 同时确认增强与自定义 VPY 仍能建立和复用 FFMS2 / L-SMASH 索引，没有被直通修改绕过。

- `directOpenWithoutIndex`：原画 / Jinc / D3D11 打开不进入 deferred，不加载 VS 脚本、不生成帧索引并呈现首帧。
- `embeddedTextSubtitleSeek`：内嵌 ASS / SRT，在 2 秒、42 秒及返回 2 秒时渲染字幕，包括暂停位置继续读取。
- 原有 `subtitleOverlay` 和 `directTimelinePreview` 检查外挂字幕及四种播放 / 暂停拖动组合。

大素材测试入口为环境变量 `VSR_DIRECT_OPEN_SOURCE`；测试读取对应文件，独立临时缓存并关闭记忆位置，结束由测试套件恢复 player.ini。不能在另一窗口使用同一便携目录时运行此测试。

2026-10-03 本窗口按要求暂停传输。2026-10-04 用户确认远端部署已完成，本窗口只读核对 `C:\PortableSoft\VS-Renderer-GUI\FFF.Native.dll` 哈希与最终基线一致，没有据此补写本窗口的大素材首帧成绩。用户新报告 780M 上 1080p48→2160p 与 2160p48 原生播放大量丢帧；播放性能与加载开销分开，后续转到远端本地开发，见 [性能交接](nuxbox-performance-handoff.md)。
