# VS Renderer / VS Player / VS Cap 1.0.11

本版更新时间组预设、VS 后端、蓝光菜单、播放器底栏与截图交互，并提供精简的单文件播放器。

## VS Renderer 与预设

- 时间组支持独立滤镜链，预设携带全部时间组；全片单组预设可导入任意现有时间组，保留目标组边界。
- 保留内置脚本型预设的实际处理逻辑。本地预设与社区支持递归目录树、展开和收起，下载保留目录结构。
- 默认社区源为 [VS-GUI 公开预设社区](https://www.modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI/PresetCommunity)。
- 右侧时间与进度条同排；路径、浏览、前后帧、播放、并排及缩放控件同排，D3D11 选项完整显示。

## VS Player

- 修复文件章节时间字段适配，全部章节在进度条正确分布。
- 统一蓝光右键与播放列表的返回主菜单入口，从正片返回选集页不再重新播放版权片；首次开盘保留原盘 FirstPlay。
- 非全屏底栏放到视频下方，初始窗口相应增高；全屏继续自动隐藏浮层。
- 独立播放器文件名固定为 **VS-Player.exe**，嵌入 Player 图标，自动创建并保留程序旁的 `player.ini`。单文件分发首次展开运行时缓存，后续复用。
- 单文件版不内置 FFmpeg，优先从 EXE 同目录、其次从 `PATH` 查找，缺失时明确提示。需要 **FFmpeg shared x64**：`ffmpeg.exe`、`ffprobe.exe`、`avcodec-63.dll`、`avformat-63.dll`、`avutil-61.dll`、`swresample-7.dll`、`swscale-10.dll`、`avfilter-12.dll` 及它们的依赖；只有静态版 `ffmpeg.exe` 不够。
- 单文件版保留原生播放、字幕、图片与 BD 导航，不含 VapourSynth、RIFE、PhotoCraft、libVLC、LAV、madVR 或高级色彩运行时。Full 保留完整运行时。

## VS Cap

- 修复设置窗口打开时，置顶截图层被模态输入阻断、无法操作的问题；退出截图后可继续编辑未保存的设置。
- 矩形/圆形的点按钮可点击选择边框模式，与填充按钮互斥显示；滚轮仍可调节粗细。
- 填充方块仅放大内部图标，按钮高度与相邻工具保持一致。

## 后端与验证边界

- 升级 **VapourSynth R81、BM3D 11.0、BM3Dvk2 1.0、BestSource 22**，验证 Vulkan 解码与 GPU 时域降噪；默认源链不因此自动切换。
- 合入 **3FP API20 / PR23**，保留本项目原生扩展。PR23 的 Native 差异主要为接口注释命名，不宣称新增 P/E 核、CCD 亲和性或进程提权策略。
- MyGO 实盘选集及重复返回菜单、章节、时间组、外部 FFmpeg 播放和截图交互已做聚焦验证；没有据此宣称全量回归通过。
- 指定 8K48 AV1 **软件播放仍未流畅通过**。同码流 20 秒对照：纯视频提交约 31.99 fps、加 AAC 约 30.95 fps、加 ASS 约 29.79 fps，目标约 47.95 fps。不同管线的纯解码数据不能直接当作音频/字幕开销。长光盘跨段 seek、UIAccess 置顶覆盖及真实 HDR 的既有待验收项仍保留。

完整条件与已知限制见 [1.0.11 验证记录](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/update-1.0.11.md)。

本草稿不含二进制附件。Full 压缩包与单文件播放器由维护者自行上传，正式发布由维护者完成；保留旧配置、原始素材和预设。
