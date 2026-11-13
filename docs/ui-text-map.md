# VSR / VSP 界面文字位置索引

按本次修改后的源码定位（2026-11-13）。手工编辑后行号会移动，优先按表中的原文/函数名搜索。此索引只记录位置，不会修改 `PlayerSettings.cpp`。

## 先区分修改方式

- C++ 中的 `QStringLiteral("中文")`、`tr("中文")`、`setText(...)` 等需要重新编译相应 EXE 才生效。
- 当前中文主要取 C++ 原文；`zh_CN.json` 是对照清单，单改该 JSON 不会替换所有中文界面。英文按中文键从 `en_US.json` 查找，运行目录 `languages/en_US.json` 可覆盖内置英文对照。改中文键时也应同步两份源码语言 JSON。
- 开机引导、菜单分类/算法说明有可编辑的运行时 `preset-descriptions.json`。Full 位于 `vpy/preset-descriptions.json`；Lite 位于播放器配置目录。已有文件优先，修改资源默认值不会覆盖旧文件。引导文字见其中 `playerGuide/center/text`（你截图的“完整版”也来自这里）。
- 英文退出按钮为 Save / Don't Save / Cancel，中文为保存 / 不保存 / 取消；标准按钮对应 `PlayerLanguage.cpp` 的映射表。

## 源码位置

