# VS Renderer 项目地图

## 1.0.3 更新包兼容修复（2026-10-02）

- ModelScope 文件大小和 SHA-256 与本地包一致；使用已发布1.0.2更新器源码复现：固定归档根目录被路径验证拒绝。
- package-portable.ps1 恢复带版本号的归档根目录；本地开发目录与安装目标路径保留。TestUpdates 新增真实发布归档对1.0.2根目录约束的专项检查。
- 重新生成 LZMA2 包及校验文件；旧包保留备份。修正版须由维护者替换 ModelScope/GitHub 附件，不自动上传二进制。

## README 文档整理（2026-10-02）

README 仅保留核心功能、启动与常用操作；历史改动、性能和验证记录由 changelog.md 及专项 docs 承载。文档更新同步 GitHub，不改程序或既有1.0.3归档。

## 1.0.3 打包与源码发布（2026-10-02）

- tools/package-portable.ps1：标准 7z LZMA2 / mx=9 / 256 MiB 字典，完整运行时及8个开发者内置预设，清理缓存/个人配置/测试文件，生成内部资源清单与归档 SHA-256。
- docs/release-1.0.3.md：汇总整轮 Player、Renderer、图片、音频、图标与 LAV 更新，说明已知边界和首次手动迁移要求。
- 用户授权推送源码、创建 GitHub Release 草稿；二进制和 ModelScope 更新源由用户手动上传。

## 图标接入（2026-10-02）

- 用户选定 Renderer R3、Player P2、视频 V1、图片 I3；稳定资源保存于 `assets/icons/`。Windows RC 嵌入 EXE，Qt 应用图标覆盖窗口 / 任务栏，ICO 保留10档32位尺寸。
- Player 分类注册 `VSPlayer.Video/Image/Audio`，视频 / 图片 `DefaultIcon` 与 `TypeOverlay` 分别引用 EXE 资源102 / 103；应用图标101。保留旧 `VSPlayer.Media` 打开命令。首次运行迁移已有本程序注册格式，不修改 Windows UserChoice，不替换缩略图处理程序。
- Release 构建、相关三项测试含初始化清理5/5通过；正式两EXE窗口图标句柄有效、启动 / WM_CLOSE退出0，构建与交付哈希一致。原有INI / VPY / 原生DLL20项哈希保持。记录 `docs/icons-integration.md`；只更新两个EXE及文档，未打包发布。

## 图标候选设计（2026-10-02，未嵌入）

- AI 生成 Renderer / Player 各三款原始图，以及视频 / 图片缩略图右下角标记；PNG 为1254×1254透明底。八款可选图标附10档32位ICO候选稿，结构与逐尺寸解码通过；两份异常透明试稿保留但不作为候选。
- 选图页、原图、提示词和交付说明位于 `assets/icon-concepts/2026-10-02/`。等待用户选型与改进；本轮只整理设计资源，未嵌入应用、未变更文件关联或正式程序。

## 1.0.3：播放器设置与手动 Anime（2026-10-02）

- PlayerSettings.cpp：左侧完整 INI 预设加载/保存，右侧取消/确定/应用，图片关联分组选择。
- PlayerAssociations.cpp：常用图片扩展名及当前用户打开方式注册。
- PlayerProfiles.cpp / PlayerWindow.cpp / PlayerMenus.cpp / PlayerInfo.cpp：生成六个固定 Anime VPY，手动版本禁止丢帧/高分辨率自动降档，Jinc/D3D11 固定算法，菜单及 Tab 信息标注。
- TestPlayer.cpp：对应专项 17/17，含实际丢帧不降档验证；保留原有 INI/VPY，不运行无关回归。详见 docs/player-manual-presets-1.0.3.md。

## 1.0.3：Renderer 三栏与内嵌设置（2026-10-02）

- `MainWindow.cpp/h`：默认窗口1800×900逻辑像素；处理链、参数、视频三栏。参数移出左侧，取消高度上限，处理链获得更多纵向空间；设置改为导航独立页面，保留应用保存、版本及检查更新。分析页仍按需初始化，设置先打开也不会错位。
- `ParameterEditor.cpp`：说明、标签和空白区白底，灰底只留在输入控件内。无源查看VPY的警告加宽，完整保留标题。
- `ExportWindow.cpp`：顶部栏目左边缘对齐页内控件；编码队列表头增加明显竖向分隔线。
- `tests/TestAnalysis.cpp`：五个相关专项含初始化/清理7/7通过，真实预设加载与合成导出回归通过；截图已检查。记录 `docs/renderer-panels-1.0.3.md`。
- 仅更新本地Renderer及文档，共享构建与便携EXE哈希一致，启动/正常关闭退出0；播放器/native/LAV/INI/VPY56项哈希保留。审计 `build/renderer-panels-delivery-audit.json`。

## 1.0.3：Renderer 布局与对比导出（2026-10-02）

- `MainWindow.cpp`：导航间距、同排输入、对比标题预热状态、视频工作区内三行控制区及中央播放组、滤镜库空间和参数滚动区；移除实验设置/重复VPY按钮。
- `ParameterEditor.*`：两列 grid，标签弹性换行、bool/数值/下拉右对齐，当前文本决定下拉宽度、弹出按最长选项显示；文件位置整行。
- `AnalysisExport.cpp` / `AnalysisPage.h`：限定 AB、ABC2+1、ABCD2×2，合成尺寸和编码约束均取最大源。
- `ExportWindow.*`：栏目选中样式；对比任务拒绝3FUI几何变换参数，普通单文件导出保留原机制。
- `tests/TestAnalysis.cpp`：只跑本轮四个专项，含初始化/清理6/6通过，实际VPY/MKV尺寸/像素及不支持模式提示验证。详见 `docs/renderer-ui-export-1.0.3.md`，交接记录 `build/renderer-handoff-result.md`。
- 本地只更新VSRenderer.exe及文档；共享构建哈希一致，启动/关闭退出0，播放器/原生DLL/INI/VPY17项哈希保留。审计 `build/renderer-ui-delivery-audit.json`。

## 1.0.3：应用设置与AVIF内容更正（2026-10-02）

后续受控分析：同尺寸安全平面输入的YUV444巨图正常解码（7996 ms），证实首次故障是编码端RGB扫描行32位偏移回绕；预测黑块边界69.399%，实际抽样69.525%。未改播放器/正式程序，证据见 `docs/avif-large-yuv444-analysis.md`。

- `PlayerSettings.cpp` / `PlayerWindow.*`：新增Apply，保留窗口、暂停/播放状态与本次媒体时间；图片应用不重新解码。
- `tools/stage-portable.ps1`：LAV默认改用 `.deps/lav/0.83`，由用户新版x64 ZIP解压；构建/便携21份根级文件哈希一致，三个AX为0.83.0，LAV播放/暂停/寻址专项通过，12份配置保留。
- `PlayerImage.cpp` / `tools/image-runtime.cmake`：AVIF显示CICP/GBR信息，静态编译libyuv SIMD转换，实际完整YUV420巨图23.675s降至6.380s。
- 首次生成的AVIF已被像素对比证实存在编码端32位扫描行偏移溢出，不能再用旧流程测试声称其内容完整。新的31604×65278 YUV420样本与原JPG整图/底部抽样对比通过，仅补一列边缘像素。
- Release、语言包312条、相关设置/GBR/grid功能6/6通过；仅处理本次专项。完整证据和TEST.avif边界见 `docs/player-apply-avif-1.0.3.md`。

## 1.0.3：图片性能与音频处理（2026-10-02）

