# VS Renderer / VS Player 1.0.10

本版合并另一台设备返回的 1.0.9 源码与 Native API 18，并更新 HDR→SDR 默认色彩路由、软解统计/性能及 VSCap 编辑条。

- Full 新配置默认按源/输出选择色彩引擎：PQ/HLG→SDR 使用 libplacebo，SDR 和 HDR 输出保留原生路径；旧配置保留，可手动选择新的自动模式。色彩窗口“确定 / 取消 / 应用”统一中文。
- 无音频 CPU 播放按真实时间推进，丢帧与视频落后可见；8K AV1 420p10 改为 16/16 工作线程与帧上下文，减少解码内存。ASS 初始化与静态字幕合成减少重复工作。
- VSCap 工具显示选中框，左侧常驻颜色和工具参数；线工具可切直线/箭头，支持 Ctrl+Z 撤销和撤销按钮。
- Full 包含 Renderer、播放器、VSCap 及完整处理/编辑依赖；PlayerLite 为单独播放器，不含 VS、RIFE、PhotoCraft、libVLC/LAV/madVR 或高级色彩运行时。

本机 4K48 444p12 与 8K48 420p10 软件解码结果见 [验收记录](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/validation-1.0.10.md)。8K 仍未达到全程流畅：不要将更准确的计时和丢帧统计解释成吞吐已达标。测试关闭硬件解码、CPU 预缩放和位深降低。

附件由维护者手动上传，本草稿不含附件：`1.0.10.7z`（Full）与 `1.0.10-PlayerLite.7z`（独立播放器）。解压到新目录运行；保留旧配置与原始素材。包内含 `release.json` 与 SHA-256 资源清单。
