# VS Renderer 1.0.3：三栏、白底参数与设置页

日期：2026-10-02。仅继续修改 Renderer，版本保持1.0.3。

## 用户要求与结果

| 要求 | 实现 |
| --- | --- |
| 参数文字下面不要灰色，控件范围保留灰色 | 参数编辑器、说明、标签和空白区域白底；数值、下拉及文件输入框使用浅灰底。 |
| 默认窗口加宽，参数移到左侧与原视频之间 | 默认1800×900逻辑像素，水平Splitter依次为处理链、参数、视频；初始宽度约280/320/剩余。参数取消260px高度上限，左侧处理链可利用腾出的纵向空间。屏幕可用面积不足时系统限制实际窗口尺寸。 |
| 没有VPY时查看的错误标题未完整展示 | 保留原错误信息和警告类型，消息正文最小宽度480逻辑像素；实际窗口超过520，标题文字及系统按钮有足够空间。 |
| 导出设置栏目偏左 | 统一三页内容边距，将顶部第一栏目左边缘对齐下方编辑器/列表左边缘。 |
| 编码队列表头分隔不明显 | 表头浅灰背景，列之间2逻辑像素灰色竖线、底部1像素分隔；拖列、调整列宽和队列机制保留。 |
| 设置单独一页，不开新窗口 | 左侧底部“设置”切换主窗口页面，源滤镜选择通过“应用”保存，版本和检查更新保留。设置页/分析页/Renderer导航状态一致；分析页仍按需创建，先打开设置不会打乱页面索引。 |

## 相关验证

- 独立 `build/renderer-ui-1.0.3` Release 构建，最终运行五项相关专项：`rendererLayoutAndParameterSizing`、`exportTabsShowSelection`、`rendererSettingsPageAndVpyWarning`、`rendererLoadsPresetDuringStartup`、`compositionExportsCanvas`。含初始化/清理 **7通过、0失败、0跳过**，7238ms。
- 布局覆盖1280×820、1440×900、1700×950及导航展开/收起。验证三个区域顺序、参数滚动区高度超过500、视频控制区对齐与播放组居中，以及所有catalog默认参数标签/字段不重叠。
- 实际绘制像素验证参数空白和说明白底；按截图设备缩放比例验证表头分隔线颜色，精确验证第一导出栏目与命令编辑器的左边缘一致。
- 先进入设置，再打开延迟创建的分析页，再返回设置和Renderer，验证页面/选中状态正确，无设置模态窗口；验证源滤镜保存到独立测试配置，恢复测试原值。
- 触发真实无源VPY警告，验证窗口宽度和标题所需宽度；真实预设启动加载、AB/ABC211/ABCD合成及成片尺寸/像素回归通过。
- 已目视检查独立构建目录截图：`renderer-ui-layout.png`、`renderer-parameters-mvtools.png`、`renderer-parameters-rife.png`、`renderer-export-tabs.png`、`renderer-settings-page.png`、`renderer-vpy-warning.png`。Qt窗口截图不包含系统标题框，标题宽度通过几何验证。
- 首轮测试暴露了测试环境差异：Windows在创建原生窗口时限制实际宽度、截图为设备像素、QtTest默认未设置配置组织名称。修正验证条件并使用独立测试配置后全部通过，没有为了测试修改播放器或解码流程。

日志：`build/renderer-panels-build.txt`、`build/renderer-panels-build-tests.txt`、`build/renderer-panels-tests.txt`。未运行无关完整回归。

## 本地交付

确认共享构建空闲后只重建 `build/mingw-release` 的VSRenderer目标，日志 `build/renderer-panels-shared-build.txt`。只覆盖便携目录 `dist/VS-Renderer-GUI-windows-x64/VSRenderer.exe` 及Renderer/根文档；未全量stage或重建Player。

正式EXE与共享构建SHA-256一致：`E9F67487B8C0FC9527765B261ED5C28153E64B6B84D9D727A64ECE1EA2BC26C6`。

更新前后以及正式启动检查后，vs-player.exe、FFF.Native.dll、INI、原有VPY及LAV目录共56个文件哈希保持。正式Renderer独立启动正常，只向本次创建PID的窗口发送WM_CLOSE，正常退出0。记录 `build/renderer-panels-protected-before.json`、`build/renderer-panels-delivery-audit.json`、`build/renderer-panels-smoke.json`，交接 `build/renderer-handoff-result.md`。

未推送Git、打压缩包或发布。前一轮导出支持范围和参数约束保留，见 `renderer-ui-export-1.0.3.md`。
