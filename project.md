# VS Renderer / VS Player：项目开发地图

核对日期：2026-10-25。源码版本 **1.0.14**，以 `CMakeLists.txt` 为准；Native 固定 API20 / 上游 main `922e4b70268fcbc83f0deb06d9e2c1b62e7a33b2`，含 #35，并通过本地补丁迁入 AI 字幕 #36/#37 Native 能力。本文件提供开发入口，使用步骤见 [docs](docs/README.md)，版本变化见 [changelog](changelog.md)，版本测试结论见 [台账](development/validation.md)。

## 1. 发心与边界

为需要可视化滤镜、画面对比和高精度媒体播放的 Windows 用户提供桌面工具。Renderer 编排处理链和导出，Player 负责播放，VSCap 负责捕获与标注，PhotoCraft 提供独立编辑工作区。

成功标准是处理路径可检查、时间轴一致、便携依赖可复现、问题可定位到模块。非目标包括通用 Python IDE、任意 FFmpeg 命令设计器、私有 NGU 算法仿制和全部设备/格式兼容保证。

## 2. 快速开始

开发需要 C++20、CMake 3.25+、Ninja、Qt 6.8+ 的 MinGW 套件；Native 使用独立 MSVC 工具链，通过 C ABI 与 GUI 通信。完整命令见 [依赖与构建](docs/dependencies.md) 和 [API20 构建](docs/native-api20-build.md)。

已有依赖时：配置 `QT_ROOT` 和工具链 PATH → `cmake --preset windows-mingw-release` → `cmake --build --preset windows-mingw-release` → 按改动模块运行 CTest/QtTest。

三个常见坑：① GUI EXE、Native 与 FFmpeg shared ABI 必须匹配；② 只复制 EXE 不构成完整部署；③ 首次索引/着色器编译与稳定播放性能分开测。

## 3. 架构决策

| 决策 | 原因与适用范围 | 不采用的方案 / 重估条件 |
| --- | --- | --- |
| Qt Widgets + C++20 | 适配 Windows 控件、HWND 和 D3D11 | 不引入 QML；跨平台或场景图需求改变再评估 |
| MinGW GUI → Native 动态 C ABI | 隔离 MinGW/MSVC C++ ABI | 不跨工具链传 C++ 对象；升级必须核对结构版本和 DLL |
| FilterGraph → VPY → VSScript → 3FP | 脚本可审查，预览与导出共用处理描述 | 不以离线 vspipe/Y4M 管道充当实时帧桥 |
| 直通与 VS 路径分开 | 普通播放不等待 VS 索引 | 不无条件初始化全部增强依赖 |
| 3FP原生色彩默认 | SDR/HDR使用现有3FP处理；旧自动色彩档迁回原生，libplacebo仅由Full手动启用以对照mpv | 不根据HDR标记自动进入libplacebo；手动档与ICC/LUT参数保留 |
| 时间组独立 FilterGraph | 同片不同段落可使用不同链 | 拼接要求各组输出规格一致，不静默改变格式 |
| 专用时间轴与请求代次 | 管理 seek、补帧和源/输出 FPS | 不使用 seek 后局部帧号直接推算全局源帧 |
| 直通相对逐帧 | `PlayerWindow::stepFrame` 调用内核当前PTS逐帧；Native重新解码局部GOP确认相邻帧 | 不用绝对帧号逐帧，也不把稀疏缓存中的近邻当成真实相邻帧；VS仍按输出帧号定位 |
| 查看与编辑解耦 | PhotoCraft 独立进程保留未保存工作 | 旧 Qt 编辑器仍有源码/测试，不作为生产编辑入口 |
| 固定上游 + 完整 Native 补丁 | 保留本地扩展并可重新构建 | 不拼装 API14/18/20 不同版本 DLL 或补丁 |
| 软件播放同步与内存预算 | 上游线程/帧池/CPU 域按负载逐级调整；音频从60ms起按告警升至240ms，呈现允许最多250ms的平滑节奏容差；保留VSP的8K关键包参考帧追赶 | 不把显示器限帧记为解码丢帧；CPU预缩放仍默认关闭，8K逐帧实时按素材验收 |
| AI 字幕外置推理 | 固定 main + #36/#37 Native 音频 tap；Qt 有界传 PCM 到外置 worker，跳转按 generation 隔离 | 模型与 llama-server 不随包下载；不移植上游 VB 页面，不将未装模型测试当作识别验收 |
| Shader 平面输出 | `third_party/vsr-placebo` 直接回读 YUV 平面、合并增强 hooks，独立命名空间与 color 依赖 | 保留标准 placebo 插件作为缺少组件时的兼容路径；不把 VS 吞吐当作 Native 播放帧率 |

