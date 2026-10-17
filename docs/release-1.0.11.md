# VS Renderer / VS Player / VS Cap 1.0.11

本版更新时间组预设、VS 后端、蓝光菜单、播放器底栏与截图交互，新增 16 位 JPEG XL 截图和 HDR 桌面白位还原，并提供精简的单文件播放器。

## VS Renderer 与预设

- 时间组支持独立滤镜链，预设携带全部时间组；全片单组预设可导入任意现有时间组，保留目标组边界。
- 保留内置脚本型预设的实际处理逻辑。本地预设与社区支持递归目录树、展开和收起，下载保留目录结构。
- 默认社区源为 [VS-GUI 公开预设社区](https://www.modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI/PresetCommunity)。
- 右侧时间与进度条同排；路径、浏览、前后帧、播放、并排及缩放控件同排，D3D11 选项完整显示。

## VS Player

- “视频 → 图像截取”和“截图到指定路径…”均支持 **PNG / AVIF / JXL**，分别可截取源画面或含字幕、显示效果的实画面。PNG / JXL 为每通道 16 位无损；AVIF 为有损压缩（Full 使用 AWJ 质量 70，独立版使用外部 FFmpeg / libaom-av1）。
- Full 与独立版内置 JXL 编解码，支持原生 PQ / HLG 色彩标记及桌面白位信息。JXL / AVIF 截取只生成单个文件；PNG 保留原有 HDR / SDR 与浮点附件流程。
- 优化 16 位 HDR PNG 解码与 SDR 预览，修正图片信息中高 DPI 视口的物理像素读数。
- 修复文件章节时间字段适配，全部章节在进度条正确分布。
- 统一蓝光右键与播放列表的返回主菜单入口，从正片返回选集页不再重新播放版权片；首次开盘保留原盘 FirstPlay。
- 非全屏底栏放到视频下方，初始窗口相应增高；全屏继续自动隐藏浮层。
- 独立播放器文件名固定为 **VS-Player.exe**，嵌入 Player 图标，自动创建并保留程序旁的 `player.ini`。单文件分发首次展开运行时缓存，后续复用。
- 单文件版不内置 FFmpeg，优先从 EXE 同目录、其次从 `PATH` 查找，缺失时明确提示。需要 **FFmpeg shared x64**：`ffmpeg.exe`、`ffprobe.exe`、`avcodec-63.dll`、`avformat-63.dll`、`avutil-61.dll`、`swresample-7.dll`、`swscale-10.dll`、`avfilter-12.dll` 及它们的依赖。
- 单文件版保留原生播放、字幕、图片与 BD 导航，不含 VapourSynth、RIFE、PhotoCraft、libVLC、LAV、madVR 或高级色彩运行时。Full 保留完整运行时。

## VS Cap

- 新配置默认保存 **JXL 无损 RGBA16**，格式顺序为 **JXL → AVIF → PNG**；保留已有明确选择的格式设置。截图工具条与贴图右键均支持三种格式，复制 JXL 文件后可用 Alt+T 贴图。
- 修正 HDR 桌面转 SDR 的亮度与颜色：捕获时查询源显示器的 Windows SDR 白位，转换时撤销系统 SDR 增益，不再固定按 203 nit 叠加 ACES。白位随选区、PNG / JXL、HDR 剪贴板和贴图传递。
- 取消三种格式的保存对话框均保留选区与标注；“✓”仍只复制剪贴板，普通 SDR 截图不自动落盘。
- 修正 PNG 压缩参数，保留 16 位四通道；3840×2040 实图 HDR PNG 从约 62.8 MB 降至 1.28 MB，像素一致。
- HDR 自动保存先显示截图层，再后台按所选格式保存；JXL / PNG 同时提供 HDR 原图和 `_SDR` 副本，不再默认生成 JSON 和裸浮点附件。SDR 副本与 VSP 共用白位转换；超出 SDR 亮度或色域的部分裁剪，完整高光保留在 HDR 原图中。
- 修复设置窗口打开时，置顶截图层被模态输入阻断、无法操作的问题；退出截图后可继续编辑未保存的设置。
- 矩形/圆形的点按钮可点击选择边框模式，与填充按钮互斥显示；滚轮仍可调节粗细。
- 填充方块仅放大内部图标，按钮高度与相邻工具保持一致。

## 后端与验证边界

- 升级 **VapourSynth R81、BM3D 11.0、BM3Dvk2 1.0、BestSource 22**，验证 Vulkan 解码与 GPU 时域降噪；默认源链不因此自动切换。
- 合入 **3FP API20 / PR23**，保留本项目原生扩展。PR23 的 Native 差异主要为接口注释命名，不宣称新增 P/E 核、CCD 亲和性或进程提权策略。
- MyGO 实盘选集及重复返回菜单、章节、时间组、外部 FFmpeg 播放和截图交互已做聚焦验证。
- 8k48在7940HX下软解仍然没有通过，当前上限仍然在30F附近。

实图新 HDR PNG 在本机加载及预览约 0.36 秒；这是文件加载采样，HDR 显示器热键端到端出框仍待实机复验。详见 [HDR 截图续修](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/vscap-hdr-1.0.11.md)。

本轮新增回归：截图交互 17 项、播放器截取与 PNG 回归 20 项、无 AWJ 的外部 FFmpeg 路径 9 项通过。3840×2034 HDR 样图转 JXL 后 RGBA16 像素逐点一致，文件由约 8.61 MiB 降至 6.12 MiB，一次本机采样编码 0.53 秒、解码 0.14 秒。色彩显示仍依赖第三方查看器的 JXL / HDR 支持；实际 HDR 屏热键、多屏和第三方查看器效果仍待实机验收。详见 [JXL 截图](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/jpeg-xl-1.0.11.md) 与 [SDR 白位修复](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/vscap-sdr-white-1.0.11.md)。

完整条件与已知限制见 [1.0.11 验证记录](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/update-1.0.11.md)。
