# 版本测试台账

核对日期：2026-10-17。此表整理仓库已留存的报告与证据，**不是本轮重新运行软件的测试结果**。当前源码为 1.0.12；旧版本、旧 DLL、不同设备或不同输出尺寸的结果不自动继承到当前产物。

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

## 1.0.12：按续修轮次区分

| 轮次 / 场景 | 已测与结论 | 保留的限制 / 证据 |
| --- | --- | --- |
| BD拖动与直接主菜单 | 普通视频/分集拖动6 passed；实盘菜单4 passed | 最初没有覆盖整卷失败，旧“慢例”已纠正；[原记录](history.md#record-docs-update-1-0-12-md) |
| 整卷点击/拖动续修 | MyGO下卷约8538.53秒，普通/BD、软/硬解、播放/暂停、跨段点击/反复拖动、双向seek与菜单；14 passed / 0 failed / 0 skipped | Lite该轮7 passed / 1 skipped；报告引用`build/bd-full-seek-after.txt`，原失败`build/bd-full-mouse-before.txt` |
| Jinc色彩与无音轨VS | 指定部署Full DLL，灰阶/8K SDR、四档Anime、时钟、VS属性、桌面白位、HDR FP16、BD；14 passed / 0 failed / 0 skipped | 不是全套，GPU像素不等于显示器绝对色准；[原记录](history.md#record-docs-player-color-vs-1-0-12-md) |
| Anime预取/大文件/菜单 | 专项13 passed / 0 failed；交互另3 passed；Lite7 passed / 1 skipped | 4K24→4K、Y41248→1440p指定20/30秒窗口通过；Y412→4K48仍非通过项；[原记录](history.md#record-docs-player-anime-performance-1-0-12-md) |
| BD VS按段索引与续播 | 四种菜单路径、全新索引、远距离定位、自然跨界，6 passed / 0 failed；冷首帧5.54～5.68秒；该轮Lite8 passed | 解析/缓存8 passed / 3 skipped（缺该套件实盘变量）；未跑全量Full；报告引用`build/bd-lazy-boundary-final.txt` |
| 软件内核最终回归 | 生命周期4 passed；聚焦9 passed / 3 skipped；默认设置6 passed；Lite6 passed；BD目录8 passed / 3 skipped | 聚焦跳过：两项缺独立素材、一项前台占用；语言脚本仍报9个已复现基线缺失；[保存的日志](evidence/player-kernel-1.0.12/final-regressions.txt) |

## 1.0.12：性能与最终 DLL 对应

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

## 维护与本轮文档校验

后续在本文件增加版本行与必要轮次，每项记录构建/素材/环境、实际范围、通过/失败/跳过、目标是否满足和证据位置。日志留build，必要数据集中到evidence，不再新增零散测试说明。

本轮仅整理文档和文档复制路径；未构建、未启动播放器、未重新测性能、未更新dist或Release，不能把上述历史数字当作本轮执行结果。

2026-10-17 文档校验通过：87份归档逐份核对字节数/SHA-256，并与Git HEAD原文比对（仅容许Git工作区换行转换）；14份证据、3份第三方文件内容保留；103处本地链接/锚点有效；11节项目骨架、13个倒序版本完整；2份Python语法、5份PowerShell语法和`git diff --check`通过。隔离目录复制10份指南、4份许可/清单成功，指南哈希与源文件一致。检查摘要在本机`build/doc-reorganization-check.json`；没有执行完整部署或打包脚本。