## 4. 目录地图与改动入口

| 目录 | 职责与关键入口 |
| --- | --- |
| `src/app/` | Renderer：`MainWindow.cpp`、`MainWindowTimeGroups.cpp`、`MainWindowPresets.cpp` |
| `src/graph/` | 滤镜定义/图/预设：`FilterCatalog.cpp`、`VpyScriptBuilder.cpp`、`PresetStore.cpp` |
| `src/backend/` | `VapourSynthFrameServer`、`ThreeFpPlayer`、`ThreeFpApi`、`FrameTimeline.h`、导出与进程 |
| `src/ui/` | `PreviewPane`、`CompareView`、`AnalysisPage`、参数、预设社区及导出窗口 |
| `src/player/` | `PlayerWindow`、`PlayerControls`、`PlayerProfiles`、字幕/音频/设置/图片；`PhotoCraftEditor.*` 连接编辑器 |
| `src/cap/` | `main.cpp` 负责截图/标注/贴图，`DesktopCapture` 捕获，`UiAccess` 启动 |
| `src/bluray/` | `BlurayCatalog` 解析节目，`BlurayWidget` 提供共用界面 |
| `src/color/`、`src/image/` | 色彩桥、高精度图片、PSD/HDR/JXL 与保留的 Qt 编辑实现 |
| `src/update/` | `PortableUpdater`、`ComponentDownloads` |
| `assets/`、`third_party/` | 图标/语言/滤镜资源与第三方来源、运行时说明和许可 |
| `patches/`、`tools/` | 固定上游补丁；构建、部署、打包和测量脚本 |
| `tests/` | 可执行回归测试源码与小型 fixtures，保留按模块划分 |
| `docs/` | 按任务组织的现行操作指南，不放逐版本测试/发布流水 |
| `development/` | 源码工作区测试台账、合并历史原文与证据；不随运行包复制 |
| `.deps/`、`build/`、`dist/` | 忽略的依赖、构建/原始日志、本地运行产物；不等于已发布附件 |

## 5. UI 地图

| 入口与导航 | 文件 |
| --- | --- |
| Renderer：导入 → 时间组/处理链 → 参数/脚本 → 源与输出预览 → 预设/导出 | `src/app/MainWindow*.cpp`、`src/ui/ParameterEditor.cpp`、`CompareView.cpp` |
| 图像分析比对：多源 → 偏移/布局 → 对比画布导出 | `src/ui/AnalysisPage.cpp`、`MultiCompareView.cpp`、`AnalysisExport.cpp` |
| BD Remux：节目识别 → 轨道/输出 → 队列 | `src/bluray/BlurayWidget.cpp` |
| Player：打开媒体 → 底栏/列表 → 右键音频/字幕/设置 → 图片工具 | `src/player/PlayerWindow.cpp`、`PlayerMenus.cpp`、`PlayerSettings.cpp` |
| Player色彩：解码设置 → 3FP默认 / 手动libplacebo对照；旧自动档迁回原生 | `src/player/PlayerColor.cpp`、`PlayerWindow.cpp` |
| PhotoCraft：图片工具栏 → 独立编辑工作区 | `src/player/PhotoCraftEditor.*`、`tools/build-photocraft.ps1` |
| VSCap：热键 → 选区调整 → 标注 → 最终裁切/复制/保存/贴图 | `src/cap/main.cpp` |

## 6. 外部依赖

| 依赖 | 固定版本 / 准备与调用 | 缺失行为 |
| --- | --- | --- |
| Qt | 6.8+；本地记录 6.10.2，MinGW；CMake 查找 | 编译或 GUI 启动失败 |
| VapourSynth / Python | VS API4 / R81；构建和 stage 脚本，`update-vapoursynth-runtime.py` 锁定哈希 | VS 链不可用；直通路径独立 |
| VS 插件 | BM3D 11.0、BM3Dvk2 1.0、BestSource 22；其余随运行时保留 | 对应滤镜不可用；不能用 API3 DLL 替代 |
| FFF.Native | API20 / main `922e4b7`；`build-native-api20.ps1` 应用完整补丁，配置112字节、snapshot464字节，动态 C ABI | 原生播放/呈现不可用 |
| FFmpeg | 当前 shared ABI：63/63/61/7/10/12；Native SDK 与运行 DLL 配套 | 相应解码/导出失败，单文件启动器先检查组件 |
| GLSL / RIFE | 随包 shader 与 NCNN 模型，`stage-shaders.ps1` / `install-analysis-plugins.ps1` | 相应增强不可用；RIFE 需要 Vulkan |
| libVLC / LAV / madVR | Full 可选后端，stage 脚本及各自许可 | 对应后端不可用，PlayerLite 不包含 |
| PhotoCraft | 固定源与补丁见 `tools/photocraft-source.json`、`build-photocraft.ps1` | 详细编辑不可用，基础查看独立 |
| JXL / AWJ / 图片库 | `tools/jxl-runtime.cmake`、`awj-runtime.cmake`、`image-runtime.cmake` | 对应格式/转换不可用；AWJ 需 AVX2 |
| aria2 / 7-Zip / MKVToolNix | `stage-update-tools.ps1`、`stage-mkvtoolnix.ps1` | 更新下载/解压或 Remux 不可用 |

