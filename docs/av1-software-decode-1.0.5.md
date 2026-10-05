# AV1 软件回退与上传优化（1.0.5，2026-10-05）

## 支持边界

RTX 4070 Laptop 属于 Ada；[NVIDIA NVDEC 官方支持表](https://docs.nvidia.com/video-technologies/video-codec-sdk/13.1/nvdec-application-note/index.html)列出的 AV1 硬解为 Main Profile。实际 1080p48 测试文件为 AV1 High / yuv444p10le，不能通过改成 High 或 Professional 标签获得硬解。HEVC 4:4:4 的支持不能推导为 AV1 4:4:4 支持。未确认 780M 驱动支持 AV1 High 硬解，也未验证 PotPlayer 所谓硬解是否指同一素材的实际解码器。

本轮不重编码、不降低位深或色度、不跳过解码滤镜；保留硬解优先及软件回退。

## 最小改动

- 大于等于 3840×2160 的 libdav1d AV1 软件解码，线程上限由 8 调整为 16，仍受 CPU 逻辑线程数约束。小视频及其它解码器不变。32 线程在合成 8K 对照中更慢且更占内存，未采用。
- CPU 平面 YUV 使用 D3D11 动态纹理、WRITE_DISCARD 逐行上传；保留原位深、行跨度及色度尺寸。硬解 NV12/P010 纹理路径不变。
- 固定 bilinear 路径复用已有格式/色彩专用 shader 缓存，避免通用多算法 shader 的额外成本；Jinc、特效和 360 路径保持各自逻辑。

可重建补丁：`patches/3fp-av1-software-threads.patch`、`patches/3fp-software-frame-upload.patch`，由 `tools/build-3fp.ps1` 应用。测试工具 `tools/perf-3fp.cpp` 可用环境变量 `VSR_3FP_DECODE_MODE=1/2` 强制软件/硬件解码，并记录进程 CPU 时间。

## 真实 780M 测试

7840HS / 780M，真实机械盘素材约 107.94 GB，7680×4320、AV1 Main、yuv420p10le、7001/146 fps；输出 HWND 实际 3840×2160。强制 CPU 解码、跳转 120 秒、预热 5 秒后取约 15 秒稳定段。测试不是 AV1 444 素材，也不是长时热稳定验收。

| 软件回退版本 | D3D11 呈现 fps | Jinc 呈现 fps |
| --- | ---: | ---: |
| 远端原有核心 | 11.07 | 9.47 |
| 本地最新核心 + 16 线程 | 10.93 | 15.32 |
| 加动态上传 | 11.65 | 18.53 |
| 再加固定 bilinear 专用 shader | 18.18 | 18.73 |

远端原有核心较旧，不能把 Jinc 的全部提升归因于本轮改动。最后两项为同一源码上的逐项对照。D3D11 累计呈现等待由约 8.87 秒下降到 0.72 秒，但最新两条路径仍新增约 305/290 个丢帧，视频时间也落后，**未达到 8K48 软件播放目标**。

纯解码 383 帧：8 线程 10.754 秒，16 线程 9.625 秒；包含跳转预滚与启动，不能作为绝对稳态上限。12 帧 framemd5 完全一致。额外试验 max_frame_delay=8/16 分别 9.249/9.700 秒，内存约 3.87/6.65 GiB，对照默认约 2.34 GiB；收益不足，未合入强制缓存设置。

## 本机与精度回归

本机 7940HX / RTX 4070 Laptop，用户已开启增强模式、可释放 120W；本轮未采样 GPU 实际功耗，不能将 120W 写成实测消耗。

真实 GBC 2160p48 AV1 420 10-bit 硬解 → 真正 7680×4320 呈现：Jinc / D3D11 各约 25 秒测量（前 5 秒预热），约 47.95 fps，无新增丢帧。不是完整 GUI、长时或全 HDR 验收。

新增 `softwareUploadAndBilinearMatchReference`：8 组 4:2:0/4:4:4、8/10/16-bit 存储、SDR/PQ，以及有 padding 的输入平面；比较 72 个像素、输出位深及输入未被修改。本机与 780M 最大 RGB 差均为 0。这是有限参考像素回归，不替代完整 HDR/杜比审计。

最新核心通过 VS 仅音频时钟和真实 BD 播放/章节/跳转回归：4 passed、0 failed、0 skipped。BD 跨媒体测试改为检查新源、起始时间及呈现帧；新媒体本来不承诺增加 seek generation，实际 seek 的 generation 断言保留。

## 交付与后续

本地 dist 仅更新 FFF.Native.dll，原文件备份，配置、VPY、缓存及色彩文件保持不变；GUI 沿用已统一的 BD/交互构建。新 DLL SHA256：`094C19E0658235DAFA0C5B5D46E59C211C9390632C71794AAD45E4DCFE95DE50`。

远端只添加隔离诊断文件，没有覆盖 PortableSoft 安装、原核心或配置。本轮没有推送、发行打包、Release 或 Draft。真实 8K48 软解仍需后续解码/上传链路分析；不能用降低色彩精度或仅统计提交帧来宣称完成。