| 程序 | 文字区域 | 文件与行号 | 搜索锚点 |
|---|---|---|---|
| VSR | 窗口标题 | [src/app/MainWindow.cpp:131](../src/app/MainWindow.cpp#L131) | `setWindowTitle(QStringLiteral("VS Renderer` |
| VSR | 左侧四个页面名称 | [src/app/MainWindow.cpp:171](../src/app/MainWindow.cpp#L171) | `button->setText(i == 0` |
| VSR | 左侧 BD / Agent / 设置名称 | [src/app/MainWindow.cpp:199](../src/app/MainWindow.cpp#L199) | `bdButton->setText` |
| VSR | 导航图标图形、颜色与线宽 | [src/ui/NavigationIcons.h:10](../src/ui/NavigationIcons.h#L10) | `inline QIcon navigationIcon` |
| VSR | 展开按钮图标、文字位置和控件高度 | [src/app/MainWindow.cpp:90](../src/app/MainWindow.cpp#L90) | `class NavigationButton` |
| VSR | 顶部工具栏按钮 / 菜单提示 | [src/app/MainWindow.cpp:489](../src/app/MainWindow.cpp#L489) | `void MainWindow::buildToolbar` |
| VSR | 左侧源文件、滤镜目录标题 | [src/app/MainWindow.cpp:557](../src/app/MainWindow.cpp#L557) | `QWidget *MainWindow::buildSidebar` |
| VSR | 高级参数区域标题 | [src/app/MainWindow.cpp:673](../src/app/MainWindow.cpp#L673) | `QWidget *MainWindow::buildParameterPanel` |
| VSR | 滤镜名称、参数名及算法说明 | [src/graph/FilterCatalog.cpp:14](../src/graph/FilterCatalog.cpp#L14) | `namespace vsr` |
| VSR | 参数说明标签及 GLSL 预览/导出提示 | [src/ui/ParameterEditor.cpp:205](../src/ui/ParameterEditor.cpp#L205) | `shaderPreviewExportHint` |
| VSR | 时间组、帧号与左下角单行说明 | [src/app/MainWindowTimeGroups.cpp:53](../src/app/MainWindowTimeGroups.cpp#L53) | `QWidget *MainWindow::buildTimeGroups` |
| VSR | 处理路名称与显示/隐藏 | [src/app/MainWindowRoutes.cpp:27](../src/app/MainWindowRoutes.cpp#L27) | `routeChoice_->clear()` |
| VSR | 预览区标题、CPU/GPU/丢帧文字 | [src/app/MainWindow.cpp:708](../src/app/MainWindow.cpp#L708) | `QWidget *MainWindow::buildWorkspace` |
| VSR | 底部播放、时间/帧号操作区 | [src/app/MainWindow.cpp:763](../src/app/MainWindow.cpp#L763) | `QWidget *MainWindow::buildTransport` |
| VSR | 状态栏入口与后续动态状态 | [src/app/MainWindow.cpp:1255](../src/app/MainWindow.cpp#L1255) | `void MainWindow::setStatus` |
| VSR | 退出保存弹窗标题与正文 | [src/app/ProjectProtection.inc:52](../src/app/ProjectProtection.inc#L52) | `项目尚未保存` |
| VSR | 导出窗口文案和编码状态 | [src/ui/ExportWindow.cpp:49](../src/ui/ExportWindow.cpp#L49) | `ExportWindow::ExportWindow` |
| VSR | 图像分析比对页 | [src/ui/AnalysisPage.cpp:39](../src/ui/AnalysisPage.cpp#L39) | `AnalysisPage::AnalysisPage` |
| VSR | Agent 页面固定文字 | [src/ui/AgentPage.cpp:69](../src/ui/AgentPage.cpp#L69) | `AgentPage::AgentPage` |
| VSP | 窗口标题、打开文件及状态消息 | [src/player/PlayerWindow.cpp:121](../src/player/PlayerWindow.cpp#L121) | `PlayerWindow::PlayerWindow` |
| VSP | 底栏播放、音量、时间/帧号与提示 | [src/player/PlayerControls.cpp:23](../src/player/PlayerControls.cpp#L23) | `namespace vsr` |
| VSP | 设置页（你正在手改的文件） | [src/player/PlayerSettings.cpp:128](../src/player/PlayerSettings.cpp#L128) | `void PlayerWindow::showSettings` |
| VSP | 右键菜单名称与提示 | [src/player/PlayerMenus.cpp:75](../src/player/PlayerMenus.cpp#L75) | `void PlayerWindow::showContextMenu` |
| VSP | 模型档位、补帧名称和算法徽标 | [src/player/PlayerProfiles.cpp:61](../src/player/PlayerProfiles.cpp#L61) | `QStringList PlayerWindow::qualityNames` |
| VSP | Tab 视频信息及音频说明 | [src/player/PlayerInfo.cpp:24](../src/player/PlayerInfo.cpp#L24) | `void PlayerWindow::updateInfo` |
| VSP | Tab 信息框下拉框、透明度等 | [src/player/PlayerInfoPanel.h:36](../src/player/PlayerInfoPanel.h#L36) | `audioMode_->addItems` |
| VSP | 功耗状态文字 | [src/player/PowerSensors.h:16](../src/player/PowerSensors.h#L16) | `PawnIO` |
| VSP | 无视频时引导文字（源默认值） | [assets/player-guide.json:3](../assets/player-guide.json#L3) | `"text"` |
| VSP | 引导读取、布局、绘制 | [src/player/PlayerIdleGuide.h:15](../src/player/PlayerIdleGuide.h#L15) | `class PlayerIdleGuide` |
| VSP | 菜单分类说明（源默认值） | [assets/player-menu-descriptions.json:1](../assets/player-menu-descriptions.json#L1) | `{` |
| 共用 | 中文/英文切换与语言文件读取 | [src/player/PlayerLanguage.cpp:15](../src/player/PlayerLanguage.cpp#L15) | `PlayerLanguage::PlayerLanguage` |
| 共用 | 保存/不保存/取消等 Qt 标准按钮翻译 | [src/player/PlayerLanguage.cpp:27](../src/player/PlayerLanguage.cpp#L27) | `static const QHash` |
| 共用 | 英文对照、对应中文键 | [assets/languages/en_US.json:1](../assets/languages/en_US.json#L1) | `{` |
| 共用 | 中文对照清单 | [assets/languages/zh_CN.json:1](../assets/languages/zh_CN.json#L1) | `{` |
| 共用 | 页面主题编辑器文字 | [src/ui/PageTheme.h:76](../src/ui/PageTheme.h#L76) | `inline bool editPageThemes` |
| 共用 | 关于页文字 | [src/ui/About.h:9](../src/ui/About.h#L9) | `label->setText` |

## 快速查某一句文字

```powershell
rg -n -F "你看到的文字" src assets
```

不要改占位符 `%1`、`%2`、`{...}`、JSON键名、控件 `objectName` 或参数ID；它们可能被代码和测试使用。纯文案改引号内的显示文字即可。