来源、安装和验证入口见 [构建指南](docs/dependencies.md)；分发许可见 [第三方声明](THIRD_PARTY_NOTICES.md)。

## 7. 核心数据

| 数据 | 位置 / 所属模块 | 兼容原则 |
| --- | --- | --- |
| 处理链/时间组 | `FilterGraph`、`FilterTimeGroup` | 整秒起点分段；每组独立链，输出规格一致 |
| Renderer 项目 | `*.vsr.json` / `MainWindow` | `schemaVersion=1`；读取能力以代码为准，未知节点不保证无损往返 |
| VPY 预设 | 程序旁 `vpy/` / `PresetStore` | `# VSR_PRESET` Base64 JSON；时间组 metadata `schema=2`，兼容旧单链；不能与项目 schema 混淆 |
| Player 配置 | `player.ini` / `PlayerSettings` | 保留用户显式选择；单文件放启动器旁 |
| Renderer / 社区设置 | QSettings；`community/sourceUrl`、`appearance/dark`、`appearance/language` | 源/主题/语言在同一滚动页；主题即时生效 |
| 说明 / 命令 / AI | `vpy/preset-descriptions.json`；QSettings `export/commandHistory`；`player.ini` 的 `ai/package` | 说明只补空键；命令历史最多 50 条；模型路径指向外置清单 |
| 临时脚本/索引 | QStandardPaths、媒体索引缓存 | 可再生；导出冻结脚本、源和参数，不删除用户 VPY |
| 分发元数据 | `release.json`、资源/组件清单 | 版本、根目录和文件哈希须对应实际产物 |

## 8. 分层与高风险区

依赖方向：graph 不依赖 UI；backend 管帧、原生和进程；app/ui/player/cap 管交互。不要把算法直接塞入按钮回调。

2026-10-17 抽样：`MainWindow.cpp` 1193 行 / 60 KB，`src/cap/main.cpp` 888 行 / 59 KB，`PlayerWindow.cpp` 645 行 / 62 KB，旧 `ImageEditorWindow.cpp` 377 行 / 97 KB。行数与字节数只是职责审查信号；确需拆分时按交互状态、媒体操作、持久化和独立算法划分，不按行数机械拆分。

高风险包括 DLL/FFmpeg ABI、HDR 白位/色域/位深、seek 时钟和交接线程生命周期、超大图内存/ICC/Alpha、更新目录交换。涉及这些内容时需要实际媒体/GPU 像素或交互验证，编译成功与离屏布局不能代替实机结果。

## 9. 更新、测试与发布

1. 明确变更范围，复现问题；修改相关模块，构建并运行对应测试。测试选择与结果写入 [版本测试台账](development/validation.md)，步骤见 [构建](docs/dependencies.md) / [测量](docs/performance-debugging.md)。
2. Native 用 API20 固定脚本和补丁；历史 API14/18 脚本只用于对应基线。GUI 使用 CMake preset。
3. 当前 Full 固定运行目录 `dist/VS-GUI-Full`，单文件为 `dist/VS-Player.exe`；精简播放器仅暂存在 `build/playerlite-runtime`，不再生成 PlayerLite 发行目录。`tools/deploy-local-runtime.py` 增量部署并核对 INI/VPY/ICC/LUT 哈希；它会修改运行目录，文档整理时不运行。
4. 打包前明确 Full、历史 Lite 或当前 PlayerLite；`package-portable.ps1 -Edition Lite` 是历史 Full 删减版，不能充当当前 PlayerLite。单文件从构建暂存用 `package-playerlite-single.ps1` 生成；完整流程见 [便携更新](docs/portable-updates.md)。
5. 解压、依赖、启动、配置和清单验证后才可报告包完成；源码推送、草稿、附件上传、正式发布分别记录，需要相应任务授权。