- `PlayerImage.*` / `tools/image-runtime.cmake`：JPEG映射输入、libjpeg-turbo 3.2.0直接BGRA SIMD输出，使用当前MinGW+NASM编译，保留ICC/EXIF与原始像素；解码元数据用于Tab。真实巨图12.477s vs Qt23.262s，49个抽样像素一致。
- `PlayerInfo.cpp` / `PreviewPane.cpp`：完整图片输入/输出信息；图片独立缩放上限65536，先裁可见区域避免巨大绘制坐标，视频范围保持原值。尚无GPU图片解码/整图显存缓存。
- `PlayerAudio.cpp` / `PlayerMenus.cpp` / `ThreeFpApi.*` / `ThreeFpPlayer.*` / `patches/3fp-player-audio-effects.patch`：音轨、外部音频、同步、非模态十频段均衡器、PCM音量和WASAPI声道协商；gain参数更新不暂停视频。当前DLL增加可选ABI，原有ABI保持兼容。
- `PlayerMediaMatching.*` / `PlayerWindow.*`：同目录集数/名称匹配；视口音频上/下50%分区、字幕上40%/下60%分区，拖悬提示及独立主次外部字幕。新音频格式纳入现有列表。

只验证本次相关功能，未运行无关导出/整套回归。版本保持1.0.3，交付记录和完整边界见 `docs/player-images-audio-1.0.3.md`。

验证：Release与语言包检查通过，交付目录相关专项9/9通过；两EXE及原生DLL哈希匹配，12份INI/VPY哈希保留。

## 1.0.3：图片与菜单（2026-10-02）

- `PlayerImage.*`：单后台线程解码当前图片；Qt与FFmpeg覆盖常用图片，libavif/dav1d直接解码AVIF。Unicode路径通过QFile流式读取，grid不经VS或巨大临时PNG；保留高位深与透明通道，切图丢弃失效结果。
- `PlayerWindow.*` / `PlayerPlaylist.cpp` / `PreviewPane.*`：图片模式关闭播放时间轴、预解码和VS滤镜；同目录名称排序左右切图、滚轮缩放、拖动与还原。CPU图像绘制避免视频纹理边长限制；视频路径保持独立。
- `PlayerMenu.*` / `PlayerMenus.cpp`：各级右键菜单180 ms缓出展开、8 ms精确定时、圆角透明窗口；方框中心位于左缘与文字之间，布尔与单选都明确显示状态。
- `tools/image-runtime.cmake`：固定来源和SHA-256，静态编译libavif1.4.2与dav1d1.5.3；仅扩大上游显式像素上限的guard，保留格式/尺寸算术/网格/AV1验证。完整许可随包IMAGE-LICENSE.txt。
- 实际巨图与回归证据见 `docs/player-images-menu-1.0.3.md`。版本保持1.0.3。

历史验证：Release、图片/菜单流程及尺寸专项通过；AVIF内容验证不足，首次生成样本后来证实编码损坏，已在本页最新记录更正。本地程序及配置保留记录见 `docs/player-images-menu-1.0.3.md`。

## 1.0.3：Anime4K 六档（2026-10-02）

- `PlayerProfiles.cpp` / `PlayerSettings.cpp` / `PlayerInfo.cpp`：CNN+增强、CNN、no CNN+增强、no CNN、Jinc、D3D11 六档，起始档位进 INI；保留稳定播放 5% 门槛及操作恢复期。
- `assets/anime4k-no-cnn.glsl`：上游 Clamp / Thin_HQ / Darken_HQ 组合，无神经网络；嵌入程序并部署到 shaders，Renderer 下拉增加此模式。
- 内置 Anime 将增强 hooks 与 Anime4K 融合成一次 Shader；普通 Anime4K 节点也不再强制降成 420。已知旧默认内置片段定向迁移并备份，用户预设不整体覆盖。
- 10 位 / Y410 格式边界及真实 1080p48 短样本见 `docs/player-six-stage-1.0.3.md`。当前插件仍固定 16 位平面输入 / 输出；不把额外 CPU 降位深当成带宽优化。
- 交付固定 dist 目录：Player 36/0/7（通过/失败/条件跳过），八组最终验证完成（导出原代码单组复验）；两正式 EXE 启动/关闭和构建哈希、73 项资源、原有 INI、12 个其他预设及 Anime 原件备份核验通过。证据 `build/six-stage-delivery-audit.json`、`build/six-stage-smoke.json`。

2026-10-01 分辨率性能诊断：真实 1080p47.95 的 2304×1296 恰好位于 Fast 的严格 1.2 倍 CNN 放大分界；2368×1332 会执行额外 CNN 并先生成 4K 中间结果。增强链实际播放器短样本复现 3.1% → 35.1% VS 跳帧。全屏纯 A 的重复结果波动，调度 / 搬运 / 最终缩放需分段测量，未部署诊断中的 5 ms 定时器或 shader 控制。详见 `docs/player-resolution-threshold-1.0.3.md`。

## 1.0.3 本地开发：固定目录与 VVC 修复（2026-10-01）

活跃程序目录改为 `dist/VS-Renderer-GUI-windows-x64`，保留原有用户 VPY、缓存和配置；原 INI 已备份，仅迁移指向旧程序目录的路径。程序版本为 1.0.3，归档名称仍使用版本号。部署与打包脚本改为 `tools/stage-portable.ps1` / `tools/package-portable.ps1`，从 CMake 读取版本；新版更新器兼容固定根目录和历史版本根目录。

实际 VVC 样本为 1920×804、24 fps、10-bit、Opus。随包 FFMS2 返回 `Source: No video track found`，L-Smash 可以读取首帧。Player 对这一项特定的 FFMS2 源错误尝试 L-Smash，沿用软件自身目录的索引缓存，继续执行当前 VS 滤镜链；其他错误仍正常报告。选择 LAV 不会替换 VPY 的源插件。

同步修复了连续缩放重建 VS 链时丢失继续播放标记的问题；回归测试等待实际播放 / 暂停状态后再验证降载。旧 1.0.2 二进制可复现该失败，修复后增强 → A/Fast → 直通测试通过。

验证：Release 成功，八个 CTest 组最终均通过。最后一轮整套运行仅比较滑块 100 次拖动的 1500 ms 耗时断言超限；单独复测通过，未改该性能门槛。Player 31 项通过、0 失败，远程链接专项未启用而跳过；固定目录专项 Player 10/10、Updater 7/7，无跳过，包括真实 VVC 的五种配置、帧 37 / 61 精确定位、暂停 / 播放与预设切换。移出测试 EXE / Qt6Test.dll 后，两正式程序独立启动并正常关闭。13 个用户 VPY 哈希不变，INI 除程序路径迁移外全部原有设置保持，两 EXE / 原生 DLL 与构建一致，GPU / 补帧 helper 校验通过。证据：`build/vvc-*`；原 1.0.2 压缩包 SHA-256 不变。

本轮仅本地开发，不创建压缩包、不推送源码、不发布新版本。已发布 1.0.2 归档保持不变。

## 1.0.2 完整更新与发布（2026-10-01）

