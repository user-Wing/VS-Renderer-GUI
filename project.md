# VS Renderer / VS Player：项目开发地图

> 本文面向接手开发的维护者与 AI，负责说明架构、模块入口、约束与当前状态，不记录逐日开发流水账。用户指南见 [README](README.md)，版本变更见 [changelog](changelog.md)，专项证据见 [docs](docs/)。核对基线：2026-10-10，工作区版本 1.0.10（不等于正式 Release 已发布）。

## 1. 发心与边界

- **VS Renderer**：为需要可视化组织视频滤镜的用户提供处理链、VapourSynth（VPY）生成、源/输出实时对比和编码导出。
- **VS Player**：共用原生播放后端的独立视频、音频、图片查看器，支持原生直通或可选 VS 增强。
- **VSCap**：截图、绘制、文字标注、贴图及 PNG/AVIF 输出的独立组件。
- **成功标准**：实际滤镜与播放路径可验证，时间轴一致，便携包依赖齐全，故障可定位到模块，软件不因首次索引或后台操作失去响应。
- **非目标**：通用 Python IDE、任意 FFmpeg 命令设计器、私有 madVR/NGU 算法仿制，以及未经验证的全部设备/格式兼容承诺。

## 2. 快速开始

**平台**：Windows 10 22H2+ x64，D3D11；部分 GLSL、RIFE 需要合适的 GPU 驱动。完整便携包可运行 `VSRenderer.exe`、`vs-player.exe`；开发机还需 C++20、CMake、Ninja、Qt 6.8+、匹配 Qt 的 MinGW 和原生组件所用 MSVC 工具链。依赖准备见 [docs/dependencies.md](docs/dependencies.md)。

典型开发构建（按本机 Qt/MinGW 实际安装调整）：

```powershell
$env:QT_ROOT = "C:/Qt/6.10.2/mingw_64"
$env:PATH = "C:/Qt/Tools/mingw1310_64/bin;C:/Qt/Tools/Ninja;" + $env:PATH
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release
```

注意：① `FFF.Native.dll` 与 FFmpeg shared major ABI、补丁和运行时文件必须匹配；② 只复制 EXE 不是便携部署，必须随附 Qt、VS/Python 及所选后端；③ 冷启动 shader 编译、索引耗时不能当作稳定播放帧率。当前 native API 18 构建入口见 `tools/build-native-api18.ps1`；不要混用 API 14 历史补丁。

**本地部署约定**：Full 的固定运行目录为 `dist/VS-GUI-Full`，目录名不含版本号。编译完成后将本轮更新的产物部署到该目录，保留配置与用户数据，同步 `release.json` / `resource-manifest.json`；不能只更新 `build/` 就报告交付完成。版本号用于内部版本记录和最终压缩包名称（例如 `1.0.10.7z`），不重命名本地运行目录。`tools/stage-editions-1.0.9.py` 的 Full 输出使用此固定名称，仍拒绝覆盖非空目录；已有运行目录使用增量部署。

## 3. 架构决策

| 决策 | 理由与适用范围 | 不选的方案 / 重估条件 |
| --- | --- | --- |
| Qt Widgets + C++20 | Windows 桌面控件与原生 HWND/D3D11 容易整合 | 当前不引入 QML；确有跨平台或复杂场景图需求再重估 |
| MinGW GUI → 3FP 的动态 C ABI | 避免跨 MSVC/MinGW C++ ABI 耦合 | 不直接链接原生 C++ 对象；升级必须匹配 DLL/API 补丁 |
| FilterGraph → VPY → VSScript 帧服务 → 3FP | 链顺序清晰、脚本可审查、可以异步预览和定位帧 | 不把 vspipe/Y4M 离线流当实时帧桥 |
| Player 原生直通与 VS 路径分开 | D3D11/Jinc/网络媒体可不等待 VS 索引；增强链按需启动 | 不让普通播放无条件初始化全部 VS 依赖 |
| 专用时间轴映射与请求代次 | seek、补帧与源/输出 FPS 必须保持同步 | 不以 seek 后的局部帧号直接映射源帧 |
| 高清图像查看与编辑解耦 | 查看由 PlayerImage/PreviewPane；详细编辑由独立 PhotoCraft 原生进程 | 不再把旧 Qt 图片编辑器当当前产品入口；需兼容特殊格式时仍用本地解码/交换 |
| 专项补丁重建原生组件 | 3FP 非完全上游镜像，需可复现补丁链 | 不覆盖式拼接不同 API 版本的 DLL |

