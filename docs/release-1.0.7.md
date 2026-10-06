VS Renderer / VS Player 1.0.7，Windows x64 便携版。

### 下载选择

- **Full `1.0.7.7z`**：包含 LAV Filters、madVR、MKVToolNix。
- **Lite `1.0.7-Lite.7z`**：不带上述三个组件；保留 Qt、Python / VapourSynth、3FP、FFmpeg、图片处理、BD 原盘菜单和更新运行时，可从设置的“组件下载”补装可选组件。
- 解压整个目录后运行 `VSRenderer.exe` 或 `vs-player.exe`，无需另装 Qt、Python、VapourSynth。发行包排除个人配置、缓存、截图和自定义 VPY，含全文件 SHA-256 清单；同名 `.sha256` 校验压缩包完整性。
- 系统要求：Windows 10 22H2+ x64、支持 D3D11 的显卡与驱动。增强版 FFmpeg 需要 x86-64-v3，图片转换后端需要 AVX2；Vulkan 滤镜与 RIFE 需要对应驱动。

### 高精度截图与保存路径

- 源画面和实画面统一保存 PNG 每通道 16-bit，采用自适应滤波与最高无损压缩级别；修复 FP16 渲染表面读回，保留源帧与图片已有精度，后台保存减少界面等待。
- 设置 → 基本设置可修改默认截图目录；右键“截图到指定路径…”可为本次源画面或实画面选择文件夹，不改变默认目录。
- HDR 默认保存经过色调映射的 sRGB SDR 预览，普通 SDR 查看器可直接查看；另存 `.hdr.png`、色彩 JSON 与未截断 `.png.rgba32f` 原始浮点数据。HDR 色准测试应使用原始 HDR 图或浮点附件。
- 浮点附件保留负值和高光；文件记录实际输入 / 读回位深，16-bit 保存不会增加原始表面的精度。madVR 当前接口读回仍为 8-bit，原盘菜单专用截图入口尚未提供。

### 底栏与信息面板

- 算法改为底栏标识框：超分显示 `A4KCNN+ / A4KCNN / A4K+ / A4K / Jinc / D3D11`，补帧显示 `Jinc / D3D11` 加 `RIFE+ / RIFE / MVT+ / MVT` 两个框，随当前实际档位更新。
- 底栏和播放列表透明度可分别设置；全屏控制栏自动隐藏，鼠标进入实际底栏范围即可唤出，顶部可唤出标题与退出全屏。
- Tab 优先显示正在使用的文件，文字可滚动，透明度条独立位于下方；新增音频简略 / 详细模式，详细面板显示真实输入 / 输出声道峰值电平。

### 软件解码与字幕

- HEVC / AV1 软件解码线程上限提升至 32，软件帧预算提升至 512 MiB；独立解码线程与会话取帧提交分开运行，避免解码尖峰阻塞已有帧呈现。硬件帧预算仍为 768 MiB，保持八帧上限和 150 ms 预取目标。
- 修复音频先结束时最后视频帧滞留；本机 HEVC 444p10 高码率样本完整播放接受 2252 帧、零丢弃 / 合并。其它硬件和素材的持续实时性能仍需实测。
- 自动匹配同目录的字幕名称变体，可识别 `xxx.sc-jp.ass` / `xxx.tc-jp.ass`，默认加载最匹配的一份，其余在首 / 次字幕菜单切换；排除明显集数冲突。

保留 1.0.6 的 BD 模式窗口、选集、长节目定位、原盘五种字幕语言与歌词等修复。RX 6600 同片实机性能仍待复测，8K 软解及所有 RIFE / MVTools 持续实时性能尚未全部验收；两项既有完整色彩测试存在阻塞，不能宣称完整色彩或显示器物理色准验收通过。

本次截图 / 路径 / 算法标识专项 18 项、相关回归 15 项均通过，0 失败、0 跳过；语言完整性检查 536 条通过。压缩包在独立目录解压后核验全文件 SHA-256、组件差异及启动，个人配置与预设保留。

格式与测试边界见 [高精度截图](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.7/docs/player-high-precision-screenshots-1.0.7.md)、[透明控制栏与音频信息](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.7/docs/player-transparent-audio-info-1.0.7.md)、[软件解码与字幕](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.7/docs/software-decode-subtitles-1.0.7.md)。
