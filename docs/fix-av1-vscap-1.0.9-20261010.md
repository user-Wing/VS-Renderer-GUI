# 1.0.9：AV1 12-bit 4:4:4 软件解码与 VS Cap 黑屏修复

日期：2026-10-10。版本保持 1.0.9。本轮性能素材仅 `D:\OP-yuv444p12-SWTest.mkv`，没有重测旧 F 盘素材。诊断依据保存在 [用户诊断报告副本](input-av1-yuv444p12-diagnosis.md)。

## 为什么解不动

该片是 AV1 Professional、3840×2160、yuv444p12le、7001/146 ≈ 47.952 fps，4415 帧，音频 FLAC，字幕 ASS。它不是本轮要诊断 AAC 的素材；12 位本身也不足以证明视频为 HDR。原报告机器为 16 核/32 线程 Ryzen 9 7940HX，本机为 8 核/16 线程 Ryzen 7 7840HS，D 盘为 Samsung SSD 980 NVMe。因此不能把报告中的 69.36 fps 直接当成本机结果。

1. dav1d 工作线程数与帧并行上下文数不同。未设置 `max_frame_delay` 时，dav1d 1.5.4 自动取工作线程数平方根向上取整：16 线程仅 4 个帧上下文，32 线程仅 6 个。该片起始帧只有一个 tile，帧内并行与帧间依赖会限制可运行任务；低 CPU 占用不说明播放器已经充分利用 CPU，也不能解释成只创建了 4/6 个系统线程。参见 [dav1d 官方实现](https://raw.githubusercontent.com/videolan/dav1d/1.5.4/src/lib.c) 和 [FFmpeg libdav1d 封装](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavcodec/libdav1d.c)。
2. 同步调用软件视频解码阻塞了 session 的音频、队列和呈现供给，进一步放大卡顿。本轮将 libdav1d 播放解码移到持久工作线程；压缩包队列最多 8 个，输出采用单帧交接，有界持有 packet/frame。session 保持时间轴、音频和呈现状态所有权，seek/flush/free 前停止并回收工作线程，codec 调用与元数据读取串行。
3. 当解码落后音频时钟，旧策略可能把每个后续迟到帧都丢掉，于是 decoded 增长但画面停在旧帧。保留此前修复：仅当有较新已解码帧可以替换时丢弃旧帧，没有替代帧仍显示最新可用画面。本轮没有增加迟到阈值掩盖丢帧，仍为 `max(两帧时长, 50ms)`。
4. 实际 GUI 还有字幕开销。内核已经返回裁剪的 ASS 位图，但 GUI 又将其铺成整张 4K 透明画布，随后作为全屏位图提交；渲染器对大量透明像素也进行颜色转换和上传。本轮保留字幕画布、坐标与颜色，只向 Native 提交可见区域的紧凑位图。裁剪前后真实呈现逐像素一致、清空字幕有效。ASS 动画仍完整更新，没有通过关闭字幕、降低位深或色度换帧率。

## 并行策略与代价

针对 libdav1d 的至少 3840×2160、yuv444p12le 重负载输入，工作线程数为 `min(逻辑线程数×2, 32)`，其他输入保持常规线程数量；显式设置 `max_frame_delay` 为所选线程数。本机得到 32/32，原报告 32 逻辑线程机器也得到 32/32。为避免自适应梯子反复重开/跳转，该 decoder 不再走通用重开梯子。软件 decoded queue 有 512 MiB/最多 8 帧约束，硬解队列预算保持原值。

32/32 消耗更多内存，并有起播、跳转预热代价。本机进程峰值约 5.65 GiB，原先约 1.40 GiB。FFmpeg 提示超过 16 逻辑线程不推荐；本轮对这个指定重负载样本的全片对照确有收益，并未默认采用报告中约 10.5 GiB 的 64/64。

原报告的 `Invalid data found when processing input` 在本轮未复现。全片独立解码及 seek/末尾/重开测试均无解码错误，不能据此声称已经证明旧探针报错的唯一根因。新的 codec 生命周期和数据所有权避免交接竞态；错误仍报告给用户，不静默吞掉。

## 本机实测

物理显示器 3840×2160 / 60 Hz，Windows HDR 已开启。强制软件解码，D3D11 原生路径，`VSR_ADAPTIVE_UPLOAD=0`。解码输出仍为 12-bit 4:4:4，没有 CPU 预缩放；显示链路报告 10-bit，与解码位深是不同概念。

| 测试口径 | 修改前/对照 | 修改后 | 说明 |
| --- | ---: | ---: | --- |
| 全片纯解码：16/16 vs 32/32 | 48.19 fps | 57.07 fps | 两者 4415 帧全片零错误；不包含音频、字幕、呈现 |
| 相同 Native 播放探针，60 秒实际新视频帧 | 882，14.70 fps | 2268，37.80 fps | 约 2.57 倍；不是 GUI 开字幕的数据 |
| Native 60 秒 dropped | 516 | 594 | 音频时间推进从 29.86 秒变为 59.79 秒；不能只比较丢帧绝对数 |
| Native 60 秒音频欠载增量 | 303 | 0 | 修改后起始累计 1，计时段不再增加 |
| Native 首帧 | 401 ms | 663 ms | 解码并行深度增加，首帧未改善；修改后预热仍丢 8 帧 |
| Native 进程 CPU 中位数 / 峰值内存 | 4.88% / 1431 MiB | 52.43% / 5785 MiB | CPU 为采样工具进程口径，不等同任务管理器频率加权值 |
| Full GUI，开字幕，30 秒，裁剪前 | 647 帧，21.57 fps | — | 同一内核、同一素材，音频欠载 0 |
| Full GUI，开字幕，30 秒，裁剪后（构建目录） | — | 856 帧，28.53 fps | dropped 567，欠载 0，首帧 345 ms |
| Full GUI，开字幕，30 秒，裁剪后（最终发行目录） | — | 744 帧，24.80 fps | dropped 646，欠载 0，首帧 338 ms；存在运行波动 |
| Full GUI，关字幕，30 秒定位对照 | 1243 帧，41.43 fps | — | 仅用于定位瓶颈，发行默认仍开字幕 |

**本机尚未达到 47.952 fps / 全程零丢帧。** GUI 的解码、12 位软件上传、字幕合成/呈现仍共同消耗余量；纯解码实时不等于播放器实时。QtTest PASS 表示恢复、像素、工作流断言通过，软件性能测试没有断言零丢帧。

seek 到 0/60/10/84 秒分别恢复于 1250/2849/1854/1751 ms；暂停稳定、恢复播放、末尾 Ended、stop/reopen 均通过。未改保真度、字幕更新和丢帧计数定义。100 ms 迟到容忍、自动 SDR 快上传和 ASS 未变化缓存的实验无显著收益，已撤回。

日志及逐秒数据在 `build/session-20261010-op`：`before.*`、`decode-full-16-16.txt`、`decode-full-32-32.txt`、`shipping.*`、`gui-subtitle-crop.txt`、`shipping-final.txt`。所有性能实验仅使用该 D 盘视频。

## VS Cap 全黑原因与修复

真实复现时桌面已有白色标记窗口，但第一次 DXGI AcquireNextFrame 成功返回 `LastPresentTime=0`、`AccumulatedFrames=0`，只有鼠标更新时间。该通知尚无有效桌面图像，纹理全黑。旧流程直接用它创建全屏截图浮层，所以按快捷键后看起来整个屏幕变黑，随后保存也是黑图。参见 [微软 DXGI_OUTDUPL_FRAME_INFO 说明](https://learn.microsoft.com/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_outdupl_frame_info)。

现在释放这类仅鼠标通知的 frame，继续等待有非零 LastPresentTime 的桌面图像，总等待上限 1000 ms；失败报告采集错误，避免打开无效黑浮层。HDR 仍采集 FP16 scRGB 并进入 RGBA32F，不通过 8-bit SDR 回退掩盖错误。

真实 HDR 屏上用白色窗口验证原始 float 像素、预览、浮层、选区交付及保存 PNG 非黑；修复前该测试失败，修复后通过。最终发行目录 PNG 精度/SDR 兼容/真实桌面 5 项、浮层/注释复制/贴图/AVIF 工作流 6 项全部通过。剪贴板被其他程序短暂占用时有系统警告，现有重试后检查通过。

PNG 为 BT.2020/PQ RGBA16 无损编码；FP16→PQ16 的转换仍有量化，原始 float 通过既有伴随文件保留，不能称浮点逐位无损。既有 AVIF CRF18 路径保持有损压缩定义。

## 构建与手动回传

- 本地构建完成：`dist/Full`（`VSRenderer.exe`、`vs-player.exe`、`vs-cap.exe`），`dist/PlayerLite`（`vs-player.exe`），均 1.0.9。
- Full 1,057,491,775 字节；Lite 195,119,827 字节。Lite 扣除 FFmpeg 专属依赖后 51,972,307 字节，即 49.56 MiB，仍未达到严格十进制 50 MB。精简 FFmpeg 不属于本轮新增修改。
- 工具链：`C:\BuildTools\2026` v145 / MSVC 14.51.36231；Qt 6.10.2 MinGW / GCC 13.1.0。Qt EXE 与 MSVC Native DLL 经 C ABI 互通。
- 用户原 Portable 安装与正在运行的播放器保留；没有提交、推送或修改文件关联。
- Native 基线：`20bcc001b24f6b6de4575bddf9b93ec452795b0d`。完整修改保存在 `patches/3fp-api18-vsrenderer-1.0.9.patch`，已通过 reverse-check。GUI 基线与逐文件 SHA256 记录在源码归档中的 `source-manifest.json`。

原设备先准备该 Native 基线和所需依赖，再运行：

```powershell
& .\tools\build-1.0.9.ps1 -NativeRoot 'C:\Private\FFF_Project-1.0.9' -RuntimeDirectory 'C:\PortableSoft\VS-Renderer-GUI'
```

如 Qt、MinGW、CMake、Ninja 或 MSBuild 不在默认路径，通过脚本同名参数指定。脚本自动应用匹配的完整 Native 补丁，已应用则跳过，冲突则停止；不会把本轮补丁套到不匹配的内核。源码包不含视频、dist、依赖缓存或工具链，因此不能单凭源码 ZIP 获得完整运行环境。原设备 CPU 更强，但仍需在原设备对同片实际 GUI 开字幕进行验收，不能将本机或纯解码结果当作原设备零丢帧证明。