- `src/update/PortableUpdater.*`：两程序共享 ModelScope 数字版本检查、分页、Revision 固定下载、aria2-next 2.8.3、异步分块 SHA-256、归档路径 / 必需组件 / 内部版本验证。仅新版本出现安装入口，不降级。
- `tools/apply-update.ps1`：等待相关进程退出，同级完整目录交换与原目录备份，保留用户 INI / 自定义 VPY / 项目 / 截图，替换内置预设；占用 / 失败不强杀播放器、不把残缺文件覆盖原目录。
- `PlayerWindow.cpp` / `PlayerSettings.cpp` / `MainWindow.cpp`：缩放后的左上角还原按钮，恢复适配与平移；版本和检查更新入口。语言两包同步，检查扩展到更新 UI。
- `tools/stage-update-tools.ps1` / `tools/package-portable.ps1`：固定 SHA-256 的下载工具部署、许可和完整包 manifest；干净暂存目录排除用户配置、缓存、测试与历史脚本；使用本机 Zstandard Ultra 22，归档不污染活跃目录。
- 验证：Release、CTest 8/8；更新专项验证真实 aria2 下载 / 校验 / 解压、目录整体替换、INI / VPY 保留、旧备份和占用保护；缩放还原含全屏。日志 `build/release-update-*`。完整交付、归档及远程验证与历史测试分别记录。
- 最新发布授权：ModelScope-Manager 本机 SDK 上传 `ARXChem/Software-List/VS-GUI/1.0.2.7z`；GitHub 推送代码并创建无附件草稿，GitHub 包由用户手动上传。开发日志落盘 Vibe Coding Guide 的 `results/2026.10`，采用用户指定 Distill 格式。当前版本仍 1.0.2。

## 当前本地更新：1.0.2（2026-10-01，继续开发中）

- 当前网络机制：HTTP / HTTPS 固定 3FP 原生直通，不加载 VS；`PlayerNetworkInput.*` 删除整文件临时准备，保留 AList / 重定向 / 有界 Range 代理。修复原生元数据平均帧率字段读取，网络逐帧定位和 Tab 源帧率正确。
- `PlayerProfiles.cpp` / `patches/3fp-native-scaling.patch`：Anime / Realistic 增加 Jinc 后的 D3D11 Native；原生 VideoProcessor 支持的 SDR 硬解输入优先，其他输入 Bilinear 回退。降载恢复期 2 秒 + 稳定观察 5 秒，暂停 / seek / resize / 全屏 / 变速重新统计。修复内置 Resample 10-bit 输入，已有已知内置文件先备份再补转换。
- `PlayerCache.*`：便携或自定义 FFMS2 / L-SMASH 索引位置、身份哈希、大小和定向清理。`PlayerSettings.cpp`：八页及完整 INI；独立音 / 视频解码、主题、自动播放、媒体时间记忆、多线程与快捷键跨度。`PlayerAssociations.*`：当前用户候选应用注册，默认交给 Windows。
- `PlayerLanguage.*` / `assets/languages/*.json`：中文默认、英文外置包，新增字符串同步两包；`tools/check-player-languages.py` 加入 CTest。`LavPlayback.cpp` 支持独立 LAV 音频 / 视频图与两种配置属性页；不替换 VPY 内部源。
- 当前性能复测使用真实 1920×1080 / 47.95 fps 的 02.mkv；与上一轮实际 4K 的 03 文件区分。完整证据、交付与边界见 `docs/player-performance-1.0.2.md`。

- 本轮最终验证：Release / 原生 DLL、CTest 7/7，交付 Player 26/26 无跳过（两个真实链接）。证据 `build/player-settings-final-ctest.txt`、`build/player-settings-portable-results.txt`、`build/player-settings-delivery-audit.json`、`build/player-settings-startup-smoke.json`；用户 VPY / INI 保留，两个已知内置位深修复有完整备份。程序输出仍为本地 1.0.2，不发 Git、不打包。

### 前一轮记录（其网络 VS 临时准备机制已由上述直通替换）

- 本轮新路径：`PlayerNetworkInput.*` 独立 Qt 网络线程解析重定向 / AList API，代理分段转发 Range，HTTP/1.1 / 独立连接、限制读缓冲和待发送字节；VS 需要有进度的四路 Range 临时媒体准备；取消先于 VS / 字幕清理，失效脚本代次不再回调。`patches/3fp-network-subtitles.patch` 接入原生网络字幕并检查位图读取状态，Player 释放失败句柄，修复真实 ModelScope 闪退。
- `PlayerProfiles.cpp`：可编辑 Anime / Realistic、A/M Fast 资源、画布目标 / SAR、5% 自动逐级降载及原生 4K 直通。`PlayerPlaylist.cpp`：停靠 QSplitter / 全屏悬停 / 宽度保存；右键滚轮音量与缩放。
- 本轮最终验证：Release 成功、CTest 6/6、交付 Player 20/20 无跳过；两个真实链接原生播放与 ModelScope 完整 VPY 链通过。两个正式 EXE 独立启动 / 关闭、交付构建一致、GPU / 补帧 helper 与 Fast 资源校验通过，原有 VPY / INI 哈希未变。证据为 build/player-network-profile-* 与 build/player-real-network-http1.txt；AList VPY 完整 2.49 GiB 输入未另做长片测试。保持 1.0.2，不发 Git、不打包。
- 本轮性能证据：`build/player-network-profile-targeted.txt`（4K48 原生 383 帧 / 8s、0 丢弃），`build/player-supplied-links-final.txt`（两个真实链接 + 降级顺序），`build/anime4k-fast-profile.json`（VL / M / 目标尺寸配对）。完整交付验证见本轮更新日志及性能文档末尾。

- 本次继续更新：三个锐化节点的 YUV 路径迁至随附 vs-placebo Vulkan Shader，连续节点合并一次调用并保留原色度；Gray/RGB 保留 CPU。旧内置 CPU 代码在加载时精确匹配替换，不写回用户 VPY。`assets/gpu-sharpen.py` 为自包含预设 helper，嵌入两个 EXE。
- `PlayerPlaylist.cpp`：右侧范围悬停或停靠显示播放列表，目录第一级包含文件和可展开的子目录；支持右键无视频打开文件 / 文件夹 / 链接、拖放目录与 URL、file URI。Enter 双向无边框全屏，底栏离开鼠标 2 秒隐藏、移到下方显示，输入与弹窗期间保留。
- 3FP 原生 HTTP/HTTPS 输入 + FFMS2 网络源、内存索引，修复 ModelScope-Manager 直接传 URL 被误判为不存在；网络首次索引仍需扫描媒体。L-SMASH 网络输入转用 FFMS2，字幕暂停缓存避免重复生成 / 上传同一整幅覆盖图。
- 最新配对实测：同一 MyGO 完整链 48 帧、四路请求，CPU 13.01 fps → GPU 合并 25.76 fps（76.88 → 38.82 ms/帧）；为短时暖态样本。详情与测试记录见 docs/player-performance-1.0.2.md。
- 本次验证：CTest 6/6，交付目录 Player 15/15、无跳过；真实 2160p 输出在极致模式暂停后呈现计数不再增长，GPU 采样 0.3%。两个正式 EXE 独立启动 / 正常关闭，EXE 与 DLL 匹配构建，GPU helper 和完整补帧色度修复均在两程序内；全部用户 VPY 与 player.ini 哈希未变。证据：build/player-gpu-network-ctest.txt、build/player-gpu-network-portable-results.txt、build/player-gpu-network-delivery-audit.json、build/player-gpu-network-startup-smoke.json。