处理总览：Renderer UI → `src/graph` 滤镜图/VPY → `src/backend` VS 帧服务 → 3FP 呈现；Player 可走 VS、3FP 直通或专用第三方后端。高精度图片交换与 HDR 色彩路线详见 [PhotoCraft](docs/photocraft-editor-1.0.8.md)、[色彩管理](docs/color-management-1.0.4.md)。

## 4. 目录与改动入口

```text
src/app/       Renderer 主窗口、项目与预设协调
src/graph/     滤镜定义、图、VPY 生成、预设存储
src/backend/   VSScript 帧服务、原生 3FP 桥、导出/进程
src/ui/        预览、比较、分析页、参数和导出窗口
src/player/    Player 播放、设置、音频/字幕、图片工具
src/cap/       VSCap 截图、标注和贴图交互
src/bluray/    Blu-ray 目录、节目识别与 Remux 共用逻辑
src/color/     颜色处理与 native ABI 接口
src/image/     高精度图片文档/PSD/HDR 的本地实现
src/update/    更新、组件下载与校验
assets/        图标、语言、GLSL 等随包资源
patches/       3FP、PhotoCraft 等上游集成补丁
tools/         构建、部署、打包、性能测量与验证脚本
tests/         分模块的程序和回归测试
docs/          专题说明、验收及历史记录
third_party/   第三方源码、许可资料
.deps/         本地依赖/缓存；不是发布清单
build/         构建产物与临时报告
dist/          本地运行/打包结果（谨慎处理用户配置）
```

**按任务找文件**：滤镜查 `FilterCatalog.cpp` / `FilterGraph` / `VpyScriptBuilder.cpp`；预览与 seek 查 `VapourSynthFrameServer.cpp` / `FrameTimeline.h`；播放控制查 `PlayerWindow.cpp`、`PlayerControls.cpp`、`PlayerSession`；字幕/菜单查 `PlayerSubtitles`、`PlayerMenus.cpp`；图片查 `PlayerImageTools.cpp`、`PlayerImageResample.cpp`；VSCap 当前主要实现于 `src/cap/main.cpp`；原生构建查 `tools/build-native-api18.ps1`；打包查 `tools/package-portable.ps1`。

## 5. UI 地图

| 入口 | 职责与导航 | 文件定位 |
| --- | --- | --- |
| Renderer / VS 实时渲染 | 导入源 → 处理链/参数 → 源与处理后预览 → 预设或导出 | `src/app/MainWindow.cpp`、`src/ui/ParameterEditor.cpp`、`CompareView.cpp` |
| Renderer / 图像分析比对 | 多路源、时间偏移、并排/网格/滑块比较 | `AnalysisPage.cpp`、`MultiCompareView.cpp` |
| Renderer / 导出与设置 | 保存项目/VPY，准备文件、编码队列、配置 | `MainWindowPresets.cpp`、`ExportWindow.cpp` |
| Player | 媒体 → 播放/列表 → 字幕音频/渲染设置 → 图像工具 | `src/player/PlayerWindow.cpp`、`PlayerPlaylist.cpp`、`PlayerMenus.cpp`、`PlayerSettings.cpp` |
| PhotoCraft | Player 图片工具打开独立图层编辑工作区 | `src/player/PhotoCraftEditor.*`、`tools/build-photocraft.ps1` |
| VSCap | 选区缩放/移动 → 整图标注 → 裁切/复制/保存（取消保留窗口）；HDR 原图提前保存，剪贴板图像/复制的图片文件均可贴图 | `src/cap/main.cpp`，详见 [交互验收](docs/vscap-interaction-1.0.10.md) |

## 6. 外部依赖

