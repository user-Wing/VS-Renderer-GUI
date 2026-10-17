# 1.0.12：Jinc 偏暗与无音轨 VS 播放修复

## 用户复现与对照

Windows HDR 关闭。指定素材 `D:/Video Compress Test/PlayerTest/ChouKaguya432048-NoAudio.mkv` 为 7680×4320、AV1、YUV420P10、limited、BT.709 SDR、无音轨，109.485 秒。`D:/Video Compress Test/Large-Test/1080p-2D.mkv` 为 1920×1080 H.264、无音轨、色彩标签未指定，16.058 秒。只读取原片，索引写入测试缓存。

旧内核在同一 8K 文件 10 秒、同一视口的 49 点实际 GPU 读回中：D3D11 平均亮度 0.395417，Jinc 0.162831，比例 **0.411795**，复现明显偏暗。测试 `requestedSdrJinc` 修复前失败。人工外观反馈与 GPU 像素一致，不是用控件属性或截图文件的标签推断。

## Jinc 与 HDR/SDR

`VideoRenderer.cpp` 中 Jinc 权重 `Texture1D` 和 kind-3 色彩变换 `Texture3D` 同时占用 t4。DrawWithShader 先绑定 Jinc 权重，又绑定变换 LUT，导致 Jinc 采样了错误资源。均匀中灰的 D3D11 输出约 0.5、Jinc 接近 0。将 Jinc 声明及全部绑定改为独立 **t5**；动态 HDR 曲线保留 t3、色彩变换 LUT 保留 t4。重新生成通用 ShaderBytecode.h 与三个 SDR layout 专用 shader，重建完整 IAMF 的 Native；没有只修改 HLSL 字符串而遗漏实际嵌入字节码。

视频数据路线分别为：原生 AVFrame → D3D11/Jinc 呈现；或 FFMS2/LSMASH → VS 滤镜与帧属性 → 外部帧 ABI → 原生呈现。VS 帧继续传递 range、matrix、primaries、transfer、chroma；不把 VS 后处理结果再次套源文件 HDR 标记。未标记的 1080p 输入保持 unspecified / SDR，不因转换到 16 bit 而标为 PQ/HLG。

早上 `HdrPreview.h` 的桌面参考白逻辑由 `PreviewPane::setImage` / 桌面 SDR 伴随图调用，处理 QImage 图片；直通视频和 VS 外部帧进入原生 GPU 纹理，不经过这个图片函数。本轮没有撤回截图白位修复，也没有通过曝光补偿掩盖渲染资源错误。

## 无音轨 VS 时钟

旧版 `requestedAnimeNoAudio:3` 实盘播放 2.5 秒后媒体位置已变成 16.058 秒，状态暂停且没有新处理帧。clock-only 会话跳过视频解码、又没有音频消费者来限制读包，快速把整段读到 EOF。另一个条件在 Play 时仍无条件启用视频预缓冲，时钟会话不会产生对应队列，会停在起点。

修复：无音轨 clock-only 使用 QPC 真实时间推进，到作者时长才结束；Play 不为 clock-only 启用视频预缓冲。正常带音轨播放和普通视频解码的节奏保持。独立时钟回归覆盖播放、暂停、8 秒定位与恢复，墙钟 1 秒对应媒体约 1.008 秒。

## 验收

- `jincColorMatchesNative`：8-bit 420 / 16-bit 444，SDR / PQ / HLG，自动 / 强制 SDR 输出，原生 / HDR→SDR 自动色彩路径；均匀中灰与算法一致性检查通过。Jinc / D3D11 最大 RGB 差 **0.000977516**，约一个 10-bit 码值。合成 HDR 输入只验证 GPU 处理及读回，不代替真实 HDR 显示器验收。
- 同一指定 8K SDR 文件 10 秒：D3D11 平均亮度 0.395417，Jinc **0.391150**，比例 **0.9892**。缩放算法可有局部差异，没有用完全相等作为画面要求。
- `requestedAnimeNoAudio` 四档 CNN+ / CNN / no CNN+ / no CNN 全部通过：自动播放启动完成预解码后连续出帧，保持 VS 模式；暂停后能跳到 8 秒并读回实际 GPU 像素，没有直接跳到结尾。日志合计 6 passed（含 init/cleanup），`build/anime-autoplay-final.txt`。此前暂停冷启动的 CNN+ 预取曾超时，不将其作为实时性能通过证据；本轮没有做帧率基准或宣称所有预取配置均流畅。
- 8.008 秒 BT.709 参考中心 RGB（FFmpeg，4×4 均值）：0.98407 / 0.82744 / 0.61251；四档处理后中心最大通道差约 0.0082，增强链允许有锐化差异，没有出现亮度量级变化或错误 HDR 标记。
- 桌面白位与 HDR FP16 截图读回回归 4 passed；BD 鼠标跨段 / 菜单回归 4 passed；Lite 7 passed、1 skipped（实盘原始 FirstPlay 环境未设）。未运行全量套件。

详细日志：`build/jinc-neutral-before.txt`、`build/jinc-color-after.txt`、`build/jinc-real-old-at10s.txt`、`build/jinc-real-final.txt`、`build/anime-clock-final.txt`、`build/jinc-hdr-regression.txt`、`build/jinc-bd-regression.txt`、`build/jinc-lite-regression.txt`。中途失败与最终通过分开保存，不覆盖失败证据。

版本保持 1.0.12，更新 Full / Lite 原生内核并重做单文件。固定 PR23 上游应用完整补丁后，PlayerSession.cpp / VideoRenderer.cpp / 两份 shader header 与实际构建源逐项一致。生成的两份字节码 header 使用 Git binary hunks 保存；.gitattributes 固定此补丁为 LF，避免 Windows CRLF 损坏二进制补丁。Full / Lite 部署核对 INI、VPY、ICC/ICM、LUT 哈希；FFmpeg 外置策略保持，发布压缩包仍由维护者制作。

最终直接指定已部署 Full 的 FFF.Native.dll，合并运行灰阶、指定 8K、无音轨时钟、四档 Anime4K、VS 色彩属性、桌面白位、HDR FP16 读回与 BD 定位/菜单，**14 passed、0 failed、0 skipped**（含 init/cleanup，44.656 秒）。日志 `build/jinc-color-delivered-final.txt`。部署与单文件核验结果分别在 `build/jinc-color-deploy.txt`、`build/jinc-color-single-audit.txt`，配置保持，内嵌 Native 与目录式 Lite 同源，原盘 index 哈希不变。