- 先修 Renderer 再更新 Player：Sharpen edges / Crispen edges 去掉未使用的第二次卷积；旧 VPY 不改写，本机另附同参数的优化预设，三帧逐像素一致。
- Player 的 Tab 使用真实 VS 跳帧计数及 GPU 引擎占用；资源采样在信息面板关闭时也继续，用于默认开启的异步预解码。CPU 控制线程，内存控制缓存目标，GPU / RAM / 进程显存预算超阈值则停止新增预取；无法强制插件瞬时占用不超限。
- `PlayerMenus.cpp`：三级右键菜单、VPY 分组、主次字幕、SRT 样式、源 / 显示 PNG、Jinc / 抗振铃和全屏入口。`PlayerSettings.cpp`：四页设置及便携 player.ini 导入导出。`PlayerResources.cpp`：PDH / DXGI / 系统与进程资源。
- `PlayerSubtitles.cpp`：独立字幕线程复用 3FP libass / 位图 C ABI。PGS 保留活动图像直到显式清屏或结束，复制并确认每个 native pending 事件，支持向前 / 后跳转。外部挂载在 VS 延迟初始化完成后恢复，媒体元数据不得覆盖挂载轨。
- `LavPlayback.cpp`：直接实例化 LAV / madVR COM，madVR 图使用 LAV 解码，VS 不启用；倍速音频仍由 3FP atempo 提供。madVR 的全局回调 DLL 保持进程内加载，重复打开复用模块；字幕使用 OSD，截图使用厂商帧接口，未注册系统。
- `patches/3fp-player-output.patch`：输出像素格式、同尺寸采样、Jinc relaxed 局部约束、扩展缩小算法、外部帧会话字幕及显示截图合成；叠加在速度补丁之后。本机在独立 .deps/fff-player 构建，补丁反向校验通过。
- 验证：Release / 原生 DLL 成功、CTest 6/6、Player 13/13（无跳过），真实 01.mkv 输出 3840×2160、Tab GPU 数字和跳帧、内置 PGS、外部 SRT、PNG、七种输出格式、madVR 实播 / OSD / 不变调倍速及 RIFE 精确帧回归。日志：build/player-final-ctest.txt、build/mingw-release/player-test-results.txt；性能详见 docs/player-performance-1.0.2.md。
- 交付保持 dist/VS-Renderer-GUI-1.0.2-windows-x64，保留用户 VPY / INI，更新两程序与匹配原生 DLL；交付目录 Player 13/13 通过，移出测试 EXE / Qt6Test.dll 后两主程序独立启动并正常关闭，三文件与构建 SHA-256 一致，完整补帧 helper 及色度修复仍在两 EXE 内。证据：build/player-oct01-delivery-audit.json、build/1.0.2-portable-player-results.txt、build/player-oct01-startup-smoke.json。不发 Git、不归档。HDR 显示器、多显示器 DPI 和长片持续播放仍未人工验证；HDR 实画面 PNG 目前为 8-bit 显示截取。

## 1. 发心

面向了解视频处理概念、但不愿反复编写脚本且需要快速 A/B 验证参数的用户。GUI 负责组织模块、顺序与参数，VapourSynth 负责处理，3FP 负责双路呈现和帧级检查。成功标准是用户能从“打开源”走到“生成并验证脚本、同步比较结果”，并把当前处理链交给熟悉的 3FUI FFmpeg 参数导出。非目标：首阶段不实现任意 Python IDE、矩阵表达式可视化编辑器、完整编码参数面板。

## 2. 快速开始

- 环境：Windows 10 22H2+、Qt 6.8+、CMake 3.25+、C++20；运行时需 API 14 补丁版 3FP 与项目固定的 VapourSynth。
- 配置：设置 `QT_ROOT` 后运行 `cmake --preset windows-mingw-release`。
- 构建：`cmake --build --preset windows-mingw-release`。
- 测试：`ctest --preset windows-mingw-release`。
- 已知坑：3FP 必须连同匹配的 FFmpeg DLL；MinGW 通过动态 C ABI 使用 MSVC 构建的 `FFF.Native.dll`；L-SMASH 首次打开媒体会建立 `.lwi` 索引，长视频可能等待数秒。

## 3. 架构决策

| 决策 | 理由 | Anti-choice | 重估条件 |
| --- | --- | --- | --- |
| Qt Widgets + C++20 | 原生 HWND、紧凑桌面布局、3FP C ABI 接入直接 | 不采用 QML：首阶段不需要动画/触控场景图 | UI 需要高度动画化或跨端 |
| 3FP 运行时动态加载 | 不绑定 MSVC/MinGW import library，缺失可诊断 | 不静态链接 fork：构建和许可分发成本过高 | 内核提供稳定 CMake 包 |
| 滤镜定义在 C++ catalog，图仅存 ID+参数 | MVP 类型安全且容易测试 | 不先造通用 DSL/插件系统 | 外部作者需要无编译扩展 catalog |
| 生成纯 VPY，由专用线程动态调用 VSScript | 脚本透明；随机帧请求无需重启管道；GUI 不阻塞在 VS 计算 | 不用 vspipe/Y4M 作为实时桥：seek 和积压控制较差 | VS 提供稳定的跨进程随机帧协议 |
| 左路 3FP 为唯一时钟，按源/输出 clip 时间轴换算绝对帧 | 3FP 时间 seek 后的 `frameIndex` 是局部 PTS 索引，不能代表源绝对帧；按呈现时间和两端 FPS 映射也兼容倍帧/抽帧 | 不让两个播放器各自计时，不直接用 3FP 局部帧号驱动 VS | 需要完整 VFR timecode 映射或独立处理后音频预听 |
| Anime4K 通过 `vs-placebo` 执行 GLSL | 官方插件能直接执行 mpv/libplacebo shader，生成 VPY 可审阅且不需要扩展 3FP 私有 shader ABI | 不把 GLSL 名称伪映射成普通 VS resize，也不重写 shader | 需要零 CPU copy 的 GPU texture 互操作 |
| 3FUI 导出使用“源输入 0 + VS Y4M 输入 1” | 保留源音频/字幕/章节映射，同时保证视频必然来自当前 VS 处理链 | 不解析和重建完整 FFmpeg AST；复杂图明确拒绝 | 需要支持多视频输入或 `filter_complex` 图 |
| A/B 滑块通过不重叠的原生父窗口裁剪现有两个 pane | 不增加第三路解码、帧请求或 GPU 处理 | 不在 CPU 合成两张帧图 | 需要离屏录制比较画面 |

## 4. 目录地图

```text
src/app/       主窗口与应用样式
src/backend/   3FP 动态 C ABI、双路播放协调、运行时探测
src/graph/     滤镜 catalog、图状态、VPY 生成
src/ui/        预览表面、可折叠侧栏与参数编辑器
tests/         无 UI 的图与脚本生成测试
docs/          UI 规格、滤镜覆盖和后端桥设计
tools/         依赖/构建诊断脚本
patches/       可应用到 FFF Project 的 3FP C ABI 扩展
third_party/   官方 VapourSynth Git 工作树；其他二进制不入库
```

## 5. UI 地图

主工作台：顶部命令栏 → 左侧三段可折叠 dashboard → 中央源/处理后双路视图 → 底部共享时间轴与播放控制。入口为 `src/app/MainWindow.cpp`，尺寸和遮挡约束见 `docs/ui-layout-spec.md`。

## 6. 外部依赖

| 依赖 | 版本/来源 | 调用协议 | 缺失时降级 |
| --- | --- | --- | --- |
| Qt | 6.8+；本机 6.10.2 | Widgets/Win32 HWND | 无法配置构建 |
| VapourSynth | 官方 Git `master`；当前 `5b2d556` / R80RC2 | `QLibrary` 加载 VSScript 4.4，VS API 4.3 按帧读取 | 仍可编辑/导出脚本 |
| 3FP / FFF.Native | FFF Project + `patches/3fp-vsrenderer-extensions.patch` | API 14 `FFF3FP_SubmitExternalVideoFrame` | 显示诊断占位，不崩溃 |
| FFmpeg | 与 3FP ABI 匹配的 shared build | 由 3FP 内部加载；桥接进程使用 CLI | 禁用播放/桥接 |
| VS 插件 | LSMASHSource、FFMS2、fmtc、RemoveGrain、AddGrain、VSZip、nlm-ispc、CAS、Zsmooth、Deblock、ZNEDI3、EEDI3、SangNom、Bwdif、VIVTC、vs-placebo 2.0.4 | 随便携运行时部署的 API 4 插件 | 启动自检失败并阻止预览 |