| 依赖 | 版本 / 来源与调用 | 缺失行为 |
| --- | --- | --- |
| Qt | 6.8+，本机记录 6.10.2；Widgets/Gui/Network | 编译或 GUI 无法启动 |
| VapourSynth + Python | VS API4、便携运行时；`tools/build-vapoursynth.ps1` 与 stage 脚本 | VS 链不可用，原生直通可独立运行 |
| 3FP / FFF.Native | native API 18 补丁链；`tools/build-native-api18.ps1` | 原生播放/呈现不可用；不得混搭旧 ABI |
| FFmpeg | 匹配当前原生模块 shared ABI；CLI 在 `runtime/ffmpeg` | 相应解码、转换或导出功能失效 |
| VS 插件、GLSL、RIFE | API4 插件；`assets/mpv-shaders`、NCNN 模型和驱动 | 仅相应滤镜/模型不可用，不能以旧 API3 或 PKL 代替 |
| libVLC、LAV、madVR | Full 的可选后端/BD 菜单；各自运行时 | 对应模式不可用，正常基本播放不应被阻断 |
| PhotoCraft | 固定上游及补丁，`tools/build-photocraft.ps1` | 详细编辑不可用，基础图片查看独立 |
| 图片库、AWJ、aria2、7-Zip | `tools/*runtime*`、`stage-update-tools.ps1`；许可见 [声明](THIRD_PARTY_NOTICES.md) | 高级转换或下载更新降级，基础查看不应被阻断 |

Full 与 PlayerLite 的依赖清单不同，实际发布以打包脚本、运行时清单和 [1.0.10 验收](docs/validation-1.0.10.md) 为准。

## 7. 核心数据与兼容性

| 数据 | 位置 / 负责方 | 兼容原则 |
| --- | --- | --- |
| 滤镜图 | 内存 `FilterGraph/FilterNode` | 顺序决定执行，参数由 catalog 定义 |
| Renderer 工程 | `*.vsr.json` / MainWindow | `schemaVersion=1`；未知滤镜/参数跳过，不保证未知节点无损往返 |
| VPY 预设 | 便携目录 `vpy/` / `PresetStore` | `# VSR_PRESET` Base64 JSON；普通外部 VPY 可执行但不强加项目元数据 |
| Player 设置 | `player.ini` / PlayerSettings | 尊重既有用户配置，不因默认值变化重置 |
| Renderer 设置 | QSettings / Renderer | 与 Player INI 相互独立 |
| 临时脚本/缓存 | QStandardPaths 下 `preview.vpy`、媒体索引目录 | 可再生数据，不作为用户工程文件；导出需冻结源、脚本和参数 |
| 更新元数据 | `release.json`、版本与组件清单 | 版本、根目录、包内文件哈希与实际产物对应 |

## 8. 分层与高风险代码

依赖方向：`graph` 不依赖 UI；`backend` 负责帧/原生与进程接口；`app/ui/player/cap` 负责呈现交互。新增算法不应直接塞入按钮处理函数。

按 2026-10-10 文件大小抽样：`src/image/ImageEditorWindow.cpp` 约 97 KB，`src/player/PlayerWindow.cpp` 约 60 KB，`src/image/ImageDocument.cpp` 约 58 KB，`src/app/MainWindow.cpp` 约 56 KB，`src/image/ImageEditorTools.cpp` 约 48 KB，`src/cap/main.cpp` 约 43 KB。这些是职责集中**审查信号**，不是强制按行数拆文件的理由。若确需拆，按交互/状态、媒体操作、数据持久化和独立算法边界拆，避免跨层循环依赖；旧 Qt 图片编辑部分不应误当现行 PhotoCraft UI。

高风险：原生 DLL 与 FFmpeg ABI、HDR/PQ/HLG 到 SDR 映射、时间轴/seek、VSCap 原始像素精度、超大图内存/ICC/Alpha、导出停止与更新目录交换。GUI 测试通过不代表 D3D11/HDR 像素或高分辨率性能通过。

## 9. 更新、测试、构建和发布

