# VS-Renderer-GUI 1.0.9 开发与交接状态

更新时间：2026-10-10，NuxBox1，UTC+8。当前为本地测试构建，仍有未通过的性能项，不能作为全部验收完成的正式版本。

本日后续 D 盘 AV1 yuv444p12 专项与 VS Cap 黑屏修复见 [专项修复与交接记录](fix-av1-vscap-1.0.9-20261010.md)。版本保持 1.0.9；本轮只测试指定 D 盘素材。最新 Full/Lite 已重构建，Cap 非黑真实桌面及工作流验收通过；实际 GUI 开字幕仍未达零丢帧。下方 F 盘和启动数据保留为前轮历史，不能视作本轮新增测试。

## 本轮已完成

- GUI 基线 `user-Wing/VS-Renderer-GUI@5743c1a01208aae28727426bd0eb68d0e0b89af2`；Native 基线 `Lake1059/FFF_Project@20bcc001b24f6b6de4575bddf9b93ec452795b0d`。本轮再次核对 GitHub 默认分支，仍为这两个提交。含上游 #18 空间音频/缩放、#16 HDR10+、#15 Native 上传更新；空间音频尚未实际素材验收。
- `C:\BuildTools\2026` 的 v145 / MSVC 14.51.36231 已构建 Native API 18；Qt 6.10.2 MinGW、QtTest、Qt SVG/工具组件可用。Qt EXE 与 MSVC DLL 通过 C ABI 通信。
- API 18 适配及旧完整版接口移植：外部 VS 帧、Jinc/高级缩放、倍速、EQ/延迟、独立音频时钟、CPU 强制预缩放、HDR 浮点回读和高级色彩桥。保留上游新 HDR 渲染路径；D3D11 档绕过 VS，当前采用原生 D3D11 shader 管线，不能误称已经使用 VideoProcessor。
- 修复连续迟到造成全片无画面：仅有较新已解码帧可替换时才丢弃迟到帧；无音轨软解允许重同步。AAC 采用有界压缩包预读，但 8K 软解仍有欠载，未彻底解决。
- VS Cap 的 DXGI FP16 真实 HDR 捕获、SDR 回退、区域注释工具、剪贴板、普通置顶贴图、热键/冲突检测/托盘设置、自启及后台 AVIF CRF18 已实现。真实 HDR 设备、PNG 精度、剪贴板与 AVIF 工作流已测试。
- Lite 排除 VS/LAV/libVLC 的实际后端源文件；仅 Jinc/D3D11、软硬解；不自动关联文件。原生 libbluray 导航适配器已接入，缺少真实蓝光素材，HDMV/BD-J 尚未实盘验收；BD-J 需要外部 Java。
- `tools/build-1.0.9.ps1` 一次构建四个程序并生成 `dist\Full`、`dist\PlayerLite`。API 18 使用单独完整补丁，避免误套历史 API 14 补丁。

## 本地产物

| 目录 | 入口 | 体积 |
| --- | --- | --- |
| `dist\Full` | `VSRenderer.exe`、`vs-player.exe`、`vs-cap.exe` | 1008.49 MiB，含完整 VS/Python/组件运行时 |
| `dist\PlayerLite` | `vs-player.exe` | 186.07 MiB，保留完整 FFmpeg |

Lite 扣除 FFmpeg 专属依赖后为 **51,964,627 字节（49.56 MiB / 51.96 MB）**。达到先前文档 50 MiB 口径，尚未达到原始提示词严格十进制 50 MB；总包 100 MB 目标尚未达到，需要专门裁剪 FFmpeg。`release.json` 列出扣除的文件；Native 直接依赖的字幕/字体/蓝光库，以及共享运行库仍计入播放器体积。

旧 `C:\PortableSoft\VS-Renderer-GUI` 和 1.0.5 工作树保留；本轮未覆盖旧安装、提交 Git 或推送。GUI 修改在本项目，Native 修改在 `C:\Private\FFF_Project-1.0.9`；完整内核补丁为 `patches/3fp-api18-vsrenderer-1.0.9.patch`。源码归档用于手动回传，不含媒体、dist、工具链或大体积依赖缓存。

## 实测与未通过项目

详细数字见 [本轮验收记录](validation-1.0.9-20261010.md)，原始日志与逐秒 CSV 在 `build\session-20261010`。

- 本地 3840×2160 / 60 Hz、HDR 已开启；F 盘 4K48、8K48 AV1/AAC 原片在硬解 D3D11 的 60 秒计时段均零丢帧、零合并、零音频欠载增量。**4K 本次启动预热阶段丢 6 帧；8K 计时开始有 0.74 秒追帧偏差，之后恢复。全程零丢帧、启动即同步尚未验收通过。**
- 8K48 软解：自适应上传关为 22.23 fps，开为 23.58 fps；CPU 中位数 31.77%→18.82%、GPU 中位数 40.30%→14.64%。该策略包含上游自适应预缩放，不是同画质提升；仍低于实时 47.952 fps，且 AAC 欠载。
- 500 ms 音频储备实验无改善，已撤回。旧内核同素材软解报 `Invalid data found when processing input`，不能给出可信的新旧 FPS 提升倍数。
- 主窗口启动中位数：旧 GUI 896 ms、新 GUI 912 ms；旧播放器 593 ms、新 Full Player 581 ms、Lite 581 ms。播放器约改善 2%，GUI 无显著提升；窗口时间与视频首帧分开统计。
- 真 HDR：3840×2160 FP16 scRGB 采集，内存为 RGBA32F。PNG 为 BT.2020/PQ RGBA16 无损压缩，**转换量化并不是 FP16 像素逐位无损**；原始浮点通过 `.png.scrgb-f32` 和 JSON 伴随文件保留。AVIF 是有损 10-bit 4:4:4 BT.2020/PQ/full、CRF18。
- Lite 独立目录软解、模式白名单、VS/libVLC 不加载测试通过；Cap 发行目录剪贴板/贴图/真实 AVIF 通过。Full 纯系统 PATH 的 13 项发行目录回归通过；中文路径的 RIFE 模型加载失败，故发行目录使用英文名，完整安装路径需纯英文。真实 GUI 发现内嵌 ASS 全片扫描，已补回有界读取，正在复测。

## 后续优先顺序

1. 修复硬解启动丢帧/追帧；使用含 AAC/字幕的实际 GUI 路径重复测试全程计数。
2. 解决 8K 软解解码/渲染阻塞和 AAC 供给；不能扩大缓冲或放宽计数冒充修复。
3. 完成严格 50 MB 非 FFmpeg 体积目标及精简 FFmpeg 总包目标。
4. 有素材后验收 HDMV/BD-J、IAMF/空间音频；人工检查区域标注、热键冲突、保存路径和自启全流程。
5. 使用单独测试目录，保留用户播放器进程与配置；失败日志保留，不用编译成功代替产品验收。