## 7. 核心数据

`FilterGraph` 保存有序 `FilterNode{id, enabled, parameters}`。参数值使用 Qt 基础类型，写入项目 JSON 时保留 catalog schema 版本。生成脚本不覆盖用户文件，预览脚本写入应用缓存目录；用户显式“导出 VPY”才写目标路径。迁移规则：未知节点原样保留但禁用，参数新增取 catalog 默认值，删除参数不输出。

## 8. 代码拆解

依赖方向固定为 `ui/app -> graph/backend`，`graph` 不依赖 UI，`backend` 不依赖 graph。单文件接近 500 行时检查职责；3FP ABI 声明虽可能偏长，但只承担协议镜像，不按行数硬拆。高风险区是实时帧桥、3FP ABI 版本变化和 seek 重建，不与 MainWindow 混写。

## 9. 更新与发布

使用 CMake Presets 构建，测试先行覆盖 VPY 转义、顺序与参数边界。结构、依赖 commit 或关键决策变更当天同步本文件。发布前运行 CTest、启动冒烟、`windeployqt`，再打包与 3FP 对应的 FFmpeg DLL；不得混用其他 major ABI。

## 10. 当前状态

- 版本：`1.0.3`。
- 已完成：项目基线、UI 规格、VapourSynth Git 源码固定、内置便携 Python/VS/API 4 插件、滤镜图/VPY 生成、教程向插件扩展、Anime4K GLSL、API 14 外部帧桥、左右同帧预览、A/B 滑块、3FUI 参数兼容导出、源像素密度感知缩放、缩放拖拽与全局空格播放。
- 下一步：扩充 VCB 全量 catalog、插件管理与 NGU 等独立高级缩放后端。
- 限制：当前帧桥支持常用整数 planar 格式，不支持 VS float/GPU-resident frame；Anime4K 在 VS 内使用 GPU，但输出仍经 CPU plane copy 提交给 3FP，4K 重 shader 未必能实时；NGU 属于独立 GPU 推理实现；处理后音频未接入，左路始终是音频时钟；3FUI 兼容导出拒绝 `filter_complex`/`lavfi`，仅自动重写常规视频 `-map` 与单路 `-filter:v`。

## 11. 故障排查

| 症状 | 原因 / 处理 |
| --- | --- |
| CMake 找不到 Qt | 设置 `CMAKE_PREFIX_PATH` 或使用仓库 preset 中的本机 Qt 路径 |
| UI 启动但预览为诊断页 | 将补丁版 `FFF.Native.dll` 与匹配 FFmpeg DLL 放到 exe 旁，或设置 `VSR_3FP_DLL` |
| `Create` 返回失败 | 检查 3FP API 版本与 FFmpeg DLL major，查看状态栏的原生错误 |
| `VapourSynth 未加载` | 当前程序包不完整；确认 exe 旁存在完整 `runtime/python`，开发构建可重跑 `tools/build-vapoursynth.ps1` |
| VPY 执行缺插件 | 当前程序包不完整或混入了旧 API 3 插件；重新 staging 内置运行时，不要求终端用户手动安装 |
| 3FP 报缺少外部帧 ABI | 对 FFF Project 应用仓库 patch 并重建；程序要求 API 14 |
| 处理顺序不对 | 在“处理链”中拖动节点；脚本严格按列表自上而下生成 |
| Anime4K 节点报 shader/namespace 错误 | 确认 `.glsl` 路径存在，并确认便携运行时含 `plugins/libvs_placebo.dll`；缩放 shader 必须给宽高，GUI 的“输出倍率”会同时计算两者 |

## 12. 1.0.1 更新记录（2026-09-26）

- 变更：新增 Anime4K GLSL 文件参数与 `placebo.Shader` 映射，构建/部署固定 `vs-placebo==2.0.4`；新增窗口拖放与源首帧预取。
- 变更：ZNEDI3/EEDI3 默认改为不改变尺寸的双倍帧率模式；纵向 2× 和 SangNom `dh` 自动同步放大宽度，维持原始宽高比。
- 变更：每次载入新 VPY 前重建右侧 3FP 会话，`DeviceFailure` 时再进行一次设备重建并重提当前帧；VS 请求改为一次只处理一帧，完成后跳到最新目标。
- 变更：缩放倍率与 VS 状态使用独立标签；native surface resize 后延迟重绘，父预览区也接收滚轮。
- 变更：新增 A/B 滑块比较；最大化时合并 resize 重绘请求并按当前算法/倍率重绘两路 surface。
- 变更：新增 3FUI 参数兼容导出，`vspipe` 使用 R80 `--container y4m` 与多帧请求，源媒体保留为输入 0，VS 视频改接输入 1；发布包隔离携带 FFmpeg CLI。
- 变更：首帧把 VS 线程数提升至全部逻辑核心并预取 6 帧，稳态仍保持单目标请求和落后帧跳过。
- 验证：MinGW Release 构建成功；CTest 4/4 通过；真实 `vspipe → FFmpeg` 双进程管道产出带源音频的 FFV1 MKV；真实 `anime4k-v4-a.glsl` 以 `placebo.Shader` 输出 30 帧成功；不同尺寸/格式处理链重建提交测试通过。
- 风险：本轮环境未开放原生 Windows GUI 自动控制，拖放与最大化手势仅有 Qt 事件级覆盖，仍需人工做一次可见交互确认；Anime4K 实时帧率取决于实际分辨率、shader 与 GPU。

## 13. 导出队列与原生预览修复（2026-09-26）

- 文件：`src/ui/ExportWindow.*`、`src/backend/ExportPipeline.*`、`src/app/MainWindow.*`、`src/ui/CompareView.*`、`src/ui/PreviewPane.cpp`、`patches/3fp-resize-flags.patch`、`tools/build-3fp.ps1`、`CMakeLists.txt`、相关测试与 README。
- 导出：独立、非模态且无主窗口 owner 的窗口；顶部设置/队列页签；任务顺序执行、独立脚本快照、FFmpeg 机器进度、处理帧数、估算剩余时间、停止。只允许 MKV 并固定 Matroska 封装；拒绝输出覆盖源视频或重复占用队列输出路径。
- 停止：关闭 vspipe 产生 EOF，让 FFmpeg 排空并封装尾部，保留输出；等待两进程退出才开始下一任务。关闭导出窗口仅隐藏；退出主界面先停止队列并等待活动任务收尾。
- 预览：移除 AB 重叠 QWidget 遮罩，改为不重叠 HWND 父容器裁剪；视频 surface 禁止 Qt backing-store 背景绘制。3FP 三处 ResizeBuffers 保留 ALLOW_TEARING，与创建标志一致；构建脚本应用额外补丁并重新编译 DLL。
- 验证：Release GUI 与原生 DLL 构建通过；CTest 4/4。真实 FFmpeg/vspipe 完成与中途停止 MKV 均可完整解码，队列双任务、取消待执行任务、导出窗口隐藏/重开通过。真实 3FP 双 surface 测试覆盖 AB 切换与多位置分界、三组尺寸、滚轮缩放、全屏往返；旧发布 DLL 在同一测试停止呈现，修复 DLL 通过。测试记录位于 `build/export-verification.txt`、`build/resize-old-runtime.txt`、`build/resize-new-runtime.txt`。
- 风险：进度时间依据源时长估算，显式裁切/变速参数会影响估算准确度；停止前零帧或编码器/磁盘故障不能保证可播放内容。原生测试使用 VS 合成帧和程序化事件，真实媒体长时间播放、实际手势与多显示器 DPI 切换仍需人工确认。