1. 明确任务边界 → 修改最小模块 → 构建相关目标 → 跑相关 CTest/工作流 → 有媒体或像素要求时做真实输入验收；记录测试环境与未通过项。
2. Native API 18 用 `tools/build-native-api18.ps1` 与对应 patch；GUI 用 CMake preset。旧 `tools/build-3fp.ps1` 及历史版本脚本并存，调用前核对版本和补丁。
3. `tools/stage-portable.ps1` 用于本地部署；`tools/package-portable.ps1` 打包 Full/精简版，1.0.10 PlayerLite 以现行版专项发布脚本/说明核对。必须保留许可、包含正确运行时，排除个人 INI、缓存与日志。
4. 验证完整解压、启动、依赖/哈希、版本清单后再推送/发布；源码推送、Release Draft、附件上传是**不同步骤**，不能互相视为已完成。
5. **维护纪律**：架构/目录/关键决策发生变化，当天修改本文件对应节；具体实验和验收归 `docs/`，对外变更归 `changelog.md`，不要再向本文件顶部增加日期流水账。

工作区清理脚本 `tools/clean-workspace*.ps1` 与 `tools/clean-build-followup-20261011.ps1` 涉及本机产物；执行前先预览、核对保留清单，不能凭脚本存在就认定清理完成。

## 10. 当前状态与下一步

- **基线**：工作区 1.0.10；Renderer、Player、VSCap、PhotoCraft 接入。Native API 18、HDR→SDR 自动色彩映射与 Full/PlayerLite 边界见 [1.0.10 验收](docs/validation-1.0.10.md) 和 [发布说明](docs/release-1.0.10.md)。
- **近期成果**：VSCap 可编辑文字/绘图、原始尺寸贴图与工具栏切换；选区可缩放/移动，标注在整图上保留，最终导出才裁切。复制不落盘，HDR 默认目录与手动保存上次目录分开记录；工作流 14 项通过。PhotoCraft 为独立图片编辑入口；3FP 直通和 VPY 链分别处理播放与增强。各项只按相应文档注明的实测范围认可。
- **下一步优先**：基于真实素材复测 VSCap/Player HDR 像素与多屏缩放；核对 1.0.10 包与源代码一致性；继续确认大分辨率软解、780M 实时播放及专项未达标项，不将短时间稳定等同全场景通过。
- **设计限制**：PlayerLite 不等于 Full；VS GPU 滤镜仍可能有 CPU 平面传输；不存在通用零丢帧、完整任意 PSD/HDR/字幕/蓝光场景保证；未实现 NGU 私有算法。发布状态以 GitHub Release 及真实附件为准，不从工作区版本推断。

## 11. 故障排查导航

| 症状 | 首查 |
| --- | --- |
| Qt/CMake 配置失败 | `QT_ROOT`、工具链、`CMakePresets.json`、[依赖指南](docs/dependencies.md) |
| 原生播放器黑屏或 ABI 错误 | `FFF.Native.dll`、FFmpeg DLL、API 18 补丁、`VSR_3FP_DLL`、[验收](docs/validation-1.0.10.md) |
| VS 缺少 namespace、预览失败 | VS API4、`runtime/python`、插件 stage、`VSR_VSSCRIPT_DLL`、[帧桥](docs/realtime-bridge.md) |
| seek 后画面、声音或帧号不同步 | `PlayerSession`、`FrameTimeline.h`、VS 帧请求代次及 [启动时钟](docs/player-startup-clock-1.0.8.md) |
| HDR 色偏、截屏不准 | `src/color/`、HDR Readback patch、[色彩说明](docs/color-management-1.0.4.md)、[高精度截图](docs/player-high-precision-screenshots-1.0.7.md) |
| VSCap 工具栏/文字/贴图尺寸异常 | `src/cap/main.cpp`、[VSCap 验收](docs/vscap-interaction-1.0.10.md) |
| 图片编辑或超大图故障 | `PhotoCraftEditor.*`、`ImageDocument`、[接入说明](docs/photocraft-editor-1.0.8.md) |
| BD 首帧花屏或菜单/字幕问题 | `src/bluray/`、[拼接前置](docs/player-bd-concat-preroll-1.0.8.md)、[BD 验收](docs/bd-menu-viewport-1.0.6.md) |
| 安装包缺 DLL 或升级失败 | stage/package 脚本、包根目录、哈希、[便携更新](docs/portable-updates.md) |

历史测试结论只代表当时环境；要更改当前行为，先复现、再修复、最后留可重跑的验收记录。
