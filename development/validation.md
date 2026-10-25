# 版本测试台账

核对日期：2026-10-25。历史轮次整理自仓库已留存报告；本轮上游内核同步单独列出。当前源码为 1.0.14；旧版本、旧 DLL、不同设备、电源状态或不同输出尺寸的结果不自动继承到当前产物。

## 如何读结论

- **专项通过**：仅列出的用例/样本满足断言；性能目标另行判断。
- **未达标**：已测但没有满足目标，不因 QtTest PASS 改写为通过。
- **未测/跳过**：没有覆盖或环境不满足，不算通过；各轮计数不相加。
- CTest 的组数与 QtTest 的用例数分开记录；历史 QtTest 计数可能包含 init/cleanup。
- 同版本多轮续修按轮次记录，旧失败须有后续相同路径的验证才能关闭，失败证据仍保留。

原报告原样合并至 [history.md](history.md)，按原文件名索引，每份保留字节数和 SHA-256。原文中的“当前”、路径和发布状态属于记录当时。`build/` 不是持久证据库：报告引用该目录，不保证换机器后日志仍可读。1.0.12 精选 CSV/日志保存在 [evidence](evidence/player-kernel-1.0.12/summary.json)。

## 各版本总览

| 版本 / 基线 | 留存测试范围与结果 | 未通过、未测与解释 | 原始依据 |
| --- | --- | --- | --- |
| 1.0.14 / API20 main 922e4b7 + AI Native 补丁 | 帧桥10、导出9、播放器新项5、Renderer设置3、Cap新项4及扩展专项8通过；4070指定链路与NuxBox对照均有实际数据 | 非全量回归；色彩9通过1既有失败；Lite7通过5跳过；AI模型/真实3FUI/UAC/目标Intel与1650s未验收；仅部分组合达2160p48 | [本轮证据](evidence/release-1.0.14/README.md) |
| 1.0.13续修 / API20 main 922e4b7 | 上游非AI内核、464字节快照、干净checkout构建；VS帧桥10、色彩专项8、播放生命周期9、增强模式长片逐帧6、Full BD4、Lite5 passed | 非全量回归；8K软件约33.87fps且有欠载，色彩套件两项旧首帧断言失败；AI仅单独拉取 | [本轮证据](evidence/native-upstream-20261024/summary.json) |
| 1.0.13 / API20 PR23 | 更新器进程/清理/可见重启、压缩工具、实盘BD与AVIF专项，详见本轮记录 | Full一条旧控件身份断言在原库同样失败；未做全量回归或重跑8K性能 | [本轮证据](evidence/release-1.0.13/summary.json) |
| 1.0.12 / API20 PR23 | BD整卷定位/按段索引/菜单续播；Jinc/无音轨VS；Anime预取；软件内核/像素/生命周期聚焦回归，详见下表 | 未运行全量Full播放器套件；8K48软件仍非实时；P/E调度等未测 | [BD](history.md#record-docs-update-1-0-12-md)、[内核](history.md#record-docs-player-kernel-performance-1-0-12-md) |
| 1.0.11 / API20，后续固定PR23 | 菜单/章节、时间组/社区、R81插件、JXL/白位、单文件FFmpeg外置和图标；VSCap工作流16 passed，窗口/全屏/底栏5 passed | 8K软件约31.03fps（4K输出）；桌面合成有跳过；HDR热键端到端、UIAccess/多DPI人工验收未完 | [更新](history.md#record-docs-update-1-0-11-md)、[JXL](history.md#record-docs-jpeg-xl-1-0-11-md)、[HDR](history.md#record-docs-vscap-hdr-1-0-11-md) |
| 1.0.10 / API18；10-16同版迁移API20 | 色彩4 passed；VSCap原生5、工作流先7后扩至14；Lite5；HEVC/AV1合成生命周期4；文字/选区/贴图分轮通过 | 8K AV1 AAC+ASS约33.16fps、61次欠载；迁移PR22当轮按要求未跑其测试；长盘跨段拖动当时失败，至1.0.12才有覆盖性验证 | [验收](history.md#record-docs-validation-1-0-10-md)、[交互](history.md#record-docs-vscap-interaction-1-0-10-md)、[迁移](history.md#record-docs-update-20261016-md) |
| 1.0.9 / API18，本地开发 | Full发行目录13 passed；Lite5；后续HDR黑屏修复原生截图5、工作流6；指定文件暂停/seek/续播/EOF/重开通过 | 8K48软件约22–24fps；D盘4K444p12开ASS最终目录约24.80fps；硬解稳定段通过但启动有丢帧；严格50MB/总包100MB目标未达；未运行全套 | [验收](history.md#record-docs-validation-1-0-9-20261010-md)、[续修](history.md#record-docs-fix-av1-vscap-1-0-9-20261010-md) |
| 1.0.8 | PhotoCraft真实GUI/16-bit PNG/float TIFF/ICC；48000×32000大图；780M双路4K48约47.9fps、10秒零丢帧；BD片段逐帧哈希及无音轨直通时钟专项 | 双路短窗口不能外推长片；CPU预缩放性能未验收；HDR/编辑器复杂对象另有限制 | [性能/大图](history.md#record-docs-performance-large-images-1-0-8-md)、[编辑器](history.md#record-docs-photocraft-editor-1-0-8-md)、[时钟](history.md#record-docs-player-startup-clock-1-0-8-md) |
| 1.0.7 | HEVC/AV1软件解码线程/队列、生命周期、字幕匹配；PNG高精度截图、透明底栏/音频信息及Full/Lite包专项 | 未记录一次统一全量回归；高码率尖峰/实时性能不能由功能通过推断；当时PNG附件规则与现行VSCap不同 | [软解/字幕](history.md#record-docs-software-decode-subtitles-1-0-7-md)、[截图](history.md#record-docs-player-high-precision-screenshots-1-0-7-md)、[包](history.md#record-docs-release-1-0-7-md) |
| 1.0.6 | 鼠标/原盘六集/滤镜字幕30项；菜单语言等另轮20项；本机8K硬解三组60秒约47.95fps、零新增丢帧；包完整性/哈希/启动 | 各项不是同一次总回归；RX6600原设备待复测；HEVC800Mbps峰值问题有复现，实验参数未部署 | [定位](history.md#record-docs-player-bd-seek-acceptance-1-0-6-md)、[硬解](history.md#record-docs-hardware-8k-queue-1-0-6-md)、[尖峰](history.md#record-docs-hevc444-peak-diagnosis-1-0-6-md) |
| 1.0.5 | 真实BD Remux/播放/seek、节目元数据、组件安装和更新回归；包完整性/解压哈希/主窗口启动 | 780M 8K48软件约18–19fps；完整RIFE/MVTools未全达实时；镜像/复杂盘型/部分HDR观感未测 | [发行记录](history.md#record-docs-release-1-0-5-md)、[软解](history.md#record-docs-av1-software-decode-1-0-5-md) |
| 1.0.4 | 色彩8/8、配置/语言/状态6/6、VS桥10/10、便携色彩smoke4/4；图片/导出分轮专项 | 不是一次全量回归；旧Qt编辑功能不能当PhotoCraft能力；完整PSD/DNG/AI等未实现 | [发行记录](history.md#record-docs-release-1-0-4-md)、[旧编辑](history.md#record-docs-image-editor-workflows-1-0-4-md) |
| 1.0.3 | 图片/音频/设置/Anime、Renderer布局/导出、图标、更新兼容和包校验专项 | 发行轮未跑无关全量；更早轮次曾8个CTest最终通过，但滑块性能断言整套超时后单测通过，不可写成末轮整套全绿 | [发行记录](history.md#record-docs-release-1-0-3-md)、[更早轮次](history.md#record-docs-history-development-records-through-2026-10-03-md) |
| 1.0.2 | Release构建；CTest8/8；便携Player27/27，更新7/7；移除测试依赖后两正式程序启动/退出 | 历史不同轮次另有6/6、7/7等，不能相加；HDR面板、多DPI、长片未全面人工验收 | [发行记录](history.md#record-docs-release-1-0-2-md) |
| 1.0.1 | Release构建、CTest五组；真实原生解码/像素、VS桥及导出 | Qt布局截图不等于完整人工画面验收；九路4K/VFR长片/多DPI未穷举 | [发行记录](history.md#record-docs-release-1-0-1-draft-md) |
| 1.0.0 | 留存基础Renderer功能记录 | 没有独立、可对应本版的完整测试结论，标记未确认，不倒填后版结果 | [早期原文](history.md#record-docs-history-development-records-through-2026-10-03-md) |

## 1.0.14：Player 设置与缩放显示续修（2026-10-25）

本轮保持版本 1.0.14，更新 Player/Renderer 的设置布局、实际缩放状态与主题；源码同步到本项目仓库，不制作压缩包、不更新远端 Release 附件。上轮不上传代码的边界属于当轮记录。

| 专项 | 本轮结果 | 覆盖与边界 |
| --- | --- | --- |
| Qt Release 构建 | Full / Lite / Renderer / Cap 及对应专项测试目标成功 | About 共用头与语言资源均纳入构建；未重封装单文件 EXE |
| Player 设置与缩放 | 10 passed / 0 failed / 0 skipped | 含 init/cleanup；字幕页与 SRT 样式入口、AI 迁移、关于作者链接、八档全列表、双排独立补帧档位、Lanczos 顺序与 D3D11 限制、切 Jinc 后徽标/Tab、跳过未勾选档位、实际 2160p FFV1 原生回退；设置应用保持播放/暂停状态 |
| Renderer 深色主题 | 4 passed / 0 failed / 0 skipped | 含 init/cleanup；参数编辑器深/浅色像素与文字颜色、既有设置页，实际抓图检查可读性 |
| 参数编辑器 | 4 passed / 0 failed / 0 skipped | 含 init/cleanup；Shader 分类、路径及宽高联动 |
| 语言资源 | 589 条通过 | 中英文键与占位符一致 |

已部署本地 Full 的 Player/Renderer 及 Lite Player，更新语言和操作文档；配置、VPY、ICC/LUT 的前后哈希一致。备份与逐文件哈希在 `build/ui-deployment-20261025-084535/deployment.json`。

本轮原始日志和截图保留在本地 `build/ui-fixes-1.0.14-*` 与 `build/player-*-1.0.14.png`、`build/mingw-release/renderer-parameters-*.png`。以上为专项验证，未运行全量 Full 播放器回归；不重新声称 AI 推理、目标 Intel/1650 Super 或上一轮性能矩阵已验收。

## 1.0.14：本地功能更新与设备性能检查

更新清单逐项状态、精确链路和能力边界见 [本轮记录](evidence/release-1.0.14/README.md)，NuxBox 原始数值见 [JSON](evidence/release-1.0.14/nuxbox.json)。

- 本机4070增强模式，用户指定1080p23.976样本：整帧540p RIFE后走Jinc/D3D11，3840×2160物理输出面30秒约47.933/47.967fps，跳帧/Native丢弃/合并均0；末端A4K仅22.161fps。保留1080原帧的半尺寸推理→A4K约42.700fps，未达标。
- 780M真实VS no CNN+额外增强：1080输出40.826→59.561fps、2160输出17.446→38.509fps；不含Native上传/呈现。
- 7840HS dav1d同编译器CLI三轮配对约+4.2%，1087帧像素MD5相同，仍低于48fps；独立FFmpeg已编译但端到端无稳定收益，未替换原软件。
- 色彩初次采样测试增加等待后，RGB/ICC测试通过；原生BT.2020 CL仍失败，在部署前旧Native相同位置复现。保留失败，不将其写成新修复。
- Native补丁可应用到固定main的干净索引，19个文件仅在FFF.Native；没有把Qt页面、模型或测试文件混入Native补丁。Shader生成头保留上游尾随空格，普通源码单独检查。
- 本次只做本地构建与目录部署，配置/VPY/ICC/LUT哈希部署及冒烟后核对一致；不封装压缩包、不上传代码、不更新外部Release。旧单文件EXE未重封装。

## 1.0.13续修：上游非AI内核同步（2026-10-24）

固定 `Lake1059/FFF_Project:main` 的 `922e4b70268fcbc83f0deb06d9e2c1b62e7a33b2`；完整[PR清单](evidence/native-upstream-20261024/pr-inventory.json)与[测试/采样汇总](evidence/native-upstream-20261024/summary.json)独立保存。API仍为20，配置112字节，snapshot由408扩到464字节并同步Qt镜像。保留VS外部帧、BD/concat、相对逐帧、clock-only、音频效果与可选色彩桥；Jinc权重从t5移到t8，避免与上游辅助视频层冲突，两个着色器字节码头已重编。

AI字幕#36、#37仅拉取在`.deps/fff-ai-subtitles`，停在#37的`edf5331`；不参与构建或分发。已实际检查DLL存在VS输入、clock-only、音频效果、流级side-data、RTX和外部声床渲染接口，缺少`FFF3FP_EnableAudioTap`/`FFF3FP_ReadAudioTap`。上游VB设置页、弹幕泵、外部授权模块没有搬入Qt。

| 专项 | 实际结果 | 覆盖与限制 |
| --- | --- | --- |
| Native与宿主构建 | Release x64 Native及Qt Full/Lite/Renderer成功 | 在另一个干净checkout用现行脚本重新应用完整补丁并构建；两个构建路径的DLL不要求字节相同，实际测试与部署的哈希见summary/artifacts |
| 上传与色彩聚焦 | 8 passed / 0 failed / 0 skipped | 新增上传次数/精确字节/重绘不重复上传/回读像素断言；原DLL在快照408≠464处失败，证明测试能识别旧库。默认色彩、手动色彩桥、硬解、VS输出通过 |
| 旧色彩完整套件 | 保留两项基线失败 | `constantLuminanceAndHdr`、`customIccLutAndRgbOrder`的首帧回读在原DLL同样失败（3 passed / 2 failed）；没有改动这些无关断言。初轮新增快照version错误已修复，初轮另一即时Present断言在专项复跑通过；不宣称全色彩套件全绿 |
| 播放生命周期/时钟 | 9 passed / 0 failed / 0 skipped | HEVC/AV1软件暂停、seek、EOF、关闭重开；VFR相对逐帧；三种直通启动/定位/恢复时钟及音频信息。小帧线程断言改为1～4起步，10-bit输出允许按显示条件选择10/16-bit |
| VS帧桥全套 | 10 passed / 0 failed / 0 skipped | 真实VS脚本、两路渲染、原生表面替换、请求合并、缩放/布局/全屏与像素回读 |
| 真实长片逐帧，增强模式开启后 | 6 passed / 0 failed / 0 skipped | GBC第1集4K48 444p12，SW/HW优先×Jinc/D3D11四组，约1/10/20分钟定位、进退帧、关键帧跳转及恢复；本机AV1 444p12硬件优先最终回退CPU，不能当成该格式硬解通过 |
| 增强模式开启前逐帧 | 2 passed / 4 failed；原DLL选取同位置2 passed / 1 failed | 关键帧后退一帧5秒断言超时，旧库同样复现；用户随后确认此前未开增强模式，开启后重新运行四组全部通过。两种电源状态不合并计数或比较性能 |
| Full BD拖动 | 4 passed / 0 failed / 0 skipped | MyGO下卷，播放与暂停状态均只在松开时提交一次定位，约220～235ms完成 |
| Lite原生路径 | 5 passed / 0 failed / 0 skipped | 同盘原生菜单控件、长卷前后跳转位置保持、无VS软件播放 |

增强模式由用户确认开启；采样时没有同时运行编译或其他播放测试。原生探针每臂先预热5秒，之后约15秒稳定窗口，输出交换链实际验证为3840×2160，D3D11缩放；单轮短窗口不构成严格A/B收益或长片稳定性结论。音频启用、静音播放，未加载VSP宿主字幕。

| 素材/模式 | 新画面呈现fps | 窗口新增丢帧/欠载 | 上传与同步 |
| --- | --- | --- | --- |
| OP 4K48 444p12 / CPU | 47.93 | 0 / 0 | 动态纹理，49,766,400字节/次；CPU复制3.63ms/次、约13.71GB/s；末端画面落后约17.5ms |
| 辉夜姬8K48 420p10 / CPU | 33.87 | 250 / 3 | 动态纹理，99,532,800字节/次；CPU复制8.27ms/次、约12.04GB/s；采样中最大画面落后约1.21秒，末端追到约30.7ms；**仍未达到逐帧实时** |
| 同一8K48 / GPU | 47.94 | 0 / 0 | 实际decodeMode=2、uploadPath=7；GPU本地复制，末端画面落后约7.3ms。快照复制计时为CPU提交耗时，不能据此计算真实GPU带宽 |

开启增强前的两份CSV仅保留诊断：早期探针误用render-target version1，目标尺寸字段为0；已改为version20且要求调用成功后重新测量，旧CSV不用于输出尺寸已验证的性能横比。所有QtTest计数含init/cleanup，不能相加视为一次全量回归。新七格式半平面、RTX增强、外部声床模块、原生AVIF、Intel P/E域和空间音频听感未做完整实机矩阵。

已部署本地`dist/VS-GUI-Full`的Renderer/Player/Native，并同步`build/playerlite-runtime`、重新生成`dist/VS-Player.exe`（约34.21MiB）。三处Native SHA-256均为`490d4cd7f7e0cbc95d2ad93b573497557aeeb3b692d91ccf73963a2192c87bbb`；单文件内嵌ZIP CRC、payload哈希和DLL内容一致，未嵌入player.ini或AI worker。Full/Lite配置、VPY、ICC/LUT前后哈希一致；VSCap运行时遇占用后改为只部署本轮文件，未替换其程序。备份在`build/pr-integration-20261024/deployment`，旧单文件另存于同轮`baseline`；[产物清单](evidence/native-upstream-20261024/artifacts.json)记录对应关系。这轮没有发布新版本或上传附件。

## 1.0.13：更新器、单文件BD与运行时精简

2026-10-18实际执行；构建为Qt 6.10.2 / MinGW Release，Native保持前轮DLL `0c5be410c1496ce6217346ba53d1546b6db2fbc1cb5f9129e36723b3daaa4580`，没有修改解码/渲染内核。1.0.12的8K/Y412性能数字属于前轮，不视为1.0.13重新测量；仍不保证8K48逐帧实时。日志留本机 `build/release-1.0.13/`，精选结果在 [summary.json](evidence/release-1.0.13/summary.json)。

| 专项 | 实际结果 | 范围与限制 |
| --- | --- | --- |
| 更新器 | 14 passed / 0 failed / 2 skipped | 隔离安装目录内关闭模拟VSCap、保留另一安装的进程；成功后备份/已验证下载包/临时目录清理、可见重启、ICC/配置保留；压缩MKV更新仍压缩；本地HTTP下载/验证/取消通过。跳过真实ModelScope在线下载及既有发布包审计 |
| 单文件后端 | 独立长卷前后跳转3 passed；之后两轮串行各9 passed / 0 failed / 0 skipped，退出码0 | MyGO下卷真实菜单/长卷、暂停恢复、硬件直通、进度目标保持。首轮串行日志未写完且未留退出码，不能计为通过；单次退出原因未定位，保留日志 |
| 单文件菜单续修 | 组合11 passed / 0 failed；终版两轮窗口级各4 passed / 0 failed，均退出码0 | 实盘菜单时间轴禁用/恢复、列表双击与右键返回、去除铺屏节目子菜单、每轮12次同盘返回、长卷列表播放后重新打开菜单、切回普通文件。候选构建曾捕获一次0xC0000005，调试器下未复现，未断言其精确崩溃点；终版移除原生关闭时的嵌套事件循环，并复用同盘信息，避免重复拆建。最终验证覆盖所述工作流，不代表穷举所有盘型 |
| Full精简VLC与图片 | 15 passed / 1 failed / 0 skipped | 六集菜单选择、返回主菜单和SDR/HDR PNG/AVIF/JXL通过。失败为旧 `bdMenuFromEmptyWindow` 要求重建surface的断言；使用未精简原VLC插件同样复现（2 passed / 1 failed），与当前复用surface的实现不符；未修改该无关测试，也不报告全套通过 |
| Full菜单续修回归 | 10 passed / 0 failed / 0 skipped，退出码0 | 精简VLC下返回菜单、六集选集、选集后交接3FP；不是上面含图片套件的重跑 |
| VSCap AWJ | 3 passed / 0 failed / 0 skipped | 真实HDR AVIF输出；Full截图另覆盖10/12-bit及CICP；不是全显示器色准验收 |
| MKV压缩CLI | 四个工具均能输出102.0版本，小样本Remux与识别成功 | 保留AVC/AAC轨道类型；未重跑长片/全盘Remux，不能外推所有媒体 |

以上QtTest计数含init/cleanup，独立运行计数不相加。AWJ 1.2.1二进制和对应源码按官方SHA-256固定，只分发 `AWJ.com` CLI、许可/NOTICE与对应源码；不分发AWJ GUI。VLC只按BD职责保留插件，不能当作完整VLC播放器使用。MKV保留四个CLI、许可证和英文文档，不分发GUI及其辅助程序；归档首次使用会在用户缓存占用展开空间，后续复用。组件更新保留压缩形式；旧受管工具路径迁移到当前缓存，外部用户指定路径保留。

版本、最终Full/单文件哈希、配置保护和体积见同目录 [artifacts.json](evidence/release-1.0.13/artifacts.json)。从本版起dist不再生成PlayerLite目录；其依赖暂存于build，只交付Full目录及 `VS-Player.exe`。本轮不制作Full压缩包，不上传Release附件。

### VSCap颜色窗口续修

2026-10-18：贴图“拖动”模式点击“颜色”后无法交互。原颜色窗口以QGraphicsProxyWidget中的画布为父级，拖动视图接管鼠标事件；改为归属真实工具栏窗口的独立Qt颜色对话框。

- 旧实现的窗口归属回归为3 passed / 2 failed：贴图拖动、画笔两行失败，普通截图行通过；为检查旧对话框结构，基线强制使用Qt对话框。
- 修复后按默认对话框环境运行三种场景，专项5 passed / 0 failed；验证真实窗口获得焦点、确认/取消、恢复拖动，以及导出线条仍保持确认的蓝色，取消选绿不改变颜色。
- 整套VSCap工作流20 passed / 0 failed / 0 skipped，退出码0；含初始化/清理，不与专项相加。期间有系统剪贴板占用告警，相关断言最终通过；不是多屏/混合DPI的人工验收。
- 证据见 [vscap-color](evidence/release-1.0.13/vscap-color/summary.json)。只部署Full的VSCap和对应指南，保留配置、Player和单文件播放器；新Full哈希以本次 [artifacts](evidence/release-1.0.13/vscap-color/artifacts.json) 为准，上方首轮artifacts保留历史值。

核对时1.0.13已正式发布且维护者已上传附件；本次不修改Release、tag或附件，不制作压缩包。源码/本地Full续修不代表既有1.0.13下载包已包含修复。

### HDR软解与默认色彩续修

2026-10-18：指定《花の塔》MP4为3840×2160 HEVC Main10、BT.2020/HLG、Dolby Vision Profile 8.4。旧默认/自动libplacebo路径下，软解Jinc和D3D11实际读回图像均只有两种绿色像素，红蓝通道为0；硬解有图像，但旧默认仍请求引擎2。原生3FP能正常输出，保留素材有意过曝的观感，本次没有修改Native DLL、HDR映射或亮度系数。

- 默认改为3FP（0），旧自动档（2）启动迁回0；移除自动高级引擎选项，Full只保留手动libplacebo（1）对照。显式手动档与ICC/LUT参数保留。
- 指定HLG实片、配置迁移和VS链不重载专项：旧版4 passed / 6 failed；修复后15 passed / 0 failed / 0 skipped，退出码0。失败旧例包括4条实际绿屏和2条旧默认引擎断言；不是硬解画面失败。
- `HDRTEST/out`实际19个AV1 10-bit文件，每个软解/硬解×Jinc/D3D11共76次呈现；38组同算法软硬解对照，在64×36平滑缩略图上的最大平均RGB绝对差为0.00019744（约0.0197%），最大单通道8-bit差2。包含9个明确PQ/BT.2020标记样本；其余10个缺传递函数等标记（含两个命名为SDR的对照），只能确认按当前标记的输出一致，不能追认为HDR色准通过。
- 目录回归与已有SDR/PQ/HLG中性灰、Jinc对照共22 passed / 0 failed / 0 skipped，退出码0；灰阶测试中最大算法通道差0.000977516。单文件后端基础回归5 passed / 0 failed，退出码0；各组计数含初始化/清理，不相加成全套回归。
- 本次实片呈现的实际输出模式为SDR（HDR→SDR），不代表物理HDR显示器或色度计校准验收。截图只留本机build，不上传用户视频或画面；19个目录样本的原文件哈希核对不变。

证据见 [hdr-native](evidence/release-1.0.13/hdr-native/summary.json)，部署与单文件信息见 [artifacts](evidence/release-1.0.13/hdr-native/artifacts.json)。版本保持1.0.13，更新本地Full和单文件；已发布tag、Release及附件不修改，Full压缩包仍由维护者制作。

### 10月18日：长视频相对逐帧与单文件打开

指定《Girls Band Cry》2160p48 AV1/Opus MKV，实际帧率7001/146、时间基1/1000。在60秒定位并按+1秒后，旧按钮调用绝对帧号定位，会从片头重建索引；HW-D3D11旧例2 passed / 1 failed，单次进帧超过5秒。改走相对PTS后又发现关键帧跳转留下的稀疏缓存不能直接视为相邻帧，已改为解码当前PTS附近的GOP确认严格前后帧，并处理片头继续退帧的无操作边界。单文件相对预设路径还暴露首次打开先用clock-only再切直通丢失会话的问题，现打开时固定直通；没有新增帧池或修改解码/色彩参数。

- Full四条实片路径（软/硬解×Jinc/D3D11）在1、10、20分钟位置，双向逐帧、关键帧后退帧、方向键、播放中逐帧暂停、片头边界和续播通过；GPU截图缩略内容确实变化，每次媒体时间移动20或21毫秒。连同25/50fps可变帧率相邻帧测试，7 passed / 0 failed / 0 skipped。60次按钮样本为50–1480ms；仍需解码局部GOP，不保证瞬时逐帧。
- 最终DLL的Full聚焦回归7 passed / 1 failed / 0 skipped，退出码1：指定实片HW-D3D11三位置、可变帧率、VS输出帧号进退、HEVC/AV1生命周期通过；失败为既有 `frameEditTakesPriorityAndPrefetchKeepsGpuBusy` 程序化移焦断言。用HEAD的 `PlayerWindow.cpp`、`PlayerControls.cpp` 与旧生产DLL重链同一测试，独立对照仍2 passed / 1 failed，同位置失败；未修改该用例或相关焦点逻辑，不报告全量回归通过。
- 最终单文件后端实片软/硬解×Jinc/D3D11、相对预设打开、逐帧后续播及既有模式/软解基础回归6 passed / 0 failed / 0 skipped，退出码0；确认未初始化VS。
- 全矩阵初测Native为 `1fc2385a4ed4ea2bf54d24fa12518f4106b46194646490e8cda5cde0328d8706`；随后只更新三行注释重建，最终Native为 `3fd4737c9e455fa2ba62d31579f80098df42a80546d8a85d2cffc6e81f724c24`，完成上述最终Full/Lite回归。完整补丁对固定上游PR23可应用，生产源码逆向检查通过，API布局不变。

证据见 [frame-stepping](evidence/release-1.0.13/frame-stepping/summary.json)，本地Full与单文件已更新；[产物校验](evidence/release-1.0.13/frame-stepping/artifacts.json)涵盖相同Native、ZIP CRC、图标、实际启动器子进程、外置FFmpeg、24份Full/1份内部单文件配置与预设等保护文件哈希保持不变，VSCap二进制保持不变。单文件35,809,965字节；版本仍1.0.13，已发布tag、Release及附件保持不变，Full压缩包由维护者制作。绝对帧号输入仍可能因未建立索引而需要较长等待，本轮修复的是相对逐帧。

## 1.0.12：按续修轮次区分

| 轮次 / 场景 | 已测与结论 | 保留的限制 / 证据 |
| --- | --- | --- |
| BD拖动与直接主菜单 | 普通视频/分集拖动6 passed；实盘菜单4 passed | 最初没有覆盖整卷失败，旧“慢例”已纠正；[原记录](history.md#record-docs-update-1-0-12-md) |
| 整卷点击/拖动续修 | MyGO下卷约8538.53秒，普通/BD、软/硬解、播放/暂停、跨段点击/反复拖动、双向seek与菜单；14 passed / 0 failed / 0 skipped | Lite该轮7 passed / 1 skipped；报告引用`build/bd-full-seek-after.txt`，原失败`build/bd-full-mouse-before.txt` |
| Jinc色彩与无音轨VS | 指定部署Full DLL，灰阶/8K SDR、四档Anime、时钟、VS属性、桌面白位、HDR FP16、BD；14 passed / 0 failed / 0 skipped | 不是全套，GPU像素不等于显示器绝对色准；[原记录](history.md#record-docs-player-color-vs-1-0-12-md) |
| Anime预取/大文件/菜单 | 专项13 passed / 0 failed；交互另3 passed；Lite7 passed / 1 skipped | 4K24→4K、Y41248→1440p指定20/30秒窗口通过；Y412→4K48仍非通过项；[原记录](history.md#record-docs-player-anime-performance-1-0-12-md) |
| BD VS按段索引与续播 | 四种菜单路径、全新索引、远距离定位、自然跨界，6 passed / 0 failed；冷首帧5.54～5.68秒；该轮Lite8 passed | 解析/缓存8 passed / 3 skipped（缺该套件实盘变量）；未跑全量Full；报告引用`build/bd-lazy-boundary-final.txt` |
| 软件内核最终回归 | 生命周期4 passed；聚焦9 passed / 3 skipped；默认设置6 passed；Lite6 passed；BD目录8 passed / 3 skipped | 聚焦跳过：两项缺独立素材、一项前台占用；语言脚本仍报9个已复现基线缺失；[保存的日志](evidence/player-kernel-1.0.12/final-regressions.txt) |

## 1.0.12：2026-10-18 软件同步与内存续修

目标：8K48允许省略画面，优先接近实际经过时间；4K48 Y412保留既有吞吐，减少缓冲和内存。以下为本轮真实运行，不能视作全量回归。原始日志在本机 `build/perf-sync-20261018/`，精选数据见 [summary.json](evidence/player-sync-20261018/summary.json)。

环境：7940HX / RTX4070 Laptop，2560×1600 /165Hz；完整Qt播放器全屏surface 2561×1601，CPU解码、D3D11，保留音频/字幕。用户确认115W增强模式、CPU可稳定95–100W；本轮没有独立采集功耗或重新查询Windows HDR开关，素材均为SDR。每轮定位10秒、预热5秒后采样；默认MMCSS/DXGI等待对象均关闭，无线程环境覆盖。

| 素材 / DLL | 实际采样窗口 | 媒体进度 / 画面PTS进度 | 呈现fps | 新增原生丢弃 / 合并 / 音频欠载 | 末尾私有内存 / 本地显存 |
| --- | --- | --- | --- | --- | --- |
| ChouKaguya432048-Part，旧DLL | 30.001秒 | 26.146 / 25.442秒 | 40.532 | 4 / 0 / 130 | 4669 / 377 MiB |
| 同一8K，最终DLL | 60.011秒 | 59.965 / 60.331秒 | 33.877 | 860 / 0 / 12 | 3835 / 277 MiB |
| OP-yuv444p12-SWTest，旧DLL | 60.009秒 | 60.008 / 60.018秒 | 47.959 | 0 / 0 / 0 | 6441 / 232 MiB |
| 同一Y412，最终DLL | 60.006秒 | 60.005 / 59.997秒 | 47.945 | 0 / 0 / 0 | 5153 / 185 MiB |
| Rebellion HEVC444p10 700Mbps，最终DLL | 30.006秒 | 30.005 / 29.988秒 | 23.962 | 0 / 0 / 0 | 2897 / 161 MiB |

本轮8K媒体进度/墙钟约99.92%，末尾画面落后媒体时钟0.482秒，每秒采样最大1.429秒；关键包之间仍有落后与追赶波动。60秒呈现2033帧、省略860帧，另插入67680个输出音频sample frames；**仍非48fps逐帧实时，也不是无缺口音频**。丢弃计数与CFR源PTS对应帧数核对误差小于4帧，暂停时位置冻结、定位第72帧并截图、恢复播放、跳至片尾均通过。定位后的恢复预填充约1.65秒，之后单独核对稳态进度。8K旧/新窗口长度不同，不作严格吞吐提升百分比。

Y412最终使用24工作线程/24帧上下文，对照旧32/32，60秒窗口私有内存下降19.99%、进程本地显存下降20.58%，零新增丢帧/合并/欠载。CPU进程时间/墙钟为8.101→8.105，基本持平，不能把线程数减少解释成计算成本同比下降。内存数字为采样末尾的进程私有提交与按adapter合计的DXGI本地用量，不是全机内存、整个显卡总占用或分配器硬上限；早期用QSignalSpy保留每次字幕QImage产生的13–16GiB结果无效，已修正测试采样并重新测基线。

实现范围：按帧字节约束dav1d深度、在途帧和显示队列；默认单交接槽、关闭额外双DEFAULT/双STAGING上传环，保留实验开关；8K AV1恒帧率持续落后时只在关键包切换完整/非参考帧过滤，定位恢复完整解码；欠载静音提交推进音频时钟并去掉晚到音频重叠。压缩视频包上限仅8K CPU AV1由16升至32MiB、仍限64包，避免长GOP堵住后续音频。未改变素材分辨率、位深、色度、色彩shader或FFmpeg版本。

构建与专项：完整Native补丁对固定上游 `git apply --check` 成功，并从独立checkout重建成功。最终生产Native SHA-256：`0c5be410c1496ce6217346ba53d1546b6db2fbc1cb5f9129e36723b3daaa4580`；旧DLL `fc61e2f52e6d418c02ea311cb1059649f18a541b8be4af62375e9af0b18fd39a`；avcodec保持 `07298f3f3bb71ba121e536f2495402ce43dddbb0a3b9d40178a20e19437c4c79`。最终8K同步/导航、Y412吞吐、HEVC吞吐各3 passed / 0 failed / 0 skipped（含init/cleanup）；[Full聚焦回归](evidence/player-sync-20261018/final-regressions.txt)12 passed、[Lite](evidence/player-sync-20261018/final-lite.txt)6 passed，均0 failed / 0 skipped（含init/cleanup）；覆盖HEVC/AV1生命周期、实际Y412多方向定位与恢复、无音轨时钟、音频轨道与欠载统计、呈现计数。3840×2160合成8/10/16-bit SDR/PQ上传与旧DLL对照最大RGB误差0，Jinc色彩专项通过；这不是显示器色准验收。语言完整性脚本仍有两包各9个已复现基线缺失，本轮新增0，详见[languages.json](evidence/player-sync-20261018/languages.json)。

本地 `dist/VS-GUI-Full`、`dist/1.0.12-PlayerLite` 已部署；初次因VSCap占用中断，用户退出后完成原目录更新。INI/VPY/ICC/LUT哈希保持不变，旧包残留的开发记录转存到本机部署备份，运行目录仅保留现行指南。[单文件与产物校验](evidence/player-sync-20261018/artifacts.json)涵盖manifest/Native一致、ZIP CRC、图标、解包子进程导入、外置FFmpeg、配置和原盘索引保持不变。`dist/VS-Player.exe`为34.15MiB，仍需外部FFmpeg组件。版本保持1.0.12，不修改已正式发布的GitHub Release附件。

## 1.0.12：10月17日性能与最终 DLL 对应

环境：7940HX / RTX4070 Laptop，Windows HDR关闭，显示器2560×1600 /165Hz；完整Qt播放器全屏surface 2561×1601、16:9内容约2560×1440，CPU解码、D3D11，保留音频/字幕。定位10秒、预热5秒后采样。

| 样本 / 轮次 | 实际窗口 | 呈现 fps | 新增丢帧 / 合并 / 音频欠载 | 结论 |
| --- | --- | --- | --- | --- |
| 4K AV1 YUV444P12 OP | 30.000秒，单轮 | 47.967 | 0 / 0 / 0 | 采样窗口通过；起采前已有2个启动期丢帧，不是全程零丢帧 |
| HEVC444p10 700Mbps | 60.004秒，单轮 | 23.982 | 0 / 0 / 0 | 指定窗口通过；初始16线程，启动曾升24，不能写为始终固定16 |
| 8K AV1 420p10，16/16 | 20.000秒 | 34.00 | 212 / 0 / 19 | 未达8K48实时 |
| 同一8K，8/16 | 20.011秒 | 40.578 | 1 / 0 / 89 | 仍非实时 |
| 同一8K，8/8+双槽+软提示+上传组合 | 20.010秒 | 40.53 | 1 / 0 / 87 | 仍非实时；该轮按用户要求停止追逐45fps |

前两项采样启用了默认关闭的MMCSS/DXGI等待对象，不能表述为默认配置成绩。与1.0.11的4K输出31.03fps条件不同，不计算严格A/B提升百分比。14700K P/E分支未在该机实测，强制绑核等负收益实验未默认启用。

采样后只追加交接槽容量互斥保护，重跑生命周期4 passed，没有重跑两项性能采样。采样Native SHA-256：`991069f9b84aa8bd568c12d031c02738ae3c940bfa7956f940df9f149a7a5fa4`；最终Native：`fc61e2f52e6d418c02ea311cb1059649f18a541b8be4af62375e9af0b18fd39a`；avcodec：`07298f3f3bb71ba121e536f2495402ce43dddbb0a3b9d40178a20e19437c4c79`。依据见 [summary.json](evidence/player-kernel-1.0.12/summary.json) 和同目录CSV/日志。

## 跨版本结论如何更新

| 历史问题 | 当前可确认的变化 | 仍不能确认的内容 |
| --- | --- | --- |
| 1.0.10/11长盘跨段点击/拖动失败 | 1.0.12同实盘整卷鼠标路径已修复并验证 | 其它盘型、菜单后端全部路径 |
| 1.0.9 4K444p12完整GUI不实时 | 1.0.12指定OP软件直通窗口47.967fps；Anime48输出1440p另有通过窗口 | 不同机器/输出4K增强/所有高压字幕片段全面实时 |
| 1.0.10/11 8K48软件不实时 | 1.0.12指定条件约40.53fps，但仍欠载 | 实时目标仍未通过，不能关闭该项 |
| 1.0.9黑屏、1.0.10文字/选区、1.0.11白位/JXL | 各轮有对应修复与专项依据 | HDR端到端耗时、多屏DPI、UIAccess及全部面板观感 |
| 1.0.10合入PR22当轮未跑测试 | 后续1.0.11/12在对应后续基线运行聚焦测试 | 不能追认当轮已测，也不等于PR23全量回归完成 |

## 维护与10月17日文档校验

后续在本文件增加版本行与必要轮次，每项记录构建/素材/环境、实际范围、通过/失败/跳过、目标是否满足和证据位置。日志留build，必要数据集中到evidence，不再新增零散测试说明。

10月17日该轮仅整理文档和文档复制路径；未构建、未启动播放器、未重新测性能、未更新dist或Release，不能把历史数字当作该轮执行结果。

2026-10-17 文档校验通过：87份归档逐份核对字节数/SHA-256，并与Git HEAD原文比对（仅容许Git工作区换行转换）；14份证据、3份第三方文件内容保留；103处本地链接/锚点有效；11节项目骨架、13个倒序版本完整；2份Python语法、5份PowerShell语法和`git diff --check`通过。隔离目录复制10份指南、4份许可/清单成功，指南哈希与源文件一致。检查摘要在本机`build/doc-reorganization-check.json`；没有执行完整部署或打包脚本。