## 14. 启动预热与批量编码队列（2026-09-26）

- 文件：新增 `src/backend/StartupWarmup.*`、`assets/startup.qrc`、自生成的 `assets/startup-warmup.mkv`、`src/ui/PreparedFiles.*`；修改 `MainWindow.*`、`ExportWindow.*`、`ExportPipeline.*`、CMake、帧桥/导出测试与 README。
- 启动：内置 128×72 H.264/AAC 黑场短片提前走通原生解码、LSMASH/FFMS2 源滤镜、VS 取帧与两路实际 GPU 呈现。预热保持原生会话供首次导入复用，首个 VS 预览也复用已预热的输出设备。启动中拖入的最新文件排到预热完成后打开；预热媒体不写入用户源路径或处理链。临时文件生命周期覆盖解码器与 VS 节点。
- 导出：三个顶部页签分别为导出设置、准备文件、编码队列；单文件导出只针对主界面当前源，按源去重。批量拖入/文件选择支持去重、移除、排序；加入时逐源生成相同设置的独立脚本快照，不依赖主界面后续修改。
- 列表：队列使用可拖动的平面列表，列顺序/列宽可调整；任务按稳定 ID 跟踪，执行顺序取列表当前顺序。显示文件名、状态、百分比、已处理帧数、预计剩余时间，完整路径位于提示中。
- 控制：暂停时挂起本程序启动的 FFmpeg/vspipe 子进程并挂起后续调度，恢复后继续，ETA 排除暂停时间。停止时先恢复进程再通过 EOF 收尾，保留 MKV；允许删除尚未开始的任务、让停止/失败任务使用原快照从零重做。FFmpeg 因 shortest/帧数限制正常退出时，不再把供帧进程的被动终止误报为失败。
- 命名：原始目录/浏览指定目录；默认毫秒时间戳，额外序号消除同名输出。不加时间戳标注高风险；阻止覆盖批次输入、重复源、与已有队列输入/输出冲突。VS 输出 clip 提供时长，批量任务无需提前载入主预览也可估算进度。
- 验证：Release 构建成功。完整原生帧桥测试 10/10，导出测试 8/8，图/VPY 与缩放交互测试通过。真实进程验证暂停无增长、恢复继续、暂停后停止保留可解码 MKV，以及停止后从头输出完整 120 帧；验证批量拖放、去重、两种命名、禁止覆盖输入、顺序调整、删除等待任务、连续执行与单文件重复点击。按应用样式查看了队列/准备页截图。
- 性能记录：当前机器测试短片启动预热 14.741 秒，其后首次加载 0.555 秒（单次合成短片测试，不代表任意媒体时延）。日志：`build/startup-frame-verification.txt`、`build/batch-queue-verification.txt`。
- 风险：预热将一次性初始化移到启动期间，不能消除具体源文件的磁盘读取/索引或未使用过的滤镜编译开销；暂停不会释放已占用的解码/编码资源。任意真实媒体、大批量长时间运行与实际鼠标拖动仍需用户场景验证；测试覆盖 Qt 拖放事件和列表顺序调整。


## 15. 页面导航与双路直接分析（2026-09-26）

- 文件：新增 `src/ui/AnalysisPage.*`、`tests/TestAnalysis.cpp`；修改 `MainWindow.*`、`CompareView.*`、CMake 与 README。
- 导航：顶部最左侧三横按钮控制图标栏/文字栏，现有控件顺延；两个页面分别保留 VS 实时渲染与图像分析比对，分析页按需创建，切页暂停离开的播放器。VS 专用工具在分析页隐藏。
- 分析：两路 MKV 原生解码，不经 VS；按钮和拖放导入，复用 CompareView 的并排/AB 原生裁剪及 PreviewPane 缩放平移、像素取色，保留底部模式、全局播放/逐帧和放大算法布局。
- 时间：每路进度条显示本地时间，拖动同步定位两路并保留偏移；±1 帧/±1 秒单独对齐。异步定位串行并合并最新拖动目标；逐帧读取实际解码后时间，不把 seek 后的局部 frameIndex 当作绝对帧号。A 提供音频与播放时钟，B 静音，超过 80ms 的漂移最多每 500ms 校正一次；一路结束暂停全部。
- 验证：真实 24fps/30fps MKV 测试覆盖首帧、正反逐帧对齐、秒级偏移、连续定位、全局逐帧、从 B 进度条定位、播放暂停、片尾、AB、窗口缩放、全屏往返、原生句柄保持；导航测试覆盖折叠/展开及页面切换，保存应用样式截图。新增分析测试 4/4 通过，原有四组 CTest 全部通过。结果记录见 `build/analysis-verification.txt`。
- 限制：不同帧率按时间选择最近可解码帧，两路独立播放时钟不保证音频采样级或硬件锁步同步；长片/VFR、多显示器 DPI 和实际拖放手势尚需用户场景验证。本轮只更新便携目录程序，不生成压缩包。


## 16. 启动性能、多路分析与开源增强（2026-09-26）

- 文件：`VapourSynthFrameServer.*`、`StartupWarmup.*`、`MainWindow.cpp`、`ThreeFpPlayer.*`、`ThreeFpApi.h`、`AnalysisPage.*`、新增 `MultiCompareView.*`、`PreviewPane.*`、滤镜 catalog/VPY builder、相关测试；新增 `patches/3fp-performance-chroma.patch`、`third_party/shaders`，更新构建/打包脚本和文档。
- 性能根因：VS 构造函数通过 BlockingQueuedConnection 等待 Python/API 初始化；每个原生渲染实例重复编译内置 D3D 着色器；GUI 播放状态读取 OutputBitDepth 等待设备锁；VS 外部首帧同步提交可能再次编译。改为异步初始化和分阶段进度，首帧编译先在原生工作线程进行；状态读取/首次重绘不等待编译锁。进程内共享字节码，并按源码哈希从随程序部署的 shader-cache 加载；缺缓存时编译后保存。VS 待处理用户导入仍在预热结束后打开，其他界面功能可用。
- 性能证据：带缓存的新进程预热 392ms，10ms 界面心跳最大间隔 38ms，随后首次导入 110ms；合成 3840×2160 H.264 MP4 首帧 283ms。无缓存的构建预编译耗时 18.398s、最大 UI 间隔 88ms，因此部署必须包含 shader-cache，不能只复制 EXE。时间是本机单次测试，不代表任意真实素材。
- 多路：最多 9 个视频，主流容器交给 FFmpeg/3FP 解码；MKV 限制只保留在导出。默认按数量选择 AB/ABC/ABCD/三列网格；ABC 横排/竖排/左大/右大，ABCD 2×2；2+1 与 2×2 提供中心整体拖动，分割线亦可单独拖。每个视频保持自身原生 HWND，不因槽位交换重建解码器。
- 对齐：文件状态与槽位排列分离，选择已占用源自动交换。无文件时隐藏底部控件；轨道数随模式变化且最多 4 行，全部 9 个视频偏移独立保存。当前显示的源和选定音频源参与全局时间定位，其他源暂停，重新显示时按保留的偏移定位。音频可选任意源/静音；底部加入 VRR 与 Pacing。
- 平移：根据真实视频适配后的显示尺寸计算超出视口的宽高，不再用含黑边的 QWidget 全高估算垂直移动范围。测试确认两个方向相同鼠标位移对应相同画面像素位移。
- 渲染：独立色度 Point/Linear/Catmull-Rom/Softcubic/Mitchell/Lanczos3/Spline36/Jinc2/Bilateral/亮度引导双边重建/Super-XBR 单阶段；色度与亮度缩放各自选择，Cubic、Spline、Reconstruction 使用子菜单。放大新增 Spline36/Super-XBR 单阶段。Super-XBR 是 MIT 开源 pass-0 对角核移植，完整三阶段与 NGU 未实现、不冒用其名称；UI 和 README 明确说明。项目专用 SetScalingAlgorithms 参数低字节为原缩放枚举，上字节为色度核 + 1，需配套本轮 DLL。
- VS：已有 deband/deblock/NLMeans 按对应英文效果归入去伪影类，新增 dering、sharpen_edges、crispen_edges、thin_edges、enhance_detail；使用 VS 原语进行范围限制、卷积与形态学操作，保留可调强度，不宣称与 madVR 私有实现完全一致。
- 验证：真实 MP4 九路导入、四行上限、交换、隐藏偏移、ABC 交点/ABCD 几何与不同帧率双路同步通过；全部八类 VS 增强组成脚本并成功取帧；11 个色度核与新增放大核实际呈现，Nearest/Bilinear 像素不同；异步构造/预热心跳、2160p 首帧计时通过。日志见 `build/multi-analysis-verification.txt`、`build/chroma-verification.txt`、`build/warmup-cached.txt`。
- 最终回归：Release 与原生 DLL 构建通过，CTest 5/5 全部通过（25.69s），包含原有导出、帧桥、缩放/拖拽与新多路分析。
- 风险：9 路 4K 同时播放的吞吐取决于硬件解码会话、显存与 GPU；本轮九路素材为小尺寸合成片，未验证九路 4K 持续播放、所有编码或 VFR 长片。Qt grab 截图仅用于检查控件布局，D3D 视频像素由原生 probe 与呈现计数验证。使用本轮 EXE、DLL 和字节码缓存一起部署；不生成压缩包。