文档纪律：结构/决策变化当天更新本地图；操作变化只改对应 docs 指南；用户可见变化写 changelog；测试按版本与轮次更新同一台账，不新建 `docs/update-版本.md`、`validation-版本.md`。原始日志留 `build/`，必要证据放 `development/evidence/`；不携带用户媒体。README、本地测试数据和开发记录不得混入上游 PR。

## 10. 当前状态与下一步

1.0.14 在固定 API20 主线补丁上迁入 AI 音频 tap 与字幕 HDR，Qt 接入 RTX VSR/TrueHDR、外置 AI 字幕、八档与组合处理、Renderer 主题设置、3FUI 命令桥及 Cap 设置改进。Player 设置新增独立字幕页；渲染页完整列出 Anime 档位，Jinc/D3D11 补帧分别保存参与列表；其他缩放算法限纯 D3D11 直通，Tab/底栏读取实际路径。ParameterEditor 跟随全局主题。PlayerControls 显示音量百分比和状态图标，均衡器由 PlayerAudio 创建与倍速互斥的 Popup，并在展开时保持控制栏可见。入口分别为 `PlayerAiSubtitles`、`PlayerProfiles`、`MainWindow`、`ExportWindow`、`src/cap/main.cpp`；插件源码在 `plugins/3fui-export-bridge`，单文件更新在 `SinglePlayerUpdater`。保留 VS 帧桥、BD 定位、音频效果和色彩桥；普通声床没有新增内置双耳渲染器。原理见[性能排查](docs/performance-debugging.md#上游内核优化)，逐项状态及实测见[台账](development/validation.md)。本轮更新本地产物并同步源码，单文件 `dist/VS-Player.exe` 同步到 1.0.14；准备 Release 草稿，发行压缩包与附件由维护者手动制作、上传。

1.0.13 增加安装进程清理、压缩运行时按需展开与单文件BD定位显示保持；列表/右键共用返回菜单入口，原生菜单页禁用时间轴，同盘重开复用盘信息，原生会话同步销毁后再替换窗口。VSCap续修将颜色窗口归属到真实工具栏窗口，避免贴图拖动模式拦截颜色交互；已发布的1.0.13附件不自动包含续修，重新分发须核对哈希。具体测试见台账。此前1.0.12 已有 BD 全卷定位/按段索引、Jinc 色彩、无音轨 VS 时钟、Anime 预取与软件内核的聚焦测试记录；**未运行全量 Full 播放器套件**。后轮结果只覆盖其实际测试范围，不能把同版多个 passed 数字相加为一次总回归。

当前仍未完成：

1. 在7940HX上的8K48 软件解码逐帧实时播放（本轮以丢帧追赶墙钟为目标，不视为逐帧实时通过；新一轮结果见测试台账）

2. 在7840HS/780M核显上的Anime4K CNN 24F 1080p--2160p（NO CNN模式已经通过）

3. 14700K P/E 调度分支

4. IAMF/OBR 真实空间音频听感

1.0.14 语言检查通过。新增验收边界：AI 未配置真实模型、3FUI 未操作真实宿主按钮、管理员计划任务未实际启用；i5-9500/1650 Super 无远程入口。BT.2020 CL 原生路径的既有断言失败在旧 DLL 同样复现，尚未关闭。逐项依据及跨版本变化见 [台账](development/validation.md)。

下一步按用户指定设备和素材复测上述未完成项。不要把 1.0.10/11 的旧失败继续写成 1.0.12 已修复路径的当前结论，也不要从工作区版本推断外部 Release 状态。

## 11. 故障排查

| 症状 | 先检查 |
| --- | --- |
| Qt 配置或构建失败 | `QT_ROOT`、MinGW PATH、CMake preset；[构建指南](docs/dependencies.md) |
| Native 黑屏/ABI 错误 | API20 补丁、FFmpeg shared DLL、`VSR_3FP_DLL`；[原生构建](docs/native-api20-build.md) |
| VS 缺插件/预览失败 | API4 运行时、模型和 `VSR_VSSCRIPT_DLL`；[Renderer](docs/renderer.md) |
| seek/音画时间不同步 | `FrameTimeline.h`、`PlayerSession`、请求代次；[性能排查](docs/performance-debugging.md) |
| HDR 色偏/截图异常 | `src/color/`、`HdrPreview.h`、源屏幕 SDR 白位；[色彩](docs/color-management.md) |
| VSCap 模态/文字/贴图异常 | `src/cap/main.cpp`；[截图](docs/capture-images.md) |
| BD 菜单或跨段失败 | `BlurayCatalog`、concat 定位、片段索引；[蓝光](docs/bluray.md) |
| 包缺 DLL/升级失败 | 分发形式、必需清单、包根目录与备份；[更新](docs/portable-updates.md) |
