# VS Renderer GUI & VS Player 1.0.5 Full

1.0.5 改进原生播放和大图交互，新增 BD 识别/Remux 与组件下载。完整便携包包含 MKVToolNix，发行附件由维护者上传。

## 播放与 BD

- Player 右键打开 BD 文件夹或装载光盘镜像后，读取 MPLS 并直接加入右侧播放列表；轨道与字幕通过右键切换。
- Renderer 新增 BD 一键 Remux，按播放列表顺序、片段边界及章节列出剧集候选；支持完整节目、手动 M2TS、自定义 JSON 模板和结构反馈。使用 MKVToolNix 保留轨道与章节，不重新编码。
- 空窗口仍可切换 3FP-HW/SW；Tab 信息面板增加统一透明背景、默认 50% 透明度、百分比及可见调整手柄，设置窗口不会被信息面板遮挡。
- 修正帧号提交、进度条预览与鼠标缩放；补帧提供 Jinc/D3D11 双自动入口和八固定组合，VS 链使用独立音频时钟，避免重复视频解码。

## 组件与图片

- 设置新增树形“组件下载”：软件本体、FFmpeg、MKVToolNix、MadVR、LAV-Filters、aria2-next。显示可下载的 Windows 程序包及旧版本，支持自动选择更高版本和清理下载缓存。
- 包校验后按实际 x64 主程序目录识别包装层级，在退出后安装到固定组件目录；配置保留、旧文件备份。旧本体只下载并提示路径，不降级安装；其它组件可手动安装旧版。
- Full 包内置 MKVToolNix 102.0，安装位置 `runtime/mkvtoolnix`。
- 大 PNG 并行预测、受内存约束的显示缓存、图层实时拖动、Delete、工具箭头及信息浮层修复。
- 大分辨率 AV1 软件解码调整 dav1d 线程上限与 YUV 纹理上传；保留 444/10-bit 精度。

## 验证与限制

真实 BD remux、直接播放、跳转与剧集列表通过；MyGO 上卷七集、Bloom 四卷十三集元数据识别通过。真实 MKVToolNix GUI 下载/校验/解压，以及组件路径/版本/配置保留、既有完整更新回归通过。

Player、大图、VS 帧桥和原生色彩相关专项已有记录。TEST.png 本机进入编辑器约 8.80 秒。780M 真实 8K48 软件回退呈现约 18–19 fps，完整 RIFE/MVTools 仍未全部达到实时目标；不宣称 AV1 444 新增硬解支持。镜像装载未实盘验收，BD-J/加密盘/复杂盘型、完整 PSD 效果和部分 HDR 显示器观感仍有边界。详见仓库 docs 中的对应专项记录。

Windows 10 22H2+ x64、D3D11；增强 FFmpeg 要求 x86-64-v3，AWJimage 要求 AVX2，RIFE 需要 Vulkan。

归档：`1.0.5.7z` 与 `1.0.5.7z.sha256`。包根目录 `VS-Renderer-GUI-1.0.5-windows-x64`，排除个人配置、用户 VPY、缓存、下载包、测试程序和日志。本 Release 保持 Draft，无二进制附件；维护者自行上传 GitHub/ModelScope。

大小：259894812 字节。SHA-256：`f2c80dd377e667b5cc64d3a1801219d0d15ec5a5fbbef4bd4f4a93b74a7e7c9d`。7z 完整性、独立解压资源哈希、Renderer/Player 主窗口启动、包内 MKVToolNix 运行和更新器包格式专项通过。