## 17. 修正 AB/ABC/ABCD 滑块语义与拖动（2026-09-26）

- 文件：`MultiCompareView.*`、`PreviewPane.*`、`AnalysisPage.cpp`、`TestAnalysis.cpp`、README。
- 根因：多路布局在每次 MouseMove 中隐藏再显示所有拖柄，使 Windows/Qt 释放正在拖动的鼠标捕获；ABC/ABCD 错把裁切拼接实现成了可变大小的完整视频分格。
- 行为：常规三路是等大 3×1 完整视频，常规四路是等大 2×2 完整视频；ABC/ABCD 滑块使用完整画布上的同源坐标裁切。三段、半幅加两个四分之一区域、四象限分别由对应视频的局部组成；交点控制两个方向，单线可独立拖。支持 ABC 镜像与纵向三段。
- 渲染：滑块各路保持同一尺寸的视频 HWND、同一个画布原点，仅改变非重叠原生父容器裁剪和子窗口偏移；隐藏 pane 标题，源选择器固定在画布上方。复用原有缩放渲染，不增加 CPU 拼图、逐帧请求或解码器。进入滑块时统一到第一格的缩放/平移，强制同步，离开后恢复同步选项。
- 拖动：保留手柄可见状态并显式保持鼠标捕获，不在移动中 Hide/Show；拖柄和视频表面均保持原生句柄，拖动不会产生视频 Resize 事件。
- 验证：六种裁切布局各连续拖动 100 次，鼠标捕获无丢失、视频表面零 Resize、共同原点不变、裁切区互不重叠且完整覆盖画布。两路真实 2160p H.264 合成视频下 100 次拖动合计 303ms，无隐藏、无尺寸变化、帧时间戳不变；这不是所有机器或高负载播放下的帧率保证。日志 `build/wipe-geometry.txt`、`build/wipe-2160p.txt`。
- 回归：Release 构建成功；CTest 5/5 通过（31.62s），覆盖原有导出、VS 帧桥、平移缩放和多路分析。
- 风险：多显示器 DPI 和真实长片播放期间的实际手势仍需用户场景确认。本轮只更新程序与说明，不打压缩包。


## 18. 视频卸载、取色提示与 1.0.1 交付（2026-09-26）

- 本轮：紧凑源选择器及 × 卸载按钮；卸载释放目标解码器，重排剩余会话与 pane，保留原生句柄和各自偏移，支持继续导入。回调按 pane 查找当前索引，避免移除中间源后访问旧编号。
- RGB 根因：提示固定在完整画布左下，被其他象限的父窗口裁剪。改为按所有祖先裁切范围计算可见区域，在其左下显示。
- 颜色处理加宽至至少 240 逻辑像素，当前算法完整悬停提示、菜单互斥标记；两页 VRR 控件增加边框和选中状态。
- 验证：真实九路导入后卸载、偏移和 HWND 保留、重导入、全部移除及恢复；ABCD 四区域真实像素探针和提示几何检查通过。全套 CTest 五组通过。Qt 截图仅核查控件，不包含原生视频画面。
- 交付：根目录 changelog.md 记录本轮及版本累计更新，docs/release-1.0.1-draft.md 保存本地发布说明。代码推送 GitHub，生成含 shader-cache 的 7z 和校验文件，不创建或上传 GitHub Release。


## 19. 对比画布导出、参数布局与 RIFE（2026-09-30）

- 文件：AnalysisExport.cpp 与内嵌 comparison-export.py 承担画布快照和合成，AnalysisPage 保留播放职责；ExportWindow 复用现有队列，保护快照所有输入，音频输入应用负偏移/静音。按归一化区域、真实视频视口、缩放和平移计算裁切，RGB float 拼图后输出 YUV444P16。分段堆叠代替九张满尺寸空白画布。
- 语义：最大源按像素总数选原始宽高；所有导入源参与尺寸选择，仅可见源合成。全局零时刻至可见源/音频最早结束，负偏移前段保持首帧。静态布局快照；VFR 按源时间基近似。Jinc/Super-XBR/双边色度拒绝不一致导出，不静默降级；实时 D3D 与 VS 非逐像素复刻。
- UI：自适配文件名宽度、最小 160px 进度条、8px 间距；底部模式/音频加宽，颜色处理采用标准组合框。SVG 勾号及显式复选框。设置按钮移动源滤镜选择并保存。根因修复：侧栏裸 border-right QSS 继承到子控件，限定为 #processingSidebar 后消失；参数使用行间距与长标签换行、展开菜单宽度自适配。
- 滤镜：新增 14 个节点，Zsmooth 0.20.0 / VSZip 22.1.0 已有 API 4 算法直接复用，Descale r11 与 RIFE r9_mod_v33 实际部署。批量下载发现旧 DFTTest/CTMF/AWarp/Retinex 等为 API 3，未加入便携 staging。build-vapoursynth 调用 install-analysis-plugins，RIFE 模型来自用户现有转换目录，SHA-256 清单与许可证在 third_party/rife。最终用户无需单独安装，源码构建需提供已验证模型目录。
- RIFE：PKL 是 PyTorch 权重，NCNN 使用 param/bin。本机两个 NCNN 模型实际插值与 2× 帧数通过，AMD 610M 小图验证成功；RTX 4070 Laptop 单线程 1080p 暖态 12.88 / 10.97 张插值帧每秒，不能保证 24→48 实时。VS 高于源 FPS 时提升状态轮询，验证真实 GUI 提交奇数插值帧；CPU plane copy 和 GPU 吞吐限制仍存在。
- 验证：所有 AB/ABC/ABCD 子模式及九路网格在缩放/平移后实际取帧；四颜色 MKV 实际编码并逐象限检查、最大尺寸包含隐藏源，长名称与最小进度条、禁止覆盖非主源、设置入口验证。14 新滤镜和两个 RIFE 模型实际输出。

