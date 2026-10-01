# VS Renderer GUI & VS Player 1.0.2

1.0.2 在 Renderer 的实时滤镜链上新增独立 `vs-player.exe`，两个程序共享便携运行时与可保存的 VPY 预设。

## 主要更新

- Renderer 预设管理：独立窗口、保存 / 加载 VPY、Anime4K 模式和自定义 GLSL；带入 MVTools 彩色色度修复与 RIFE 4.26 / Heavy NCNN Vulkan。
- Player：章节时间线、精确时间 / 帧跳转、前后帧 / 秒 / 关键帧、前后视频、0.10–16.00× 不变调倍速、Tab 运行信息。
- 三级右键菜单、主次与外部字幕、SRT 样式、源 / 显示 PNG、Jinc 等显示缩放；Enter 双向无边框全屏，底栏自动隐藏，缩放后左上角还原。
- 播放列表范围悬停 / 停靠 / 调宽、目录与子目录、链接输入；修复 ModelScope 重定向和 PGS 闪退，AList API 解析，网络视频固定 3FP 原生直通，不执行 VS。
- 锐化增强迁到 Vulkan GPU 路径并合并相邻节点；暂停字幕层复用。Anime / Realistic 内置策略，正常播放超 5% 丢帧逐步降载至 Jinc / D3D11，用户操作不误触发。
- 八页设置：语言 / 自动播放、字体 / 背景 / 透明度、播放位置 / 多线程 / 快捷键、预解码预算、独立音视频解码、VS / madVR、索引缓存、文件关联；完整 INI 导入 / 导出。
- FFMS2 / L-SMASH 索引改存软件或自定义目录；10-bit Resample 修复，用户 VPY 保留。
- 完整更新：设置显示版本并检查 ModelScope `VS-GUI`，随包 aria2-next 2.8.3 / 7-Zip Zstandard，验证大小 / SHA-256 / 安全路径 / 内部版本，再完整替换，保留配置、自定义预设和旧目录备份。

## 验证与边界

Release 构建、CTest 8/8；实际便携目录 Player 27/27（含用户提供的 ModelScope / AList），更新专项 7/7。移走测试运行时后两个正式程序独立启动 / 正常关闭。

真实 1080p47.95 的短时 VS 暖态测试：Fast 输出 1440p 48.71 fps，叠加增强后 35.79 fps。帧读回、复制、上传及呈现仍有成本，不能保证所有设备 / 素材实时。RIFE Heavy、HDR 显示器、多显示器 DPI 与长片持续播放没有全面人工验证；HDR 显示 PNG 目前为 8-bit。抗振铃仅 Jinc；D3D11 VideoProcessor 不适用时轻量 Bilinear 回退。

LAV 的本地播放图不替换 VPY 内部源。madVR / LAV 是随包的独立组件，许可证见第三方声明；没有包含 madVR 专有算法源码。

## 下载

完整便携包与 SHA-256 放在 [ModelScope VS-GUI](https://www.modelscope.cn/datasets/ARXChem/Software-List/files?Root=VS-GUI)。本 GitHub Release 保持草稿且无附件，GitHub 压缩包由维护者手动上传。本包使用 Zstandard Ultra 22，随附更新解压工具支持该编码。
