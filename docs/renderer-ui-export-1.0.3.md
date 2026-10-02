# VS Renderer 1.0.3：布局、参数控件与对比导出

日期：2026-10-02。仅修改 renderer，版本保持 1.0.3。

本页保留前一轮验证与交付记录。后续三栏布局、设置页和导出对齐更新已交付，当前程序与证据见 [后续记录](renderer-panels-1.0.3.md)。

## 布局与参数

- 展开的页面导航使用独立图标区和文字区，间隔 12 个逻辑像素；收起仍为图标。
- 删除右上“实验设置”和参数区底部重复的“查看生成的 VPY”；顶部 VPY 操作保留。
- 底部控制区属于视频工作区，进度条位于两窗格下面，时间与并排模式栏左端对齐；帧/播放/帧通过左右等伸缩列保持在视频工作区中央，展开导航、调整侧栏或 resize 后自动重新布局。
- 输入框与浏览在同一行；解码/VS 环境文字移出输入区，对比标题右边只显示“预热中/已预热”，详细状态保留在悬停提示及状态栏。
- 左侧默认从 328 缩到 280 个逻辑像素，最小宽度随最长滤镜名称计算。列表仅显示完整滤镜名称，分类与说明在提示中，分类搜索保留；取消原 150 高度上限，更多空间分给滤镜库。
- 参数区可纵向滚动。标签占剩余列宽并换行，bool 和其他控件右对齐；下拉按当前文字计算宽度，弹出列表按最长选项计算宽度。文件位置单独整行，避免挤压标签。参数值、信号与滤镜机制不变。
- 导出三栏目使用蓝色选中背景、加粗文字和蓝色下边线，未选中栏目保持灰色。

## 对比导出限制

| 布局 | 支持 |
| --- | --- |
| AB 滑块 | 是 |
| 双路普通并排 | 否 |
| ABC 左半+右两区 / 右半+左两区（2+1，即 2+1+1 面积） | 是 |
| ABC 普通三路并排 / 横向三段 / 纵向三段 | 否 |
| ABCD 常规 2×2 / ABCD 滑块 2×2 | 是 |
| 5–9 路三列网格及单路 | 否 |

不支持的布局点击导出时显示明确提示，不创建导出窗口或加入任务。输出宽高取所有导入视频中像素数最大的源，即使该视频暂时未显示；不依赖窗口大小。合成脚本与编码输出都使用这个宽高。对比导出拒绝 3FUI 中常见的尺寸、缩放、裁切、旋转参数，避免重复处理和改变画布；普通第一页的单视频 VS 导出不受此限制。

原有 GPU Jinc/Super-XBR/双边算法缺少一致 VS 导出实现的提示继续保留，本轮不静默替换算法，也不新增这些实现。

## 验证

- 独立 `build/renderer-ui-1.0.3` Release 构建 VSRenderer 和 analysis 测试，避免与播放器构建冲突。
- 最终仅运行本轮四个相关测试，含初始化/清理共 **6 通过、0 失败、0 跳过**：`rendererLayoutAndParameterSizing`、`exportTabsShowSelection`、`rendererLoadsPresetDuringStartup`、`compositionExportsCanvas`。日志 `build/renderer-ui-final-tests.txt`，构建日志 `build/renderer-ui-build-final.txt`。
- 布局验证覆盖 1280×820、1440×900、1700×950 与导航展开/收起；验证控制区宽度/位置和几何中心、输入与浏览同排、所有滤镜默认参数标签与控件不相交、bool 右侧位置、当前下拉文字所需宽度。
- 实际 VPY/MKV 导出验证 AB、ABC 两种 2+1、ABCD 两种 2×2；其它模式验证错误提示。320×180 的最大源与其它 160×90 输入混合，最大源隐藏时仍输出 320×180；对四象限进行实际 RGB 像素检查，拒绝尺寸/裁切/缩放及 3FUI 转义 scale_cuda 参数。
- 截图已目视检查：`renderer-ui-layout.png`、`renderer-parameters-mvtools.png`、`renderer-parameters-rife.png`、`renderer-export-tabs.png`，位于独立构建目录。
- 独立测试第一次启动因未提供 3FP 的 FFmpeg 动态依赖而退出；补齐运行时搜索路径后通过，没有修改产品代码处理测试环境缺失。
- 协调确认 `build/mingw-release` 空闲后，只重建 VSRenderer 目标，保持后续整体部署脚本的 renderer 产物同步；不构建或替换播放器、原生 DLL。

未运行无关完整回归；未推送、打包或发布。播放器 Apply、AVIF 内容验证由另一会话负责，其代码和文档保留。

## 本地交付

已仅更新 `dist/VS-Renderer-GUI-windows-x64/VSRenderer.exe` 及 renderer/根文档。正式 EXE 与 `build/mingw-release/VSRenderer.exe` SHA-256 一致：`0E5E4A4F9EA5497C32D9F91DAACCCB07DD5EBB854CCE8C0035611852730850D7`。`vs-player.exe`、FFF.Native.dll、player.ini 和 vpy 目录原有文件合计 17 项哈希在更新及启动检查后均保持一致；未复制 LAV 或其它运行时。

正式 Renderer 独立启动成功，只向本次创建 PID 的窗口发送 WM_CLOSE，正常退出码 0。记录 `build/renderer-ui-delivery-audit.json`、`build/renderer-ui-smoke.json`，不是人工长时操作验证。最终交接结果 `build/renderer-handoff-result.md`。