- 最终回归：Release 构建，CTest 5/5 通过；RIFE 实际主页面 700ms 测试提交 29 帧，其中 13 帧为补出来的奇数中间帧。日志 build/rife-preview.txt、build/new-filters.txt、build/composition-final.txt、build/rife-1080p.txt、build/rife-amd.txt。

## 20. MVTools 与补帧计算优化（2026-09-30）

- 文件：FilterCatalog.cpp、VpyScriptBuilder.cpp、TestVpyScript.cpp、build-vapoursynth.ps1、stage-vapoursynth-runtime.ps1、verify-interpolation.py 和本轮文档。播放器/主窗口/3FP ABI 不在本轮编辑范围；保留另一任务已存在的 Anime4K 和播放器改动。
- MVTools：API 4 v29 wheel，VSRepo v24 已验证为旧 API 3 并排除部署。Super/双向 Analyse/FlowInter 50% 构成固定节点，仅按需生成中间帧，与输入交错；镜头默认保持而非混合，最后一帧保持。小图填充至有效块网格，再裁回，以避免原生插件小尺寸堆损坏。
- RIFE：上游本来只推理非原始时间点，不声称新增 50% 推理节省；新增原格式原帧支路，避免偶数帧 RGB 往返，仅取/转换插值支路。保留场景属性检测和已有 3×/4×；单帧输入直接重复。
- 速度：两者可显式选择低分辨率计算，原始帧保持；默认原尺寸，低分辨率中间帧会损失细节。原节点按 VS 请求缓存共享相邻源/运动向量，不生成完整临时高帧率流。
- 验证：独立 Release GUI 构建、graph-and-vpy 通过；API 4 实际执行覆盖 MVTools 三组质量参数、两档缩小、镜头切换、单帧；RIFE 两模型三档尺寸、3×/4×、单帧。逐像素检查原帧、2× 帧数/FPS/帧时长、乱序 seek 和尾帧；推理陷阱确认原帧不请求生成支路。
- 性能：本机 1080p FFV1 testsrc2，源帧预取，12 张插值帧暖态：MVTools 原尺寸 block16 19.28、半尺寸 69.33 张/秒；RIFE 4.26 GPU0/线程1 原尺寸 11.59、半尺寸 38.84、四分之一 97.07 张/秒。合成短片且不含显示，不承诺所有核显/真实媒体实时；日志 build/interpolation-verification.txt。
- 交付：独立 build/mingw-interpolation，依赖安装/staging 已更新；不覆盖 dist/现有 Release 归档，最终播放器任务可直接使用本轮源码和插件。补帧速度仍受后续滤镜、CPU copy、显示与材质复杂度限制；既有 RIFE 色彩转换沿用 709，不新增 HDR 色彩语义。

## 21. MVTools 绿色闪烁修复（2026-09-30）

- 根因：将色度搜索开关同时传入 Super 与 Analyse；默认关闭时 Super 不携带 U/V 数据，FlowInter 插值帧色度为零，原帧和插值帧交替呈现绿色。跳帧是暴露现象，调度本身不是此问题的根因。此前中性灰测试及只校验原始支路未能检测插值帧色度异常。
- 修改：VpyScriptBuilder.cpp 的 Super 固定 chroma=True，Analyse 继续保留用户 chroma 开关。未改变跳帧机制、RIFE 和播放器路径。
- 验证：verify-interpolation.py 添加彩色 8/10/16-bit、两种色度搜索状态、三档尺寸、并发及乱序请求，逐平面校验中间帧；实际 D:\TEST.mkv 的输出 771/770/1201/7/771 帧 U/V 均非零，第 771 帧平均 U=0.441132、V=0.538802。其余 MVTools/RIFE 回归全部通过，独立 Release 构建与图脚本测试通过。
- 性能复测：修复后 1080p testsrc2 MVTools block16 原尺寸 28.04、半尺寸 93.29 张插值帧/秒；RIFE 原尺寸 13.11、半尺寸 47.14。短片暖态单次测量、未含显示，机器并发负载影响结果；上一节数据保留历史记录，以本轮有效色度输出复测为准。完整日志 build/mvtools-green-regression.txt。
- 交付：更新 build/mingw-interpolation/VSRenderer.exe，未覆盖并行播放器的 dist 或发行归档；旧生成/保存的 VPY 必须重新生成，已有脚本需将 Super 的 chroma 改为 True。实际 GUI 可见播放手势本轮未自动检查，已直接验证用户媒体的 VS 插值输出。

## 22. Renderer 预设与 VS Player 1.0.2（2026-09-30）

- 新增 PresetStore / PresetDialog / MainWindowPresets：便携 vpy 目录、三栏管理、原子写入、图元数据和源覆盖；外部 VPY 直接运行，图修改恢复生成路径。Anime4K 下拉模式与本机 14 文件部署、自定义路径。
- 新增独立 VSPlayer CMake 目标，输出 vs-player.exe；共享 VS 帧服务、3FP 外部帧、PreviewPane 与图生成器，播放器 UI/控制/信息分文件。音频播放源为时间基准，VS 按目标时间请求最新帧；暂停时支持输出帧独立选择。
- 原生补丁在独立 FFF.Native 副本开发：optional SetPlaybackRate C ABI、atempo 0.5–2 分段链、速率音频时钟、暂停位置保护、滤镜 seek 重置与尾部排空、真实章节媒体信息。
- LAV 不注册 COM：直接从随附 AX 调用类工厂建立 DirectShow 图，原速提供时钟/音频；VS 输入仍由 VPY 源负责，变速显式使用 3FP 保持音调。madVR 仅携带，用户已确认后续接入。
- 输出 dist/VS-Renderer-GUI-1.0.2-windows-x64，两程序与 Shader / vpy / LAVFilters64 / madVR09217 目录；不推送、不打包。不覆盖旧 dist 1.0.1，也保留并行补帧专项的源码与依赖。
- 验证覆盖：真实 VPY 输入替换及尺寸变化、章节、中文空格路径、精确输出帧、时间输入、关键帧、两种速率与暂停改速、原速 LAV、VS 画面及 LAV→不变调变速。Qt 抓图只验证控件，视频通过原生呈现计数/输出帧号验证。
- 最终交付验证：CTest 6/6 通过；交付目录播放回归 5/5 通过，包含 RIFE 48fps 输出帧号 7（从 0 起）的精确定位；14 个 Anime4K 文件逐一实际取帧成功。移出测试可执行文件与 Qt6Test.dll 后，两主程序从交付目录独立启动通过。MVTools 补帧与绿色闪烁修复已合入本次构建。
- 限制：任意 VPY 对时间轴的剪辑不会自动重建音频；LAV 的缓冲/PCM 细节未提供时如实显示。真实 HDR 显示器、多显示器和长片持续播放待人工验证。
- 补帧接入复核：两份 1.0.2 交付 EXE 与 Release 构建文件 SHA-256 一致，程序内完整补帧 helper 与最新源码一致。直接提取交付 EXE 的 helper，在交付 Python/VS/MVTools/RIFE 下运行回归，彩色 8/10/16-bit、并发跳帧、D:\TEST.mkv 色度、两套 RIFE 三档计算尺寸及 3×/4× 全部通过；graph-and-vpy 通过。已正常接入，无需重新构建。证据：build/interpolation-build-audit.json、build/1.0.2-packaged-interpolation.txt。
