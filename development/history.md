# 历史文档原文归档

2026-10-17 整理前快照。仅供核查历史证据；原文中的“当前”、路径、命令和授权均属于当时，不能作为现行操作指令。

当前测试结论见 [版本测试台账](validation.md)，当前操作说明见 [docs](../docs/README.md)。每节记录原路径、字节数和 SHA-256；代码围栏内保留原始字节，原文链接不作为现行导航。

## 原文件索引

- [docs/3fp-api20-and-singlefile-lite-20261016.md](#record-docs-3fp-api20-and-singlefile-lite-20261016-md)
- [docs/anime4k-fast-source.md](#record-docs-anime4k-fast-source-md)
- [docs/audio-bd-mks-1.0.6.md](#record-docs-audio-bd-mks-1-0-6-md)
- [docs/av1-software-decode-1.0.5.md](#record-docs-av1-software-decode-1-0-5-md)
- [docs/avif-large-yuv444-analysis.md](#record-docs-avif-large-yuv444-analysis-md)
- [docs/bd-menu-subtitle-languages-1.0.6.md](#record-docs-bd-menu-subtitle-languages-1-0-6-md)
- [docs/bd-menu-viewport-1.0.6.md](#record-docs-bd-menu-viewport-1-0-6-md)
- [docs/bd-remux-1.0.5.md](#record-docs-bd-remux-1-0-5-md)
- [docs/color-management-1.0.4.md](#record-docs-color-management-1-0-4-md)
- [docs/components-player-1.0.5.md](#record-docs-components-player-1-0-5-md)
- [docs/dependencies.md](#record-docs-dependencies-md)
- [docs/development-1.0.9-status.md](#record-docs-development-1-0-9-status-md)
- [docs/ffmpeg-enhanced-1.0.4.md](#record-docs-ffmpeg-enhanced-1-0-4-md)
- [docs/filter-coverage.md](#record-docs-filter-coverage-md)
- [docs/fix-av1-vscap-1.0.9-20261010.md](#record-docs-fix-av1-vscap-1-0-9-20261010-md)
- [docs/hardware-8k-queue-1.0.6.md](#record-docs-hardware-8k-queue-1-0-6-md)
- [docs/hevc444-peak-diagnosis-1.0.6.md](#record-docs-hevc444-peak-diagnosis-1-0-6-md)
- [docs/history/development-records-through-2026-10-03.md](#record-docs-history-development-records-through-2026-10-03-md)
- [docs/icons-integration.md](#record-docs-icons-integration-md)
- [docs/image-editor-interaction-1.0.4.md](#record-docs-image-editor-interaction-1-0-4-md)
- [docs/image-editor-plan.md](#record-docs-image-editor-plan-md)
- [docs/image-editor-round2-1.0.4.md](#record-docs-image-editor-round2-1-0-4-md)
- [docs/image-editor-workflows-1.0.4.md](#record-docs-image-editor-workflows-1-0-4-md)
- [docs/input-av1-yuv444p12-diagnosis.md](#record-docs-input-av1-yuv444p12-diagnosis-md)
- [docs/jpeg-xl-1.0.11.md](#record-docs-jpeg-xl-1-0-11-md)
- [docs/local-1.0.5-performance.md](#record-docs-local-1-0-5-performance-md)
- [docs/mpv-shaders-1.0.4.md](#record-docs-mpv-shaders-1-0-4-md)
- [docs/mpv-shaders-interpolation-1.0.3.md](#record-docs-mpv-shaders-interpolation-1-0-3-md)
- [docs/native-api20-build.md](#record-docs-native-api20-build-md)
- [docs/nuxbox-performance-handoff.md](#record-docs-nuxbox-performance-handoff-md)
- [docs/nuxbox-performance-session-2026-10-04.md](#record-docs-nuxbox-performance-session-2026-10-04-md)
- [docs/packaging-1.0.6.md](#record-docs-packaging-1-0-6-md)
- [docs/performance-large-images-1.0.8.md](#record-docs-performance-large-images-1-0-8-md)
- [docs/photocraft-editor-1.0.8.md](#record-docs-photocraft-editor-1-0-8-md)
- [docs/player-anime-performance-1.0.12.md](#record-docs-player-anime-performance-1-0-12-md)
- [docs/player-apply-avif-1.0.3.md](#record-docs-player-apply-avif-1-0-3-md)
- [docs/player-bd-concat-preroll-1.0.8.md](#record-docs-player-bd-concat-preroll-1-0-8-md)
- [docs/player-bd-lyrics-editor-fixes-1.0.6.md](#record-docs-player-bd-lyrics-editor-fixes-1-0-6-md)
- [docs/player-bd-seek-acceptance-1.0.6.md](#record-docs-player-bd-seek-acceptance-1-0-6-md)
- [docs/player-color-vs-1.0.12.md](#record-docs-player-color-vs-1-0-12-md)
- [docs/player-direct-open-1.0.4.md](#record-docs-player-direct-open-1-0-4-md)
- [docs/player-high-precision-screenshots-1.0.7.md](#record-docs-player-high-precision-screenshots-1-0-7-md)
- [docs/player-image-tools-1.0.4.md](#record-docs-player-image-tools-1-0-4-md)
- [docs/player-images-audio-1.0.3.md](#record-docs-player-images-audio-1-0-3-md)
- [docs/player-images-menu-1.0.3.md](#record-docs-player-images-menu-1-0-3-md)
- [docs/player-input-layout-1.0.6.md](#record-docs-player-input-layout-1-0-6-md)
- [docs/player-interaction-performance-1.0.5.md](#record-docs-player-interaction-performance-1-0-5-md)
- [docs/player-kernel-d3d11-performance-plan-2026-10-17.md](#record-docs-player-kernel-d3d11-performance-plan-2026-10-17-md)
- [docs/player-kernel-performance-1.0.12.md](#record-docs-player-kernel-performance-1-0-12-md)
- [docs/player-manual-presets-1.0.3.md](#record-docs-player-manual-presets-1-0-3-md)
- [docs/player-overlay-layout-1.0.8.md](#record-docs-player-overlay-layout-1-0-8-md)
- [docs/player-performance-1.0.2.md](#record-docs-player-performance-1-0-2-md)
- [docs/player-resolution-threshold-1.0.3.md](#record-docs-player-resolution-threshold-1-0-3-md)
- [docs/player-six-stage-1.0.3.md](#record-docs-player-six-stage-1-0-3-md)
- [docs/player-startup-clock-1.0.8.md](#record-docs-player-startup-clock-1-0-8-md)
- [docs/player-transparent-audio-info-1.0.7.md](#record-docs-player-transparent-audio-info-1-0-7-md)
- [docs/portable-updates.md](#record-docs-portable-updates-md)
- [docs/realtime-bridge.md](#record-docs-realtime-bridge-md)
- [docs/release-1.0.1-draft.md](#record-docs-release-1-0-1-draft-md)
- [docs/release-1.0.10.md](#record-docs-release-1-0-10-md)
- [docs/release-1.0.11.md](#record-docs-release-1-0-11-md)
- [docs/release-1.0.2.md](#record-docs-release-1-0-2-md)
- [docs/release-1.0.3.md](#record-docs-release-1-0-3-md)
- [docs/release-1.0.4.md](#record-docs-release-1-0-4-md)
- [docs/release-1.0.5.md](#record-docs-release-1-0-5-md)
- [docs/release-1.0.6.md](#record-docs-release-1-0-6-md)
- [docs/release-1.0.7.md](#record-docs-release-1-0-7-md)
- [docs/release-1.0.8.md](#record-docs-release-1-0-8-md)
- [docs/renderer-panels-1.0.3.md](#record-docs-renderer-panels-1-0-3-md)
- [docs/renderer-ui-export-1.0.3.md](#record-docs-renderer-ui-export-1-0-3-md)
- [docs/software-decode-subtitles-1.0.7.md](#record-docs-software-decode-subtitles-1-0-7-md)
- [docs/tracks-size-1.0.4.md](#record-docs-tracks-size-1-0-4-md)
- [docs/ui-layout-spec.md](#record-docs-ui-layout-spec-md)
- [docs/update-1.0.11.md](#record-docs-update-1-0-11-md)
- [docs/update-1.0.12.md](#record-docs-update-1-0-12-md)
- [docs/update-20261016.md](#record-docs-update-20261016-md)
- [docs/update-compatibility-1.0.3.md](#record-docs-update-compatibility-1-0-3-md)
- [docs/validation-1.0.10.md](#record-docs-validation-1-0-10-md)
- [docs/validation-1.0.9-20261010.md](#record-docs-validation-1-0-9-20261010-md)
- [docs/vscap-2026-10-16-multipin-awj-hdr.md](#record-docs-vscap-2026-10-16-multipin-awj-hdr-md)
- [docs/vscap-hdr-1.0.11.md](#record-docs-vscap-hdr-1-0-11-md)
- [docs/vscap-interaction-1.0.10.md](#record-docs-vscap-interaction-1-0-10-md)
- [docs/vscap-sdr-white-1.0.11.md](#record-docs-vscap-sdr-white-1-0-11-md)
- [docs/workspace-cleanup.md](#record-docs-workspace-cleanup-md)
- [README.md](#record-readme-md)
- [project.md](#record-project-md)
- [changelog.md](#record-changelog-md)

<a id="record-docs-3fp-api20-and-singlefile-lite-20261016-md"></a>

## docs/3fp-api20-and-singlefile-lite-20261016.md

原始字节数：5908；SHA-256：`42df7cc32b89b65aa4fb3b126e0c46b9cafdfcd1b9752510789603d128a10e2c`。

<!-- begin:docs/3fp-api20-and-singlefile-lite-20261016.md -->
````````markdown
# 3FP API 20 上游核对与 VSPlayerLite 单文件交付（2026-10-16）

## 1. 已确认上游

- 上游仓库：`Lake1059/FFF_Project`；主分支本次取得的 commit：`856b320`（2026-10-15，合并 PR #21）。
- [PR #9](https://github.com/Lake1059/FFF_Project/pull/9)：D3D11 DXGI 适配器选择、批量像素读取、渲染目标状态、视图变换快速路径；已合并。
- [PR #19](https://github.com/Lake1059/FFF_Project/pull/19)：HDR10+/Vivid 动态元数据注入、IAMF OBR 双耳空间音频、旋转感知 RTX VSR、API 20 和音频延时；已合并。
- [PR #21](https://github.com/Lake1059/FFF_Project/pull/21)：刷新节奏锁定、kind-3 色彩变换、图片缩放几何、RTX VSR 宿主设置，以及渲染修复；已合并。
- 直连 GitHub 曾失败，本机使用 `https://tvv.tw/https://github.com/Lake1059/FFF_Project.git` 成功取得代码，保留于 `.deps/fff-upstream-api20`。
- 本工程自身 `origin/main` 与本地 `17b51f9` 完全同步（领先/落后：0/0），因此新功能位于外部 3FP 原生仓库，并非本项目漏拉 `main`。

## 2. API20 迁移进展（本地续修）

已在固定上游 `856b3200c2130db75a2ab06730e32fb5c65f0bd1` 重新移植 VS 外部帧、clock-only、色彩桥、缩放和音频扩展，得到 `patches/3fp-api20-vsrenderer-1.0.10.patch`。MSVC v145 成功构建完整 IAMF/OBR 内核，未通过关闭 IAMF 降级；补齐 FDK-AAC 链接项。构建入口和固定依赖见 [API20 构建说明](native-api20-build.md)。

GUI 的 snapshot、字幕 renderer/command/layer、音频电平结构按 API20 传版本号，同时保留 API18 兼容分支；MinGW 与 MSVC 的结构大小及关键偏移已核对一致。真实回归已覆盖音频时钟、原始/Jinc/D3D11 播放、外部 VS 帧、输出格式切换、像素探测、字幕叠加和 Lite 无 VS 软解。

原网页阶段“API20 未部署”“Lite 测试超时”的记录由本地续修替代；最终部署/验证以 [续修记录](update-20261016.md) 为准。上游20不再提供旧私有 DV/RPU 测试扩展，本轮不恢复该接口；公开 HDR 元数据和 kind-3 色彩路径继续使用上游实现。IAMF/OBR 已编译，但缺少对应空间音频素材，不能宣称听感验收已通过。

## 3. 已完成的播放器修改

- `src/player/PlayerControls.cpp`：只删除了控制栏右下角重复的 `☰` 菜单按钮和连接的动作；操作仍在 `src/player/PlayerMenus.cpp` 右键菜单中，保留其入口。
- `src/player/PlayerWindow.cpp`：Lite 优先从 `VSR_PLAYER_CONFIG` 读取 INI 路径，否则使用 EXE 所在目录 `player.ini`；首次执行应用 Lite 默认设置，已有配置保留所有可用设置与字体，不再无条件重置合法选项；如配置要求 Lite 根本不提供的 LAV/madVR 等后端，则只将这些无效选项回退至 Lite 支持的 3FP/VS。
- `src/backend/ThreeFpApi.h`：API20 snapshot 布局及调用版本已配套迁移，详见上节。
- 完成 `VSPlayer` 与 `VSPlayerLite` 的 MinGW CMake Release 编译（无编译错误）。

## 4. 网页阶段单文件 Lite（历史产物，本轮不重打包）

- 输出：`dist/VSPlayerLite.exe`，约 **31.72 MiB**。
- 构建入口：`tools/package-playerlite-single.ps1`；启动器：`tools/playerlite-launcher.c`。
- 这是**单文件分发、首次解压、以后复用缓存**的便携实现，而非把 Qt/FFF Native 静态链接进一个原生 PE。EXE 附带 ZIP，首次运行展开至 `%LOCALAPPDATA%/VSPlayerLite/<payload-hash>/`；之后识别 `.ready` 不再解压，减少热启动前置开销。
- 包内没有 `ffmpeg.exe`、`ffprobe.exe` 或 `avcodec-*.dll`、`avformat-*.dll`、`avutil-*.dll`、`avfilter-*.dll`、`swresample-*.dll`、`swscale-*.dll` 等 FFmpeg 动态库。
- 运行机需要把与原生 DLL **同 ABI** 的 FFmpeg 共享库放在单文件 EXE 所在目录，或加入系统 `PATH`；仅安装一个其他 major 版本的 `ffmpeg.exe` 不代表 ABI 兼容。
- 首次启动器在**单文件所在目录**安全创建 `player.ini` 并传递 `VSR_PLAYER_CONFIG`；已有 `player.ini` 时不覆盖；若该目录不可写则明确提示权限错误。
- 已验证：使用同目录 FFmpeg 库时启动器返回码 0；从独立位置仅通过 PATH 提供依赖时启动器也返回 0；配置文件能创建，并在下一次启动保留测试写入的 `[test] custom=42`。
- 启动器实际派发测量（并非 GUI 绘制或起播时间）：某次冷启动约 **428 ms**，暖启动约 **50 ms**；仅验证进程派发，不能据此宣称播放画面或 HDR 已正常输出。
- 单文件脚本以已编译的 Lite GUI 和现有 1.0.10 发行依赖打包，**新 API20 内核未放入交付版**。保留了原有 `dist/1.0.10-PlayerLite` 和 `dist/1.0.10-Full` 作为可回退基线。

## 5. 网页阶段的验收边界（历史）

已证明：上游 commit/PR 核对、GUI/Lite 编译、重复按钮删除、单文件构建、FFmpeg 排除、运行时缓存生成、INI 路径/重开保留、FFmpeg 所在目录与 PATH 两种启动器路径。

失败或未验收：在本机 AgentDock 非交互式会话下运行 `ctest -R ^lite-native-playback$`，测试 #22 超过 90 秒超时，**未通过**；这不是能用编译成功替代的回归验证。仍需在可交互桌面核对是否属于测试会话 GUI 阻塞还是产品问题，随后执行完整 Lite 回归、打开视频/音视频输出检查、真实启动到首帧计时、HDR/空间音频/RTX VSR 与 D3D11 实机性能对比；以及上游 API20 与本地 VS 定制模块的源码移植、编译与交付。不要将新上游 PR 已合并误记为本项目核心已升级。

追加：上游PR22已合并到固定内核并编译，按用户要求未运行测试。此前API20运行证据来自PR22合入前；最终验收边界见 [续修记录](update-20261016.md)。

````````
<!-- end:docs/3fp-api20-and-singlefile-lite-20261016.md -->

<a id="record-docs-anime4k-fast-source-md"></a>

## docs/anime4k-fast-source.md

原始字节数：1713；SHA-256：`e3d1d1038151a23c8d5f99e91a8264f490489ff3b69ce967f2f8937085b7e8aa`。

<!-- begin:docs/anime4k-fast-source.md -->
````````markdown
# Anime4K A/Fast 来源

2026-10-02：新增独立 `anime4k-no-cnn.glsl`，由同一 commit 的 Clamp_Highlights、Experimental-Effects/Thin_HQ、Darken_HQ 组合，保留各段 MIT 许可与原系数；采用梯度 / DoG 线条处理，没有 CNN 或 2× CNN 中间图。CNN 文件仍为下面的 A/Fast。两种画质不等价，六档与格式限制见 [实现记录](player-six-stage-1.0.3.md)。

`assets/anime4k-a-fast.glsl` 及便携 `shaders/anime4k-a-fast.glsl` 按官方 Windows Low-end 的 Mode A (Fast) 顺序组合，未改动六段算法，保留各段原有版权与许可。

上游：[bloc97/Anime4K](https://github.com/bloc97/Anime4K)，commit `7684e9586f8dcc738af08a1cdceb024cc184f426`。配置：[GLSL_Windows_Low-end/mpv.conf](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/md/Template/GLSL_Windows_Low-end/mpv.conf)。

1. `glsl/Restore/Anime4K_Clamp_Highlights.glsl`
2. `glsl/Restore/Anime4K_Restore_CNN_M.glsl`
3. `glsl/Upscale/Anime4K_Upscale_CNN_x2_M.glsl`
4. `glsl/Upscale/Anime4K_AutoDownscalePre_x2.glsl`
5. `glsl/Upscale/Anime4K_AutoDownscalePre_x4.glsl`
6. `glsl/Upscale/Anime4K_Upscale_CNN_x2_S.glsl`

当前 A/M Fast 使用 M / S CNN，不能把用户图片中的 5 ms 自动归给这个网络：上游早期 v0.9 是非 CNN 的梯度方法，图的版本 / 测试方法没有给出。早期上游提供 Vega64 与 Vega8 数字，不能解读为任意核显的当前整套 VS / 解码 / 音频 / 呈现性能。[早期官方说明](https://github.com/bloc97/Anime4K/blob/v0.9/Preprint.md) vs-placebo 在 VS 中返回 CPU 可读取帧，GPU 上传与读回等成本仍存在。本机配对数据见 `player-performance-1.0.2.md`。

````````
<!-- end:docs/anime4k-fast-source.md -->

<a id="record-docs-audio-bd-mks-1-0-6-md"></a>

## docs/audio-bd-mks-1.0.6.md

原始字节数：4998；SHA-256：`1c5774f6d3e3523fb2d6bf94c08418ff7bbe233463174266da8e8f94f806f8f1`。

<!-- begin:docs/audio-bd-mks-1.0.6.md -->
````````markdown
# 1.0.6 本地播放器更新

本轮只修改源码、编译和更新本地 dist；不打包、不推送、不上传，也不修改 1.0.5 的归档、标签或发行草稿。

后续同版本已修复下文首次验收留下的 BD PGS -4、短节目和 Tab 持续增长问题，并增加原始菜单交互；新的原生核心与验证以 [菜单与稳定视口记录](bd-menu-viewport-1.0.6.md) 为准。下文原生未修改与旧哈希描述是首次音频更新记录。

## 音频与歌词

打开音频直接走音频时钟和解码，跳过 VS 初始化等待；内嵌 attached_pic 封面由 FFmpeg 提取为静态图片、Qt Raster 显示，不创建第二个视频解码路径。禁止自动匹配或手动挂载外部音频，避免同名 FLAC/OPUS 重叠。Tab 显示封面尺寸、音频输入/输出与渲染信息，视频帧号不可编辑。

`PlayerAudioMetadata` 在后台使用便携目录内的 ffprobe/ffmpeg，读取流级和容器级标签。歌词候选支持大小写不敏感的歌词键、MP3 USLT/TXXX 经 FFmpeg 映射的标签，以及确实包含 LRC 时间戳的 COMMENT；普通 COMMENT 不作为歌词。优先使用同名 UTF-8 LRC 的时间歌词，再使用内嵌时间歌词，最后提取 ASS/SubRip/mov_text/WebVTT 等文本字幕轨；纯文本歌词显示为非同步文本，不推测时间点。

LRC 支持 UTF-8 BOM、重复时间戳、毫秒及 offset；不丢第一行，最后一行显示至音频结束。歌词随音频时间轴、跳转与倍速更新，右键“显示字幕”控制可见性；新音频会清空旧封面、歌词及字幕。切回视频继续使用原有字幕和轨道匹配。

## BD 完整列表

读取所有可安全准备的 MPLS，盘内剧集顺序放在前面，再列整卷连续播放、花絮/其它节目及未被播放描述引用的 M2TS。其它节目用原 MPLS 文件名和时长标识；没有元数据的内容不推测正式名称。单独音频递归读取并按目录、自然曲号排序，收在默认折叠的“音频”项中。列表双击和上一/下一文件保留同一顺序。

MyGO 下卷真实目录识别到盘内 6 集、4 个完整播放描述、2 个独立 M2TS、29 个 FLAC。验证剧集出帧、音频分组展开、默认折叠，以及从剧集切到 OST。此处是完整内容浏览；BD-J 交互菜单、加密盘仍沿用已有边界。该盘自动 PGS 加载出现原生字幕错误 -4，视频播放和音频切换通过；本轮未修改原生 PGS 解码链。

## MKS

MKS 是 Matroska 的字幕专用容器，可携带多条字幕、附件和章节，详见 [Matroska FAQ](https://www.matroska.org/faq.html) 与 [章节规范](https://www.matroska.org/technical/chapters.html)。

可以通过“挂载字幕文件”、拖入/打开或视频同名匹配加载 MKS。后台读取字幕轨，右键首/次字幕菜单可选 MKS 内的轨道；普通章节加入时间轴，并提供右键“章节”跳转。切换首/次字幕时保留对应外部轨道选择；卸载恢复源媒体章节。字幕渲染复用原生接口，没有重新封装或修改媒体源文件。

实测用 kit 的 ASS 轨生成 MKS，加两个普通章节；字幕可见像素、章节读取和通过右键跳到 4 秒均通过。有序章节跨文件拼接、交互式菜单和全部字体/位图字幕组合未做完整验收。

## Tab 与验收

增加默认纵向空间，为底部透明度区预留高度。按实际控件文字尺寸计算最小尺寸；透明度标签和百分比使用与正文一致的缩放字体，并随字体宽度重新定位 slider。保留默认 50% 透明度、可见调整手柄及设置窗口优先级。

`Downloads/kit.7z` 独立解压到 build 下，没有改动原包。A–P 共 16 个样本全部识别 12 行，首句 0.50 秒、末句 39.00 秒正确；逐文件在真实 Player 跳到 6 秒均显示第 2 行，包括有封面的 O 和只有 mov_text 字幕的 P。Aegleseeker.flac 实测封面 480×480，音频时钟推进、解码计数正常，视频解码计数为 0，同级同名 OPUS 未被挂载。

验证日志与截图位于 build：`audio-1.0.6-tests.txt`、`audio-cover-tab-1.0.6.png`、`audio-lyrics-1.0.6.png`、`bd-full-playlist-1.0.6.png`。原生核心保持 1.0.5 的已验收补丁版本；本轮改动位于 Qt 播放器层。

最终聚焦回归：9 passed、0 failed、0 skipped。覆盖 16 样本的解析与 GUI 同步、真实封面音频时钟/视频零解码、MyGO 剧集出帧和音频树、MKS 实际字幕与菜单章节跳转、Tab 缩放/透明度/设置优先级，以及既有音频记忆位置和音频专用时钟。

本地 dist 的 Renderer、Player 和 release.json 已更新到 1.0.6，原程序与文档备份到 `build/dist-backup-1.0.6-20261005-105356`。部署前后 27 个配置/VPY/色彩文件哈希一致，FFF.Native.dll SHA-256 保持 `094C19E0658235DAFA0C5B5D46E59C211C9390632C71794AAD45E4DCFE95DE50`。审计结果在 `build/audio-1.0.6-delivery.json`；没有打包或网络发布操作。

````````
<!-- end:docs/audio-bd-mks-1.0.6.md -->

<a id="record-docs-av1-software-decode-1-0-5-md"></a>

## docs/av1-software-decode-1.0.5.md

原始字节数：4222；SHA-256：`cfd048fa44298e28e63c884f8338dbc7e61e6a4ef026473975878ee983f03790`。

<!-- begin:docs/av1-software-decode-1.0.5.md -->
````````markdown
# AV1 软件回退与上传优化（1.0.5，2026-10-05）

## 支持边界

RTX 4070 Laptop 属于 Ada；[NVIDIA NVDEC 官方支持表](https://docs.nvidia.com/video-technologies/video-codec-sdk/13.1/nvdec-application-note/index.html)列出的 AV1 硬解为 Main Profile。实际 1080p48 测试文件为 AV1 High / yuv444p10le，不能通过改成 High 或 Professional 标签获得硬解。HEVC 4:4:4 的支持不能推导为 AV1 4:4:4 支持。未确认 780M 驱动支持 AV1 High 硬解，也未验证 PotPlayer 所谓硬解是否指同一素材的实际解码器。

本轮不重编码、不降低位深或色度、不跳过解码滤镜；保留硬解优先及软件回退。

## 最小改动

- 大于等于 3840×2160 的 libdav1d AV1 软件解码，线程上限由 8 调整为 16，仍受 CPU 逻辑线程数约束。小视频及其它解码器不变。32 线程在合成 8K 对照中更慢且更占内存，未采用。
- CPU 平面 YUV 使用 D3D11 动态纹理、WRITE_DISCARD 逐行上传；保留原位深、行跨度及色度尺寸。硬解 NV12/P010 纹理路径不变。
- 固定 bilinear 路径复用已有格式/色彩专用 shader 缓存，避免通用多算法 shader 的额外成本；Jinc、特效和 360 路径保持各自逻辑。

可重建补丁：`patches/3fp-av1-software-threads.patch`、`patches/3fp-software-frame-upload.patch`，由 `tools/build-3fp.ps1` 应用。测试工具 `tools/perf-3fp.cpp` 可用环境变量 `VSR_3FP_DECODE_MODE=1/2` 强制软件/硬件解码，并记录进程 CPU 时间。

## 真实 780M 测试

7840HS / 780M，真实机械盘素材约 107.94 GB，7680×4320、AV1 Main、yuv420p10le、7001/146 fps；输出 HWND 实际 3840×2160。强制 CPU 解码、跳转 120 秒、预热 5 秒后取约 15 秒稳定段。测试不是 AV1 444 素材，也不是长时热稳定验收。

| 软件回退版本 | D3D11 呈现 fps | Jinc 呈现 fps |
| --- | ---: | ---: |
| 远端原有核心 | 11.07 | 9.47 |
| 本地最新核心 + 16 线程 | 10.93 | 15.32 |
| 加动态上传 | 11.65 | 18.53 |
| 再加固定 bilinear 专用 shader | 18.18 | 18.73 |

远端原有核心较旧，不能把 Jinc 的全部提升归因于本轮改动。最后两项为同一源码上的逐项对照。D3D11 累计呈现等待由约 8.87 秒下降到 0.72 秒，但最新两条路径仍新增约 305/290 个丢帧，视频时间也落后，**未达到 8K48 软件播放目标**。

纯解码 383 帧：8 线程 10.754 秒，16 线程 9.625 秒；包含跳转预滚与启动，不能作为绝对稳态上限。12 帧 framemd5 完全一致。额外试验 max_frame_delay=8/16 分别 9.249/9.700 秒，内存约 3.87/6.65 GiB，对照默认约 2.34 GiB；收益不足，未合入强制缓存设置。

## 本机与精度回归

本机 7940HX / RTX 4070 Laptop，用户已开启增强模式、可释放 120W；本轮未采样 GPU 实际功耗，不能将 120W 写成实测消耗。

真实 GBC 2160p48 AV1 420 10-bit 硬解 → 真正 7680×4320 呈现：Jinc / D3D11 各约 25 秒测量（前 5 秒预热），约 47.95 fps，无新增丢帧。不是完整 GUI、长时或全 HDR 验收。

新增 `softwareUploadAndBilinearMatchReference`：8 组 4:2:0/4:4:4、8/10/16-bit 存储、SDR/PQ，以及有 padding 的输入平面；比较 72 个像素、输出位深及输入未被修改。本机与 780M 最大 RGB 差均为 0。这是有限参考像素回归，不替代完整 HDR/杜比审计。

最新核心通过 VS 仅音频时钟和真实 BD 播放/章节/跳转回归：4 passed、0 failed、0 skipped。BD 跨媒体测试改为检查新源、起始时间及呈现帧；新媒体本来不承诺增加 seek generation，实际 seek 的 generation 断言保留。

## 交付与后续

本地 dist 仅更新 FFF.Native.dll，原文件备份，配置、VPY、缓存及色彩文件保持不变；GUI 沿用已统一的 BD/交互构建。新 DLL SHA256：`094C19E0658235DAFA0C5B5D46E59C211C9390632C71794AAD45E4DCFE95DE50`。

远端只添加隔离诊断文件，没有覆盖 PortableSoft 安装、原核心或配置。本轮没有推送、发行打包、Release 或 Draft。真实 8K48 软解仍需后续解码/上传链路分析；不能用降低色彩精度或仅统计提交帧来宣称完成。

````````
<!-- end:docs/av1-software-decode-1.0.5.md -->

<a id="record-docs-avif-large-yuv444-analysis-md"></a>

## docs/avif-large-yuv444-analysis.md

原始字节数：4237；SHA-256：`65bb7485906f83fbcdb77956616e49c2b44295b9a148c8e38176dd2b2647b9d6`。

<!-- begin:docs/avif-large-yuv444-analysis.md -->
````````markdown
# 巨大 YUV444 与 YUV420 AVIF 的差异分析

日期：2026-10-02。本次仅分析及生成受控对照样本，没有修改播放器、解码器或正式程序。

## 结论与范围

首次生成的YUV444 AVIF损坏来自avifenc 1.4.2的JPEG读入：格式不匹配时先读取整张RGB图，扫描行偏移乘法只有32位，超过4 GiB后回绕。相同巨图通过安全的平面数据路径生成YUV444后，现有播放器正常解码、显示和缩放，证明该样本的问题不是YUV444的固有限制。

该结论针对 `build/supplied-large-grid.avif`。用户的 `TEST.avif` 缺少对应原图和编码命令，不能仅凭相似偏色就判定具有同一根因。正常10位GBR参考样本此前已通过；首次损坏样本CICP为1/13/6，并非identity/GBR。

## 为什么输出444会触发

原JPEG为31603×65278、8位YUV420。`avifJPEGReadCopy()` 要求JPEG的采样格式、位深、范围及矩阵与目标兼容，才能直接复制Y/U/V平面。请求YUV444时格式不匹配，退回 `jpeg_read_scanlines()` 读取RGB，再转换成目标YUV。

源码 `.deps/libavif/libavif-1.4.2/apps/shared/avifjpeg.c` 第1324行选择直接复制或RGB回退，第1372行使用：

```c
uint8_t * pixelRow = &rgb.pixels[row * rgb.rowBytes];
```

`row` 为int，`rgb.rowBytes` 为uint32_t。在当前Windows构建中乘法是32位无符号运算，指针本身为64位并不会改变乘法位宽。宽31603时RGB行长94809字节，整图RGB有6188941902字节（5.764 GiB）。第45302行（从0开始）真实偏移4295037318字节，回绕后只剩70022字节，开始覆盖顶部。

70022模3为2，因此新RGB数据从旧像素的第三字节开始写入，RGB通道相位错开；加上写入的是图像下部的像素，顶部构图和颜色均改变。约第45302行之后的真正尾部没有按正确地址写入，形成黑块或未初始化内容。算术预测边界为图高69.399%，旧AVIF直接像素抽样预览首次近乎全黑的行是454/653，即69.525%，与预测吻合。编码器可以成功压缩这些错误像素，解码器也可以正常解码其码流，所以“解码没有报错”不能证明图像完整。

## 为什么420正常

YUV420与原JPEG采样匹配时可直接读取平面，绕过这张RGB缓冲区。原尺寸8位YUV420的Y平面2062980634字节，两个色度平面各515761478字节，总计3094503590字节；各平面的行偏移都未达到4 GiB。先前修正样本直接提取JPEG的YUV420平面，再用Y4M喂入编码器，亦绕过有问题的JPEG RGB回退。

因此不是“420永远安全、444永远失败”。如果420也因为目标位深、范围或矩阵不匹配而走同一RGB回退，它仍可能触发这个问题。类似大图路径的乘法应在相乘前转为size_t，例如 `rgb.pixels + (size_t)row * rgb.rowBytes`；在结果算完后才强转已经太晚。本次没有改动供应商编码器。

## 受控YUV444实测

使用同一原JPEG，libjpeg-turbo提取YUV420平面，由libyuv上采样色度到444，再写Y4M喂给相同avifenc/aom；不经过上述整图RGB扫描行读入。原图奇数宽度仅补一列，输出尺寸与有效420参考一致，均为31604×65278，8位、8×16 grid、CICP1/13/6。420来源的色度上采样只用于验证巨图444路径，不宣称产生新的原始色彩细节。

| 样本 | 结果 | 解码耗时 | 对原JPG平均RGB误差 | 底部三分之一误差 |
| --- | --- | --- | --- | --- |
| 首次444，经JPEG→RGB回退 | 顶部错色、底部黑块 | 原日志有成功返回 | 不能作为完整参考 | 黑块边界与4 GiB回绕吻合 |
| 修正420，安全YUV平面 | 完整显示 | 6380 ms | 5.85619 | 6.03726 |
| 受控444，安全YUV平面 | 完整显示 | 7996 ms | 5.85093 | 6.02135 |

受控444文件：`build/supplied-large-yuv444-corrected.avif`，287642763字节。日志 `build/avif-444-controlled-encode.txt`、`build/avif-444-controlled-pixels.txt`；原图抽样预览 `build/jpg-original-pixel-sample.png`，受控444预览 `build/avif-444-controlled-pixel-sample.png`。抽样共316×653个位置，并检查真实像素、缩放、平移及图片模式；此次只有实际巨图专项1项功能加初始化/清理3/3通过，没有运行无关回归。

````````
<!-- end:docs/avif-large-yuv444-analysis.md -->

<a id="record-docs-bd-menu-subtitle-languages-1-0-6-md"></a>

## docs/bd-menu-subtitle-languages-1.0.6.md

原始字节数：2980；SHA-256：`eb86a204dca1553d48f9ac8608505640011f82d661080d41d03d1746e53b4be0`。

<!-- begin:docs/bd-menu-subtitle-languages-1.0.6.md -->
````````markdown
# BD 原盘菜单字幕语言验收（1.0.6）

本轮修复用户报告的原盘菜单字幕语言选项错误，保持 1.0.6 本地版。更新 `dist/VS-Renderer-GUI-windows-x64` 的 Player、Renderer 和说明文档；没有推送、打包或发布。

## 根因与修改

原盘 libVLC 菜单正确选择了字幕 PID，但交接到普通播放器时，生成的 `player.ffconcat` 没有声明轨道。FFmpeg 拼接后的字幕流 ID 全为零，Player 无法匹配菜单选择的 PID，因而回退到日语首轨。

`src/bluray/BlurayCatalog.cpp` 的 `playbackInput` 读取各片段 CLPI ProgramInfo，按出现顺序收集轨道 PID，在拼接输入中写入标准 `stream` / `exact_stream_id`，并保留已解析的语言元数据。片段路径、起止时间、章节及原菜单交接流程保持原有逻辑。缓存内容不同会自动重写，重新打开 BD 即使用新输入。

## 实盘验收

素材为本机 MyGO 下卷 BRMM-10775，整卷连续节目 02:22:18.530，包含第 8–13 集。测试真实点击原盘菜单的字幕 ON、语言按钮和 PLAY ALL，等待自动交接后检查选中 PID，并在节目第 3 秒核对实际显示的字幕图像。

| 原盘菜单语言 | PID | 播放器流索引 | 图像对照 |
| --- | --- | --- | --- |
| 日语 | 0x1200 / 4608 | 2 | 一致 |
| 英语 | 0x1201 / 4609 | 3 | 一致 |
| 韩语 | 0x1202 / 4610 | 4 | 一致 |
| 繁体中文 | 0x1203 / 4611 | 5 | 一致 |
| 简体中文 | 0x1204 / 4612 | 6 | 一致 |

对照通过 FFmpeg 从原始 `00000.m2ts` 的对应轨道独立提取前 8 秒 SUP，归零原始 M2TS 的 36600 秒时间起点，再与播放器渲染结果逐像素比较。五种语言均有可见字幕，直接切换轨道的五张图像哈希互不相同。截图留在 `build/bd-menu-subtitle-0.png` 至 `build/bd-menu-subtitle-4.png`。

修复前回归复现英语选择交接后 PID 为零，日志为 `build/bd-menu-subtitle-before-fix.txt`。修复后最终日志为 `build/bd-subtitle-final-acceptance.txt`：**20 passed, 0 failed, 0 skipped**，耗时 71.388 秒，包括初始化与清理。

- 原盘菜单五种语言选择及实际字幕图像对照。
- 播放器直接切换全部五条字幕轨道。
- BD 软件/硬件解码、暂停/播放四种鼠标跳转情形。
- 原盘第 8–13 集菜单选集。
- BD PGS、短节目与原盘菜单交接，以及实际 VapourSynth 滤镜出帧。

本轮验证范围为上述实盘及回归用例，不据此宣称所有光盘结构均已验收。

## 本地部署

Player、Renderer 和测试程序均从修改后的源码重新编译。部署前备份旧文件，部署后核对两份程序与构建产物的 SHA-256 一致，并更新 `local-resource-sha256.json`。审计记录为 `build/bd-menu-subtitle-deployment-audit.json`。

本轮不替换原生 DLL；其 SHA-256 保持 `6753a865a43c9c8a40008d640b573b4ccbba1c5d6efd1cedf30d67ebdda1a792`。便携目录的 INI 与 VPY 同样按部署前后哈希核对保留。

````````
<!-- end:docs/bd-menu-subtitle-languages-1.0.6.md -->

<a id="record-docs-bd-menu-viewport-1-0-6-md"></a>

## docs/bd-menu-viewport-1.0.6.md

原始字节数：6927；SHA-256：`232fe036d911d08931963468985298ff456a13868d7aafe9d93ee5afddcac02f`。

<!-- begin:docs/bd-menu-viewport-1.0.6.md -->
````````markdown
# 1.0.6 BD 菜单与稳定视口

本轮继续更新本地源码与 `dist/VS-Renderer-GUI-windows-x64`，不打包、不推送、不上传。先前音频、歌词、MKS 功能保留。

## 使用

右键“打开 BD 文件夹用于播放”选择完整光盘目录后，可选择“1 - 节目列表模式”或“2 - 光盘菜单交互模式”。节目列表保留剧集、整卷、花絮、其它节目与音频分组；“BD 菜单”按钮可进入原盘菜单。菜单模式直接用鼠标点击原盘按钮，也可用方向键选择、Enter 确认；底部整排菜单导航按钮已移除。右上角“BD 菜单”可直接重新打开菜单。右键音轨与字幕菜单读取正在播放的 libVLC 轨道。

菜单和短节目使用便携 `runtime/vlc` 中的官方 libVLC 3.0.23 运行时；正常剧集继续使用 3FP/VS。菜单模式不运行 VS 增强链，切回节目列表后恢复。运行时准备命令：`tools/stage-bd-menu.ps1 -SourceDirectory <官方 VLC 解压目录> -OutputDirectory build/bd-1.0.5/runtime/vlc`。组件许可和原始源码地址见 [libVLC 说明](libvlc-NOTICE.txt)。

“设置 - 渲染设置 - 划出播放列表/底部控制时不改变渲染分辨率”默认开启。播放列表、底栏、右键及其它弹窗覆盖视频，不改变视频视口；全屏切换与调整窗口尺寸才改变视口。右键只弹出菜单，不强制显示底栏。该选项关闭时保留旧的控件占用布局方式。

Tab 信息面板按基础字体和文本行宽确定最小尺寸，避免上次缩放后的 sizeHint 反过来增大下一次尺寸；最大尺寸限制在显示器可用区域内。透明度默认为 50%，标签、百分比、滑条和调整手柄随面板缩放，设置窗口保持优先。

界面固定文本统一使用英文括号，默认西文字体改为 Comic Sans MS，中文字体保持原设置。旧默认 Segoe UI 自动迁移，自定义字体保留；媒体文件名、原盘图形和源字幕不修改。Jinc 设置补充说明：视频超过显示器分辨率时先缩小，增强滤镜在更高分辨率下开销更大，不先放大再增强。

## 原因与修复

BD 字幕原先将 `bluray:` 地址当作普通磁盘文件送入 SharedFileInput，返回 -4；现在走 FFmpeg Blu-ray 协议。PGS 还需探测节目起始时间，否则字幕时间留在原盘约 36600 秒处，无法对应切片播放的零点。真实字幕像素而非仅成功返回值作为验收依据。

MyGO 下卷的 `00010.m2ts`、`00011.m2ts` 对应 CLPI application_type=5、IG 0x91，是交互菜单图形，不含独立视频/音频。完整列表保留它们并显示具体诊断，禁止作为普通视频双击播放。

上一项/下一项跳过这些菜单图形流，节目与 OST 之间仍可按顺序切换。切换时先解除旧 libVLC 播放器与状态轮询的关联，再在后台停止旧输出，避免等待期间误用旧音频信息探测 MPLS。

三条其它 MPLS 分别含 NCED/尾部短段、9 秒提示片和 1 秒菜单背景。FFmpeg 的 Blu-ray 标题列表过滤不足 180 秒的标题，libVLC 非菜单路径也会过滤短标题。现在为短节目生成缓存内独立 HDMV 播放入口，保留 MPLS 顺序与裁切边界，通过菜单 VM 播放。原盘元数据和大视频保持原样。

真实菜单缓存将 FirstPlay 指向原盘 TopMenu 对象，保留 MovieObject、IG 与全部素材引用，从而直接打开原菜单。首次从空窗口进入菜单时，现在主动切换 PreviewPane 到视频显示层；此前只创建了菜单输出窗口，父级仍停留在占位层，导致黑屏和“BD 菜单”看起来无效。

切换 libVLC 节目时保留旧 HWND，由后台停止并释放旧播放器，完成后删除旧输出窗口；下一节目立即使用新的输出窗口。GUI 无需同步等待旧 vout 退出。软件退出时仍等待全部后台释放任务，同时处理原生窗口消息，避免关闭死锁。

BD 扫描、所有节目准备和音频遍历在后台执行，节目缓存并行生成，命中缓存直接复用。BD 主播放和 PGS 探测限制为 2 MiB/1 秒，减少启动扫描；多线程默认开启，新配置 CPU/GPU 预算默认为 100%，已有自定义预算保留。此优化没有改变软件 YUV 上传、色彩精度或硬解能力。

## 验证与边界

使用完整 MyGO 下卷验证空窗口直接进入菜单、右上角按钮重新打开菜单、鼠标点击原盘 PLAY ALL 进入正片、方向选择、确认、字幕实际像素、三条短节目出帧及关闭/切换；普通剧集保留章节和 PGS。原生鼠标验证适配本机 150% DPI，避免测试脚本将逻辑坐标误当作 Windows 物理坐标。

本轮对实际播放列表双击信号路径测量，分别验证 3FP-HW 和 3FP-SW。本机已有 BD 缓存的结果如下；不据此宣称冷启动或其它存储设备必然快于 VLC/mpv。

| 切换目标 | 双击处理返回 | 起播 |
| --- | --- | --- |
| 普通剧集 | 4–6 ms | 1.25–1.35 s (已播放并呈现 3 帧) |
| OST 音频 | 3–4 ms | 0.18–0.19 s |
| 首次创建 libVLC 的花絮 | 258–263 ms | 0.51–0.52 s |
| 已加载 libVLC 后切换花絮 | 15–16 ms | 0.25 s |

修复前花絮之间切换的同步返回耗时约 1167 ms、起播约 1.47 s；现在旧播放器释放不会挡住新节目。另验证连续快速切换四次，最终出帧且旧输出窗口全部释放。缩小 3FP BD 探测预算的试验未改善剧集起播，已撤回；本轮保留既有原生 DLL 和色彩/解码行为。

稳定视口检查覆盖播放列表固定/取消、底栏隐藏、右键、普通弹窗、全屏往返及实际窗口缩放；Tab 连续 1000 次相同文本刷新保持尺寸不变。音频 16 样本、Aegleseeker 封面、29 首 OST、MKS 章节与字幕、字体与设置持久化继续回归。上一轮记录在 `build/bd-menu-1.0.6-delivery.json`；本轮测试日志为 `build/bd-switch-final-player-tests.txt`，部署和文件哈希为 `build/bd-switch-1.0.6-delivery.json`。

本轮播放器专项回归为 16 passed / 0 failed / 0 skipped，包含空窗口菜单、原生鼠标确认、HW/SW 列表切换及快速连续切换；确认后进入超过 180 秒的正片。原生窗口右键使用实际鼠标事件验收，原生 Tab 消息验证开关信息面板。BD 元数据与预览缩放测试通过。扩大检查发现 `TestVpyScript::anime4kUsesShaderPathAndUniformScale` 旧断言仍期待直接 `open("D:\\Shaders\\Anime4K.glsl")`，当前既有实现通过 `_shader_path` 读取并支持压缩 shader；该测试和生成器在本轮均未修改，保留失败记录 `build/bd-graph-tests.txt`，不宣称整个测试集通过。

当前验证对象是无加密 HDMV 菜单。BD-J 需要可用 Java 环境，尚未验收；加密盘、镜像挂载以及全部其它光盘菜单结构没有在此次测试中验证。

````````
<!-- end:docs/bd-menu-viewport-1.0.6.md -->

<a id="record-docs-bd-remux-1-0-5-md"></a>

## docs/bd-remux-1.0.5.md

原始字节数：6461；SHA-256：`6223af0132b53f9d15e7a827f009ee10fe90e837fbc91fc7f9e351d76d15a68b`。

<!-- begin:docs/bd-remux-1.0.5.md -->
````````markdown
# BD 识别、播放与一键 Remux

2026-10-05，本地 1.0.5 开发记录。适用于已解密 BDMV；不包含 AACS 解密、BD-J 菜单或在线发行。

## 操作入口

Renderer 左侧打开 **BD Remux**。选择光盘根目录、`BDMV` 或包含多卷光盘的上级目录；程序在后台查找 `PLAYLIST/*.mpls`、`STREAM/*.m2ts`、`CLIPINF/*.clpi`。选择“装载光盘镜像”时先调用 Windows `Mount-DiskImage`，再读取返回的盘符根目录。镜像保持装载，使用完可在资源管理器中弹出。

默认按 MPLS 中的播放顺序、PlayItem 起止时间、章节与片段匹配节目。同一条 MPLS 包含多个 15–45 分钟 PlayItem 时，提供各集候选和完整节目；重复播放列表去重，花絮及疑似循环菜单不默认勾选。集号按卷目录自然顺序推断，需核对后编辑名称，不把推断当作官方剧集标题。

勾选需要的节目，双击时长列或点击“播放所选节目”预览。指定输出目录，点击“一键 Remux 所选节目”。默认查找便携目录、PATH 及 `C:\PortableSoft\Mkvtoolnix\mkvmerge.exe`，也可以手动指定。队列使用 MKVToolNix 保留全部音视频、字幕与章节，不重新编码；输出先写 `.partial.mkv`，成功后改为 `.mkv`。工具警告会留在日志，失败或停止保留未完成文件，已有目标不会覆盖。

Player 右键 **打开 BD 文件夹…** 或 **打开 BD 光盘镜像…** 后，直接后台读取节目并加入右侧播放列表，可以双击切换；Player 不打开 Remux 窗口，Remux/模板选择位于 Renderer。BD 通过 3FP 直通播放；MPLS 当前不进入 VPY 补帧链。

## 结构与路径

`BlurayCatalog` 有界读取 MPLS0100/0200/0300，核对长度、引用、片段 ID 和时间范围。播放与导出共用一个节目描述。拆集时在用户本地缓存中写入仅包含所选 PlayItem 的小 MPLS，重映射章节引用；使用 Windows 目录 junction 指向原始 STREAM/CLIPINF，不复制大视频，也不修改光盘源。

通常每盘都有标准 BDMV 目录，发行差异主要体现为“一集一条播放列表”“整卷一条播放列表”“正片与花絮混排”，无需为每个作品硬编码。多角度或音视频副播放路径保留完整原始 MPLS，不自动拆分；交互菜单图形副路径在普通拆集时移除。没有 MPLS 时可手动添加 M2TS，但这种方式没有原始章节和裁切边界。

P: 网络挂载可以读取目录及小元数据，但当前 junction/底层读取不保证这种远程挂载能直接播放。若加载失败，选择“通过 ModelScope Manager 下载 BD”，指定 Manager 程序目录和本地下载目录。支持挂载的 `ARXChem/Animations-List` 路径映射，使用其下载引擎后台下载完整盘；日志为 `bd-download.log`，断点状态为 `.download-state`。完成后重新打开本地目录。脚本也可独立运行，关闭 GUI 不会停止已启动下载。

## 自定义模式与反馈

勾选、命名正确节目后，“保存所选模式”导出 JSON；下一次选择“自定义 JSON 模板”并加载。PlayItem 从 0 编号，`itemCount` 可指定连续多个片段，保留拼接顺序。示例：

```json
{
  "schemaVersion": 1,
  "entries": [
    {"discContains": "Vol1", "playlist": "00000.mpls", "firstItem": 2, "itemCount": 1, "label": "第 3 集"}
  ]
}
```

也可用 `minimumMinutes`、`maximumMinutes`、`firstEpisode`、`splitPlayItems` 调整自动模式。“导出结构反馈”保存节目清单、错误和 MPLS/CLPI 小元数据，最多约 8 MiB，不包含视频；可据反馈分析新的发行布局，再添加模板。不能从结构可靠推断的多角度、伪播放列表或 BD-J 动态顺序仍需人工确认。

## 已验证与边界

- MyGO 上卷：识别七集候选及完整节目；Bloom Into You Vol1：识别三集候选，28:55 特典未默认选择。不是完整 BD 类型覆盖。
- 另通过 Manager 下载 Bloom 全四卷 190 份小元数据，识别全部 13 集候选；这一参考目录只有元数据，不用于视频播放验收。
- MPLS 截断/坏偏移/路径穿越、章节切片、模板、菜单与副路径分支、真实目录和界面均有测试。
- Bloom 第三集已完成真实 MKVToolNix remux：7,541,879,081 字节，AVC 视频、两条日语 PCM 音轨、五个输出章节，时长约 23:42；输出后段通过 FFmpeg 解码检查。
- Player 使用该完整片段的 MPLS 直接播放与跳转验收通过。MPLS 原始章节末尾标记与 MKVToolNix 最终输出章节数可能不同。
- 后续 GUI 按钮驱动第一集实际输出 7,537,297,909 字节，三条轨道与章节保留，`.partial.mkv` 正常转为最终文件。BD 专项测试 9 项通过；第一/第三集播放、跳转、下一集，以及新增 Native 对既有音频时钟的回归通过。
- 本轮参考盘下载遇到 CDN 超时/断连；只有通过大小与 SHA 检查的文件按完整处理。不能把本地部分大文件当作已下载完整盘；缺失/空视频在列表中提示并不默认勾选。
- ISO 装载入口已实现，但未使用真实镜像验收；加密盘、3 分钟以下 MPLS（FFmpeg/libbluray 默认短节目筛选）、多角度选择、BD-J、伪播放列表混淆和远程直读未完成支持验证。短片可手动添加 M2TS 或先 remux。

源码入口：`src/bluray/`、`PlayerPlaylist.cpp`、`PlayerWindow.cpp`。原生 `bluray:` 输入补丁在 `patches/3fp-bluray-input.patch`，由 `tools/build-3fp.ps1` 应用；下载桥接在 `tools/download-bd.py`，没有修改 ModelScope Manager 仓库。

## 本地交付

已覆盖 `dist/VS-Renderer-GUI-windows-x64` 的 Renderer、Player、Native、QtConcurrent 和下载脚本，包含另一会话本轮交互/音频时钟源码；没有推送、打包或发布。19 个 INI/VPY/color.md 文件部署前后 SHA256 相同。旧程序与审计记录在 `build/bd-deploy-backup-20261005`；新 Native SHA256 为 `FF49CFC45B22F073E079E85878E0C19BF23AC2E02263B1FCE375FD2BFC610719`。已有缓存未执行清理。

格式/工具依据：[MKVToolNix 文档](https://mkvtoolnix.download/doc/mkvmerge.html)、[libbluray MPLS 解析](https://raw.githubusercontent.com/ShiftMediaProject/libbluray/master/src/libbluray/bdnav/mpls_parse.c)、[FFmpeg bluray 协议](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavformat/bluray.c)。

````````
<!-- end:docs/bd-remux-1.0.5.md -->

<a id="record-docs-color-management-1-0-4-md"></a>

## docs/color-management-1.0.4.md

原始字节数：9311；SHA-256：`dab7cd949873224732c11e8cd87d24ad15273cea19b55e13fa07883a9df48974`。

<!-- begin:docs/color-management-1.0.4.md -->
````````markdown
# 3FP 色彩管理与 HDR（1.0.4 开发版）

本轮依据主目录 `color.md` 接入可选的 libplacebo D3D11 显示链。默认仍是 3FP 原生 709 / 2020 模式；未选择高级模式时，不加载新的色彩 DLL。版本沿用当前工作区的 1.0.4，没有打包或发布。

## 使用

播放器右键 → 设置 → 解码设置 → **打开3FP 解码配置…**。下方依次是 LAV Video 和 LAV Audio 配置按钮。

色彩引擎可以选择默认原生或自定义 libplacebo。高级模式的初始方案如下：

| 项目 | 默认行为 |
| --- | --- |
| 目标输出 | HDR 源且 Windows HDR 显示器可用时 FP16 scRGB；否则 SDR |
| 色调映射 | 普通 SDR 原值；HDR 使用 Spline；有效 HDR10+ 使用 ST2094-40 |
| 色域映射 | 普通 SDR 相对色度；HDR / 广色域使用感知映射 |
| 质量 | libplacebo 高质量路径，关闭额外 deband、sigmoid 和图像增强 |
| SDR 参考峰值 | 100 nit；HDR 峰值 0 表示使用现有显示器检测 |
| 字幕参考白 | 203 nit，沿用原生 HDR 字幕合成 |
| 峰值检测 | 开启；暂停时复用已映射帧，不重复测量 |
| 显示 ICC | 自动读取当前显示器配置；没有配置时使用标准 SDR 目标 |
| 抖动 | 整数输出末端启用；FP16 scRGB 不重复抖动 |
| 逆色调映射 / 对比恢复 | 关闭 / 0 |

允许手动选择 Spline、ST2094-40、BT.2390、Clip、Linear，设置目标峰值、ICC 或 SDR 输出 `.cube`。ICC 文件支持中文路径。高质量 ICC 使用 65³ LUT；配置和监视器未改变时复用结果。

HDR scRGB 输出跳过 SDR 显示 ICC 和 SDR `.cube`，按显示器报告的 HDR 色域映射。没有适用于 HDR 的通用 Windows SDR ICC 转换，不把它强行套到 HDR 信号。madVR 模式的颜色仍由 madVR 管理，本配置不参与其显示链。

所有配置存入已有便携 INI 的 `color/` 分组，原有加载 / 保存完整 INI 功能包含这些设置。应用色彩配置不重开媒体、不重新执行 VPY，也不改变播放位置和暂停状态。

## 处理链与颜色来源

```text
3FP 解码纹理 ─────────────┐
                         ├→ 原生显示或 libplacebo D3D11 → 原有字幕 / OSD → swap chain
VS 处理后的输出 → 上传 ──┘
```

高级分支导入现有 D3D11 YUV / RGB 纹理，映射到缓存的 GPU 目标，再通过 GPU CopyResource 进入现有合成器。不新增逐帧 GPU→CPU→GPU 回传。VS CPU 帧仍按原桥接方式上传；本分支不会改写其像素、frame props、滤镜顺序、分辨率或帧率。

仅复制解码帧的属性，不持有解码表面。解码帧缺少 CICP 时允许补充同一媒体流的参数和静态 HDR 信息；**VS 输出不补回原始流的 HDR 信息**，防止用户 VPY 已经映射到 SDR 后再次被标为 HDR。缺失字段在诊断中保留 unknown，并在显示副本中推断，不回写源帧。

内置脚本生成器的必要 RGB↔YUV 转换改用当前 clip 的 matrix / primaries，709 主流输入仍得到 709，2020 输入不会被硬编码为 709。手写或已经保存的 VPY 不自动重写；改变 transfer / primaries 的用户滤镜仍需要正确设置自身输出 props。

原生路径修正了 BT.2020 constant-luminance：矩阵 10 使用其分段色度公式，矩阵 9 仍是 NCL；709 和既有 NCL 路径不变。

## HDR 与降级边界

- HDR10 / HLG：使用原有显示器检测和 scRGB 交换链，libplacebo 执行目标感知的色调 / 色域映射。API 360 的线性 RGB 单位是 203 nit，末端转换为 Windows scRGB 的 80 nit 单位；不能把两者直接等同。
- HDR10+：使用 FFmpeg 已解析的 `AVDynamicHDRPlus`，映射 MaxSCL、统计、knee 和 Bezier anchors，自动选择 ST2094-40。库当前采用第一窗口；未宣称覆盖多窗口和全部饱和度调整语义。测试确认改变 knee / anchor 会改变输出，而非只显示识别标志。
- Dolby Vision：有效的 FFmpeg decoded metadata 可以交给 libplacebo 做不依赖增强层的 reshaping；要求仍保留原始编码分量。已转换为普通 RGB、缺失 / 不合法元数据或依赖 EL 的情况回到底层颜色。**不重建 FEL，也不宣称完整 Dolby 授权认证。**
- HDR Vivid：识别 side data 并显示提示，目前仅映射其 PQ / HLG 底层；动态 Vivid 曲线未实现。
- 错误 ICC、LUT、缺少 DLL 或设备初始化失败：继续原生显示，Tab 显示原因。360 投影和音频封面继续现有特殊合成器，并报告原生路径。
- 高级模式的缩放由 libplacebo 执行：Nearest、Bilinear、Bicubic、Lanczos3/4、Spline36、Jinc。原生 D3D11 缩放请求在该模式使用 Bilinear，Super-XBR 请求使用 Jinc；这些只在高级显示分支发生，不变更 VS 处理链。抗振铃由 libplacebo 的滤波器能力决定。

`color.md` 的后续扩展尚未全部完成：JPEG Ultra HDR / gain-map 编解码、完整 HDR10+ 多窗口、Vivid 动态曲线、FEL、全部 source/display/HDR 截图格式与增强像素探针，以及与 mpv 的逐像素自动对照。本轮未修改图片编辑器的 gain-map 或 HDR 导出功能。

## 信息与验证

Tab 显示请求 / 实际引擎、源 matrix / primaries / transfer / range / chroma / 位深、来自解码还是 VS、显式字段和流补充字段、实际 tone / gamut / dither、HDR 元数据状态、ICC / LUT、输出位深 / HDR、目标峰值与回退原因。`renderSubmitMs` 是 CPU 提交耗时，不能当作 GPU 执行时间或完整帧时延。

独立构建目录为 `build/color-1.0.4`。Qt 色彩专项、离屏 HDR 元数据专项和播放器配置专项分别验证：

- 默认原生、暂停切换、暂停色彩缓存与尺寸改变后重新映射。
- BT.2020 CL 与 NCL 不混淆；PQ / HLG 输出有限且可读取。
- sRGB ICC、中文文件名、identity `.cube`、RGB 红蓝通道顺序；错误配置回退后可恢复。
- 硬解模式实际启用，既有硬件回传和软件转换统计均为 0。
- 真实 VS 输出矩阵 / transfer 和像素保持不变；配置应用不重新加载 VS 脚本。
- 203 nit PQ 白色经 FP16 离屏输出为 scRGB **2.5352**，期望 **2.5375**；只证明单位和离屏处理正确，不代替真实 HDR 面板观感验证。
- 畸形 HDR10+ / 截断 DV 元数据不使处理崩溃，不改写源 side data。
- 合成残差关闭的 DV metadata 实际进入 reshaping shader；残差依赖打开后回退。该测试不替代商用 DV 样片颜色验收。
- 16bit 常量量化到 8bit，抖动关闭得到 1 个级别、平均 124；打开得到 2 个级别、平均 123.5156，理论 123.5175，无明显均值漂移。

已有脚本测试的一项旧断言仍失败：`anime4kUsesShaderPathAndUniformScale` 期待 `open("固定路径")`，但当前着色器归档机制改为 `_shader_path` 变量再读取，与本轮矩阵更改无关。本轮保留这个不匹配并记录，未以“全量测试通过”替代专项结果。VS 桥接专项为 10/10，通过实际脚本执行另验证了生成器的 709 / 2020 矩阵选择与 PQ props 保留。

详细结果与 UI 截图保存在本机构建目录。真实 HDR 面板、跨显示器 / HDR 开关、商用 DV / Vivid 样片及长期性能尚需实机验证；本轮不报告 mpv 等效质量或性能倍数。

## 依赖与重建

本地已有 vs-placebo 是 Vulkan VS 插件，无法导出 D3D11 入口，因此新增固定版本二进制 SDK：libplacebo **7.360.1-2**、LittleCMS **2.19.1**，其余运行时依赖按导入表取闭包。下载地址和 SHA-256 在 `third_party/color/packages.json`，没有克隆上游工程。

```powershell
./tools/prepare-color.ps1
./tools/build-3fp.ps1 -FffProject .deps/fff-player -OutputDirectory build/mingw-release
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
```

`patches/3fp-color-management.patch` 只包含本轮 3FP 修改；`build-3fp.ps1` 在旧补丁之后应用，并复制权威 C ABI / loader 头文件。没有 SDK 时 GUI 和原生回退仍可构建，高级模式报告依赖缺失。

`color/` 为可替换的独立 DLL 目录。MSYS2 库使用较新 GCC，与 Qt 的旧 MinGW runtime 不能混放；staging 将本闭包 DLL 文件名 / 导入名隔离为 `v3_` 前缀，未修改可执行代码。原始 / 部署哈希和修改说明在 `color/runtime.json`，上游许可证在 `color/licenses/`，本轮不会覆盖现有 FFmpeg、VS 或 Qt runtime。

上游说明：[libplacebo](https://github.com/haasn/libplacebo)、[renderer](https://libplacebo.org/renderer/)、[MSYS2 固定包来源](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-libplacebo)。

## 本机交付

2026-10-03 更新到 `dist/VS-Renderer-GUI-windows-x64/`，沿用 1.0.4 开发版。合并使用“开发 VapourSynth 实时渲染器”窗口完成字幕回归后的最终 3FP DLL；色彩 8/8、配置 / 语言 / 播放状态 6/6 和离屏 HDR 专项再次通过。部署后指定便携 DLL 的切换 / 回退 smoke 为 4/4。

部署更新 37 个文件，其余 509 个既有文件保持 SHA-256；INI、VPY 和缓存未被部署替换。备份在 `build/color-backup-20261003-220118`，更新哈希和边界记录在 `build/color-deployment-audit.json`。本轮未打包、推送 Git 或发布。

````````
<!-- end:docs/color-management-1.0.4.md -->

<a id="record-docs-components-player-1-0-5-md"></a>

## docs/components-player-1.0.5.md

原始字节数：5472；SHA-256：`b609ea604f5d27612bee434354afdbf44d33bd20990a3d63b7a127cfdbf1b809`。

<!-- begin:docs/components-player-1.0.5.md -->
````````markdown
# Player 播放入口与 Full 组件下载

2026-10-05，1.0.5 Full 本地交付及发行草稿记录。

## BD 与播放器界面

Player 右键“打开 BD 文件夹”直接后台读取 MPLS，选择完整可用的剧集候选，加入右侧播放列表并按自动播放设置打开首集，不显示 Remux 页面。光盘镜像入口先装载再读取根目录；轨道和字幕仍由右键菜单切换。Renderer 的 BD Remux、模板和结构反馈功能保留。

未打开文件时，底部显示可点击的 `3FP-HW` / `3FP-SW`；打开文件后显示实际播放路径。Tab 面板用统一透明背景绘制，左侧“透明度”、右侧百分比，默认 50%；边框与右下角斜线调整手柄保持可见。设置窗口激活时信息面板隐藏，返回播放器恢复，不遮挡设置。

## 组件下载

Player 设置左侧新增“组件下载”，原基本设置的更新按钮迁移到该页面。树形分类展示软件本体、FFmpeg、MKVToolNix、MadVR、LAV-Filters、aria2-next；展开后只展示该组件目录内的 Windows 二进制程序包（7z/zip/exe），排除源码、网页、说明、签名和安装器。列表显示远端版本、本地版本与大小，保留旧版本。

来源为 `ARXChem/Software-List` 的对应目录，API 按目录递归分页读取。aria2-next 可从版本子目录识别版本；普通包按名称版本排序，FFmpeg 当前按包名构建日期，`madVR09217` 识别为 `0.92.17`。开启“自动下载最新版本”会刷新目录，只下载比本地更高的版本；本地组件不存在时仍可下载。读取实际 EXE/AX 版本，FFmpeg 日期版本由 `components.json` 安装记录维护。

当前仓库实测有 FFmpeg、MKVToolNix、`MadVR` 和 `LAV-Filters` 程序包。aria2-next 当前公开子目录只有源码包；页面不会拿源码包替代 EXE。本 Full 包已包含 aria2-next 2.8.3。

| 组件 | 固定安装位置 | 验证入口 |
| --- | --- | --- |
| 软件本体 | 完整便携目录 | release.json、Renderer/Player 与核心依赖 |
| FFmpeg | 程序根目录，CLI 与原生 DLL 共享 | ffmpeg.exe |
| MKVToolNix | runtime/mkvtoolnix | mkvmerge.exe |
| MadVR | madVR09217 | madVR64.ax、madHcCtrl.exe |
| LAV-Filters | LAVFilters64 | LAVVideo.ax、LAVAudio.ax、LAVSplitter.ax |
| aria2-next | runtime/tools | aria2-next.exe |

下载缓存位于用户 CacheLocation 下的 `components`。下载到 `.partial`，按源目录的大小与 SHA-256 校验后转为最终缓存；解压前拒绝路径穿越、链接及不安全名称，解压后递归定位唯一完整的 x64 主程序目录。包装目录名称或层数改变不会改变安装位置；定位不唯一时停止安装。

点击“应用组件并重启”，退出后由 `apply-components.ps1` 按固定目录替换对应文件。要求另一个 Renderer/Player 窗口已关闭；保留 INI、VPY、madVR settings.bin 和更新工具，其余旧文件备份到组件缓存，出错回滚已修改文件。组件安装版本写入 `components.json`。清理缓存只删除下载包及断点文件，保留已安装组件、解压暂存和备份。

旧版本本体可以下载，完成后再次提示“不会降级更新，包已经下载到 …”。“安装已下载的新版本本体”仅接受高于当前版本的完整包，复用既有便携更新器；其它组件允许用户手动下载并安装旧版。新软件本体更新和组件替换均需退出后应用。

## 验收

- Player：BD 直接加入播放列表并出帧、章节、空窗口解码切换、Tab 背景 alpha/底部一致性/百分比/调整大小、设置窗口优先级，以及既有音频时钟聚焦测试通过。
- 更新：数字/日期版本排序、源码过滤、MadVR 压缩版本号、x64 嵌套与重复 EXE 目录识别、固定安装目录、配置保留、禁止组件接口安装软件本体、原完整更新下载/哈希/取消/目录替换回归通过。
- 实际 ModelScope MKVToolNix 102.0 GUI 下载与 SHA/解压通过，暂存后的 mkvmerge --version 返回 102.0；没有将测试包安装覆盖用户其它软件目录。
- Full 包使用验证过的 MKVToolNix 102.0 源包，去除其中 INI 与缓存。现有性能核心保持 `094C19E0658235DAFA0C5B5D46E59C211C9390632C71794AAD45E4DCFE95DE50`；8K48 CPU 回退仍未达到实时目标。

核心入口为 `src/update/ComponentDownloads.*`、`tools/apply-components.ps1`、`tools/stage-mkvtoolnix.ps1`。发布归档排除个人数据、缓存、下载包、日志、测试程序及 AWJ 更新状态；发行附件由维护者上传。

## Full 归档验收

2026-10-05 最终聚焦 Player 回归 5 passed / 0 failed，更新回归 11 passed / 0 failed / 2 个可选实包用例 skipped；发行包格式专项 3 passed / 0 failed。此前真实 MKVToolNix 下载用例已单独通过。

部署前后 27 个配置、VPY 与色彩文件哈希一致。`dist/1.0.5.7z` 为 259894812 字节，7z 完整性、独立解压、682 个资源清单条目逐文件哈希核对通过；包内共 683 文件，保留 MKVToolNix 资源和许可，排除个人与测试状态。清空开发 Qt 插件路径后，解压目录的 Renderer 与 Player 均创建真实 Qt 主窗口并正常关闭，包内 mkvmerge 返回 102.0。

SHA-256：`f2c80dd377e667b5cc64d3a1801219d0d15ec5a5fbbef4bd4f4a93b74a7e7c9d`。旧归档与旧暂存目录已改名保留。归档后的交付记录保存在源码文档中，不修改已审计的发行包。

````````
<!-- end:docs/components-player-1.0.5.md -->

<a id="record-docs-dependencies-md"></a>

## docs/dependencies.md

原始字节数：7417；SHA-256：`7d1b48c18928a2850f1ff3eeadff021481ec2430183900ef4cf92c13863a390e`。

<!-- begin:docs/dependencies.md -->
````````markdown
# 依赖安装与构建

## 普通用户

下载 `VS-Renderer-GUI-1.0.1-windows-x64.7z` 后直接解压并运行 `VSRenderer.exe`。正式包已包含 Qt、MinGW runtime、Python、VapourSynth、vs-placebo、内置 VS 插件、3FP，以及隔离部署在 `runtime\ffmpeg` 的导出 CLI，不读取系统中的 Python/VapourSynth 安装。

系统要求：Windows 10 22H2 或更新的 64 位 Windows，支持 Direct3D 11 的显卡与驱动。

## 源码构建依赖

可选高级色彩 SDK 由 `tools/prepare-color.ps1` 根据 `third_party/color/packages.json` 下载 / 校验固定二进制包。已经准备好的 `.deps/color` 优先复用；无需克隆 libplacebo。构建自动产生 `color/` 独立 DLL 目录，便携部署携带许可证和哈希；不替换 FFmpeg / VS / Qt DLL。3FP 重建使用新增 `3fp-color-management.patch` 和 `src/color/` 头文件。用法及边界见 [色彩依赖说明](color-management-1.0.4.md)。

1.0.4图片转换使用AWJimage 1.1.0独立Windows CLI（AVX2）。`tools/awj-runtime.cmake`自动下载并校验原始发布包和对应源码，构建后保存到 `runtime/awj`，便携部署同时携带许可证与NOTICE。图片查看与基础操作不依赖AWJ。来源、采样与算法边界见 `player-image-tools-1.0.4.md`。

本轮便携LAV运行时采用 `LAVFilters-0.83-x64.zip`，解压到 `.deps/lav/0.83` 后由 `tools/stage-portable.ps1` 默认携带；也可以通过 `-LavDirectory` 指定准备好的解码器目录。三个AX为0.83.0，完整依赖和GPL `COPYING` 一起分发，不运行注册批处理。源包SHA-256：`0126982f47157bb86a6dbb43c4f332f7f98beba9ad552c19b65f9db2e7d4f186`。

补帧新增 `vapoursynth-mvtools==29`，由 `build-vapoursynth.ps1` 固定安装，staging 携带 `plugins/mvtools.dll` 与 GPL-2.0 许可。不要使用 VSRepo 的旧 v24/API 3 版本；R80 已不能加载。现有开发环境可先运行 `.deps\vs-python\Scripts\python.exe -m pip install vapoursynth-mvtools==29`，再重跑 staging 或链接 GUI。

- Git、CMake 3.25+、Ninja、7-Zip。
- Qt 6.8+；官方构建使用 Qt 6.10.2 MinGW 64-bit 与配套 MinGW。
- Python 3.12+。
- 带所需编码器的 FFmpeg CLI；发布脚本通过 `-FfmpegExecutable` 指定，并把其 DLL 隔离复制到 `runtime\ffmpeg`。

1.0.4本地使用维护者提供的 `ffmpeg-autokey444-full.7z` 增强版。解压到 `.deps/ffmpeg-enhanced` 后运行 `tools/stage-ffmpeg.ps1`，同步CLI与3FP的FFmpeg共享库；保留原来的FFF.Native、LAV和VS插件。便携部署自动采用构建目录 `runtime/ffmpeg`，旧发布脚本也优先从这里读取。构建为x86-64-v3，具体编译配置、来源哈希和验证见 [增强版FFmpeg记录](ffmpeg-enhanced-1.0.4.md)。
- Visual Studio 2022 的“使用 C++ 的桌面开发”工作负载，用于构建 VapourSynth 和 3FP。

1.0.4后续体积优化改用根目录ffmpeg.exe与共享DLL，runtime/ffmpeg仅保存来源记录；发布脚本优先读取根目录CLI。stage-shaders.ps1压缩GLSL库，trim-portable.ps1裁剪实际未使用的副本；以 [最新布局](tracks-size-1.0.4.md) 为准。更新库源码后需重新部署资源库，首次无归档的源码构建仍复制普通Shader用于开发。

先设置 Qt 与编译器环境；路径按本机 Qt 安装位置调整：

```powershell
$env:QT_ROOT = "C:\Qt\6.10.2\mingw_64"
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;$env:PATH"
```

构建 VapourSynth 及 GUI 使用的 VS 插件。脚本会在本地创建 `.deps`、自动克隆固定的 VapourSynth 源码目录并安装插件；这些依赖不会进入 Git：

```powershell
.\tools\build-vapoursynth.ps1 -PythonVersion 3.12 -SevenZip "C:\Program Files\7-Zip\7z.exe"
```

脚本固定安装 `vs-placebo==2.0.4`，其 Windows wheel 提供 `libvs_placebo.dll`。Anime4K shader 文件本身不复制进仓库或便携运行时；GUI 默认读取 `C:\PortableSoft\FFmpegFreeUI ReadyToRun x64\libplacebo`，也可在节点参数中浏览任意兼容 `.glsl`。

构建 API 14 版 3FP：

```powershell
git clone https://github.com/Lake1059/FFF_Project.git .deps\FFF_Project
git -C .deps\FFF_Project checkout ee2bfde51a8f85ac156a2253845d5dcb4a07df09
.\tools\build-3fp.ps1 -FffProject .deps\FFF_Project -OutputDirectory build\mingw-release
```

最后构建和测试 GUI：

```powershell
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release
```

构建正式包时显式提供与其 DLL 同目录的 FFmpeg CLI，避免把导出进程依赖与根目录的 3FP FFmpeg ABI 混用：

```powershell
.\tools\package-release.ps1 -Version 1.0.1 `
  -FfmpegExecutable "C:\path\to\ffmpeg.exe" `
  -SevenZip "C:\path\to\7z.exe" `
  -WinDeployQt "$env:QT_ROOT\bin\windeployqt.exe"
```

VapourSynth 固定参考提交为 `5b2d5562726a91d9a75441cc4728a90e6c9f4f27`。3FP 补丁基于 `ee2bfde51a8f85ac156a2253845d5dcb4a07df09`；若上游接口变化导致补丁无法应用，请先 checkout 该提交。


### 多路分析构建补丁与着色器缓存

`tools/build-3fp.ps1` 还应用 `3fp-performance-chroma.patch`：共享 D3D 字节码、非阻塞状态读取/首帧重绘、独立色度核与 Spline36/Super-XBR 单阶段。它与当前 GUI 的缩放参数编码配套，不可只替换 EXE。

Release 构建后运行 `vsr_frame_bridge_tests.exe startupWarmsDecodeAndBothRenderers`，生成输出目录中的 `shader-cache/*.cso`。编译缓存缺失时首次测试会较慢，后续新进程直接读取字节码。发布必须一起复制 `shader-cache`，打包脚本会检查此目录存在；Super-XBR 原始源文件/许可证从 `third_party/shaders` 部署到 `shader-licenses`。


### RIFE / 分析导出扩展（2026-09-30）

`tools/install-analysis-plugins.ps1` 安装 Descale 并下载固定 r9_mod_v33 RIFE Vulkan DLL，校验 SHA-256，再从 `-ModelDirectory` 拷贝已转换的 `rife-v4.26` 与 `rife-v4.26-heavy` 目录（每个包含 flownet.param / flownet.bin）。默认采用用户的 FFmpegFreeUI VideoEnhancer 模型目录；模型清单见 third_party/rife/runtime-sha256.json。stage 脚本随后将这些文件放入便携运行时，发布脚本附带许可证。PKL 不能替代 NCNN 文件。

14 个新增节点其余使用已固定的 Zsmooth / VSZip；不部署 VSRepo 返回的 API 3 旧二进制。源码依赖安装需要模型目录，最终便携使用无需下载。RIFE Vulkan 不要求 CUDA/TensorRT，需支持 Vulkan 的显卡驱动。

## 1.0.2 本地 Player

CMake 生成 VSRenderer.exe 与 vs-player.exe；播放器额外链接 Windows DirectShow / COM / DXGI / PSAPI / PDH / GDI 系统库，无新增 Qt 模块。3FP 构建增加 patches/3fp-player-rate.patch、patches/3fp-player-output.patch，API 14 版本兼容，新增 optional SetPlaybackRate / SetExternalOutputFormat 导出，字幕和位图读取复用已有 C ABI。tools/stage-portable.ps1 只更新本地目录，不调用归档或 Git 工具，首次复制旧便携依赖，重复执行保留用户 VPY / INI。默认复制用户指定的 Anime4K / LAV / madVR 本机资源；madVR 通过随附 ax / DLL 实例化，不要求注册系统。VPY 位于应用旁 vpy；播放器所有设置位于 player.ini。PNG 源截图及嵌入 SRT 提取使用 runtime/ffmpeg/ffmpeg.exe，源码测试目录也需部署此 CLI。

````````
<!-- end:docs/dependencies.md -->

<a id="record-docs-development-1-0-9-status-md"></a>

## docs/development-1.0.9-status.md

原始字节数：5702；SHA-256：`90cd5adf37311ec034f65938ee0bebb5c7cb506d5152c8dc1b70db7b93ec30ad`。

<!-- begin:docs/development-1.0.9-status.md -->
````````markdown
# VS-Renderer-GUI 1.0.9 开发与交接状态

更新时间：2026-10-10，NuxBox1，UTC+8。当前为本地测试构建，仍有未通过的性能项，不能作为全部验收完成的正式版本。

本日后续 D 盘 AV1 yuv444p12 专项与 VS Cap 黑屏修复见 [专项修复与交接记录](fix-av1-vscap-1.0.9-20261010.md)。版本保持 1.0.9；本轮只测试指定 D 盘素材。最新 Full/Lite 已重构建，Cap 非黑真实桌面及工作流验收通过；实际 GUI 开字幕仍未达零丢帧。下方 F 盘和启动数据保留为前轮历史，不能视作本轮新增测试。

## 本轮已完成

- GUI 基线 `user-Wing/VS-Renderer-GUI@5743c1a01208aae28727426bd0eb68d0e0b89af2`；Native 基线 `Lake1059/FFF_Project@20bcc001b24f6b6de4575bddf9b93ec452795b0d`。本轮再次核对 GitHub 默认分支，仍为这两个提交。含上游 #18 空间音频/缩放、#16 HDR10+、#15 Native 上传更新；空间音频尚未实际素材验收。
- `C:\BuildTools\2026` 的 v145 / MSVC 14.51.36231 已构建 Native API 18；Qt 6.10.2 MinGW、QtTest、Qt SVG/工具组件可用。Qt EXE 与 MSVC DLL 通过 C ABI 通信。
- API 18 适配及旧完整版接口移植：外部 VS 帧、Jinc/高级缩放、倍速、EQ/延迟、独立音频时钟、CPU 强制预缩放、HDR 浮点回读和高级色彩桥。保留上游新 HDR 渲染路径；D3D11 档绕过 VS，当前采用原生 D3D11 shader 管线，不能误称已经使用 VideoProcessor。
- 修复连续迟到造成全片无画面：仅有较新已解码帧可替换时才丢弃迟到帧；无音轨软解允许重同步。AAC 采用有界压缩包预读，但 8K 软解仍有欠载，未彻底解决。
- VS Cap 的 DXGI FP16 真实 HDR 捕获、SDR 回退、区域注释工具、剪贴板、普通置顶贴图、热键/冲突检测/托盘设置、自启及后台 AVIF CRF18 已实现。真实 HDR 设备、PNG 精度、剪贴板与 AVIF 工作流已测试。
- Lite 排除 VS/LAV/libVLC 的实际后端源文件；仅 Jinc/D3D11、软硬解；不自动关联文件。原生 libbluray 导航适配器已接入，缺少真实蓝光素材，HDMV/BD-J 尚未实盘验收；BD-J 需要外部 Java。
- `tools/build-1.0.9.ps1` 一次构建四个程序并生成 `dist\Full`、`dist\PlayerLite`。API 18 使用单独完整补丁，避免误套历史 API 14 补丁。

## 本地产物

| 目录 | 入口 | 体积 |
| --- | --- | --- |
| `dist\Full` | `VSRenderer.exe`、`vs-player.exe`、`vs-cap.exe` | 1008.49 MiB，含完整 VS/Python/组件运行时 |
| `dist\PlayerLite` | `vs-player.exe` | 186.07 MiB，保留完整 FFmpeg |

Lite 扣除 FFmpeg 专属依赖后为 **51,964,627 字节（49.56 MiB / 51.96 MB）**。达到先前文档 50 MiB 口径，尚未达到原始提示词严格十进制 50 MB；总包 100 MB 目标尚未达到，需要专门裁剪 FFmpeg。`release.json` 列出扣除的文件；Native 直接依赖的字幕/字体/蓝光库，以及共享运行库仍计入播放器体积。

旧 `C:\PortableSoft\VS-Renderer-GUI` 和 1.0.5 工作树保留；本轮未覆盖旧安装、提交 Git 或推送。GUI 修改在本项目，Native 修改在 `C:\Private\FFF_Project-1.0.9`；完整内核补丁为 `patches/3fp-api18-vsrenderer-1.0.9.patch`。源码归档用于手动回传，不含媒体、dist、工具链或大体积依赖缓存。

## 实测与未通过项目

详细数字见 [本轮验收记录](validation-1.0.9-20261010.md)，原始日志与逐秒 CSV 在 `build\session-20261010`。

- 本地 3840×2160 / 60 Hz、HDR 已开启；F 盘 4K48、8K48 AV1/AAC 原片在硬解 D3D11 的 60 秒计时段均零丢帧、零合并、零音频欠载增量。**4K 本次启动预热阶段丢 6 帧；8K 计时开始有 0.74 秒追帧偏差，之后恢复。全程零丢帧、启动即同步尚未验收通过。**
- 8K48 软解：自适应上传关为 22.23 fps，开为 23.58 fps；CPU 中位数 31.77%→18.82%、GPU 中位数 40.30%→14.64%。该策略包含上游自适应预缩放，不是同画质提升；仍低于实时 47.952 fps，且 AAC 欠载。
- 500 ms 音频储备实验无改善，已撤回。旧内核同素材软解报 `Invalid data found when processing input`，不能给出可信的新旧 FPS 提升倍数。
- 主窗口启动中位数：旧 GUI 896 ms、新 GUI 912 ms；旧播放器 593 ms、新 Full Player 581 ms、Lite 581 ms。播放器约改善 2%，GUI 无显著提升；窗口时间与视频首帧分开统计。
- 真 HDR：3840×2160 FP16 scRGB 采集，内存为 RGBA32F。PNG 为 BT.2020/PQ RGBA16 无损压缩，**转换量化并不是 FP16 像素逐位无损**；原始浮点通过 `.png.scrgb-f32` 和 JSON 伴随文件保留。AVIF 是有损 10-bit 4:4:4 BT.2020/PQ/full、CRF18。
- Lite 独立目录软解、模式白名单、VS/libVLC 不加载测试通过；Cap 发行目录剪贴板/贴图/真实 AVIF 通过。Full 纯系统 PATH 的 13 项发行目录回归通过；中文路径的 RIFE 模型加载失败，故发行目录使用英文名，完整安装路径需纯英文。真实 GUI 发现内嵌 ASS 全片扫描，已补回有界读取，正在复测。

## 后续优先顺序

1. 修复硬解启动丢帧/追帧；使用含 AAC/字幕的实际 GUI 路径重复测试全程计数。
2. 解决 8K 软解解码/渲染阻塞和 AAC 供给；不能扩大缓冲或放宽计数冒充修复。
3. 完成严格 50 MB 非 FFmpeg 体积目标及精简 FFmpeg 总包目标。
4. 有素材后验收 HDMV/BD-J、IAMF/空间音频；人工检查区域标注、热键冲突、保存路径和自启全流程。
5. 使用单独测试目录，保留用户播放器进程与配置；失败日志保留，不用编译成功代替产品验收。

````````
<!-- end:docs/development-1.0.9-status.md -->

<a id="record-docs-ffmpeg-enhanced-1-0-4-md"></a>

## docs/ffmpeg-enhanced-1.0.4.md

原始字节数：3347；SHA-256：`d01c0382ea8554fef840e882262b8b6fc3a23170a0311f8dce1eafaada601470`。

<!-- begin:docs/ffmpeg-enhanced-1.0.4.md -->
````````markdown
# 1.0.4 本地增强版 FFmpeg

后续体积优化已将ffmpeg/ffprobe/ffplay移至程序根目录共用3FP的DLL，runtime/ffmpeg仅保留README/build-info；最新布局见tracks-size-1.0.4.md。下面的隔离部署是首次替换时的历史记录。

来源：维护者提供的 `C:/Users/ARXChem/Documents/Open Source Projects/ffmpeg-autokey444-full.7z`，2026-10-03 构建，FFmpeg 8.0.git，clang 23.1.2，x86-64-v3。归档SHA-256：`b56bcecdb5de1e768f14b2e18564dd052446b73e36af8903b043561c86684811`。提取后的原始文件及逐文件哈希保存在 `ffmpeg-enhanced-runtime.json`。

## 部署

完整原始包保存在本机 `.deps/ffmpeg-enhanced/04-full-expanded`。运行 `tools/stage-ffmpeg.ps1` 更新以下两处：

- `build/mingw-release/runtime/ffmpeg` 和对应构建目录旁的FFmpeg共享库/依赖。
- `dist/VS-Renderer-GUI-windows-x64/runtime/ffmpeg` 和对应程序目录旁的FFmpeg共享库/依赖。

独立CLI目录整体更新，保留ffmpeg/ffprobe/ffplay、全部配套DLL、上游README和build-info。原生3FP使用同主版本的avcodec63/avformat63/avutil61/swscale10/swresample7等更新库；未替换FFF.Native.dll、LAV、VapourSynth源插件和用户配置。VS插件自身的内置编解码器不会因这次替换自动改变。

旧CLI目录与被替换的原生依赖保存在 `build/ffmpeg-backups/20261003-111635-042`。便携部署从构建运行时同步；旧发布脚本优先采用构建目录 `runtime/ffmpeg/ffmpeg.exe` 并携带构建记录。此轮没有重新构建EXE、制作归档或推送代码。

## 能力与边界

实际枚举确认AVS3编码 `libuavs3e` 和解码 `libuavs3d`；还有 `libzenrav1e`、`librav1e`、`libopenh264`、`libkvazaar`、`liboapv` 等。导出自定义命令可指定编码器，不额外增加本轮未要求的导出预设。上游构建保留旧编解码器，实际完整列表随包 `build-info/encoders.txt` 和 `decoders.txt`。

上游描述AVS3接入输出10bit；SVT采用修复后的simple分支，支持YUV444P10和自动关键帧，默认scd=1。上述来源与构建补丁保留在build-info；未对所有旧编码器或GPU编解码做运行验证。

实际构建包含 `--enable-gpl --enable-version3 --enable-nonfree`；不能将它标记为纯LGPL或自由可再分发版本。来源清单与构建记录不等于完整对应源码，本轮仅本地使用。

## 验证

- libuavs3e实际编码128×128四帧短片，libuavs3d完整解码四帧；ffprobe确认AVS3、尺寸和帧数，帧哈希见 `build/ffmpeg-enhanced-avs3-frames.md5`。
- `enhancedFfmpegAvs3Playback`等待原生异步加载完成后实际播放，并检查呈现帧数与尺寸。
- 新库下原生Lanczos4呈现、音频均衡器与声道策略、后台图片输出、图片缩放含Anime4K输出均通过。只运行五项相关用例，含Qt初始化/清理7/7；日志 `build/ffmpeg-enhanced-final-tests.txt`。
- 两处CLI启动与PNG输出通过；部署文件哈希与原始包一致。FFF.Native和两个EXE哈希未变；部署后即时检查player.ini未变，最终复核时便携player.ini已变化，保留其当前内容，没有回写旧配置。PowerShell部署/发布脚本语法检查通过。核对见 `build/ffmpeg-enhanced-delivery-audit.json`。

没有用当前机器负载下的耗时作性能结论；未运行无关全量回归。

````````
<!-- end:docs/ffmpeg-enhanced-1.0.4.md -->

<a id="record-docs-filter-coverage-md"></a>

## docs/filter-coverage.md

原始字节数：4794；SHA-256：`3b756ff1847f418a7c42b8ca81cf2cbdc5653e4c61f16b7ee7f220fadd59de20`。

<!-- begin:docs/filter-coverage.md -->
````````markdown
# 滤镜覆盖矩阵

“支持”指 catalog 有明确参数、能生成可审阅 VPY；插件实际缺失时运行前必须报出 namespace。顺序由处理链决定。表达式/矩阵计算节点暂不实现。

| 类别 | 首批节点 | VS 映射 | 依赖 | 状态 |
| --- | --- | --- | --- | --- |
| 输入 | L-SMASH、FFMS2 | `lsmas.LWLibavSource`、`ffms2.Source` | 第三方 | 已建模 |
| 基础操作 | Trim、Crop、AddBorders、Transpose、AssumeFPS | `std.*` | Core | 已建模 |
| 精度/格式 | Depth、Resize | `fmtc.bitdepth` / `resize.*` | fmtc 或 Core | 已建模 |
| GPU 超分/着色器 | Anime4K GLSL | `placebo.Shader` | vs-placebo / libplacebo | 已建模；文件路径与 1×–4×等比输出可调 |
| 降噪 | RemoveGrain、Bilateral、NLMeans、Zsmooth TTempSmooth | `rgvs`、`vszip`、`nlm_ispc`、`zsmooth` | 第三方 | 已建模 |
| 去色带 | neo_f3kdb Deband、多次串联 | `neo_f3kdb.Deband` | 第三方 | 已建模 |
| 去块 | Deblock | `deblock.Deblock` | 第三方 | 已建模 |
| 锐化 | CAS、UnsharpMask | `cas.CAS`、`std.Convolution` 方案 | Core/第三方 | 首批仅 CAS |
| 加噪 | Grain Add | `grain.Add` | Core | 已建模 |
| 抗锯齿/插值 | ZNEDI3、EEDI3、SangNom | `znedi3.nnedi3`、`eedi3m.EEDI3`、`sangnom.SangNom` | 第三方 | 已建模为底层单节点；蒙版组合待实现 |
| 反交错/IVTC | Bwdif、VIVTC、QTGMC | `bwdif.Bwdif`、`vivtc.VFM` + `VDecimate`、`havsfunc.QTGMC` | 第三方 Python/插件 | Bwdif、VIVTC 已建模；QTGMC 待组合节点 |
| 字幕 | xy-VSFilter、assrender | `xyvsf.TextSub`、`assrender.TextSub` | 第三方 | 待实现 |
| 显示缩放 | Nearest、Bilinear、Bicubic、Lanczos 3、Jinc 2 | 3FP shader | Core/内核 | 已接通；默认 Nearest 放大、Lanczos 3 缩小，达到源像素密度后才转入放大重建 |
| 高级缩放 | Anime4K、NNEDI3、NGU | VS GLSL / VS 插值 / 独立 GPU 推理 | 第三方/新内核 | Anime4K 已实现；NGU 未实现，不伪映射 |

VCB 教程覆盖以公开章节 6、8、9、10 为入口。扩展节点优先选择单输入 clip 加标量/枚举参数的 API 4 原生插件。第 9 章中 mask/merge/diff、TAAmbk、QTGMC 和 BM3D 色彩空间/两阶段处理属于高阶组合节点；在基础节点稳定后按教程实例加入，而不是把任意 `Expr` 暴露成字符串框。

## 扩展插件来源

| 插件 | 上游 | 选择理由 |
| --- | --- | --- |
| Zsmooth | [zsmooth](https://github.com/adworacz/zsmooth) | 现代多架构时域平滑实现，替代已归档的 FluxSmooth/TTempSmooth 插件 |
| Deblock | [VapourSynth-Deblock](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-Deblock) | 教程明确列出的常用强力去块工具 |
| ZNEDI3 | [znedi3](https://github.com/sekrit-twc/znedi3) | 教程推荐的常用神经网络插值器 |
| EEDI3 | [VapourSynth-EEDI3](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-EEDI3) | 教程推荐的更强边缘导向插值器 |
| SangNom | [vapoursynth-sangnom](https://github.com/dubhatervapoursynth/vapoursynth-sangnom) | 教程列出的强力抗锯齿底层滤镜，参数少且风险可清楚提示 |
| Bwdif | [VapourSynth-Bwdif](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-Bwdif) | 单节点、场序可枚举的运动自适应反交错 |
| VIVTC | [vivtc](https://github.com/vapoursynth/vivtc) | VFM 与 VDecimate 可封装成固定两步 IVTC 节点 |
| vs-placebo | [vs-placebo](https://github.com/Lypheo/vs-placebo) | 官方 `Shader` 接口直接执行 mpv/libplacebo GLSL，可加载 Anime4K；输入 16-bit YUV、输出 YUV444P16 |

DFTTest、Neo FFT3D 与 HQDn3D 的 VSRepo Windows 包仍使用已被 R80 拒绝的 API 3，因此没有列入可用 catalog；等上游提供 API 4 Windows 构建后再评估。


## 2026-09-30 扩展

已集成并执行验证：Zsmooth 的 TemporalMedian、FluxSmoothT、FluxSmoothST、SmartMedian、InterQuartileMean、DegrainMedian、Cnr4、CCD、DCTFilter、TemporalSoften、VerticalCleaner；VSZip CLAHE；Descale r11；RIFE NCNN Vulkan 4.26 / Heavy。界面新增 14 个节点，不是 14 个重复下载的插件。

旧 API 3 版 DFTTest、CTMF、AWarpSharp2、Retinex、KNLMeansCL 等不能直接放入 R80 bundle；本轮使用其现代同类实现，不以不可加载的 DLL 充数。RIFE 核心与模型随包部署，不能直接加载 PKL 到 NCNN；性能记录见 README。

MVTools 2× 补帧采用 `vapoursynth-mvtools==29` API 4 wheel（VSRepo v24 是 API 3，不可部署）。固定 Super → 双向 Analyse → FlowInter 50% → 原始/中间帧交错；标量/布尔参数适配现有 GUI。MVTools/RIFE 可选择中间帧计算分辨率，保留原始支路；只降低中间帧细节，不默认降低处理质量。

````````
<!-- end:docs/filter-coverage.md -->

<a id="record-docs-fix-av1-vscap-1-0-9-20261010-md"></a>

## docs/fix-av1-vscap-1.0.9-20261010.md

原始字节数：9006；SHA-256：`b277ed871b8cb0fc6ba9a0f8b3ee3e4bd3ee175eb4092ea4da7086ff5d75d8b0`。

<!-- begin:docs/fix-av1-vscap-1.0.9-20261010.md -->
````````markdown
# 1.0.9：AV1 12-bit 4:4:4 软件解码与 VS Cap 黑屏修复

日期：2026-10-10。版本保持 1.0.9。本轮性能素材仅 `D:\OP-yuv444p12-SWTest.mkv`，没有重测旧 F 盘素材。诊断依据保存在 [用户诊断报告副本](input-av1-yuv444p12-diagnosis.md)。

## 为什么解不动

该片是 AV1 Professional、3840×2160、yuv444p12le、7001/146 ≈ 47.952 fps，4415 帧，音频 FLAC，字幕 ASS。它不是本轮要诊断 AAC 的素材；12 位本身也不足以证明视频为 HDR。原报告机器为 16 核/32 线程 Ryzen 9 7940HX，本机为 8 核/16 线程 Ryzen 7 7840HS，D 盘为 Samsung SSD 980 NVMe。因此不能把报告中的 69.36 fps 直接当成本机结果。

1. dav1d 工作线程数与帧并行上下文数不同。未设置 `max_frame_delay` 时，dav1d 1.5.4 自动取工作线程数平方根向上取整：16 线程仅 4 个帧上下文，32 线程仅 6 个。该片起始帧只有一个 tile，帧内并行与帧间依赖会限制可运行任务；低 CPU 占用不说明播放器已经充分利用 CPU，也不能解释成只创建了 4/6 个系统线程。参见 [dav1d 官方实现](https://raw.githubusercontent.com/videolan/dav1d/1.5.4/src/lib.c) 和 [FFmpeg libdav1d 封装](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavcodec/libdav1d.c)。
2. 同步调用软件视频解码阻塞了 session 的音频、队列和呈现供给，进一步放大卡顿。本轮将 libdav1d 播放解码移到持久工作线程；压缩包队列最多 8 个，输出采用单帧交接，有界持有 packet/frame。session 保持时间轴、音频和呈现状态所有权，seek/flush/free 前停止并回收工作线程，codec 调用与元数据读取串行。
3. 当解码落后音频时钟，旧策略可能把每个后续迟到帧都丢掉，于是 decoded 增长但画面停在旧帧。保留此前修复：仅当有较新已解码帧可以替换时丢弃旧帧，没有替代帧仍显示最新可用画面。本轮没有增加迟到阈值掩盖丢帧，仍为 `max(两帧时长, 50ms)`。
4. 实际 GUI 还有字幕开销。内核已经返回裁剪的 ASS 位图，但 GUI 又将其铺成整张 4K 透明画布，随后作为全屏位图提交；渲染器对大量透明像素也进行颜色转换和上传。本轮保留字幕画布、坐标与颜色，只向 Native 提交可见区域的紧凑位图。裁剪前后真实呈现逐像素一致、清空字幕有效。ASS 动画仍完整更新，没有通过关闭字幕、降低位深或色度换帧率。

## 并行策略与代价

针对 libdav1d 的至少 3840×2160、yuv444p12le 重负载输入，工作线程数为 `min(逻辑线程数×2, 32)`，其他输入保持常规线程数量；显式设置 `max_frame_delay` 为所选线程数。本机得到 32/32，原报告 32 逻辑线程机器也得到 32/32。为避免自适应梯子反复重开/跳转，该 decoder 不再走通用重开梯子。软件 decoded queue 有 512 MiB/最多 8 帧约束，硬解队列预算保持原值。

32/32 消耗更多内存，并有起播、跳转预热代价。本机进程峰值约 5.65 GiB，原先约 1.40 GiB。FFmpeg 提示超过 16 逻辑线程不推荐；本轮对这个指定重负载样本的全片对照确有收益，并未默认采用报告中约 10.5 GiB 的 64/64。

原报告的 `Invalid data found when processing input` 在本轮未复现。全片独立解码及 seek/末尾/重开测试均无解码错误，不能据此声称已经证明旧探针报错的唯一根因。新的 codec 生命周期和数据所有权避免交接竞态；错误仍报告给用户，不静默吞掉。

## 本机实测

物理显示器 3840×2160 / 60 Hz，Windows HDR 已开启。强制软件解码，D3D11 原生路径，`VSR_ADAPTIVE_UPLOAD=0`。解码输出仍为 12-bit 4:4:4，没有 CPU 预缩放；显示链路报告 10-bit，与解码位深是不同概念。

| 测试口径 | 修改前/对照 | 修改后 | 说明 |
| --- | ---: | ---: | --- |
| 全片纯解码：16/16 vs 32/32 | 48.19 fps | 57.07 fps | 两者 4415 帧全片零错误；不包含音频、字幕、呈现 |
| 相同 Native 播放探针，60 秒实际新视频帧 | 882，14.70 fps | 2268，37.80 fps | 约 2.57 倍；不是 GUI 开字幕的数据 |
| Native 60 秒 dropped | 516 | 594 | 音频时间推进从 29.86 秒变为 59.79 秒；不能只比较丢帧绝对数 |
| Native 60 秒音频欠载增量 | 303 | 0 | 修改后起始累计 1，计时段不再增加 |
| Native 首帧 | 401 ms | 663 ms | 解码并行深度增加，首帧未改善；修改后预热仍丢 8 帧 |
| Native 进程 CPU 中位数 / 峰值内存 | 4.88% / 1431 MiB | 52.43% / 5785 MiB | CPU 为采样工具进程口径，不等同任务管理器频率加权值 |
| Full GUI，开字幕，30 秒，裁剪前 | 647 帧，21.57 fps | — | 同一内核、同一素材，音频欠载 0 |
| Full GUI，开字幕，30 秒，裁剪后（构建目录） | — | 856 帧，28.53 fps | dropped 567，欠载 0，首帧 345 ms |
| Full GUI，开字幕，30 秒，裁剪后（最终发行目录） | — | 744 帧，24.80 fps | dropped 646，欠载 0，首帧 338 ms；存在运行波动 |
| Full GUI，关字幕，30 秒定位对照 | 1243 帧，41.43 fps | — | 仅用于定位瓶颈，发行默认仍开字幕 |

**本机尚未达到 47.952 fps / 全程零丢帧。** GUI 的解码、12 位软件上传、字幕合成/呈现仍共同消耗余量；纯解码实时不等于播放器实时。QtTest PASS 表示恢复、像素、工作流断言通过，软件性能测试没有断言零丢帧。

seek 到 0/60/10/84 秒分别恢复于 1250/2849/1854/1751 ms；暂停稳定、恢复播放、末尾 Ended、stop/reopen 均通过。未改保真度、字幕更新和丢帧计数定义。100 ms 迟到容忍、自动 SDR 快上传和 ASS 未变化缓存的实验无显著收益，已撤回。

日志及逐秒数据在 `build/session-20261010-op`：`before.*`、`decode-full-16-16.txt`、`decode-full-32-32.txt`、`shipping.*`、`gui-subtitle-crop.txt`、`shipping-final.txt`。所有性能实验仅使用该 D 盘视频。

## VS Cap 全黑原因与修复

真实复现时桌面已有白色标记窗口，但第一次 DXGI AcquireNextFrame 成功返回 `LastPresentTime=0`、`AccumulatedFrames=0`，只有鼠标更新时间。该通知尚无有效桌面图像，纹理全黑。旧流程直接用它创建全屏截图浮层，所以按快捷键后看起来整个屏幕变黑，随后保存也是黑图。参见 [微软 DXGI_OUTDUPL_FRAME_INFO 说明](https://learn.microsoft.com/windows/win32/api/dxgi1_2/ns-dxgi1_2-dxgi_outdupl_frame_info)。

现在释放这类仅鼠标通知的 frame，继续等待有非零 LastPresentTime 的桌面图像，总等待上限 1000 ms；失败报告采集错误，避免打开无效黑浮层。HDR 仍采集 FP16 scRGB 并进入 RGBA32F，不通过 8-bit SDR 回退掩盖错误。

真实 HDR 屏上用白色窗口验证原始 float 像素、预览、浮层、选区交付及保存 PNG 非黑；修复前该测试失败，修复后通过。最终发行目录 PNG 精度/SDR 兼容/真实桌面 5 项、浮层/注释复制/贴图/AVIF 工作流 6 项全部通过。剪贴板被其他程序短暂占用时有系统警告，现有重试后检查通过。

PNG 为 BT.2020/PQ RGBA16 无损编码；FP16→PQ16 的转换仍有量化，原始 float 通过既有伴随文件保留，不能称浮点逐位无损。既有 AVIF CRF18 路径保持有损压缩定义。

## 构建与手动回传

- 本地构建完成：`dist/Full`（`VSRenderer.exe`、`vs-player.exe`、`vs-cap.exe`），`dist/PlayerLite`（`vs-player.exe`），均 1.0.9。
- Full 1,057,491,775 字节；Lite 195,119,827 字节。Lite 扣除 FFmpeg 专属依赖后 51,972,307 字节，即 49.56 MiB，仍未达到严格十进制 50 MB。精简 FFmpeg 不属于本轮新增修改。
- 工具链：`C:\BuildTools\2026` v145 / MSVC 14.51.36231；Qt 6.10.2 MinGW / GCC 13.1.0。Qt EXE 与 MSVC Native DLL 经 C ABI 互通。
- 用户原 Portable 安装与正在运行的播放器保留；没有提交、推送或修改文件关联。
- Native 基线：`20bcc001b24f6b6de4575bddf9b93ec452795b0d`。完整修改保存在 `patches/3fp-api18-vsrenderer-1.0.9.patch`，已通过 reverse-check。GUI 基线与逐文件 SHA256 记录在源码归档中的 `source-manifest.json`。

原设备先准备该 Native 基线和所需依赖，再运行：

```powershell
& .\tools\build-1.0.9.ps1 -NativeRoot 'C:\Private\FFF_Project-1.0.9' -RuntimeDirectory 'C:\PortableSoft\VS-Renderer-GUI'
```

如 Qt、MinGW、CMake、Ninja 或 MSBuild 不在默认路径，通过脚本同名参数指定。脚本自动应用匹配的完整 Native 补丁，已应用则跳过，冲突则停止；不会把本轮补丁套到不匹配的内核。源码包不含视频、dist、依赖缓存或工具链，因此不能单凭源码 ZIP 获得完整运行环境。原设备 CPU 更强，但仍需在原设备对同片实际 GUI 开字幕进行验收，不能将本机或纯解码结果当作原设备零丢帧证明。

````````
<!-- end:docs/fix-av1-vscap-1.0.9-20261010.md -->

<a id="record-docs-hardware-8k-queue-1-0-6-md"></a>

## docs/hardware-8k-queue-1.0.6.md

原始字节数：7148；SHA-256：`453826d7da4d5dd5ca7a14aeb0900307a89410f35a286be12b66161995dcd1fe`。

<!-- begin:docs/hardware-8k-queue-1.0.6.md -->
````````markdown
# 8K 硬解预取队列修复（1.0.6 本地版）

日期：2026-10-05。修复 1.0.5 测试报告指出的硬解队列预算问题，在当前 1.0.6 源码上合入，保留已完成的 BD 等修复。本轮只更新本地原生 DLL 和文档，没有推送、发行打包或线上发布。

## 报告条件与根因

用户提供的 Tab 截图显示 RX 6600、8K AV1 4:2:0 10-bit、47.952 fps、约 119316 kb/s、六声道 AAC，VS 未启用，Jinc 直通输出 1182×665；累计提交 383 帧、直通丢弃 389 帧。截图的某一时刻显示 0 fps，不能据此认定全过程帧率为零。

用户补充：780M 的后续验收中，D3D11 和 Jinc 均通过了 3840×2160 输出测试。因此，输出尺寸或 Jinc 选择差异不能作为本次倒挂的解释；较早的专项记录不能覆盖这次补充的验收范围。

源码中的缺陷可以直接验证：

- 7680×4320 P010 每帧估算为 99,532,800 字节，即 94.921875 MiB。
- `VideoFrameQueueLimit` 原来共用 128 MiB 预算，计算结果被最低两帧钳位为 2。
- `PumpPlayback` 和 `PumpVideoPresentation` 在 `queueLimit - 1` 时停止追加解码，因而常态只预解码一帧。呈现后发布的 `queuedVideoFrames` 为零。

这会让驱动的解码、纹理拷贝与呈现等待更容易耗尽预取余量。RX 6600 更强的通用图形算力不能绕开程序自身的排队限制。但没有 RX 6600 修复后同片日志，目前不能把截图中全部丢帧都归因于这一处限制。

AMD 的[媒体硬件支持表](https://github.com/GPUOpen-LibrariesAndSDKs/AMF/wiki/GPU-and-APU-HW-Features-and-Support)将 RX 66xx / Navi23 列为 VCN 3.0，Phoenix 列为 VCN 4.0，并分别列出 AV1 支持范围。媒体解码单元、驱动调度和通用图形算力不是同一项能力；该表未给出这段素材的可达帧率，不能据此宣称 RX 6600 存在确定的 8K30 上限。

## 修改与边界

`patches/3fp-hardware-video-queue.patch` 在 `PlayerSession.cpp` 为带 `hw_frames_ctx` 的硬件帧采用 **768 MiB** 队列预算，软件帧继续使用 **128 MiB**。8K P010 的队列上限由 2 恢复到 8，常态可预取七帧；呈现后的剩余队列约六帧。最大八帧、150 ms 预取目标、原有解码表面池、播放时钟、暂停和跳转清队列逻辑均保留。

硬解 AVFrame 持有已有解码池表面的引用；本轮没有新增 CPU 帧下载、扩大 `extra_hw_frames` 或改变缩放核、位深、垂直同步及播放速度。768 MiB 是排队策略预算，不是全部显存上限。解码 DPB、交换链、工作纹理等仍另占显存。Microsoft 说明[进程超过 DXGI 显存预算可能引起卡顿](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/ns-dxgi1_4-dxgi_query_video_memory_info)，因此验证同时记录专用/共享显存，而不只看队列帧数。

构建入口 `tools/build-3fp.ps1` 自动应用补丁，保留重复执行检查。独立 `tools/perf-3fp.cpp` 的 CSV 新增 `queued_video_frames`，便于目标机器核对预取是否正常。补丁正向检查、实际应用、反向检查及与构建源码的 SHA-256 一致性均通过。

## 本机测量

本机 RTX 4070 Laptop，NVIDIA 驱动 32.0.16.1656，DXGI adapter 0。使用已有 8K AV1 10-bit 合成样本循环素材，视频直接复制，并新增 48 kHz 六声道静音 AAC；总长 360 秒，原样本不修改。该样本不等于 RX 6600 截图中的原文件，其码率、帧复杂度和驱动等待也不同。

测试先定位到 120 秒，预热 5 秒；候选各测 60 秒稳定段，旧核心小窗口对照测 20 秒。独立真实 HWND 的交换链/目标物理尺寸、解码模式、缩放路径、10-bit 输出、帧数及时间推进逐秒核对。音频输出正常运行，静音不停止解码或音频时钟。

| 核心与路径 | 输出 | 稳定时间 | 接受帧率 | 新增丢帧 / 合并帧 | 剩余队列 |
| --- | --- | --- | --- | --- | --- |
| 旧核心 Jinc | 1182×665 | 20.006 s | 47.936 fps | 0 / 0 | 0 |
| 修复后 Jinc | 1182×665 | 59.999 s | 47.950 fps | 0 / 0 | 6 |
| 修复后 Jinc | 3840×2160 | 60.007 s | 47.928 fps | 0 / 0 | 6 |
| 修复后 D3D11 | 3840×2160 | 59.997 s | 47.952 fps | 0 / 0 | 6 |

同尺寸 Jinc 对照的进程专用显存峰值均为 **2352.770 MiB**，共享显存峰值由 241.691 增至 242.512 MiB。修复后 Jinc 4K 的专用显存峰值为 2481.262 MiB，D3D11 4K 为 2299.980 MiB。稳定段平均每个接受帧的累计 Present 等待由约 2.402 ms 降至 0.091 ms；这是本次两次采样的计时，不能当作 RX 6600 的性能倍数。

本机旧核心也能达到源帧率，故本轮实测证明预取深度恢复、该机播放无回退，未复现 RX 6600 的大量丢帧。原始 CSV、按 PID 的显存/各 GPU 引擎计数及汇总在 `build/hw-queue-20261005/`，汇总为 `summary.json`。

## 回归与本地部署

新增 `hardware8kQueueLookAhead` 使用实际原生 DLL，分别检查 D3D11 / Jinc 的硬解模式、队列余量、时间推进、10-bit 输出、真实 Present、无新增丢帧/合并/音频欠载，以及暂停、跳转清队列、捕获和恢复播放。修复前 Jinc 用例在队列余量断言失败；修复后通过。日志为 `before-fix-test.txt`、`after-fix-test.txt`。

最终相关回归包括这两个用例、普通视频/BD 的 HW/SW 鼠标跳转、五种原盘字幕语言、六集原盘选集、PGS/短节目、VS 滤镜、模式框宽度、VS 音频时钟、软件上传像素参考及硬解/Shader 捕获。**30 passed, 0 failed, 0 skipped**，耗时 110.572 秒，包括初始化及清理；软件参考最大 RGB 差为零。最终日志为 `build/hw-queue-20261005/final-regression.txt`。

首轮有一个测试读取到了忙碌时返回零的位深快照；该字段采用非阻塞锁读取，测试已改为等待有效采样，完整回归重新通过。实际 RenderTargetInfo 在三组性能测试中持续报告 10-bit。本轮未为这次采样调整渲染实现，首轮日志保留为 `regression-first-attempt.txt`。

新核心 SHA-256：`f80fc559dafdf255bc3ef3129d699504f477cc44897ae1cad559f27c331e2730`。部署前备份旧核心及说明文件，保留 Player / Renderer、配置、VPY 和色彩目录；部署后核对构建产物哈希并更新资源清单。审计记录为 `build/hw-queue-20261005/deployment-audit.json`。

部署后实际加载 `dist/VS-Renderer-GUI-windows-x64/FFF.Native.dll`，进程模块路径及哈希已核对。8K 两种路径、BD 硬解鼠标跳转、原盘简中选择和 VS 音频时钟复测为 **7 passed, 0 failed, 0 skipped**；日志为 `deployed-smoke.txt`，模块记录为 `loaded-deployed-dll.json`。部署和复测前后，受保护的程序、版本、INI、VPY、色彩及既有着色器缓存共 73 个文件哈希保持不变。

RX 6600 的剩余验收是使用更新后的核心，在原片、原 Jinc 条件下记录稳定段帧率、丢帧增量、队列和各 GPU 引擎负载。若恢复预取后仍无法达到约 47.95 fps，需要继续区分 AV1 解码吞吐、驱动/纹理同步与呈现成本；本轮没有 RX 6600 实机数据，未写为该机验收通过。

````````
<!-- end:docs/hardware-8k-queue-1.0.6.md -->

<a id="record-docs-hevc444-peak-diagnosis-1-0-6-md"></a>

## docs/hevc444-peak-diagnosis-1.0.6.md

原始字节数：7311；SHA-256：`fd86bc49bc81ffda8ece4f2721f64de0ad73ca09580397e0ac09a3e32ed7b7e5`。

<!-- begin:docs/hevc444-peak-diagnosis-1.0.6.md -->
````````markdown
# HEVC 444p10 高码率尖峰诊断

日期：2026-10-05。本轮复现并分析本机样本，不部署性能改动，不打包、不上传，版本保持 1.0.6。

## 条件与复现

- 文件：`D:\RebellionHEVC444p10-700MBPS-SoftwareLast.mkv`，2,542,012,252 字节，HEVC Rext，3840×2160，24000/1001 fps，94.052 秒。
- 本机：AMD Ryzen 9 7940HX，32 个逻辑处理器；保持 Windows 平衡电源方案。D 盘为 Samsung SSD 990 PRO NVMe。
- FFprobe 实际解码像素格式为 `yuv444p10le`，未降低位深或色度采样。用户提到的 Y410 为 10-bit 4:4:4 打包格式，不能按 Y210 的 4:2:2 计算，也不能把 Y410 每像素 4 字节直接套到此处的 CPU 平面帧队列。
- 使用当前部署的 3FP DLL，在独立 HWND 上强制软件解码，D3D11 原生直通档、交换链和目标均固定为 3840×2160，快照持续为 10-bit；从片头连续播放 93 秒，20 ms 采样。音频解码和时钟继续运行，输出静音。没有加载 VS 滤镜，没有开启其它播放器。
- 部署 DLL 的 SHA-256：`f80fc559dafdf255bc3ef3129d699504f477cc44897ae1cad559f27c331e2730`。实验结束再次核对，未覆盖该 DLL 或修改 `player.ini`。

以视频包 PTS 按整数秒累加，44 秒为 614.679 Mbps，80 秒为 812.147 Mbps。用户截图的 Second based 峰值约 826 Mbps；统计窗/帧起点口径不同，本记录不将两者视作精确相同数值，均指同一处 80 秒尖峰。这里 Mbps 为兆比特/秒，812 Mbps 约为 101.5 MB/s。

| 构建与参数 | 44 秒尖峰新增丢帧 | 80 秒尖峰新增丢帧 | 测量段全部丢帧 |
| --- | ---: | ---: | ---: |
| 部署核心：128 MiB、8 解码线程，GPU 计时开启 | 0 | 7 | 7 |
| 部署核心重复：128 MiB、8 解码线程，GPU 计时关闭 | 0 | 5 | 5 |
| 隔离诊断核心：128 MiB、8 解码线程 | 0 | 7 | 7 |
| 隔离诊断核心：仅预算改为 512 MiB，仍为 8 线程 | 0 | 3 | 4 |
| 隔离诊断核心：仍为 128 MiB，仅线程上限改为 16 | 0 | 0 | 0 |
| 16 线程重复，GPU 计时关闭，仍为 128 MiB | 0 | 0 | 0 |

部署核心两轮丢帧分别集中在 80.591–80.862 秒和 80.573–80.851 秒，合并帧均为零。512 MiB 那轮另在 62.417 秒丢弃一帧，该离散事件未作根因归属。表中的单轮预算对照不能用于宣称确定的性能改善比例。

## 等待发生在哪里

隔离构建在 `av_read_frame` 周围及视频 `DecodePacket` 入口/出口记录墙钟耗时、媒体时钟、包 PTS、包大小与队列深度。解码计时包含送包、取帧以及排队，**是调用阻塞时间，不是整帧实际 CPU 解码耗时**；FFmpeg 的帧线程在后台工作。

| 128 MiB / 8 线程诊断 | 40–49 秒 | 78–83 秒 |
| --- | ---: | ---: |
| 读盘/解复用单次最长调用 | 1.164 ms | 1.503 ms |
| 视频送包/取帧单次最长调用 | 13.673 ms | 172.221 ms |
| 视频送包/取帧调用累计等待 | 37.125 ms | 925.919 ms |

80 秒附近还出现 152.525、136.988、90.638、83.118 ms 的连续解码流程等待。源帧间隔为 41.708 ms，单次最长等待超过四个帧间隔。部分长等待发生在尖峰后的较小压缩包上，反映解码器仍在等待此前的复杂帧，不能只用当前包大小解释。

部署核心的 GPU 计时在 78–83 秒采样到上传最长 9.403 ms、合成最长 1.973 ms；该段平均 Present 等待约 0.088 ms/接受帧，与 40–49 秒相近。这些是采样结果，不覆盖每一帧，但与丢帧时几乎没有增加 Present 等待的连续快照一致。本次缓存状态下未观察到读盘或 GPU 呈现成为主要瓶颈；没有清空 Windows 文件缓存，不将这一结果外推到冷缓存/其它存储设备。

## 128 MiB 的作用与更关键的限制

1. `PlayerSession.cpp` 按实际 `AVFrame` 格式估算内存。4K `yuv444p10le` 用三个 16-bit 平面，每帧为 `3840*2160*6 = 49,766,400` 字节，即 47.461 MiB。128 MiB 预算产生两帧上限，饱和条件为 `queueLimit - 1`，常态只预解一帧；呈现后的快照余量约为零。
2. 单独提高到 512 MiB 后，快照队列余量恢复到约 3–4 帧，但仍受 150 ms 预取目标与八帧上限约束。因此继续增加预算不会按内存容量比例增加预取时间。
3. `Worker → PumpPlayback` 在同一个会话工作线程先运行 `PumpVideoPresentation`，再读包/解码。视频解码调用阻塞期间，应用队列中的帧也无法由该线程继续提交。512 MiB 对照中，即使调用入口仍排着 3–4 帧，仍出现约 164 ms 的阻塞和尖峰丢帧。
4. 当前大帧 AV1 已单独放宽到 16 线程，HEVC 等其它软解仍被 `MaximumSoftwareDecoderThreads = 8` 限制。本机有 32 个逻辑处理器；UI 的资源百分比不解除此原生上限。
5. 只把这一上限改为 16、保留 128 MiB 后，两处尖峰均未丢帧；关闭 GPU 计时后重复连续播放 93 秒，仍全部零丢帧。首轮 78–83 秒主线程送包/取帧最长阻塞为 0.085 ms。输入包相对播放时钟的超前量也由约 0.3–0.5 秒扩大到约 0.65–0.87 秒：更多工作线程和更深的解码器内部帧流水线提前完成了工作，不能把 0.085 ms 解释为整帧解码速度。

结论：本样本最有力的解释是 **HEVC 软件解码的八线程限制及内部流水线余量，使复杂码率尖峰产生主线程等待；会话中解码与取帧提交共用线程，又把等待转化为错过呈现时限**。128 MiB 的浅应用队列会放大影响，但“预算太小”不足以单独解释或解决问题。GPU 负载低符合等待新解码帧的情况，不代表增加显存预算即可提高 CPU 解码速度。

后续最小实现方向是针对高分辨率 HEVC 软件解码验证适当线程上限与有限预取，不建议直接占用内存/显存的一半或三分之二。本轮 16 线程结果仅适用于本机与此样本；7840HS、低核心数设备、启动/跳转延迟和总体内存开销仍需单独回归，尚未把候选部署为默认值。

## 原始证据

本地诊断目录为 `build/rebellion-peak/`：

- `probe.cpp` / `probe.exe`：隔离测量入口；DLL、视频、目标宽高、测量秒数、缩放算法为六个参数。`VSR_3FP_DECODE_MODE=1` 强制软解，`VSR_3FP_PROFILE=1` 打开可选 GPU 计时。
- `baseline-d3d11.csv/.log`、`baseline-repeat.csv/.log`：部署核心连续复现。
- `packets.csv`：FFprobe 视频包 PTS/DTS、大小、关键帧标记；`analyze.py` / `summary.json`：分秒码率、丢帧和耗时汇总。
- `trace128`、`trace512`、`trace16`：各自隔离 DLL；同名 CSV/日志及 `*-calls.csv` 保留过程。`native-source` 是实验专用源码，其 FFmpeg/Libass 依赖引用已有目录。
- `loaded-modules.json`：确认 512 MiB 实验实际加载隔离 DLL，FFmpeg 仍为部署目录中的同一 `avcodec-63.dll`。

原生回放命令示例，依赖搜索目录通过 `VSR_PEAK_RUNTIME` 指向部署目录：

```powershell
$env:VSR_3FP_DECODE_MODE = '1'
$env:VSR_PEAK_RUNTIME = (Resolve-Path dist/VS-Renderer-GUI-windows-x64).Path
& build/rebellion-peak/probe.exe (Resolve-Path build/rebellion-peak/trace16/FFF.Native.dll).Path 'D:/RebellionHEVC444p10-700MBPS-SoftwareLast.mkv' 3840 2160 93 1
```

````````
<!-- end:docs/hevc444-peak-diagnosis-1.0.6.md -->

<a id="record-docs-history-development-records-through-2026-10-03-md"></a>

## docs/history/development-records-through-2026-10-03.md

原始字节数：92074；SHA-256：`4ab7922f110eca4d52a33c4504f0c17a4bb0f7ec55a4a15f937c72792ca44bb1`。

<!-- begin:docs/history/development-records-through-2026-10-03.md -->
````````markdown
# 开发记录原文归档（截至 2026-10-03）

本文件保留本次整理前的 project.md 与 changelog.md 原文，供核对历史实现、测试、性能、交付和发布记录。原文中的“当前”“未发布”“待实现”等只描述当时状态，可能已被后续记录替代；当前项目地图见 [project.md](../../project.md)，分类版本摘要见 [changelog.md](../../changelog.md)。

历史正文内的相对路径以仓库根目录为基准。本次整理不重新验证历史测试，也不将开发日期视为正式发布日期。

## 原 project.md

<details>
<summary>展开整理前项目文档全文</summary>

# VS Renderer 项目地图

## 开发工作区清理盘点（2026-10-03，删除被执行策略阻止）

- 实际占用11.81 GiB；历史发布/解包副本、旧构建与测试素材等24个候选可释放约7.79 GiB。
- 保留当前Release构建、便携输出及依赖源码；41份历史INI/VPY单独保存，1171份受保护文件哈希未变。
- 两种PowerShell删除调用均返回 `blocked by policy`，尚未删除。清单、审计与边界见 `docs/workspace-cleanup.md`；不能将盘点称作已清理。

## 1.0.4 增强版 FFmpeg（2026-10-03）

- 使用维护者提供的 `ffmpeg-autokey444-full.7z`，FFmpeg 8.0.git / 2026-10-03 构建，归档 SHA-256：`b56bcecdb5de1e768f14b2e18564dd052446b73e36af8903b043561c86684811`。
- `tools/stage-ffmpeg.ps1`：更新独立 `runtime/ffmpeg` CLI 与程序旁同ABI主版本的 FFmpeg DLL/依赖；旧文件保存于 `build/ffmpeg-backups`。不改 FFF.Native.dll、LAV、VS 插件和个人设置。
- `stage-portable.ps1` 同步构建目录的新运行时；`package-release.ps1` 优先采用构建目录的 `runtime/ffmpeg`，保留 ffprobe、ffplay 和构建来源记录。
- 新增 `enhancedFfmpegAvs3Playback`：实际 libuavs3e 编码后经原生3FP解码呈现，验证CLI与播放器库同时更新。专项记录见 `docs/ffmpeg-enhanced-1.0.4.md`。版本保持1.0.4，本轮不推送、不打包。

## 1.0.4 图片编辑与补帧细化（2026-10-03）

- `PlayerImageTools`：38px顶栏，宽窗口，自动AVIF位深，可移动八点裁剪框与固定/自定义比例；裁剪默认PNG无损，像素截取不做重采样。
- `PlayerImageResample`：后台CPU径向Jinc2与可分离核Lanczos3/4，随缩小比例扩展采样范围，透明像素在预乘空间处理，保留8/16位及ICC；最多四个线程，不以当前机器负载下的耗时宣称性能。
- `tools/resize-image-anime.py`：嵌入Qt资源，仅Anime4K放大导出时使用随包VS/Vulkan和PNG临时输入，恢复ICC与单独处理Alpha；图片查看仍无VS图。
- `patches/3fp-lanczos4.patch` / `build-3fp.ps1`：编号8保留旧缩放ABI；原生4-tap采样、放大与缩小呈现均验证，Renderer/Player菜单接入。
- `PlayerProfiles` / `PlayerSettings`：补帧自动入口和每次打开采用配置起始档；最低档丢帧保持播放并提示，正常恢复时消失。仅执行相关专项，保留1.0.4、不推送、不打包。

## 1.0.4 开发中：图片工具栏（2026-10-03）

- `PlayerImageTools`：图片顶部九图标、悬停提示；详细编辑禁用。非破坏性旋转/镜像、回收站、桌面背景、比例尺寸、三分网格裁剪、独立 AWJimage 原生转换。
- `PreviewPane`：图像矩阵绘制，先计算可见源区域再局部映射，保持超大缩放与旋转兼容；源像素共享，无需点击就重新解码或复制全图。
- `tools/awj-runtime.cmake`：固定 AWJimage 1.1.0，源码修订 `26b07c852fe2a51ad7a0bdb796ab7b89b16f8f3d`，原始二进制与源码哈希验证，部署到 `runtime/awj`；保存 LICENSE/NOTICE 和对应源码。
- 输出在线程中进行，AWJ.exe 直接启动，退出取消后端进程；Qt 图片写入/最终输出用可中断的 QSaveFile。源文件禁止覆盖，后台任务结束再启用操作。
- 本轮保持1.0.4，未推送、未打包；验证与边界见 `docs/player-image-tools-1.0.4.md`。

## 1.0.4 开发中：直通模式进度条预览（2026-10-03）

- `PlayerControls`：D3D11 / Jinc 直通拖动进度条时持续定位并渲染；33ms 合并窗口仅保留最新位置，上一张预览完成后再提交下一次定位。
- 拖动时暂停视频时钟，松手精确定位并恢复拖动前的暂停/播放状态；关闭或换文件取消待处理预览。
- Jinc / D3D11 × 暂停 / 播放四项专项用例通过，覆盖拖动中实际呈现、快速连续移动、松手实际帧时间戳和状态恢复。版本继续1.0.4，未推送、未打包。

## 1.0.4 开发中：GLSL 分类与输出尺寸（2026-10-03）

- `FilterCatalog` / `ParameterEditor`：GLSL 按超分、缩小、基础锐化、线条增强/抗锯齿、降噪/修复、去色带、色度、色彩/LUT、几何效果等功能筛选，另有自定义 GLSL。保留路径作为参数值，短名称与完整路径提示用于显示。
- 硬件标签注明 NVIDIA NIS 来源（非 N 卡独占）与 DP4A 要求；RIFE / MVTools 仍为补帧节点，不把单帧 GLSL 标作补帧。
- `VpyScriptBuilder`：倍率或指定分辨率输出，保持宽高比开启时以最后编辑的宽/高计算另一项；UI 跟随当前节点输入尺寸，支持上游裁切、转置、缩放。
- 本轮仅本地构建与专项验证；版本为1.0.4，更新尚未结束，不推送 GitHub、不打包。
- 分类、控件布局、宽高联动、旧倍率与实际 Vulkan 输出尺寸验证通过，记录见 `docs/mpv-shaders-1.0.4.md`。

## 1.0.3 mpv 着色器与内置补帧（2026-10-02）

- `PortableUpdater`：按必要文件识别解压顶层或下一层任意名称的唯一完整便携目录；保留路径、哈希和版本检查。
- `assets/mpv-shaders` / `FilterCatalog` / `VpyScriptBuilder`：915 份 GLSL / hook，固定来源与哈希，目录发现、名称搜索、缩小倍率及旧纹理格式兼容；打包与运行时部署携带库。
- `PlayerProfiles` / `PlayerMenus`：Anime 自动版和 Realistic 置顶；内置补帧自动链及四个固定档，开启采用 Jinc YUV444P16，关闭恢复 Anime 自动超分。
- `PlayerSettings`：修复字体只保存但子控件/设置窗口未应用，显式配置中文 Han 字体回退。
- 使用本轮专项验证，详细边界见 `docs/mpv-shaders-interpolation-1.0.3.md`。

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

</details>

## 原 changelog.md

<details>
<summary>展开整理前更新日志全文</summary>

# 更新日志

## 1.0.4 — 开发中（2026-10-03）

- 开发工作区盘点：识别约7.79 GiB历史构建/发布副本和测试素材，保留历史配置；删除被执行策略阻止，尚未实际释放空间。详见 `docs/workspace-cleanup.md`。

- 换用维护者提供的2026-10-03增强版FFmpeg 8.0.git，更新独立CLI和3FP使用的共享库，增加AVS3/libuavs3e编码、libuavs3d解码，以及zenrav1e、OpenH264、kvazaar、APV等支持；保留原版参数接口及该构建的旧格式编解码器。
- 构建与便携目录同时同步完整DLL依赖、ffprobe/ffplay及上游构建记录；旧FFmpeg单独备份。便携部署/旧发布脚本改为使用当前构建运行时，保留1.0.4、不发布或打包。

- 图片顶栏高度由48缩至38逻辑像素，图标按钮由40缩至32；转换和尺寸窗口加宽。
- AVIF位深增加自动：8→10、10/12保持、>12→12；保留手动8/10/12。
- 裁剪框支持内部移动、四边/四角调整、框外重新拖选，新增16:9、4:3、3:4、9:16及自定义比例；默认PNG对解码像素无损保存。
- 图片调整尺寸默认真正的EWA Jinc（2 lobes），提供Lanczos3/4、双线性、最近邻与Anime4K Mode A Fast放大。常规重采样在CPU后台处理，不受GPU纹理尺寸限制；Anime4K只在离线输出时执行Vulkan/VS，不影响图片查看路径。
- 原生3FP视频后端新增Lanczos4（ABI编号8），Player放大/缩小及Renderer预览缩放列表接入，保留原编号和配置。新增可复用补丁 `3fp-lanczos4.patch`。
- 渲染设置增加补帧起始档位；自动入口、重新打开文件及应用设置使用该起始档位。最低MVTools档持续丢帧超过5%时左上角提示性能不足，保留补帧，由用户决定关闭；恢复正常后清除提示。加载失败在最低档停止并保留提示。

- 图片模式新增顶部九个图标：详细编辑（暂禁用）、左转、右转、镜像、删除到回收站、桌面背景、调整尺寸、方格裁剪、压缩和格式转换；悬停显示名称。
- 旋转/镜像采用显示变换，不复制超大源图；尺寸支持保持比例，裁剪采用三分网格和可选1:1方形，后台另存且保护原图，失败不替换已有输出。删除使用 Windows 回收站，背景保存为独立 BMP。
- 复用 AWJimage 1.1.0 独立原生后端：AVIF/AOM、WebP、JXL、JPEGli、PNG；固定编码质量和目标视觉质量搜索；AVIF 默认 YUV 4:2:0，支持420/422/444/auto、8/10/12位和 Alpha 策略。视觉质量 GPU 仅用于指标计算，编解码仍为 CPU。
- 随后端保留 AGPL 许可证、上游 NOTICE、固定修订对应源码；构建校验下载哈希。专项记录见 `docs/player-image-tools-1.0.4.md`，仅更新本地便携目录。

- 修复 Player 的 D3D11 / Jinc 直通模式拖动进度条时只在松手后更新画面的问题；拖动中合并最新目标、持续渲染预览，松手精确定位并恢复原暂停/播放状态。

- Renderer 的 mpv GLSL 增加功能分类与单独的“自定义 GLSL”入口；着色器显示文件名和来源，完整路径保留用于执行与悬停提示，长路径不再撑宽控件。
- 支持倍率或指定宽高输出；“保持宽高比”开启后编辑一项联动另一项，以最后编辑的尺寸为准，脚本根据实际滤镜输入比例计算。
- 硬件标签区分 NVIDIA NIS 算法来源和 DP4A 支持要求。ArtCNN / CuNNy 的 DS 按带降噪锐化的超分版本处理；补帧使用 RIFE / MVTools。
- 版本号改为1.0.4。本轮开发未结束，仅修改本地代码，不推送、不打包。

## 1.0.3 — 2026-10-02（着色器、补帧、字体与更新布局）

- 新更新器接受程序文件直接位于归档顶层或下一层任意名称的完整目录，不再依赖归档根目录名字；仍拒绝危险路径、多个完整候选、不完整包、更深嵌套及错误版本。
- 对照 hooke007 与 mpv User Shaders，加入915份去除内容重复后的 GLSL / hook，以及上游许可、修订、SHA-256 清单；节点支持目录发现、名称搜索与0.5× 缩小，保留半精度纹理格式及对应数据。lensfix-ewa 旧 LUT 调用做等价兼容修改。
- 开发者内置超分菜单将 Anime 自动版和 Realistic 放在最上方。
- 新增右键“图像处理 → 补帧”：RIFE 4.26 4queue → 半宽高 RIFE → 最高质量 MVTools → 默认 MVTools → 关闭，支持自动丢帧/失败降档或手动固定。开启补帧采用 Jinc 直通 YUV444P16，关闭恢复 Anime 自动超分；保持暂停状态和播放位置。
- 修复 UI 字体只保存但设置窗口/子控件仍使用旧字体；中文字体采用显式 Han 回退，应用立即生效并可保留到下一次启动。
- 仅验证本轮功能；本地高占用 FFmpeg 作业下的编译耗时不作为性能结论。详见 `docs/mpv-shaders-interpolation-1.0.3.md`。

## 1.0.3 — 2026-10-02（兼容1.0.2更新器）

- 修复固定归档根目录不符合已发布1.0.2路径验证规则，导致正确上传的更新包被拒绝的问题；大小及SHA-256没有错误。
- 归档根目录恢复为 `VS-Renderer-GUI-1.0.3-windows-x64`，安装仍保持已有目标目录，本地开发目录继续使用固定名称。
- 新增真实归档兼容检查，并用1.0.2原始更新器源码验证旧包拒绝、修正版接受；重新生成LZMA2归档和SHA-256。下方先前固定根目录包的哈希与大小仅作为历史记录。

## 文档整理 — 2026-10-02

- README 精简为核心功能、下载启动、Renderer/Player 操作和快捷键；版本演进、性能数据、实现及专项验证记录集中于本日志与链接的 docs，不再逐轮叠加到首页。
- 旧 README 的版本记录已在下方对应条目和专项文档保留；例如图片/音频/AVIF/手动 Anime、Renderer 布局/导出、图标、补帧与历史更新机制。阶段性的“未推送/未打包”仅表示该次开发完成时的状态。

## 1.0.3 — 2026-10-02（打包与源码推送完成）

- 源码已推送 GitHub main，版本提交 `4271ff0`；Release v1.0.3 保持草稿，无附件，维护者手动上传 GitHub 和 ModelScope。
- `1.0.3.7z` 为标准 LZMA2 9级极限压缩，大小168,399,042字节；不使用 Zstandard。包含313个文件与8个内置VPY，排除缓存、个人配置、用户预设、截图及测试文件。
- 归档完整性、完整解压与312项内部资源SHA-256检查通过。归档SHA-256：`d3f9239acd66be80fa834c87040eee0050cd9de2d55d1e34386711e237d8fc9e`。

## 1.0.3 — 2026-10-02（完整便携包）

- 打包改用兼容的 LZMA2 9级极限压缩，排除缓存、个人配置、自定义 VPY、日志和测试产物，包含六个新增手动 Anime 内置预设。
- 补充完整 Release 说明与手动上传安排；生成归档 SHA-256，推送源码并创建无二进制附件的 Release 草稿。

## 图标接入 — 2026-10-02

- 内嵌用户选定 R3 / P2 应用图标与 V1 / I3 文件标记；Qt 窗口和Windows EXE资源一致。
- 视频 / 图片 / 音频分开注册，视频与图片采用 TypeOverlay 右下角标记，兼容旧打开命令、保留系统缩略图及默认应用选择。
- Release与相关专项5/5通过，两正式程序启动 / 正常关闭、窗口图标与交付哈希核验通过；20项原有INI / VPY / native哈希不变。仅本地更新，未打包发布。详见 `docs/icons-integration.md`。

## 图标候选资源 — 2026-10-02（未嵌入）

- 新增两程序各三款AI图标原图和视频 / 图片缩略图小标记；可选八款附10档32位ICO候选稿，保留全部提示词及原始生成图。
- 新增深浅背景选图页与角标示意，ICO文件结构、逐尺寸读取及透明通道检查通过；等待选型改进，未修改程序资源或文件关联。路径 `assets/icon-concepts/2026-10-02/`。

## 1.0.3 — 2026-10-02（播放器设置与手动 Anime）

- 设置底部改为左侧加载/保存预设，右侧取消(N)、确定(Y)、应用(A)；保留 Apply 暂停状态与时间。
- 文件关联加入常用图片和全选图片，全选所有包含图片。
- 开发者内置新增六个手动 Anime 档位，保留默认自动版；固定档不受丢帧/大分辨率自动降档影响，Jinc 与 D3D11 直通固定对应算法。
- 中英文更新，相关专项 17/17（含初始化/清理）通过；仅更新本地播放器和新增预设，保留原有 INI/VPY 与其它运行时。详见 docs/player-manual-presets-1.0.3.md。

## 1.0.3 — 2026-10-02（Renderer 三栏与设置页面）

- 默认窗口加宽至1800×900逻辑像素，“参数与脚本”移至处理链与原视频之间的独立栏，取消参数区高度上限；左侧处理链可占更多空间。
- 参数说明、标签和空白区白底，灰色背景只留在输入控件范围。
- 无源查看VPY的警告加宽，避免标题被截断。
- 导出顶部栏目与下方控件对齐，编码队列表头增加清晰竖向分隔线。
- 设置改为主窗口导航独立页面，保留源滤镜应用保存、版本与检查更新；分析页仍按需初始化。
- 五项专项含初始化/清理7/7通过；仅更新本地Renderer和文档，启动/正常关闭通过，播放器/native/LAV/INI/VPY56项哈希保留。详见 `docs/renderer-panels-1.0.3.md`，未推送/打包/发布。

## 1.0.3 — 2026-10-02（Renderer 布局与导出限制）

- 扩大导航图标文字间隔，删除实验设置和重复VPY按钮；输入浏览同排，预热状态移到对比标题右侧。
- 时间轴对齐视频工作区，播放组随resize居中；滤镜库增高收窄，参数可滚动，标签换行、bool右对齐、下拉紧凑显示。
- 导出栏目增加蓝色选中提示；画布仅允许AB滑块、ABC左右2+1和ABCD常规/滑块2×2，输出保持最大源宽高，拒绝3FUI尺寸变换参数。
- 本轮相关专项6/6通过，实际VPY/MKV与像素检查通过；详细结果见 `docs/renderer-ui-export-1.0.3.md`。保留其它会话播放器修改，未运行无关回归、未推送/打包/发布。
- 仅更新本地VSRenderer.exe和文档，正式程序哈希匹配共享构建、启动/关闭正常；播放器/原生DLL/配置与预设17项哈希保留，未全量stage或覆盖LAV。

## 1.0.3 — 2026-10-02（应用设置与AVIF更正）

- 后续分析：安全YUV平面生成的同尺寸巨大444样本正常，原图整图/底部抽样通过（解码8.0s）；首次RGB读入32位偏移回绕与黑块边界吻合。仅补充分析和对照样本，未改播放器/正式程序，详见 `docs/avif-large-yuv444-analysis.md`。

- 新增Apply，应用后设置窗口保持打开，保留暂停/播放及本次播放时间，图片不重新解码。
- 采用用户提供的 LAV Filters 0.83.0 x64，更新构建/便携依赖与许可，删除旧版带 `-lav-` 名称的FFmpeg DLL；暂存默认使用本轮新版，LAV相关专项通过。
- AVIF接入libyuv SIMD正常色彩转换，实际完整YUV420巨图23.7s降至6.4s；Tab补充CICP/GBR标识。
- 定位首次生成AVIF的编码端32位扫描行偏移溢出，更正之前仅凭可解码作出的完整性结论。新YUV420样本与原图整图/底部抽样及正常10位GBR测试通过。
- Release、312条语言检查及相关6/6专项通过；不扩大无关回归。详细证据见 `docs/player-apply-avif-1.0.3.md`。

## 1.0.3 — 2026-10-02（图片性能与音频设置）

- 图片Tab补齐输入/输出/资源信息；修复超大图16倍缩放上限，裁切可见源区域。
- JPEG使用当前工具链编译的libjpeg-turbo SIMD直解和映射输入，保留ICC/EXIF及原始像素；真实1.7GiB巨图12.5s vs Qt23.3s。
- 右键新增音频设置：轨道/外部音频、±0.1s同步、静音、非模态十频段均衡器与MST/WAV音量；源2.0/5.1/7.1优先、设备不接受时2.0回退。
- 视频同目录自动匹配音频/字幕；拖悬实时提示，音频上下50%挂载/单独播放，字幕上40%次字幕/下60%主字幕，主次字幕可独立挂载。
- 仅运行相关专项，更新本地便携程序与文档，保留配置和用户VPY；未发布压缩包。详细结果见 `docs/player-images-audio-1.0.3.md`。

验证：Release与语言包检查通过，交付目录相关专项9/9通过；两EXE及原生DLL哈希匹配，12份INI/VPY哈希保留。

## 1.0.3 — 2026-10-02（图片与右键菜单）

- 修复JPG进入视频/VS源造成找不到帧序列：本地图片采用独立模式，支持Qt与FFmpeg的多种图片格式，不加载VS滤镜、不预解码相邻图片。
- 独立AVIF解码使用libavif/dav1d，支持grid、高位深、透明通道与大尺寸；修正上游256MP默认上限，避免临时PNG及视频纹理边长限制。
- 图片按左右键切换同目录名称排序的图片，滚轮始终缩放，支持拖动/还原；异步旧请求不会覆盖当前图片，损坏文件显示解码错误。
- Player右键菜单主级和子级增加180ms缓出展开、圆角与明显方框勾选，勾选框居中于左边缘和文字之间。
- 专项与真实巨图结果见 `docs/player-images-menu-1.0.3.md`；保留1.0.3版本。

验证：Release构建、图片/菜单专项及三份真实巨图通过；交付目录专项7/7，相关八组回归最终均通过（导出组一次失败后单组复测通过）。本地程序已更新，INI和用户VPY保留。详细证据见 `docs/player-images-menu-1.0.3.md`。

## 1.0.3 — 2026-10-02（继续开发中）

- Anime 自动降载细化为 CNN+增强 → CNN → no CNN+增强 → no CNN → Jinc → D3D11；设置新增起始档位，支持 INI 保存 / 加载，Tab 显示当前级别。保持 5% 门槛与暂停 / seek / resize 恢复规则。
- 新增随包 `anime4k-no-cnn.glsl`，采用上游梯度 / DoG 算法，完全不执行 CNN；Renderer 的 Anime4K 下拉也支持。
- 内置增强与 Anime4K 合并为一次 GPU Shader，减少中间读回 / 上传及色度重组；Anime4K 节点保留输入色度采样，不再强制转 420。当前 vs-placebo 的必要 16 位接口转换仍保留。
- 旧默认内置 Anime 定向迁移，保存 `.before-six-stage` 备份；新增六档、10 位 420 / 422 / 444 输入、精确跳帧、设置持久化和降载回归。实现与短样本性能限制见 `docs/player-six-stage-1.0.3.md`。
- 已交付固定便携目录，保留原有 INI 与其他预设；Player 回归36项通过、0失败、7条件跳过，八组最终验证完成（导出原代码单独复验通过），正式EXE启动/关闭、构建哈希和迁移备份校验通过。未推送、打包或发布。

## 1.0.3 — 2026-10-01（继续开发中）

- 新增 1080p48 分辨率诊断记录：确认 Anime4K Fast 严格 1.2 倍 CNN 放大开关，复现增强链从 2304 到 2368 的跳帧突变；记录纯 A 全屏结果波动与调度 / 搬运 / Jinc 待验证成本。本项仅分析，未修改正式播放实现，见 `docs/player-resolution-threshold-1.0.3.md`。

- 活跃程序目录更名为 `dist/VS-Renderer-GUI-windows-x64`；保留用户 VPY / 缓存 / 配置并迁移 INI 内程序路径，版本仅写入程序、manifest 和归档名。
- 部署 / 打包脚本更名为 `stage-portable.ps1` / `package-portable.ps1`，读取 CMake 版本；更新器允许固定根目录，同时兼容历史版本根目录。
- 修复 Player 的 FFMS2 无法识别 VVC 轨道：仅对 `Source: No video track found` 尝试 L-Smash，继续处理 VS 预设，使用自身索引缓存；不把所有 VS 错误降级为直通。
- 设置窗口版本由构建版本生成，中英语言包同步。
- 修复连续缩放重建 VS 链丢失继续播放意图；测试确认实际播放 / 暂停状态后检查降载。
- 验证：八组 CTest 最终通过（比较拖动耗时断言一次超限，原门槛单独复测通过）；固定目录 Player 10/10、Updater 7/7，真实 VVC 五组配置；正式 EXE 独立启动 / 关闭，构建哈希一致，13 个用户 VPY / 原有 INI 设置保留，完整 GPU / 补帧 helper 校验通过。记录 `build/vvc-*`。
- 本轮只更新本地程序，不打包、不推送、不发布；原 1.0.2 归档不修改。

## 1.0.2 — 2026-10-01（完整更新与发布）

- Player 缩放后新增左上角“还原画面”：恢复默认适配比例并重置拖动偏移，支持普通窗口和无边框全屏。
- Renderer 设置 / Player 基本设置显示版本与检查更新；从 ModelScope `VS-GUI` 自动识别最新数字版本，旧版本不降级。
- 集成上游最新核验版 aria2-next 2.8.3、随包 7-Zip Zstandard；更新下载 / 分块校验 / 解压保持 UI 可响应，检查大小、SHA-256、安全路径和内部版本。
- 完整目录更新保留 INI、自定义 VPY、用户项目与截图，旧目录作为备份；新内置替换旧内置，占用时等待退出，失败保护原目录。
- 干净发布包移除缓存、测试产物、个人配置、历史 VPY 与无用 madVR 注册 / 重置脚本，使用本机 Zstandard Ultra 22；活跃目录用户文件保留。
- 更新 UI 同步中英文语言包，增加真实下载 / 哈希拒绝 / 安全路径 / 全目录替换 / 用户文件保留 / 占用保护与全屏缩放还原测试。Release 与 CTest 8/8 通过。
- 按最新授权通过本机 ModelScope-Manager SDK 上传完整包至 `ARXChem/Software-List/VS-GUI`；GitHub 更新源码并创建无附件 Release 草稿，GitHub 压缩包由用户手动上传，不发布草稿。前几轮暂不发布的记录保留为历史状态。

## 1.0.2 — 2026-10-01（继续开发中）

### 本轮：网络直通、缓存与播放器设置

- HTTP / HTTPS 自动走 3FPlayer 直通，固定绕过 VS / VPY；仍支持显示层 Jinc 等缩放。移除整文件临时媒体准备，保留 AList API、ModelScope 重定向及有界 Range 代理。
- Anime / Realistic 在 Jinc 后增加 D3D11 Native 最低负载档；支持的 SDR 硬解使用 VideoProcessor，不适用时轻量 Bilinear 回退。右键可选；同尺寸仍跳过额外缩放。
- 丢帧降载仅统计稳定正常播放：暂停 / 恢复、跳转、改速、窗口 / 全屏 / 画布变化后重启恢复期与观察窗口。修复原生平均帧率元数据字段，网络精确帧定位与信息面板正确。
- 修复“先 Jinc 到目标再增强”与内置 10-bit VPY 的 Resample 位深冲突；已知内置脚本先留 `.before-bitdepth-fix` 备份再补转换，其他用户 VPY 保留。
- 本地寻帧 ffindex / lwi 改存软件 `cache/indexes` 或自定义目录；新增路径、大小、定向清理。索引按路径 / 大小 / 修改时间区分，不在媒体旁残留新索引。
- 解码设置分开视频 / 音频，完整展示 3FPlayer / LAV 文件名、硬解 / 软解方式，提供 LAV Video / Audio 原生配置；网络和内置直通固定 3FP，VPY 内部源不冒充 LAV。
- 设置新增中文 / English 外置语言包、自动播放（默认开）、主题字体 / 背景 / 透明度、媒体位置记忆（默认开）、源多线程（默认开）、左右键与 Ctrl / Ctrl+Alt 时间跨度；完整 INI 读写。
- 缓存下方新增文件关联页及四种批量选择，注册当前用户的候选播放器；默认应用仍通过 Windows 设置确认。抗振铃标注“仅 Jinc / 0.5”。
- 新增语言包完整性检查与网络绕过故障 VPY、10-bit 先缩放、索引复用、语言 / 设置、独立音频、文件关联、媒体时间记忆与 RIFE 输出帧恢复、暂停 / resize 防误降级、D3D11 实画面等回归。

- 本轮最终验证：Release / 原生 DLL、CTest 7/7、交付 Player 26/26（真实 ModelScope / AList，无跳过）；移出测试 DLL 后两个正式程序独立启动 / 正常关闭，构建 / 资源 / 语言 / 文档与交付一致，用户 VPY / INI 保留，两个内置位深迁移有原文件备份。证据为 `build/player-settings-*`。真实 1080p47.95：Fast 1440p 暖态 48.71 fps，增强 + Fast 35.79 fps，索引 2.091→0.054 秒；在线旧版与当前 CNN 差异及读回 / 重复解码瓶颈写入性能记录。版本仍 1.0.2，不发 Git、不打压缩包。

### 前一轮（网络 VS 临时准备已被本轮替换）

- 网络 VS 增加有进度的临时媒体准备（四路 8 MiB Range / 独立连接池）；HTTP/1.1 及每路代理连接隔离，临时媒体释放后删除。直通不需要整段临时媒体。
- 修复 AList 网页链接：解析文件 API 返回的签名直链；ModelScope 重定向与 HTTPS 使用 Qt 独立线程和有界本地 Range 代理。内置 PGS 打开失败仍保留句柄导致空输入读取的闪退已修复，原生 ASS / PGS 支持网络输入。
- 播放列表右侧范围悬停，右键永久停靠 / 窗口扩宽、拖动横向分隔栏调整、× 关闭；全屏临时悬停并恢复普通停靠。文件名随栏宽适配。
- 新增右键滚轮音量 / 放大视频，默认音量；Tab 帧大小使用实时画布物理像素；菜单子级箭头留出间距。播放源不再写 lwi / ffindex；保留原已有用户文件。
- 内置预设改为 Anime / Realistic，新增官方 A/M Fast 着色器。Anime 直接输出到目标尺寸、最多 2160p并保留宽高比；超 5% 丢帧按增强 → A/Fast → Jinc 直通降载。4K 源与 Realistic 走原生直通，新增先缩放再增强的设置；保留自定义 VPY 与旧文件。
- 本轮最终验证：Release 成功、CTest 6/6、交付 Player 20/20 无跳过；两个真实链接原生播放与 ModelScope 完整 VPY 链通过。两个正式 EXE 独立启动 / 关闭、交付构建一致、GPU / 补帧 helper 与 Fast 资源校验通过，原有 VPY / INI 哈希未变。证据为 build/player-network-profile-* 与 build/player-real-network-http1.txt；AList VPY 完整 2.49 GiB 输入未另做长片测试。保持 1.0.2，不发 Git、不打包。
- 标准功耗下真实 4K48 AV1 原生路径 8 秒样本 383 呈现 / 0 丢弃 / 0 合并；Fast 配对数据和官方 5 ms 的适用边界写入性能记录。

- Sharpen edges / Crispen edges / Enhance detail 的 YUV 处理改用随附 Vulkan GPU 着色器；连续锐化合并提交，保留色度和邻域限幅，Gray/RGB 继续 CPU。已有内置 VPY 加载时只替换精确识别的旧 CPU 块，不改写文件。
- 修复暂停时反复生成、上传相同字幕覆盖图而持续占用 GPU；字幕时间、尺寸、显隐与轨道不变时复用，换轨 / 样式 / 调整尺寸仍正确刷新。
- 无边框全屏底栏 2 秒后隐藏，鼠标到下方恢复；Enter 进入和退出全屏。右侧范围悬停显示播放列表，文件夹以文件和可展开子目录显示，支持双击及上下视频切换。
- 无视频时也能右键打开文件、文件夹和链接；支持 HTTP/HTTPS 视频直链、file URI 及拖放 URL，接通 ModelScope-Manager 命令行直链输入。网络使用 FFMS2 帧索引，不持久写入索引，首次索引需要扫描媒体。
- 前一次本地构建：CTest 6/6，便携目录 Player 15/15；两个正式程序独立启动 / 关闭通过，原有 VPY 与 INI 保留，完整补帧修复仍随构建交付。继续保持 1.0.2，不推送 Git、不打压缩包。

- 修复 Tab 丢帧统计遗漏 VS 时间轴跳过的帧；区分跳过、渲染丢弃与合并，seek 不计跳帧；CPU 右侧显示实际 GPU 引擎占用百分比。
- 倍速弹窗可连续点左右及预设，点外部 / 再次点下拉收起；Enter 切换无边框全屏，Esc 退出，时间与倍速输入框的 Enter 仍提交输入。
- 增加蓝色悬停的三级右键菜单：内置 / 自定义 VS 预设、字幕、无损 PNG 截图、放大 / 缩小算法、全屏与设置。
- 字幕支持主次轨、交换、显示开关及 ASS/SSA/SRT/SUP 外部挂载；外部字幕优先于 MKV 内置轨。新增 SRT 字体、尺寸、颜色、透明度、边框、阴影及位置样式窗；ASS 保留特效。修复 PGS 无明确结束时间、清屏事件及 pending 像素未确认导致的不显示。
- VS 源帧按原尺寸保存 16-bit RGB PNG；显示截图含字幕。原生字幕覆盖支持外部 VS 帧会话，避免其 Idle 内部状态拒绝字幕层。
- 默认 Jinc + relaxed 抗振铃，同尺寸跳过额外缩放；补齐缩小 Spline36 / Super-XBR。抗振铃为开放局部约束实现，不宣称复刻 madVR。
- 新增四页设置和单个便携 player.ini，支持完整配置导入 / 导出；默认预解码、四项资源阈值与常规 / 极致预设，异步 VS 多帧预取、线程及缓存目标控制。阈值是后台调度条件，不是全机硬上限。
- 新增保持 VS 输出及多种 YUV / P010 / P016 / RGB 输出选择，转换保留分辨率；LAV 原生配置窗口可直接打开。
- madVR 完成实际 DirectShow 接入：随附 madVR64.ax + LAV 视频输入、原生控制器、字幕 OSD 和截图。此模式绕过 VS，切换 VPY 明确提示不生效；不注册系统组件。
- Renderer 新生成的 Sharpen edges / Crispen edges 移除无用卷积，实际三帧输出一致；保留旧 VPY，并另附 Anime4K Default Optimized.vpy。01.mkv 负载关闭后的性能数据及限制记录于 docs/player-performance-1.0.2.md。
- 本地 Release 与原生 DLL 构建成功；全套 CTest 6/6、交付目录播放器 13/13（含真实 01.mkv、PGS、madVR、截图、输出像素格式和 RIFE 精确帧定位）通过。两主程序移出测试 DLL 后独立启动并正常关闭；构建 / 交付三文件 SHA-256 一致，补帧修复仍内嵌，原有三份 VPY 校验值未变。
- 更新现有 dist/VS-Renderer-GUI-1.0.2-windows-x64；不发 GitHub、不生成压缩包，版本仍为 1.0.2。

## 1.0.2 — 2026-09-30

- Renderer 新增独立预设管理窗和 VPY 加载：保存到便携 vpy、预览/读写/导入导出、名称备注、拖动排序、回收站删除；输入覆盖支持同一处理链用于不同视频。
- Anime4K 新增 14 文件模式下拉及自定义 GLSL 位置，随本地目录部署现有文件。
- 新增 vs-player.exe：两排控制、章节、六种进退、前后视频、精确时间/帧输入、倍速配置弹窗、Tab 信息、自动 HDR 请求及 3FP HW/SW。
- 3FP 新增不变调 0.10–16.00× 音频链与速率时钟；修复暂停改速时位置推进，seek 重置和尾部排空。
- 集成便携 LAV 原速播放时钟/音频，变速显示并使用 3FP 不变调路径；madVR 按确认只携带，后续接入。
- 仅生成新的 1.0.2 程序目录；不推送 GitHub、不打压缩包。
- 验证：CTest 6/6、交付目录播放回归 5/5、14 个 Anime4K 实际取帧和两主程序启动通过；RIFE 补帧输出支持精确帧定位，合入 MVTools 绿色闪烁修复。
- 补帧接入复核：两程序内代码及构建文件一致；交付运行时执行程序内补帧逻辑，彩色色度、并发跳帧、真实媒体和 RIFE 回归通过，无需重建。旧保存 VPY 需重新生成以包含修复。

## 补帧专项 — 2026-09-30

- 修复 MVTools 绿色闪烁：Super 固定携带色度平面，色度搜索开关只影响 Analyse，避免 FlowInter 中间帧 U/V 为零；新增彩色 8/10/16-bit 跳帧、并发及真实媒体回归。

- 新增 API 4 MVTools v29 2× 补帧，原帧直通、50% 中间帧按需生成；可调运动块、精度、重叠、色度搜索及切镜行为。
- RIFE 原始时间点绕过 RGB 往返，仅转换生成的中间帧；保留原格式/原像素，支持 2×/3×/4× 及单帧/末帧保持。
- 两种算法新增原始/半宽半高/四分之一宽高计算选项，默认原始；低分辨率中间帧更快但细节较软。MVTools 补齐小图块网格，避免 FlowInter 小尺寸崩溃。
- 独立构建于 build/mingw-interpolation，未覆盖并行播放器任务的交付目录。

## 1.0.1 — 2026-09-30

- 分析页新增“导出对比画布”，复用 3FUI 参数和 MKV 队列；静态快照合成当前布局、切割、缩放平移和时间偏移，输出尺寸取所有导入源中像素最多的源，使用所选音频及其偏移。九路导出以裁切块堆叠，避免九张全尺寸 RGB 画布的额外内存。
- 长文件名自适配轨道下拉宽度，保留进度条最小宽度，统一 8px 相对间距；扩大布局/音频控件。颜色处理改为与放大算法一致的列表及蓝色悬停，VRR 显式方框和勾选。
- 汉堡导航底部新增“设置”，L-SMASH / FFMS2 移入设置并持久化。修复侧栏 border-right 样式被所有子控件继承造成的文字旁竖线；参数标签换行，行距增大，下拉菜单展开后显示完整选项。
- 新增 14 个可调滤镜：TemporalMedian、FluxSmoothT/ST、SmartMedian、InterQuartileMean、DegrainMedian、Cnr4、CCD、DCTFilter、TemporalSoften、VerticalCleaner、CLAHE、Descale、RIFE。避免打包已不兼容 R80 的 API 3 插件。
- 内置 RIFE 4.26 / 4.26 Heavy NCNN Vulkan 插件和两套转换模型，NVIDIA 与 AMD 实际插值通过；高帧率处理链播放提高时间轮询频率以呈现中间帧。
- 限制：Jinc/Super-XBR/双边色度尚无一致的合成导出路径，选择这些核会明确提示；预览 D3D 与 VS 导出不保证逐像素相同。RIFE 单线程 1080p 本机实测约 12.88 / 10.97 张插值帧每秒，不能承诺实时。


## 1.0.1 — 2026-09-26

### 本轮更新

- 缩短每路视频顶部的源选择框，右侧新增 × 卸载按钮。卸载不删除源文件，其余视频保留解码会话、缩放和时间偏移；移除后可继续导入，全部移除后隐藏轨道。
- 颜色处理控件加宽；悬停显示当前算法完整名称，菜单项提供提示和互斥勾选标记。
- 修复 ABCD 滑块模式中 RGB 提示只在左下区域可见：提示定位到各路实际可见裁切区域，四个象限都能显示取样结果。
- VS 页面和分析页面的 VRR 低延迟、VRR Pacing 开关增加边框、悬停反馈和选中高亮。

### 本版本累计更新

- 新增独立图像分析比对页，主流视频格式直接解码，最多导入 9 路；AB、ABC、ABCD 裁切滑块与常规并排/网格对比分别提供。
- 滑块共用完整画布和同步缩放；修复拖柄丢失鼠标捕获、窗口缩放/全屏后渲染失效、上下平移灵敏度不一致。
- 最多 4 行对齐轨道，视频源切换自动交换位置，每个视频保留独立偏移；支持音频源选择。
- VS 异步初始化并显示预热进度；共享和部署原生着色器缓存，降低启动及首帧等待。
- 独立导出窗口，提供导出设置、准备文件和编码队列；仅输出 MKV，支持批处理、顺序编码、暂停/恢复、移除等待任务、停止保留输出和从头重做。
- 新增开源色度上采样、Spline36、Super-XBR 单阶段，以及 VS 去伪影和细节增强滤镜。Super-XBR 为开源 pass-0 移植，不是完整三阶段算法；不包含 NGU。

### 验证与发布边界

- Release 构建与 CTest 五组回归；增加真实视频卸载/重导入、保留会话与偏移、ABCD 四区域 RGB 提示和颜色选择状态检查。
- Qt 截图用于控件布局检查，原生视频通过呈现计数和像素探针验证；未覆盖所有真实长片、九路 4K 长时间播放及多显示器 DPI 场景。
- 生成 Windows x64 7z 与 SHA-256；Release 说明仅保存在本地，不上传或发布 GitHub Release。

</details>


````````
<!-- end:docs/history/development-records-through-2026-10-03.md -->

<a id="record-docs-icons-integration-md"></a>

## docs/icons-integration.md

原始字节数：2404；SHA-256：`a63c19b20572123ffb6b3d6d41d91e18664d639f3d7fd1e2c98c560d5860c180`。

<!-- begin:docs/icons-integration.md -->
````````markdown
# 图标接入 · 2026-10-02

用户选择：Renderer R3、Player P2、视频 V1、图片 I3。原始AI图片和候选保持不变，所选ICO复制至 `assets/icons/renderer.ico`、`player.ico`、`video.ico`、`image.ico`。

Windows资源：Renderer组101为R3；Player组101为P2、102为V1、103为I3。Qt通过内嵌qrc设置应用窗口图标。资源编译采用配置后的绝对ICO路径，MinGW图标RC不传无关C++头文件路径，解决空格目录的预处理问题；ICO设为资源对象依赖，后续替换会触发重编。

Player注册分别使用 VSPlayer.Video、VSPlayer.Image、VSPlayer.Audio；前两者DefaultIcon及TypeOverlay使用对应图标资源，音频使用应用图标。遵循[微软TypeOverlay机制](https://learn.microsoft.com/zh-cn/windows/win32/shell/thumbnail-providers)，由Windows叠加角标，不直接修改缩略图或替换已有处理程序。不安装新的图片/视频缩略图解码器，某格式能否显示缩略图仍取决于系统已有支持。

已有本程序注册格式在正式Player首次启动时更新图标和候选ProgID，兼容VSPlayer.Media打开命令，不改写UserChoice、不新增未注册的格式。Windows旧统一VSPlayer.Media默认关联可继续打开媒体，但图片专用角标需要在Player设置注册图片格式，并在Windows默认应用中重新选择对应格式的VS Player；新分类关联后视频/图片使用各自标记。

验证：Release两主程序及Player测试构建通过。`build/icons-tests.txt`三项相关专项含初始化/清理5/5、无跳过：Qt多尺寸图标读取、Windows组资源及LoadImage、分类关联/TypeOverlay、旧关联迁移与取消注册、其它应用项保留，以及原有图片/音频关联回归。只运行本轮相关测试。正式两EXE通过独立启动、读取WM_GETICON非空、仅向各自产生的窗口发送WM_CLOSE并退出0；未进行资源管理器缩略图视觉验收。

正式目录 `dist/VS-Renderer-GUI-windows-x64` 只替换两个EXE，旧EXE备份在 `build/icons-backup-20261002-203912`。构建与交付SHA256一致：

- VSRenderer.exe：4E052AB1D55547431F2C7C1B8FF62551C6A5FDA082F853D53F064D325E6E22F4
- vs-player.exe：E53B8CA17D5DF63B8B38237C26B331DC5075A1EEAAF284E0319786B7FC6435A0

原有INI、VPY及两份FFF.Native.dll共20项校验值保持；没有全量stage、修改LAV或发布压缩包，版本保持1.0.3。

````````
<!-- end:docs/icons-integration.md -->

<a id="record-docs-image-editor-interaction-1-0-4-md"></a>

## docs/image-editor-interaction-1.0.4.md

原始字节数：4151；SHA-256：`842b5d46ae04decbe6e8e3e536566e6884f41b9d4c9d388a105ba85ae22013aa`。

<!-- begin:docs/image-editor-interaction-1.0.4.md -->
````````markdown
# 图像编辑器：图层交互更新

2026-10-03，1.0.4 本地开发。代码不推送，不制作发布压缩包。

## 操作

- 编辑器控件和面板采用一致的灰色背景；颜色取样区、前景/背景色块仍显示实际颜色。选择、绘画、裁剪、文字等参数只在对应工具中显示，Shift/Alt提示移至悬停提示。
- 点击右侧图层行选中层并切换移动工具。混合模式、不透明度、蒙版及剪贴等控件统一作用于当前选中层；像素修改和移动不重建图层行。
- 移动工具显示整体内容边框及八个手柄，框内拖动移动，拖边或角缩放，Shift等比。框外点击按可见像素命中选择其它图层；图层列表仍可直接选择透明或被遮挡图层。
- 新文字建立独立、自动选中的文字像素层，可移动、缩放、合并和撤销。此轮文字仍栅格化，不支持回改文字内容。
- Ctrl+T：自由变换，可连续调整位置/尺寸，Enter提交一个撤销步骤，Esc回退进入变换之前的状态。缩放期间显示目标框，松手后处理图层像素，避免拖动每个鼠标点都重采样。
- Ctrl+E：当前层向下合并；编辑菜单还提供水平/垂直翻转、顺时针/逆时针90°和180°旋转。此轮自由变换是位置和尺寸调整，不含任意角度旋转、斜切、透视和变形网格。
- 移动工具的顶部X/Y/宽/高输入可精确设置图层范围并点击“应用”；自由变换期间这些输入暂时禁用，使用Enter或Esc结束本次事务。
- 标尺显示刻度数字与鼠标对应位置的轴标记；状态栏显示文档坐标X/Y，缩放、平移后仍使用同一坐标换算。

## 数据与边界

变换和合并保持RGBA8、RGBA16、RGBA32F和ICC，蒙版按同一变换映射。使用不可变数据快照及受限区域缓存按需读取，不因巨型PSD的一个图层操作生成整张扁平QImage；撤销恢复原图层数据。首次读取PSD压缩通道仍可能需要展开整个通道，与上一轮相同。

向下合并只处理同一父组内相邻、可见、未锁定且均使用Normal混合的像素层；组及剪贴链明确拒绝，以免改变与其它层的混合结果。不是多选图层合并或完整Photoshop图层类型支持。复杂PSD对象仍按上一轮说明导入为像素或显示警告。组本身可整体移动、缩放、翻转和直角旋转，同时变换后代和组蒙版；含锁定后代时拒绝。

## 验证

专项测试检查图层移动、Shift等比缩放、自由变换提交/取消、独立文字、选层及共享控制、快捷键、标尺位置和背景色。另用 `D:\Chem\自编讲义\OrganicARX\封面相关\封面-新.psd`（520.25MiB、5950×8420、16bit、9层）检查窗口、选层和变换撤销；原文件不覆盖。当前机器同时执行高负载FFmpeg，不以本次等待时间宣称性能倍数。

测试日志与截图保存在开发工作区 `build/`，不随便携程序分发。实际通过结果和部署哈希记录于 `build/image-interaction-delivery-audit.json`。

本轮最终13个专项case通过（Qt含初始化/清理共21项，零失败、零跳过）。UI检查使用Qt事件及窗口截图，不自动关闭用户当前播放器；新版EXE已部署到原便携路径，原进程继续运行原代码，需要重启播放器查看新版。Renderer、FFF.Native和player.ini部署前后SHA256保持一致，PSD样本SHA256保持 `6DA69581D0BA8E8EC901328A0132ED3C31DFBF76012345DDB34643F178304795`。

## 行为参考

菜单位置、边框、图层共享控制和快捷键参考 Adobe 官方的 [自由变换](https://helpx.adobe.com/photoshop/desktop/crop-resize-transform/resize-adjust-resolution/transform-images-shapes-and-paths.html)、[图层与合并](https://helpx.adobe.com/photoshop/using/layers.html)、[不透明度与混合](https://helpx.adobe.com/photoshop/desktop/create-manage-layers/apply-layer-effects/set-layer-opacity-and-blending-modes.html) 和 [标尺](https://helpx.adobe.com/photoshop/using/rulers.html)。使用本项目 MIT 代码实现，没有读取或复制本机 Photoshop 的专有程序代码。

````````
<!-- end:docs/image-editor-interaction-1.0.4.md -->

<a id="record-docs-image-editor-plan-md"></a>

## docs/image-editor-plan.md

原始字节数：10675；SHA-256：`331698da89bb3b96ed1483dcb999b37be78b8beaaae3dcc0c6578d2821fa9348`。

<!-- begin:docs/image-editor-plan.md -->
````````markdown
# 图像编辑方案与接口（2026-10-03）

## 已确认决策

- Qt 原生编辑器；主程序自有代码保持 MIT，独立依赖保留原许可。排除 Open-Photopea 提取代码，不复制 GIMP GPL 工具实现。
- 自建分块文档基础，16bit 整数和 32bit 浮点均为正式编辑精度。最初把真实 HDR 显示后置；本轮用户明确要求加入 HDR，因此已接入 Windows D3D11/scRGB 输出及 SDR 回退。高精度编辑、显示能力检测和真实 HDR 显示分别验收；当前用户没有 HDR 显示器，不能以回退测试代替真实面板验收。
- RAW 以手机 DNG/RAWPlus 为主；AVIF 复用已有原生解码。PSD 独立模块，首期像素图层、像素蒙版、混合模式；复杂对象提示或有可靠像素时栅格化。
- 本会话开发基础接口；额外工具在独立会话开发，共享下面的接口，不各自建立文档/撤销体系。
- 仍为 1.0.4 开发版，不推送 GitHub、不打包。

## 分期与候选组件

1. a0：分块像素、受限 RAM 缓存/临时磁盘、图层元数据、16bit 蒙版/选区、局部撤销、合成、颜色元数据。
2. a1：DNG 用 LibRaw 候选，仍未实现。PSD 初期候选为 PhotoshopAPI / PSD SDK；本轮实际采用自有 RGB PSD/PSB 读写模块与 zlib，支持像素图层、组、像素蒙版和有限混合模式。PSD 图层工程与扁平导出分开，不承诺完整 Photoshop 对象保真。
3. b：移动、矩形/椭圆、套索/多边形/磁性、魔棒/快速/框选辅助分割、裁剪、吸管/标尺、污点/源修复、画笔/颜色替换，再推进图章、橡皮、渐变/填充、局部调整、路径/文字/形状、历史画笔及切片。
4. c：文件、编辑、图像、图层、选择、滤镜、视图、窗口八组菜单；菜单/工具/快捷键共享 QAction，文档拥有撤销栈。
5. d：初期候选为 LittleCMS；本轮使用 Qt QColorSpace 的 RGB ICC 转换，自实现色阶/曲线与基础颜色调整，未整合 LittleCMS / OpenColorIO。浮点负值与大于1值保留；SDR 显示映射不修改文档数据。

OpenCV、libmypaint 曾作为候选，本轮未引入。磁性选择采用局部亮度梯度吸附，魔棒/快速选择采用颜色连通区域，框选辅助分割从边框背景颜色连通扩展；无源污点修复采用固定边界的谐和插值，源修复为取样纹理与局部均值匹配，均使用当前原始精度。它们不是 GrabCut、Telea 或 GIMP 算法的复制。GEGL/babl 首期不引入；体积及最终部署验证见本轮记录。

## a0 接口契约 v1

`src/image/ImageDocument.h` 是工具会话唯一文档入口。

- RGBA8 / RGBA16 / RGBA32F 原始非预乘载荷，256×256稀疏块；缺失像素透明，缺失图层蒙版白色。首次选区外部默认黑色，未启用选区时默认白色。
- layers() 从底到顶，稳定 QUuid；图层偏移采用文档坐标，readRegion/writeRegion/writeMask 采用图层本地坐标。
- 图层蒙版与选区用 Grayscale16 覆盖率，不做 ICC/gamma 变换。
- composite(rect) 返回文档原精度；首期四种混合本轮扩展为正常、正片叠底、滤色、叠加、变暗/变亮、颜色/线性加深与减淡、柔光、强光、差值、排除、减去、划分。支持组隔离/穿透与剪贴属性，但不承诺完整 PSD 混合等价。
- beginEdit/commitEdit/cancelEdit 使连续笔划成为一次历史；像素块不可变共享，撤销只引用变化块，不保存全图快照。
- document 接口仅在所属线程调用；异步工具接收独立区域图像，提交前比较 revision，过期结果丢弃。
- setColorSpace 仅标记工作空间，调用者负责颜色转换；源ICC/CICP/来源记录在metadata中，不能把相机RGB直接标成sRGB。
- 普通图片查看解码仍输出完整 QImage；PSD 详细编辑走独立图层读取，避免先扁平化再导入。PSD 像素通道延迟展开到临时磁盘，编辑按区域加载；首次显示相关图层仍可能展开完整压缩通道，不是全链路零解码。文档快照用于后台预览与保存；预览仍以受限分辨率采样，不是高质量多级金字塔。

## 分工与验收

基础会话拥有 src/image/ImageDocument.*、tests/TestImageDocument.cpp 及这些文件的 CMake 接入。工具会话拥有新 ImageEditorWindow/Canvas/Tools 文件，Player 详细编辑入口及相关集成测试；不得修改基础实现。共享 CMake 和文档变动需串行协调。

基础专项：跨块写入、强制换出后原像素一致、16bit低位保持、浮点负值/HDR高值保留、蒙版/选区、Alpha合成、图层变换、笔划提交/取消/撤销/重做。只跑本次相关测试，不做性能结论。

## 调研来源

- https://github.com/EmilDohne/PhotoshopAPI
- https://github.com/MolecularMatters/psd_sdk/blob/master/src/Psd/PsdExport.cpp
- https://www.libraw.org/about
- https://www.libvips.org/API/current/how-it-works.html
- https://opencv.org/license/
- https://github.com/mypaint/libmypaint/blob/master/COPYING
- https://www.littlecms.com/plugin/
- https://www.photopea.com/learn/

精确实时菜单叶项/排序仍需逐项确认；上述是功能分组与本项目实现方案，不是闭源算法推断。

## 首轮基础接口的历史验证

已实现a0接口：8/16bit整数与Float32分块载荷、四种基础混合、图层属性/排序、16bit蒙版/选区、局部事务撤销/取消、默认64 MiB LRU缓存与临时磁盘换出。compositePreview直接采样原块，输出受指定尺寸限制，当前最近邻预览，不是高质量金字塔。数据读取/缓存换出保留透明像素隐藏RGB；扁平Alpha合成不承诺保留完全透明区域的隐藏颜色。

独立工具会话接入详细编辑窗口、图层/历史面板、移动、矩形/椭圆、套索/多边形、画笔/橡皮、吸管、抓手和缩放。PNG仅8/16bit扁平输出，256 MiB像素数据暂限；Float32明确拒绝PNG降精度保存。几何选区当前栅格范围限64M像素，巨图选区相交暂不支持。窗口关闭时提醒丢失未保存的图层与历史。

专项日志：build/image-core-tests.txt 四项（含初始化/清理6通过），build/image-editor-tests.txt 九项（含初始化/清理11通过）。包括强制换出、缓存释放、跨块高精度、浮点负值/12.5、笔划取消/撤销、蒙版/选区/图层、PNG低位/ICC、真实鼠标交互、无修改关闭、稀疏100000px画布及Player入口。窗口截图build/image-editor-smoke.png用于布局检查，不是巨图性能或真实HDR证明。

以上是首轮基础阶段的状态与日志，不代表本轮 B–D 仍全部缺失；本轮实际能力如下。

补充：selectionId()跟随选区内容进入历史状态，非选区编辑不改变ID，撤销/重做恢复旧ID；轮廓不绑定可复用历史索引。真实画布新增A/B选区撤销后分支替换、101次编辑触发历史上限后轮廓保持专项，均通过。旧Player工具栏测试同步详细编辑启用行为，只跑相关入口/对话框专项。

## 本轮 B–D、PSD 与 HDR 状态

界面按 Photoshop 系列工作区重新组织：窄图标工具栏及右键同组工具、随工具变化的顶部选项、文档页签、双轴标尺、中央画布、右侧颜色/色板、属性/调整、图层/通道/历史/路径面板。菜单与工具共享 QAction；窗口不是完整 Photoshop 菜单的复制。

| 范围 | 已接入功能 | 实现边界 |
| --- | --- | --- |
| B 选择与几何 | 矩形/椭圆、套索/多边形/磁性套索、魔棒、快速选择、框选辅助分割、反选/羽化、可移动和边角调整的比例裁剪、吸管/标尺、切片 | 框选分割不具备人物等语义识别；几何选区 64M 像素，颜色连通/羽化等整幅操作 16M 像素限制；超过限额明确报错 |
| B 像素工具 | 画笔/橡皮、颜色替换、图章、污点/源修复、历史画笔、渐变/油漆桶、局部模糊/锐化/加深/减淡/海绵 | 无源修复半径≤128；图章源缓存2048²；历史源≤64 MiB，保留原精度 |
| B 路径与内容 | 直线锚点路径，可描边/填充/转选区；文字与矩形/椭圆形状 | 尚无 Bezier 控制柄；文字/形状明确栅格化，尚无可回改文字图层、完整矢量对象或路径工程持久化 |
| C 菜单与图层 | 文件/编辑/图像/图层/选择/滤镜/视图/窗口；新建/复制/删除/重命名/排序/锁定/显隐/透明度/混合/组/蒙版；图层工程另存及扁平导出 | PSD 保留支持的图层数据，不保存编辑历史；复杂对象未实现完整编辑器 |
| D 颜色与通道 | RGB/Alpha 预览、蒙版预览、曝光、亮度/对比度、色阶、RGB/单通道曲线、色相/饱和度、颜色增益、通道混合、反相/灰度/阈值/色调分离；Qt RGB ICC 转换 | 曲线为控制点分段线性和端点斜率外推；直方图为预览采样；没有完整 LittleCMS 印刷/软打样、ACES/LUT、专色与多通道工作流 |
| HDR | Float32 编辑、SDR 显示曝光/白点映射、Windows FP16 scRGB swap chain、自动检测与 SDR 回退、流式32bit浮点 TIFF | 原始数据与显示映射独立；真实 HDR 面板观感尚待验证，不能把普通 SDR 回退当作 HDR 显示验收 |

PSD/PSB 当前读取 RGB 8/16/32bit、RAW/RLE/ZIP/ZIP 预测通道，支持层组、像素蒙版、有限混合与锁定/显隐等属性。复杂对象有可用 RGB 像素时栅格化并提示；没有像素时明确提示不渲染。CMYK/Lab 等文档需要先转 RGB，不支持完整文字、智能对象、矢量蒙版、调整层和图层效果保真。

8/16bit PSD/PSB 可另存支持的图层及有效合并预览。32F PSD 保存必须保有源 Photoshop HDR 色彩数据（`hdrt`）；新建或从普通图片转来的 Float32 文档目前不能自行构造这些数据，应导出保留负值及大于1值的32bit浮点 TIFF。PNG 仅8/16bit扁平输出，暂限256 MiB像素；导出PNG/TIFF不包含图层与历史。原文件受到另存路径保护。

真实样本使用 `D:\Chem\自编讲义\OrganicARX\封面相关\封面-新.psd`：545,521,646字节（约520.25 MiB）、5950×8420、16bit RGB、9层。专项覆盖图层读取、预览、单层局部写入和撤销，不用当前高负载环境推导打开耗时或性能提升比例。

最终构建、40项相关专项、截图、PSD独立读取与往返、部署审计见 [本轮交付记录](image-editor-round2-1.0.4.md)。未执行本次无关的全量回归；DNG、完整复杂PSD对象和上述高级矢量/颜色工作流仍未实现。

````````
<!-- end:docs/image-editor-plan.md -->

<a id="record-docs-image-editor-round2-1-0-4-md"></a>

## docs/image-editor-round2-1.0.4.md

原始字节数：9415；SHA-256：`b799d2fd5156125437ff9183f18b7f2d709015bbc7a21cd544dd234e7a595789`。

<!-- begin:docs/image-editor-round2-1.0.4.md -->
````````markdown
# 图像编辑第二轮：B–D、PSD 与 HDR

日期：2026-10-03。版本：1.0.4 本地开发；本轮不推送 GitHub，不制作发布压缩包。

## 本轮行为

“详细编辑”使用 Photoshop 系列工作区：左侧42px图标工具栏，右键切换同组工具；顶部显示当前工具的大小、透明度、选区组合、容差、文字或裁剪比例；中央文档页签和双轴标尺；右侧颜色/色板、属性/调整、图层/通道/历史/路径面板。面板可从窗口菜单显隐，工具和菜单使用共享 QAction。

PSD 详细编辑直接读源文件图层，不以播放器的扁平预览作为编辑文档。图层树可选择组内子层，支持新建/复制/删除/重命名/排序、显隐、锁定、透明度、混合、组穿透和剪贴属性；移动组同时移动后代。像素与蒙版采用同一文档撤销事务，蒙版黑隐藏、白显示，支持像素层和组蒙版。

画布预览使用不可变文档快照在后台生成，旧预览结果与修订号不一致时丢弃；PSD/浮点 TIFF 另存也使用后台快照。查看器也复用原生PSD/PSB图层合成，支持ZIP合并预览在FFmpeg中不能读取的文件；普通查看输出完整QImage，详细编辑仍直接读原文件图层。编辑器“打开”复用播放器JPEG/AVIF等解码器。文档仍保留8/16bit整数和32bit浮点原始像素、256px稀疏块、受限RAM缓存及临时磁盘。

## 工具与颜色

| 功能组 | 当前实现 |
| --- | --- |
| 选择 | 矩形/椭圆、自由/多边形套索、局部边缘吸附磁性套索、颜色连通魔棒/快速选择、边框背景辅助分割、选区添加/减去/相交、反选/羽化 |
| 几何与测量 | 可移动并调整四角/边的裁剪框；自由、1:1、16:9、4:3、3:4、9:16和自定义比例；吸管、标尺、切片PNG、抓手和缩放 |
| 像素工具 | 画笔/橡皮、按颜色容差替换且保留Alpha、取样图章、无源谐和污点修复/源纹理修复、显式历史画笔源、线性渐变、颜色连通油漆桶 |
| 局部调整 | 模糊、锐化、加深、减淡、海绵去饱和；图层蒙版支持画笔/橡皮、模糊/锐化、填充与渐变 |
| 路径与内容 | 直线锚点路径，可描边、填充或转选区；按字体/字号栅格化文字，栅格化矩形/椭圆形状 |
| 颜色 | 曝光、亮度/对比度、色阶、RGB及单通道曲线、色相/饱和度、RGB颜色增益、通道混合器、反相、灰度、阈值、色调分离 |
| 滤镜与空间 | 高斯模糊、USM锐化、中值降噪、边缘检测；工作RGB ICC转换；RGB/独立RGB通道/Alpha及蒙版预览 |

颜色调整只修改当前未锁定像素图层，按选区覆盖率应用，并保持原精度与Alpha。曲线使用分段线性控制点及端点斜率外推；显示直方图来自256×128合成预览采样。ICC 转换使用 Qt QColorSpace，可选 sRGB、Linear sRGB、Display P3、Adobe RGB、ProPhoto RGB；没有把未标记相机RGB直接当作正确sRGB。

## PSD/PSB 与真实样本

自有模块支持 RGB 8/16/32bit PSD/PSB，读取RAW、RLE、ZIP与ZIP预测压缩通道，保留支持的像素图层、组、像素蒙版、显隐、锁定、透明度、剪贴和混合属性。首次需要相关图层时，压缩通道展开至临时磁盘，随后按区域读取；隐藏层不会仅因建立图层树就全部解码。首次显示仍可能解压完整通道，并未实现压缩格式内部随机区域解码或高质量预览金字塔。

PSD超过30000px应使用PSB；PSD/PSB均不超过300000px。8/16bit图层可另存PSD/PSB，并写入有效合并预览，供其它读取器查看。32bit保存需保留源Photoshop HDR色彩数据 `hdrt`；新建或由普通图片转换的Float32文档没有这项数据时明确拒绝PSD保存，应导出32bit浮点TIFF。PSD不保存本编辑器撤销历史，蒙版按目标PSD通道位深编码。

测试样本：`D:\Chem\自编讲义\OrganicARX\封面相关\封面-新.psd`，545,521,646字节（520.25 MiB）、5950×8420、16bit RGB、9层。专项检查层名称/数量、预览、局部像素编辑和撤销；原文件不得覆盖。当前机器同时执行高负载FFmpeg任务，不能据本轮等待时间给出解码性能倍数或稳定打开耗时。

## HDR 与保存

高精度编辑、显示映射与HDR输出分别处理。Float32的负值及大于1的RGB值不因预览被改写。SDR预览可调整曝光/白点并启用HDR→SDR映射；原生Windows输出检测显示器及系统HDR开关，使用D3D11 FP16 scRGB交换链（1.0为80nits），不支持时回退SDR并显示原因。

AVIF保留ICC优先；没有有效ICC时识别已知CICP RGB原色及sRGB/线性/PQ/HLG传递函数，不把未知色彩空间伪标记。PQ/HLG输入自动启用SDR显示映射；Qt 6.10将PQ的10000nit端点归一为64线性单位，原生输出据此换算到1.0=80nit的scRGB；相对线性/SDR使用203nit纸白，HLG还未实现基于场景元数据的完整OOTF适配。依据：[Qt PQ/HLG归一实现](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/gui/painting/qcolortransfergeneric_p.h)。

32bit浮点TIFF按条带输出、非预乘Alpha及ICC，保留负值与高光，超过经典TIFF偏移范围时使用BigTIFF。PNG只导出8/16bit扁平结果，当前像素数据限256 MiB；Float32拒绝PNG隐式降精度。扁平TIFF/PNG不含图层与历史，另存保护源文件。

当前没有HDR显示器：可以验证shader编译、能力检测、SDR回退、原始数值及导出数据，不能宣称已验证真实HDR面板的亮度或观感。

## 明确限制

- 尚无Bezier控制柄、可回改文字图层、完整矢量对象、路径工程持久化或语义AI对象选择；框选分割以边框背景颜色连通为依据。
- 复杂PSD对象有可用RGB像素时栅格化并提示，没有像素则明确提示不渲染；不承诺完整智能对象、文字、矢量蒙版、调整层、图层效果和全部混合模式保真。CMYK/Lab等需先转RGB。
- 几何选区栅格化范围限64M像素；颜色连通、框选辅助分割、整幅填充/渐变/反选/羽化等当前限16M像素；超限报错，快速选择可在较大图上操作局部区域。
- 无源污点修复半径≤128px，较大区域需取样源修复；图章源缓存2048²，历史画笔源≤64 MiB原精度像素。
- 原生PSD图层读取当前不导入只含合并图像、没有可编辑图层记录的PSD；普通TIFF/EXR等查看仍走已有解码路径，不承诺所有外部高精度格式的Float32导入往返。浮点TIFF导出数值已独立验证。
- 尚未实现DNG/LibRaw、LittleCMS专业印刷/软打样、多通道/专色、ACES/LUT及无损保存所有未知PSD资源；本轮不引入巨型AI模型，不复制GIMP GPL或提取的闭源Photopea代码。

## 验证与交付

只运行本轮相关专项，不执行无关全量回归。40个实际测试槽通过（QtTest初始化/清理另计，日志合计54 passed），0失败；PSD压缩测试槽内部遍历8/16/32bit × RAW/RLE/ZIP/ZIP预测共12组输入。可选真实样本已启用。

| 项目 | 结果与证据 |
| --- | --- |
| Release构建 | `build/image-round2-final-build.txt`、`build/image-round2-sample-viewer-build.txt`；无新增运行时DLL |
| 核心 | 4项，`build/image-round2-core-final.txt`；高精度、缓存、蒙版/选区、混合极值与柔光暗部 |
| PSD | 6项，`build/image-round2-psd-final.txt`；12组独立压缩fixture、组/蒙版往返、Lazy精度链、快照生命周期、损坏输入、真实520MiB源文件 |
| 工具和画布 | 5+9项，`build/image-round2-advanced-final.txt`、`build/image-round2-editor-final.txt`；原精度、连通填充、组移动、蒙版、撤销和真实鼠标手势 |
| 工作区 | 6项，`build/image-round2-layout-final.txt`；组内子层编辑、对话框、CICP P3/PQ AVIF打开、后台保存快照/继续编辑/源文件保护，以及9层真实PSD窗口笔划和精确撤销 |
| 颜色/HDR | 7项，`build/image-round2-adjustments-final.txt`；父组锁定、ICC/曝光、滤镜跨块边界、负值/高光浮点TIFF、shader编译/SDR回退、Qt PQ归一常量 |
| Player | 3项，`build/image-round2-player-final.txt`；轨道默认选择（无Default字幕取首条）、图片工具栏、原生AVIF编码/解码入口 |
| 独立PSD读取器 | psd-tools 1.23.0确认组/子层、RGB16及蒙版123低位、合并Alpha；`build/image-round2-psd-independent.json`。FFmpeg尚未实现ZIP合并预览，未把这一解码器限制误判为PSD输出损坏 |
| 实际大PSD普通查看 | `build/image-round2-sample-viewer.txt`；5950×8420完整RGBA16、原生PSD解码路径、与图层合成像素一致；不作为性能基准 |
| 本地部署 | 已更新 `dist/VS-Renderer-GUI-windows-x64/vs-player.exe` 与本轮文档/许可；`build/image-round2-delivery-audit.json`记录构建/便携exe一致、Renderer/native/player.ini及样本哈希保持。仅本地1.0.4，未推送/打包 |

开发工作区截图：`build/image-editor-large-psd.png`（不随便携包分发）。源文件SHA-256：`6da69581d0ba8e8ec901328a0132ed3c31dfbf76012345ddb34643f178304795`，本轮未覆盖原文件。

真实HDR面板亮度、色彩观感和稳定呈现未验证；保留SDR回退与数值验收结论，不以编译或普通屏截图替代HDR实屏验证。

````````
<!-- end:docs/image-editor-round2-1.0.4.md -->

<a id="record-docs-image-editor-workflows-1-0-4-md"></a>

## docs/image-editor-workflows-1.0.4.md

原始字节数：4297；SHA-256：`5cef237826edcdb70027aebf4ac212aa3a2331d43f04c3fc29268a9fb0a98f3f`。

<!-- begin:docs/image-editor-workflows-1.0.4.md -->
````````markdown
# 1.0.4 图像浏览、文档与混合选项

本轮仍为本地开发版，未发布、未打包。图像工具入口为播放器顶部“详细编辑”。

## 浏览与显示

- 查看器和编辑器看图时，滚轮上下移动，Shift+滚轮左右移动，Ctrl+滚轮缩放；编辑器还提供缩放工具、视图菜单和加减快捷键。视频原有滚轮设置保持有效。
- 编辑器放大使用可见区域的原生像素和平滑显示，预览按屏幕缩放比例计算；缩小采用双线性采样。普通整数 SDR 显示直接转换到 sRGB，避免无必要的浮点往返；原始8/16位和Float32编辑数据不受显示转换影响。
- 预览包含视口周边缓存，缓存覆盖范围内平移或继续放大不重复合成。原生预览像素预算为16M，单边不超过8192；超出预算调整预览采样，不限制图像的编辑尺寸。
- 双线性缩小不是面积抗混叠、Jinc或Lanczos；预览质量不代表离线导出的重采样算法。第一次读取PSD压缩通道仍需解压，缓存改善后续操作，不能免除首次读取。

## 锁定与多文档

- 普通图片初始“背景”层锁定；副本和新建层解锁。PSD保留文件原有锁定标记。图层面板下方增加锁定按钮；锁定层或其锁定父组不能移动、缩放。
- 多张图片在同一编辑窗口使用文档标签；再次打开同一路径激活已有文档，不替换未保存编辑。新建的未命名文档分别保留。
- 每份文档保留自己的画布位置、工具、图层选中、历史及显示设置。Ctrl+Tab/ Ctrl+Shift+Tab切换，窗口菜单列出文档；关闭标签或整个窗口检查未保存改动。
- 后台PSD保存按开始保存时的文档和版本记录完成状态，不因切换标签而错误标记另一个文档已保存。

## 图层右键与混合选项

右键图层提供新建图层/组、复制、从图层建组、导出图层PNG、另存PSD、向下合并、合并可见层、拼合、锁定、重命名、显隐、混合选项与删除。像素模型不能实现的智能对象、CSS/SVG、画板等操作不显示。

混合选项提供16种现有混合模式、不透明度、填充不透明度、RGB参与通道，以及灰色/红/绿/蓝的本层与下一层Blend If范围。黑白端点可按Alt拆分过渡，也可输入数值；已设置的多个通道范围同时作用。预览可以开关，复位还原默认混合参数；取消恢复打开前状态，确定合并为一次历史记录。

填充在本轮作为像素覆盖率参与合成，不声称完全复现Photoshop某些特殊混合模式的专用Fill公式。当前没有图层特效、挖空及向量蒙版特效模型，相应选项未添加。PSD使用标准混合范围及iOpa/brst字段保存本轮可表达参数。

依赖其它下方图层的RGB通道限制或下一层Blend If，不能通过两层向下合并无损固化，操作会明确提示。变换/合并后的惰性来源仍可能产生原生瓦片读取成本，首次缩小不保证复用原PSD行预览。

单层PNG保持画布大小、原位深和蒙版，Float32要求浮点TIFF或PSD，不静默降位深；PNG分配沿用256MiB上限。

行为参考：[Adobe混合与不透明度](https://helpx.adobe.com/photoshop/using/layer-opacity-blending.html)、[Adobe文档窗口](https://helpx.adobe.com/photoshop/desktop/get-started/learn-the-basics/rearrange-document-windows.html)。未复制Photoshop或GPL编辑器代码，主程序仍为MIT。

## 专项验证

只运行本轮相关的显示采样、视口缓存、滚轮、锁定、多文档与混合/PSD往返专项。使用维护者提供的5950×8420、16位、约520MiB PSD验证可见区域和文档切换；本机并发高负载影响耗时，不将测试时间作为性能基准。详细结果和交付哈希记入build下本轮审计文件。

2026-10-03：14个相关用例通过，涵盖平滑参考图对照、SDR精确显示、100k²惰性视口缓存、锁定、多文档隔离、混合预览取消/确认与撤销、原位深PSD参数往返、HDR AVIF同窗打开、跨文档后台保存，以及实际PSD的100%/200%放大与平移。psd-tools独立检查iOpa、brst及四组标准混合范围通过。测试中发现并修复撤销栈析构信号导致的窗口关闭崩溃。

````````
<!-- end:docs/image-editor-workflows-1.0.4.md -->

<a id="record-docs-input-av1-yuv444p12-diagnosis-md"></a>

## docs/input-av1-yuv444p12-diagnosis.md

原始字节数：5459；SHA-256：`8202aa53a53896b9db48108903cc5e48dab2ce12a28210a4aeb4a7f869779b41`。

<!-- begin:docs/input-av1-yuv444p12-diagnosis.md -->
````````markdown
# AV1 yuv444p12 软件解码诊断

测试日期：2026-10-10。仅诊断，未修改源代码、程序、设置或原始媒体。

vsp的内存占用极致模式能开到满，因此还可以继续提高占用（上述估算值仍未满）。根据这个报告继续进行更新，自动根据处理器线程数执行解码，大幅提高SW解码性能。下方放大卡顿等内容似乎在刚才一轮中已经修复，可做一遍验证。视频已经放在NUXBOX设备的D盘根目录下。



## 文件与环境

- 文件：`D:\Animation Enhance\GBC 108048\OP-yuv444p12-SWTest.mkv`
- AV1 Professional，3840×2160，yuv444p12le，7001/146 ≈ 47.952 fps。
- 时长 92.09 秒，4415 个视频帧；文件 943306349 字节，平均容器码率约 81.95 Mbps。
- CPU：Ryzen 9 7940HX，16 核、32 逻辑线程。
- 当前解码器：libdav1d 1.5.4-0-g54706fc6。
- 起始帧头显示 1×1 tile，没有 film grain。未逐帧检查整片 tile 布局。

## 解码吞吐对照

直接使用当前 FFmpeg/libdav1d 解码至 null，无音频、字幕、缩放、像素格式转换和显示输出。保留 12 位 4:4:4。下表均取相同的前 480 个输出帧。CPU 为进程累计 CPU 时间除以墙钟时间及 32 个逻辑线程的估算值，不等同于任务管理器的频率加权读数。

| 工作线程 | max_frame_delay | 平均解码 fps | CPU 估算占用 | 峰值进程 RSS |
|---|---|---:|---:|---:|
| 32 | 自动，实际 6 帧上下文 | 36.45 | 14.43% | 1.48 GiB |
| 32 | 16 | 70.47 | 28.41% | 3.09 GiB |
| 32 | 32 | 78.70 | 44.03% | 5.57 GiB |
| 64 | 64 | 85.44 | 65.96% | 10.53 GiB |

64 线程超出本机逻辑线程数，只用于对照；相对 32/32 提升有限、内存明显增加，不建议直接作为默认设置。

32 线程、32 帧并行的全片测试：4415 帧全部解码，耗时 63.654 秒，平均 69.36 fps（约 1.45 倍实时），CPU 估算 43.39%，峰值 RSS 5.60 GiB，零解码错误，退出码 0。该结果证明纯解码能力足够，不代表播放器修改后已经通过播放验收。

默认 32 线程在源片 10 秒位置开始、输出 600 帧的对照测试仍仅 36.30 fps，零解码错误。因此限制不只发生在起播阶段。

初始短测试期间存在用户已有的 3FUI VMAF 后台任务，未中止它。该任务自行结束后重测默认配置；上表默认行采用重测结果，不采用最初约 21 fps 的受干扰结果。

## CPU 低占用的原因

播放器软解路径设置了 32 个工作线程，但没有设置 dav1d 的 max_frame_delay。dav1d 1.5.4 在该参数为零时，对于 32 个线程自动选择 ceil(sqrt(32)) = 6 个帧上下文。工作线程数不是同时参与解码的帧数。

本片起始帧只有一个 tile，帧内并行机会受到限制；帧间参考依赖同样限制任务可运行的时机。增加帧上下文后吞吐和 CPU 活跃程度显著增加，说明当前默认帧并行深度是实测的重要限制，而非 CPU 总算力不足。不能将“6 帧上下文”解释为“只使用 6 个操作系统线程”。

官方实现：
- https://raw.githubusercontent.com/videolan/dav1d/1.5.4/src/lib.c
- https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavcodec/libdav1d.c

## VSP 额外放大卡顿的机制

使用项目已有 perf-3fp.exe，1920×1080 显示窗口、强制软解、预缩放关闭、原生算法。探针在就绪后先 seek(0) 再播放，不能完全等同于普通 UI 无跳转起播。

当前运行库首次测试：约 2 秒后 accepted 停在 83；到约 19 秒，decoded 已增长到 711，dropped 达 629，呈现画面不再推进。重复测试 accepted 停在 125，decoded 和 dropped 继续增长。

PlayerSession.cpp 的播放路径会丢弃落后音频时钟超过 max(两帧时长, 50ms) 的帧。本片阈值为 50ms。当解码持续约 36 fps，而播放要求约 48 fps 时，落后后可能连续丢弃全部后续帧，无法靠较慢解码自行追上音频。这解释了低吞吐被放大成停留旧画面的现象。

同一原生 DLL 配合较旧 FFmpeg 运行库，约 20 秒测试也发生 accepted 停在 94 的现象。因此连续过期丢帧问题不只出现在用户手动更新后的运行库。

## 尚未定位的解码报错

当前运行库的两次原生探针都出现：

`Software video decoder failed: Invalid data found when processing input`

两次发生位置不同，约 16.2 秒和 7.9 秒。独立 FFmpeg 全片测试零错误；默认并行的 10 秒起点测试也零错误。不能据此认定媒体存在固定坏帧。

较旧运行库的一次短测试未出现该错误，但不足以证明是新库兼容问题。异步解码调用、跳转交接或运行库差异的具体责任尚未定位，本轮未增加代码诊断或进行修复。

## 后续修复方向（本轮未实施）

1. 针对该类软件解码，提高帧并行深度；16 帧在当前样本中已有实时余量，并且比 32 帧明显省内存。仍须评估首次出帧及跳转延迟。
2. 修复持续落后时的追赶策略，避免一直丢帧却一直无法恢复新画面。
3. 独立定位原生探针的解码报错；解码吞吐提升不能替代此项回归。

这些结论不需要以降低位深、减少色度或 CPU 预缩放来解释。其他播放器是否使用同一并行策略未逐一确认，不能一概认定其内部机制相同。

原始日志和 CSV 保存在本报告所在目录；git 工作区检查无源码改动。

````````
<!-- end:docs/input-av1-yuv444p12-diagnosis.md -->

<a id="record-docs-jpeg-xl-1-0-11-md"></a>

## docs/jpeg-xl-1.0.11.md

原始字节数：3744；SHA-256：`0507c1c5fea0b8f872cb9be0c3330c0c3844956233f3357a2b1deb76e0b35187`。

<!-- begin:docs/jpeg-xl-1.0.11.md -->
````````markdown
# JPEG XL 截图（1.0.11 续修）

VSCap 新配置的默认保存格式为 JXL，顺序为 JXL、AVIF、PNG；已有明确保存的格式设置不被重置。截图工具条、贴图右键菜单均有三种格式。取消保存仍保留截图层和标注，打勾只复制剪贴板。复制 JXL 文件后 Alt+T 支持贴图。

JXL 为无损 RGBA16，每通道 16 位，不使用 XYB 有损变换。HDR 桌面 scRGB 按 80 nit 单位转换为 BT.2020 / ST 2084 PQ，色彩信息写入 JXL 原生结构，不依赖 PNG 的 ICC TRC 兼容路径。桌面 SDR 白位嵌入标准容器 XML box，VSP 读取后继续使用白位修复逻辑。JXL 的无损指编码后的 16 位像素无损，不表示浮点 scRGB 转为整数 PQ 时完全没有量化或色域裁切。

HDR 自动保存跟随默认格式。JXL / PNG 继续同时输出 HDR 与 `_SDR` 副本，无 JSON、裸浮点附件；AVIF 使用原有 AWJ 异步保存。普通 SDR 截图不自动落盘，手动保存每次选择路径并记住目录。PNG 仍为压缩 RGBA16；AVIF 仍为质量 70 的有损格式，不声称 16 位无损。

VSP 的“视频 → 图像截取”和“截图到指定路径…”均包含 PNG、AVIF、JXL，分别支持源画面与含字幕/显示效果的实画面。JXL / AVIF 截取只输出单个文件；PNG 保留原有 HDR 附件流程。AVIF Full 优先使用 AWJ（AOM，质量 70，4:4:4；SDR 10 位、HDR 12 位）；无 AWJ 的独立版使用当前目录 / PATH 的 FFmpeg（需要 libaom-av1，CRF 19），不把 40 MB AWJ 塞回单文件。

libjxl 0.12.0 与 Brotli、Highway、Little CMS 使用同一套 MinGW 静态编译，Full / Lite 均内置 JXL 编解码，不依赖外部 FFmpeg 来保存或打开 JXL。修订和依赖固定在 `tools/jxl-runtime.cmake` 及 libjxl 子模块提交，许可全文随 `licenses/jpeg-xl-LICENSE.txt` 分发。MinGW 构建对 `stage_write.cc` 的 `MakeUnsigned` 增加 `HWY_INLINE`，绕过 GCC 13 / Win64 的未对齐 AVX 参数栈槽问题，保留 AVX2；未修改色彩计算。

色彩正确依赖文件标记与查看器的 HDR 支持。JXL 原生标记能避开此前 PNG ICC 解释分歧，但不能保证不支持 JXL / HDR 的 QQ、照片等所有版本都显示相同；SDR 兼容分发使用 `_SDR` 副本。不会把 480 nit 硬编码为新截图默认白位。

验证记录：`build/audit-jxl-20261017/`。自动测试覆盖 sRGB / PQ / HLG 的 RGBA16 逐点往返、透明像素、截断文件、嵌入白位、VSCap 自动保存与 JXL 文件贴图、取消三种保存、播放器三种格式和源画面截取。实机 HDR 热键采集、第三方查看器显示仍需维护者验收。

AVIF 外部 FFmpeg 路径同时给编码器参数与帧 `setparams` 写入色域 / 传递函数 / 矩阵；只设输出参数时本机 FFmpeg 会产生 2/2 的未指定标记。已在无 AWJ 的隔离目录验证输出为 SDR 1/13/1、HDR 9/16/9，避免将 HDR 写成色彩信息不明的 AVIF。

`D:/Screens/过曝.png`（3840×2034）转换测试：原 PNG 9,030,654 字节；JXL 6,418,884 字节。一次本机测量编码 531 ms、解码 136 ms，全部 RGBA16 像素一致，嵌入历史验证白位后 SDR 预览也完全一致。该 480 nit 值只用于历史样图测试；原文件未改动。尺寸与耗时仅代表此图和此机器，不表示所有内容均有相同比例。

本轮结果：精度 / 样图测试 15 通过、2 项可选设备测试跳过；截图交互 17 通过；播放器截取和已有 PNG 回归 20 通过；无 AWJ 隔离路径的格式 / scRGB 截取 9 通过，均无失败。初次构建暴露的 SIMD 崩溃已由内联补丁修复；新增 203 nit scRGB 测试使用直接浮点像素，避免 `QImage::fill(QColor)` 的高值裁切干扰夹具。

````````
<!-- end:docs/jpeg-xl-1.0.11.md -->

<a id="record-docs-local-1-0-5-performance-md"></a>

## docs/local-1.0.5-performance.md

原始字节数：7112；SHA-256：`85cbf0460573daf1cb2b36b1d26dee83dafb1f627d7140653d638eb8cf48dd9c`。

<!-- begin:docs/local-1.0.5-performance.md -->
````````markdown
# 1.0.5 本机修复与验收记录

日期：2026-10-04。本轮以返回的 `VS-Renderer-GUI-1.0.5-源码.7z` 为基线；先完成候选构建与测试，再按维护者要求更新本地 dist，未发布、未覆盖用户配置。本记录不替代远端 780M 的实测。

## 本地 dist 更新

2026-10-04 重新使用独立目录 `build/dist-1.0.5-20261004` 配置并编译 Release，3FP 使用 MSBuild 完整 Rebuild。已覆盖 `dist/VS-Renderer-GUI-windows-x64` 的 Renderer、Player、FFF.Native，修正版本清单为 1.0.5，并同步根目录说明、本记录及 libdeflate 许可。新文件时间分别为 17:55:15、17:55:54、17:54:56；源/目标 SHA-256 核对一致。原程序、版本清单和说明备份于 `build/dist-backup-1.0.5-20261004`。

保留现有 Qt、FFmpeg、VS、LAV、madVR 与色彩运行时，不整目录重铺；原有配置、预设、缓存及色彩后端共 104 个文件的覆盖前后 SHA-256 摘要一致。部署后使用 dist 中的 FFF.Native 进行 1080p48 → 1440p Jinc 原生播放冒烟测试，稳定段未丢帧；测试正常新增两个着色器缓存，未改写原有保护文件。日志为 `build/dist-1.0.5-20261004/dist-native-smoke.csv`。新构建的编辑交互测试为 11 通过、0 失败、2 跳过。本次未执行主界面及 HDR 全量回归，没有覆盖其它 PortableSoft 安装，也没有生成或发布新的发行压缩包。

## Jinc 渲染链

780M 外部参照由维护者提供：PotPlayer / LAV / madVR，Jinc 色度 + Jinc 放大 + sharpen edges + enhance detail，1080p48 → 2160p48 约 35–40 fps，24 fps 有余量。这是独立参照，不能视为本项目已经达到相同画质或运行性能。

本机：RTX 4070 Laptop，指定 DXGI adapter 0。独立 `perf-3fp` 创建真实 7680×4320 HWND，并逐秒记录交换链与目标尺寸。算法编号 1284 = Jinc luma + Jinc chroma；关闭抗振铃，不启用额外锐化或细节增强。先 seek 到 120 秒、预热 5 秒，然后统计稳定段。

| 源文件 | 解码 | 稳定窗口 | 稳定段丢帧增量 |
| --- | --- | --- | --- |
| MyGO.01.108048.yuv444p10.FLAC.mkv，1920×1080、7001/146 fps | AV1 444 硬解不支持，回退软件解码 | 约 60 秒、47.95 fps | 0 |
| Girls.Band.Cry.01.AV1.FLAC.1080p48F_2026.09.22-10.20.13.mkv，实际 3840×2160、7001/146 fps | D3D11 硬解 | 约 60 秒、47.95 fps | 0 |

原始 CSV：`build/native-1.0.5-candidate/jinc-108048-8k-60.csv`、`jinc-216048-8k-60.csv`。两次交换链与目标均为 7680×4320、10 bit。计数证明这两次原生播放稳定段通过，不等于 GUI、HDR/DV、所有素材或 780M 已完成验收。

同一 2160p48 源另测 Jinc luma/chroma + 抗振铃（编号 66820），30 秒稳定段约 47.95 fps、丢帧增量 0；CSV 为 `jinc-216048-8k-antiring.csv`。仍没有加入 sharpen edges / enhance detail，不能拿它充当 madVR 增强链的同质量对照。

关键修复：Jinc 核权重改为不可变查表纹理，保留原公式参考分支；按输入布局、色度算法、色彩模式、传递函数和原色编译专用着色器，缓存键包含所有宏。只做权重查表、仍保留混合运行时分支时，8K 测试仍只有个位数 fps；输入/色彩专用编译后上述测试达到源帧率。编译失败只回退一次，不在每帧重编译。

2160p→8K 运行期间 NVIDIA 驱动遥测约 23–38W、GPU 43–67%；不与此前不同条件的 90W 直接作功耗降幅比较。本机没有取得 PawnIO 数据；本轮未安装监控驱动。

## 3FP 上游兼容更新

核对官方 [FFF_Project](https://github.com/Lake1059/FFF_Project/) 提交 `15995b8`。选择移植适配器配置、SDR scRGB 配置、Windows SDR white 查询及 P3 显示矩阵，不直接替换整个核心。当前配置 ABI 为本项目扩展的 16；不是官方 API16 全部功能镜像，未合入其商业扩展、原生图片或 MKS 等旁支。

保留返回基线的 `Hdr/HdrProcessor.cpp`，杜比视界解析/映射不被官方基础实现覆盖。其文件哈希与返回版本相同；这证明代码保留，不是本轮杜比视界显示验收。原生修改完整保存在 `patches/3fp-jinc-upstream-1.0.5.patch`，构建脚本在既有性能补丁后应用；已检查返回源码正向适用、当前源码逆向适用。

## 大图与图层操作

超过 64MiB 的图像背景使用共享原图和惰性 tile reader；编辑器缩略预览限制为视口尺寸，不在进入编辑器时复制整图。RGBA8 非交错 PNG 使用固定版本 libdeflate、CRC 校验和精确 PNG 预测器；在同一缓冲区内解滤波，避免第二份完整输出分配。其他位深、交错或未支持的色彩/方向元数据仍走原解码路径，不以降精度换启动时间。

`TEST.png` 为 48000×32000，压缩文件约 2.07GB，原像素约 6.14GB。原路径解码约 29.5 秒；AVX2 Paeth 快速路径两次完整解码为 9.670 / 9.611 秒，进入可用编辑器为 **9.825 / 9.767 秒**，初始化约 0.16 秒、resident tiles 为 0，两次均通过 10000ms 预算。原始日志为 `build/tests-1.0.5/large-png-avx2.txt`、`large-png-avx2-repeat.txt`。这是本机重复打开、系统文件缓存可能命中的结果；未执行全机缓存清除，冷启动未验收，也不保证机械盘或没有 AVX2 的 CPU 仍在 10 秒内。

没有 AVX2 时保留 SSE2 预测器；未降精度。线程按通道并行和预提交页面未得到稳定收益，已移除这两个实验。新快速路径与 Qt 解码的逐像素比较测试通过。

选框/魔棒选区右键新增剪切图层、复制图层、取消选择、反向选择。Ctrl+C/V 使用当前图层的选区及覆盖率，保留不规则孔洞、位置和应用内原始精度；剪切为一个可撤销事务。组工具切换使用不同图标。剪贴板给其他程序提供标准显示图，应用内粘贴保留原始像素。

编辑交互测试：11 通过、0 失败、2 个需外部大图/PSD 的测试跳过；新增 RGBA PNG 与 Qt 像素一致性、48000 像素宽惰性图层的预览/编辑/撤销、选区复制/剪切/撤销及图标区分测试通过。此前 SSE2 的约 10.7 秒预算测试失败，最终 AVX2 两次通过。HDR Qt 窗口测试在隐藏启动/无有效可见窗口的环境中未完成，不能据此宣称 HDR 全量回归通过。

## 实时降级

Anime 模式首次遇到超过 1GB、没有 FFMS2/LSMAS 索引的本地素材，直接进入 Jinc，避免先触发整片索引；已存在索引时允许走 VS。等待 VS 首帧超过 8 秒时也切入 Jinc，后续按既有丢帧阈值降至 D3D11；固定 Anime 模式发生启动降级后也允许继续降级。菜单提供独立 Jinc 直通与 D3D11 直通入口。Realistic 保留 Jinc→D3D11 路径。

限制：进程内 VS 插件的索引调用不提供安全强制中断，8 秒 watchdog 不能保证已开始的插件扫描立刻停止；大文件无索引预检用于避免启动该扫描。125MB/s 机械盘、缓存损坏、GUI 播放、780M 回归及增强算法同质量比较仍需实测，不能由本机 NVMe 的原生播放测试代替。

````````
<!-- end:docs/local-1.0.5-performance.md -->

<a id="record-docs-mpv-shaders-1-0-4-md"></a>

## docs/mpv-shaders-1.0.4.md

原始字节数：2296；SHA-256：`13750b06fec868b59a0a5ea2f2e4ac59daa8905931880bbf12d8fcacae3ae0cd`。

<!-- begin:docs/mpv-shaders-1.0.4.md -->
````````markdown
# 1.0.4 GLSL 分类与输出尺寸（开发中）

在 Renderer 处理链添加“mpv GLSL 着色器”，先选功能分类，再选着色器。分类包含超分、缩小、基础锐化、线条增强与抗锯齿、降噪与修复、去色带、色度重建、色彩与 LUT、几何与画面效果、其他，以及独立的“自定义 GLSL”。复合算法按主要用途分类；完整来源路径始终保留为执行参数，旧预设的 `anime4k` ID 和倍率参数兼容。

着色器控件固定140逻辑像素宽，列表宽度限制为420–520；过长名称截断，悬停查看完整路径、分类与硬件要求。名称搜索继续可用，显示名称和执行路径分离。

输出方式有“倍率”和“指定分辨率”。指定分辨率时可输入宽度、高度；默认开启“保持宽高比”，修改一项立即按该节点输入尺寸计算另一项。最后编辑的尺寸为准；关闭后独立使用两个数值。上游裁切、转置、缩放的尺寸在界面中同步考虑，脚本以实际 `clip` 比例计算，正数像素统一四舍五入。原0.5×、1×、2×、3×、4×方式继续可用。

硬件标签按实际要求设置：

- NVIDIA NIS 标为“NVIDIA NIS · 非 N 卡独占”，[NVIDIA 官方说明](https://developer.nvidia.com/rtx/image-scaling)列出 AMD / Intel 等硬件支持；不能仅凭算法名称标成 N 卡专用。
- CuNNy Q / DP4A 版本标为“需要 DP4A”；[上游说明](https://github.com/funnyplanter/CuNNy#mpv)要求 Vulkan 后端与整数点积支持。
- [ArtCNN](https://github.com/Artoriuz/ArtCNN)与 CuNNy 的 DS 为带降噪锐化训练的超分版本，归入超分。
- 现有 GLSL 为单帧处理，不承担时间补帧；补帧仍使用 RIFE / MVTools 节点。

本轮专项验证：分类与硬件标签、旧倍率和生成脚本、分类切换后的原路径保存、固定控件宽度与标签不重叠、横竖输入尺寸及宽高联动；实际 Vulkan 取帧得到102×77（指定宽度，包含半像素取整）、80×60（指定高度）、102×60（比例关闭），输出均为 YUV444P16。没有重跑915份着色器，也没有开展无关回归或性能测量。

版本1.0.4仍在开发。本轮只更新本地源码与便携目录，不推送GitHub，不生成压缩包。

````````
<!-- end:docs/mpv-shaders-1.0.4.md -->

<a id="record-docs-mpv-shaders-interpolation-1-0-3-md"></a>

## docs/mpv-shaders-interpolation-1.0.3.md

原始字节数：4612；SHA-256：`99485afdb47ec409644cba926739721f99957e7b6db76b273b5714ac8c0e29db`。

<!-- begin:docs/mpv-shaders-interpolation-1.0.3.md -->
````````markdown
# 1.0.3 着色器、内置补帧与更新目录识别

## 更新包布局

`PortableUpdater` 在安全检查及解压之后，以必要程序文件和 `release.json` 识别完整目录，接受解压顶层直接放程序文件，或下一层任意名称目录放程序文件。只接受唯一完整候选，不递归猜测更深目录；内部版本、下载大小、SHA-256 与危险路径检查仍然有效。打包脚本继续采用带版本号的单个目录，兼容已发布的 1.0.2 更新器。

## mpv GLSL 库

来源为维护者指定的 [hooke007 着色器说明](https://hooke007.github.io/unofficial/mpv_shaders.html) 与 [mpv User Shaders](https://github.com/mpv-player/mpv/wiki/User-Scripts#user-shaders)，同时从其直接关联的仓库、gist 与 GLSL Release 资产获取文件。`assets/mpv-shaders/manifest.json` 记录原始地址、修订、大小和 SHA-256，包含同内容文件的别名来源；各目录保留许可证和原文件版权头。

本轮加入 915 份内容不同的 GLSL / hook，单文件最大约 5 MiB，没有单个着色器超过维护者设定的 50 MB 门槛。文件按来源目录显示在 Renderer 的 **mpv GLSL 着色器** 节点；下拉框可输入名称搜索。原有 `anime4k` 节点 ID 与预设参数保留，原 Anime4K 预设继续可用。运行时从 `shaders` 递归识别 `.glsl` / `.hook`，自定义文件也可加载；新增 0.5× 输出供缩小着色器使用。

执行沿用 Vulkan / vs-placebo；[libplacebo 的 mpv hook 语法](https://libplacebo.org/custom-shaders/)覆盖片段、计算、纹理、缓冲区与参数等块。保留原始 `rgba16hf` / `rgba16f` 等纹理格式和对应上传数据，不能简单互换格式名：前者上传半精度数据，后者采用32-bit浮点上传后转换为半精度。lensfix-ewa 的旧 `tex1D` / `LUT_POS` 改用等价 GLSL `textureLod` 与 LUT 中心坐标公式，改动及原始哈希单独记录在 manifest。

根据 [vs-placebo 的 Shader 接口](https://github.com/sgt0/vs-placebo#shader)，输入采用 16-bit YUV，保留原色度采样，输出为 YUV444P16。HOOK / WHEN 会根据输入色彩平面、输出倍率和阶段决定是否执行；放大算法应选择对应倍率，缩小算法应选择 0.5×，锐化 / 调色通常选择 1×。库的存在与小尺寸取帧验证不代表任意着色器都能实时处理 4K，也不代表每个条件块在所有输入上都会触发。

## Player 内置补帧

右键 → **图像处理 → 补帧**，可选自动切换、四个固定档位或关闭。开发者超分预设菜单前两项为 Anime 自动切换和 Realistic，之后为六个手动 Anime 档位。已有用户 MVTools / RIFE VPY 保持独立。

| 顺序 | 自动补帧档位 | 参数 |
| --- | --- | --- |
| 1 | RIFE 4.26 · 4queue | 2×，GPU 工作线程 4，原始推理尺寸，场景检测 |
| 2 | RIFE 4.26 · 4queue · 半宽高 | 2×，GPU 工作线程 4，仅中间帧推理半宽半高 |
| 3 | MVTools 最高质量 | 当前高质量预设：block=8，pel=2，overlap=true，chroma=true，searchparam=2，blend=false |
| 4 | MVTools 低质量 | 默认配置：block=16，pel=1，overlap=false，chroma=false，searchparam=2，blend=false |
| 5 | 关闭 | 恢复 Anime 自动超分，从最高档开始 |

切换后稳定 2 秒，再观察 5 秒；样本至少 48 帧且丢帧超过 5% 时降一档。自动档加载或取帧失败同样尝试下一档；固定档不自动降载。新文件的自动补帧从 RIFE 原始尺寸开始。补帧输出保持原分辨率和 YUV444P16，显示缩放固定 Jinc；不叠加内置超分。关闭补帧恢复 Anime 自动超分，切换保留播放位置和暂停/播放状态。图片、网络直通、madVR 与纯音频不提供 VS 补帧。

## UI 字体

Player 的主题页“应用”同步更新主界面、设置窗口、菜单和播放列表字体，并保存到 `player.ini`。西文字体用于基础界面，中文字体显式设为 Han 回退字体，避免系统默认中文字体覆盖所选字体。已打开的设置窗口也立即应用；原有粗体状态保持。

## 验证范围

只验证本轮相关项目：真实下载/解压的两种更新目录布局、危险路径与错误布局拒绝，着色器脚本生成及实际 Vulkan 取帧，四个补帧档的 24→48 FPS / 保持尺寸 / 暂停切换、自动降档与失败降档，以及字体应用和重新打开时的实际字形选择。

维护者本地另有高占用 FFmpeg 作业；着色器编译耗时及超时不用于实时性能结论。详细取帧结果随验证记录保存；未开展无关全量回归。

````````
<!-- end:docs/mpv-shaders-interpolation-1.0.3.md -->

<a id="record-docs-native-api20-build-md"></a>

## docs/native-api20-build.md

原始字节数：4578；SHA-256：`039a647356c66ed5c766058ec5e1568e12e9435c88a9afa992a850538420d9bc`。

<!-- begin:docs/native-api20-build.md -->
````````markdown
# API 20 原生内核构建

1.0.12 继续使用同一固定上游与完整补丁；补丁已包含上传纹理／暂存环、8 线程策略、HEVC 异步交接、拓扑提示及可选 MMCSS / DXGI 等待对象。构建脚本无需手工修改 `build/native-api20`，开关见 [性能记录](player-kernel-performance-1.0.12.md)。

本轮最终固定上游 `Lake1059/FFF_Project` 的 `ce195ebd06e9cbb8352a443f0c874cf569a60733`（PR23，含 PR22），在独立 checkout 应用 `patches/3fp-api20-vsrenderer-1.0.10.patch`。不能把 API18 补丁直接应用到 API20，也不能直接使用未移植扩展的上游 DLL。

保留本项目的外部 VS 帧输入、clock-only、输出格式、像素探测、缩放、音频 EQ/tempo、软件解码队列和可选 libplacebo 色彩桥。snapshot、字幕及音频电平结构均按 API20 版本号传递，MinGW/MSVC 的配置、snapshot、字幕命令及图层布局已实测一致。上游20移除了旧私有 DV/RPU 测试扩展；本补丁沿用上游公开色彩和元数据路径，没有复活该私有接口。

## IAMF/OBR

先取得以下固定源，初始化 libiamf 的 OAR 子模块。不要下载 libiamf 仓库的媒体测试集合；可用 sparse checkout 仅取 `code/`、`LICENSE`、`.gitmodules`。

| 组件 | 固定源 |
| --- | --- |
| libiamf | [AOMediaCodec/libiamf](https://github.com/AOMediaCodec/libiamf/tree/b276f4371a8ae4f370a656e914f7b07e4cd5a2fa)，`b276f4371a8ae4f370a656e914f7b07e4cd5a2fa` |
| OAR/OBR | libiamf 的 `code/dep_external/src/oar` 子模块，`ca14b54381b6e8482a72dcc65c3adb17d6751db0` |
| Eigen 3.4 | [同 SHA 镜像](https://github.com/eigen-mirror/eigen/tree/3147391d946bb4b6c68edd901f2add6ac1f31f8c)，`3147391d946bb4b6c68edd901f2add6ac1f31f8c` |
| PFFFT | [官方快照](https://bitbucket.org/jpommier/pffft/get/0aec0327a6912e1a0ec5326eef737c2ce19bc836.zip)，ZIP SHA256 `53e81a31839a28c393a07e7de0ce041de94922caa71e2ce32fe37fa839b37ea0` |
| Abseil / GoogleTest | OAR 的 FetchContent 固定为 `20250512.1` / `v1.17.0`；只编译 iamf 目标，不编译测试目标 |

使用 MSVC v145、CMake 3.30、Ninja。libiamf 的 CMake 入口是 **code**，不在仓库根。使用 `/MD` 静态库，开启 binauralizer。脚本同时收集 IAMF、OAR、OBR、PFFFT、FLAC、Opus、FDK-AAC 和 Abseil 链接库，避免“关闭 IAMF 才能构建成功”的降级。

```powershell
& tools/build-iamf-api20.ps1 `
  -SourceRoot .deps/iamf-api20-src -EigenRoot .deps/eigen-api20 -PffftRoot .deps/pffft-api20 `
  -BuildDirectory build/iamf-api20 -OutputDirectory build/iamf-api20-stage `
  -VcVars 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvarsall.bat' `
  -CMake 'C:/Private/Soft/QT/Tools/CMake_64/bin/cmake.exe' `
  -Ninja 'C:/Private/Soft/QT/Tools/Ninja/ninja.exe'
```

首次构建需要联网下载 libiamf/OAR 固定的其他依赖。若直连 GitHub 不可用，可在调用脚本前为 Git 配置可用镜像；脚本本身不改变全局 Git 配置。

## Native

在一个独立、固定提交的 Git checkout 内构建；`DependenciesRoot` 指已有 `ffmpeg` 和 `vcpkg_installed` 的父目录。脚本只为缺少的目录建立 junction，不修改原依赖。原生 bridge 使用 libbluray 1.4.1 的 `bluray-3.dll`；FFmpeg 自己可同时加载 `bluray-4.dll`，但二者的公开结构不兼容，不能按文件名选择最高版本替代 bridge 依赖。

```powershell
& tools/build-native-api20.ps1 `
  -NativeRoot <独立的固定提交checkout> `
  -DependenciesRoot build/native-1.0.10/third_party `
  -LibiamfRoot build/iamf-api20-stage `
  -OutputDirectory build/mingw-release
```

脚本强制 `FFF_FORCE_LIBIAMF=true`。vcxproj 补齐上游遗漏的 `fdk-aac.lib`。运行目录需保留 MSVC runtime、FFmpeg ABI63/61/7/10/12、libass、匹配的 libbluray3 及它们的依赖。第三方静态许可位于 `third_party/native-api20/`，分发时一起复制。

本机实际构建目录为 `build/native-api20`（上游 archive 加同一补丁），依赖目录通过 junction 引用；这只是本机产物，不作为源码交付。补丁已在固定上游执行 `git apply --check`，构建日志位于 `build/api20-build-*.log`。完整内核包含 IAMF/OBR，但没有对应真实空间音频素材的听感验收；不能由成功链接推断该功能已完成实机验收。

追加：上游PR22已合并到固定内核并编译，按用户要求未运行测试。此前API20运行证据来自PR22合入前；最终验收边界见 [续修记录](update-20261016.md)。

````````
<!-- end:docs/native-api20-build.md -->

<a id="record-docs-nuxbox-performance-handoff-md"></a>

## docs/nuxbox-performance-handoff.md

原始字节数：8248；SHA-256：`fad85d37098dd26837d99f6ec9c08a84d8f46375abc57d561f4fe8895264d802`。

<!-- begin:docs/nuxbox-performance-handoff.md -->
````````markdown
# NuxBox：3FP 直通性能开发交接

## 任务

先修复 AMD Radeon 780M 上的 3FP 实时播放性能，不新增 UI，不改图片编辑，不为了跑分降低画质或静默丢掉帧。用户实测：1080p48 以 D3D11 直通升到 2160p 已大量丢帧、GPU 100%；2160p48 无法正常观看；同设备 mpv 播放 2160p48 没有大量丢帧。以上是用户报告，不是本窗口量化测量，不能据此直接断言某个 shader 是原因。

“VS 直通”是界面叫法：无预设原画、固定 Jinc / D3D11 档位实际绕过 VS，走 3FP。真正的 VPY 有 CPU 平面读回和上传，需要分开测。秒开与稳定 48 fps 是两项指标，前一轮加载修复不能代替播放性能优化。

2026-10-04 已只读核对远端部署目录 `C:\PortableSoft\VS-Renderer-GUI`，FFF.Native.dll SHA-256 为 `0f3deb531915d327adacf0972ab753ac7d7535ea51158677259b8d7f6e02f90a`。此哈希是待优化基线，不是优化结果。远端已安装 Git、VS 2022 BuildTools，MSVC 14.44（v143）；当前 PATH 未找到 CMake / Ninja，常用 Qt 路径未找到开发包。不得把“有 Codex”当成构建环境齐备。

## 四份原文件（不要传回本机，不要改写或全文件哈希）

目录：`F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬`

| 文件 | 字节数 | 内容标识 |
| --- | ---: | --- |
| 辉夜姬 V31080-48-HEVC.mkv | 8481606098 | 用户标记 1080p48 / HEVC |
| Chou.Kaguya-hime.the.Movie.2160p.48F.AV1.HDR-Fixed.mkv | 9415816759 | 用户标记 2160p48 / AV1 / HDR |
| Chou-Kaguya 2160p48F AV1 Real-ESRGAN Ver.mkv | 29920024980 | 用户标记 2160p48 / AV1 |
| Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv | 107938080889 | 已探测 7680×4320 / AV1 / AAC / ASS |

前三份编码、位深、颜色、实际 FPS 和 VFR 要用远端 ffprobe 核对，不能仅凭文件名判断。原生 SDK 为 avcodec-63 / avformat-63 / avutil-61 / avfilter-12 / swscale-10 / swresample-7，必须配套运行时。

## 从包开始

解压到新的开发目录，例如 `C:\Private\VS-Renderer-GUI-dev`，不要覆盖正在使用的便携目录。包内完整 GUI 源码来自 manifest 记录的 Git commit 加本次工作区改动；`.deps/fff-player/FFF.Native` 是已经包含全部补丁的源码快照，不能再盲目应用所有旧补丁。`handoff-manifest.json` 给出文件列表和 SHA-256，除清单自身以外覆盖全部文件。包不带主仓库 Git 历史；先初始化本地仓库并提交交接基线，之后的优化单独提交。

```powershell
git init
git add .
git commit -m "Import verified performance handoff baseline"
# .deps 默认忽略，另为 native 快照建本地仓库，记录后续原生修改。
git -C .deps/fff-player init
git -C .deps/fff-player add -f FFF.Native third_party
git -C .deps/fff-player commit -m "Import patched 3FP source and SDK baseline"
# 远端 v143；参数不改 vcxproj 默认的 v145，输出到独立目录。
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build-3fp.ps1 -FffProject .deps/fff-player -OutputDirectory build/native-v143 -PlatformToolset v143
```

优先只改 3FP，用远端已部署的 GUI 作独立测试副本，替换副本中的 FFF.Native.dll。复制运行时的根目录 DLL、Qt plugins、color/、runtime/ 和内置 vpy；排除个人 INI、cache、shader-cache、日志和用户预设。先用原始 DLL验证副本可以播放，再换构建版；测试副本不得抢文件关联或覆盖原安装。默认无预设即可测试原画，Jinc / D3D11 用副本自己生成的 builtin 预设，不能使用指向原安装的绝对预设路径。

需要改 GUI 时再准备 Qt 6.10.2 MinGW（编译器和 Qt 必须配套）、CMake 3.25+、Ninja、Python，设置 QT_ROOT/PATH，构建 `VSPlayer` / `vsr_player_tests`。包包含 VS API headers、FFmpeg/libass link SDK、已展开的图像依赖和可选 color SDK，不重复携带 ~600 MB 运行时，也不携带完整 Python venv 或工具链。VSRenderer 的 post-build 会调用 VS staging，仍需按 dependencies.md 准备 venv；此次性能任务先只构建 VSPlayer。运行测试需 Qt6Test.dll；VS 测试设置 `VSR_VSSCRIPT_DLL` 指向测试副本的 vsscript.dll，不能用旧构建日志宣称新设备已通过。

## 先测量再优化

1. 固定驱动、显示器刷新率、Windows HDR 状态、窗口实际物理像素、播放器缩放/色度/抗振铃、颜色引擎、字幕和音频。1080p48 分别测原尺寸和实际 3840×2160 输出，两份 2160p48 分别测原尺寸和同一目标；8K48 用同一 2160p目标作为压力项。没有 4K 显示器时用独立测试 HWND/渲染目标核实输出，不能把“选了 2160p”当作真实尺寸。
2. 每项记录冷启动至首帧；预热 5 秒后同一片段连续 60 秒，至少 3 次。记录实际呈现 fps、源时间进度、解码帧/呈现帧/丢帧/合并帧差值、seek次数、GPU各引擎而非单一总占用、CPU、IO、显存/共享内存。统计只算窗口内增量，明确原生和 VS 跳帧的口径，避免 seek / 暂停 / 起始计数污染。
3. mpv 用无用户配置的基线（`--no-config`）、D3D11VA 硬解和同样的实际输出尺寸；核对是否退回 CPU。分别做双线性基线和相同质量的缩放/色度对照。HDR/tone-map、字幕、音频、显示同步与程序相同。mpv 配置不相等时只能报告参考，不写性能倍数。
4. 先区分解码、纹理搬运、luma/chroma shader、VideoProcessor、色彩映射、字幕、present/pacing。GPU timestamp query 和 CPU计时要分开，query异步采集，不每帧同步GetData；不要用关闭 vsync / 丢帧换“更快”。检查帧上传/拷贝次数和中间纹理尺寸、每帧创建资源/编译shader、无意义两路会话、过度 Flush/阻塞等待，以及一帧重复呈现。候选瓶颈必须有证据。
5. 先保留原生/709色彩路径测 D3D11 与 Jinc，再单独测 libplacebo 高级色彩；逐项开关字幕与抗振铃定位。若 D3D11 固定档位仍使用昂贵 chroma 核，应先记录实际管线，不能只根据档位名称判断。

主要入口：`.deps/fff-player/FFF.Native/3FP/Render/VideoRenderer.cpp`（D3D11渲染/缩放/present）、`3FP/Core/PlayerSession.cpp`（解码/时钟/队列）、`3FP/Subtitle/AssSubtitleRenderer.cpp`（有界流字幕）、`src/player/PlayerProfiles.cpp`（直通切换/缩放）、`src/backend/ThreeFpPlayer.cpp`（C ABI）、`src/color/ColorBridge.c` 与 `3FP/Render/NativeColorEngine.h`（可选高级颜色）。

## 必须保留的功能与验收

- 以两份 2160p48 及 1080p48→2160p 为主验收；目标是在相同画质、同设备同片段下，达到接近 mpv 的持续呈现和无大量丢帧。8K48 是否受硬件解码或带宽限制单独报告，不承诺必然实时。
- 保留时间轴/逐帧/seek、暂停低GPU占用、字幕及高位深、HDR/色彩、缩放/平移、GUI及便携配置。不能无提示降低分辨率、改最近邻或禁用字幕来“修复”。
- 原有专项至少运行 `directOpenWithoutIndex`、`embeddedTextSubtitleSeek`、`directTimelinePreview`、`indexCacheAndBitdepthResize`；颜色引擎改动再跑色彩/HDR相关组。本机13/13与色彩专项是历史基线，不是远端成绩。
- 优化要可重建：新增原生 patch，接入 build-3fp.ps1，并保留 patch逆向检查；不能只提交忽略目录里的修改。原生工程与主仓库分别提供 commit/差异、测量 CSV 和参数、源码对应 DLL哈希，以及不改源视频/INI的审计。最终先在测试副本验证，不自动发布或覆盖正式安装。

交给此设备 Codex 时，可直接使用：

> 阅读 START-HERE.md、project.md、README.md、changelog.md。以这台 NuxBox 780M 和说明中的四份本地视频复现原生 D3D11/Jinc 丢帧，并与 mpv 做同条件对照。先建立独立工作区和基线记录，逐阶段定位 GPU 性能瓶颈，再修复最主要问题，提供可复现 native patch 和测量结果；不增加无关功能，不动原视频、原安装、用户配置，不以降画质或丢帧掩盖性能问题。构建缺少的工具先盘点并说明；不要从另一台设备传输大视频。完成后更新项目文档和交接记录。

````````
<!-- end:docs/nuxbox-performance-handoff.md -->

<a id="record-docs-nuxbox-performance-session-2026-10-04-md"></a>

## docs/nuxbox-performance-session-2026-10-04.md

原始字节数：11928；SHA-256：`f0f3539ffc722c571b35de38835ebdfc44a9192f3b182f917a485a83eca662d2`。

<!-- begin:docs/nuxbox-performance-session-2026-10-04.md -->
````````markdown
# NucBox K6 780M：v145 环境与 D3D11 性能修复

## 当前状态

v145 与 Qt 开发环境、D3D11 修复及本地 4K60 重复验收已完成。独立原生 15 轮、真实 GUI 9 轮的预热后 60 秒窗口均为零丢帧、零合并帧；GUI 音频欠载增量也为零。结果以 `build/performance/final-*-summary.json` 和各轮原始 CSV 为准。本记录替换此前停在 UAC 安装阶段的交接状态；此前远程桌面测量仅保留作探索证据。

## 环境与素材

- VS 2026 BuildTools 18.10.3 安装在 `C:\BuildTools\2026`，MSBuild 18.10.1、MSVC 14.51.36231、v145；安装完整，无需重启。v143 保留，候选 DLL 实际使用 v145 编译。
- Qt 6.10.2 MinGW 开发环境在 `C:\Qt\6.10.2\mingw_64`，补齐 qtbase / QtTest、qttools、qtsvg、qttranslations；配套 MinGW 13.1 在 `C:\Qt\Tools\mingw1310_64`。Qt 与 MSVC 原生模块通过 C ABI 连接。
- CMake 4.4.3、Ninja 1.13.2 位于项目 `build/setup/python-packages`；构建前点源 `tools/enter-nuxbox-dev.ps1`。
- Ryzen 7 7840HS、Radeon 780M、32 GB RAM；AMD 驱动 32.0.21030.2001。最终测试在本地 `console` 会话、物理 3840×2160 / 60 Hz 显示器执行，窗口实际客户区尺寸逐秒核对。DXGI 显示输出为 SDR `colorSpace=0`、8-bit 桌面；高位深视频仍按原管线使用 10-bit 交换链。
- F 盘为 12 TB USB/SCSI 桥接机械硬盘。全部直接读取原路径，没有复制视频、改写视频或做全文件哈希。

素材目录：`F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬`。

| 简记 | 文件名 | 视频流 | 字幕 |
| --- | --- | --- | --- |
| 1080 | 辉夜姬 V31080-48-HEVC.mkv | 1920×1080 HEVC 10-bit | ASS |
| 4k8 | Chou.Kaguya-hime.the.Movie.2160p.48F.AV1.HDR-Fixed.mkv | 3840×2160 AV1 8-bit | 无 |
| 4k10 | Chou-Kaguya 2160p48F AV1 Real-ESRGAN Ver.mkv | 3840×2160 AV1 10-bit | ASS |
| 8k10 | Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv | 7680×4320 AV1 10-bit | ASS |

全部流的标称/平均帧率为 `7001/146`（约 47.952 fps），颜色为有限范围 BT.709；未证明整片 CFR。`HDR-Fixed` 的流标签为 SDR，不能据文件名宣称 HDR 验收。

## 修复及证据

1. 呈现线程尚未消费上一帧时，保留队列内 AVFrame，不提前覆盖保留纹理。等待每次最多 2 ms，允许播放线程继续补充音频和有界预解码，不以合并帧或音频饥饿换计数。
2. Qt 字幕仅在合成像素变化时上传。播放中的字幕更新随下一视频帧合成，避免字幕和视频分别触发一次阻塞 Present；暂停和停止时补一次重绘，保留静止画面的字幕更新。
3. 原生 D3D11 VideoProcessor 直接使用硬解纹理数组中的对应 slice，并持有 AVFrame 引用直到消费。Shader / Jinc / 平移缩放 / 高级色彩需要纹理时才执行原有拷贝；不改变缩放核、色彩转换、抗振铃和输出精度。
4. 交换链改为三缓冲，使用 `IDXGIDevice1::SetMaximumFrameLatency(2)` 控制排队。原 `IDXGISwapChain2::SetMaximumFrameLatency(1)` 因未启用等待对象标志而无效。保持默认 `Present(1,0)`，未关闭垂直同步或启用 tearing。
5. `VSR_3FP_PROFILE=1` 才开启阶段计时；GPU timestamp 异步读取，每 16 帧采样，不在每帧同步等待 query。最终验收关闭诊断。

8K 的定位过程：仅修队列和字幕时，60 秒内丢 566 帧；另一次诊断丢 544 帧，GPU 上传阶段平均 5.199 ms、CPU Present 平均 24.435 ms。直接使用解码数组 slice 后，上传阶段约 0.002 ms，60 秒丢 107 帧；再增加并行缓冲后，两次完整诊断窗口均为零丢帧，其中有效两帧排队版本的 CPU Present 平均 1.280 ms。GPU 区间会包含共享上下文中的调度与等待，不能把这些值直接当作单个 shader 的纯执行成本，或据此算同画质性能倍数。

DXGI 接口依据：[SetMaximumFrameLatency 的适用条件](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-setmaximumframelatency)、[CPU/GPU 重叠与帧队列](https://learn.microsoft.com/en-us/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains)。三缓冲增加一个可用缓冲，可能增加排队延迟；这是保持画质及同步时提高吞吐的取舍。

## 测量口径

独立副本为 `build/perf-player`，运行时来自原安装，本轮 Player 和 DLL 来自源码构建；未复制个人 INI、缓存或用户预设。测试自行生成 builtin 预设。未注册关联、未覆盖原安装。

所有性能项先 seek 到 120 秒，预热 5 秒，再连续统计 60 秒；每项三轮，测试串行。视频音频正常播放。统计窗口内的计数增量，启动、seek、暂停不计入。启动期间的原始计数仍保留在 CSV。

- GUI：真实 PlayerWindow，D3D11 原生固定档位，硬解、字幕开启；每秒核对物理客户区 3840×2160、解码模式 2、VideoProcessor 模式 1、状态和 seek 代际。核对字幕有可见像素、源时间推进、帧率、音频欠载及暂停后画面捕获。
- 独立原生：`tools/perf-3fp.cpp`，1080 原尺寸 / 1080→4K / 两种 4K 原尺寸 / 8K→4K；无 Qt 和字幕。算法 `519 = 7 | (2 << 8)`，保持 D3D11 原生及双线性色度档位，音频开启。输出位深由 API 记录。
- `presentedVideoFrames` 是接受帧数，不能单独当作显示帧数；另检查成功 Present 数和合并计数。Present 总数包含界面重绘，因此额外 Present 单独记录，不把它当作新增源帧，也不因额外重绘判定丢帧。
- 系统采样逐秒记录各 GPU 引擎、CPU、IO、工作集、专用和共享显存，按固定 PID 过滤，覆盖每次播放全过程。不是只看任务管理器的单一 GPU 百分比。
- 窗口尺寸变化、设备/音频错误、测试失败都停止矩阵，不把失败样本混入最终通过项。

| 场景 | 三轮稳定窗口 | 接受帧率范围 | 原生输出精度 |
| --- | --- | --- | --- |
| 原生 1080p 原尺寸 | 全部通过 | 47.926–47.961 fps | 10-bit |
| 原生 1080p→4K | 全部通过 | 47.943–47.950 fps | 10-bit |
| 原生 4K 8-bit | 全部通过 | 47.950–47.957 fps | 8-bit |
| 原生 4K 10-bit | 全部通过 | 47.950–47.962 fps | 10-bit |
| 原生 8K→4K | 全部通过 | 47.949–47.951 fps | 10-bit |
| GUI 4K 8-bit | 全部通过 | 47.946–47.963 fps | 同原生管线 |
| GUI 4K 10-bit，ASS 开启 | 全部通过 | 47.950–47.955 fps | 同原生管线 |
| GUI 8K→4K，ASS 开启 | 全部通过 | 47.940–47.959 fps | 同原生管线 |

总计 24 个稳定窗口，累计 24 分钟统计，均直接读取 F 盘原文件。测试重复同一片段，不能等同整片持续播放。新进程打开至首个成功 Present：GUI 194–281 ms，独立原生 193–888 ms；系统缓存未清除。

“不丢帧”限于上述预热后稳定窗口。GUI 4K 8-bit 第一轮统计起点累计已有 1 帧丢帧；GUI 8K 三轮统计起点分别已有 15 / 11 / 4 帧丢帧、2 / 1 / 1 次音频欠载，之后均没有增长。独立原生 8K 起点已有 8 / 5 / 2 帧丢帧；mpv 8K 起点已有 23 帧显示丢帧。启动和 seek 的瞬时计数没有删除或当作修复完成。

GUI 全过程（含启动、seek、预热、暂停捕获）的三个进程平均值：

| 素材 | GPU 3D 引擎 | GPU Video Codec 引擎 | CPU，占全部逻辑处理器 |
| --- | ---: | ---: | ---: |
| 4K 8-bit | 10.91% | 37.54% | 0.21% |
| 4K 10-bit | 15.34% | 38.71% | 1.02% |
| 8K 10-bit | 34.56% | 60.67% | 0.42% |

这些是全过程采样平均，不能冒充精确 60 秒窗口均值。原始系统 CSV 包含 IO、显存及各引擎；汇总保留越界样本数量，计算百分比时排除驱动计数器偶发的 <0 / >100% 异常值，原始数值不删改。

必须的 `directOpenWithoutIndex`、`embeddedTextSubtitleSeek`、`directTimelinePreview`、`indexCacheAndBitdepthResize`，加字幕去重及输出格式专项，合计 **14 passed / 0 failed / 0 skipped**。针对真实 8K 硬解的原生 / Jinc / 缩放平移 / 返回原生的画面捕获专项为 **3 passed / 0 failed / 0 skipped**（均含初始化及清理）。暂停后接受帧数不再增长，捕获图像有有效颜色；代表性 4K / 8K 图像已人工检查。旧缩小测试依赖逻辑像素，已改为固定 640×360 物理视频区域，适配本地显示缩放。

Jinc 压力项**未达标**：1080p→4K、算法 516、Shader 路径，候选 60 秒仅推进 2.731 秒、约 2.18 fps；原安装 DLL 同条件仅推进 3.285 秒、约 2.38 fps，3D 引擎接近满载。该已有 Shader 瓶颈未在本轮优化，不能因丢帧计数低就判通过。本轮达标范围是用户指定的 D3D11 原生直通；Jinc 的图像输出和路径切换已保留。

mpv 参数：`--no-config --vo=gpu --gpu-api=d3d11 --gpu-context=d3d11 --hwdec=d3d11va --scale=bilinear --cscale=bilinear --dscale=bilinear --start=120 --sid=no --video-sync=audio`，独立物理 3840×2160 HWND。这是双线性、无字幕的参考，未逐像素匹配 VideoProcessor / 输出精度，不能宣称同画质性能倍数；`estimated-vf-fps` 是输入帧率估计，不是独立显示帧数。

本地 mpv 三种素材各完成一次约 60 秒参考窗口，均为 D3D11VA、3840×2160、60 Hz，显示丢帧及解码丢帧增量均为零；见 `final-mpv-summary.json`。

此前 RDP 的 `remote-*`、`mpv-*-r1.csv`，以及诊断中的 `local-gui-*`、`diagnostic-gui-*` 均不算最终矩阵。第一次加强断言时因把少量界面重绘当作多余源帧而失败，已修正判定并完整重跑，原始日志保留。mpv 首次启动因脚本数组参数引号加入多余空格，未进入有效播放，已舍弃并修正脚本后重测。

## 可重建及审计

主仓库基线 `4f54edfc68e1d7275c836020999f2d53c687c1d9`；原生基线 `734131a0af8d7051a8faa95383efb8b4bd767478`；原生最终提交 `e599911b22cc005900fe2d5e9e947e7132013a06`，之前队列与诊断提交为 `57304dd`。本地 Git 身份仅设置在仓库内。

修复完整保存在 `patches/3fp-present-performance.patch`，包括上述两次原生提交的合并差异。补丁已在基线的三个源码文件上通过正向检查、实际应用及反向检查；`tools/build-3fp.ps1` 识别已应用的完整补丁，避免重复套用相互重叠的旧补丁。

```powershell
cd C:\Private\VS-Renderer-GUI-dev
. .\tools\enter-nuxbox-dev.ps1
# 使用 PowerShell 7 执行；工具集和 Qt 编译器各自配套。
.\tools\build-3fp.ps1 -FffProject .deps/fff-player -OutputDirectory build/native-v145 -PlatformToolset v145
cmake --build build/mingw-release --target VSPlayer vsr_player_tests -j 8
.\tools\measure-nuxbox-native.ps1
.\tools\measure-nuxbox-gui.ps1
.\tools\measure-nuxbox-mpv.ps1
python tools/summarize-nuxbox-performance.py
```

冻结候选 DLL SHA-256：`204f2a37d3ba954804bd28344a179f7bd0c4c86a18a3b89facd63db708528e86`。
Player SHA-256：`91f9df7546574d09aa5ca6c9876a5e0e178b06706108350f0a39ebfab43f7a58`。

原安装 DLL SHA-256 应保持 `0f3deb531915d327adacf0972ab753ac7d7535ea51158677259b8d7f6e02f90a`，原 `player.ini` 应保持 `f8ab10becb3f98d8961bca615bdfd14037e8a369113928c1e7ce934d4661d99f`。系统文件缓存未清除，因此这些是从机械盘原路径读取的重复播放测试，不代表每次都强制冷盘 IO。原素材只核对大小与 LastWriteTimeUtc；最终审计见 `source-audit-final.json`、`binary-config-audit-final.json`。最终审计四份源文件均未变化，原安装 DLL / INI 哈希与基线一致。构建脚本再次执行成功，生成 DLL 哈希与冻结验收版本完全一致。候选不自动发布或替换正式安装。

已准备可运行的独立副本：`C:\Private\VS-Renderer-GUI-dev\build\perf-player\vs-player.exe`。交付包仅包含候选 GUI / DLL、补丁、测量工具和证据，不携带大运行时；现有独立副本已配套运行时。

````````
<!-- end:docs/nuxbox-performance-session-2026-10-04.md -->

<a id="record-docs-packaging-1-0-6-md"></a>

## docs/packaging-1.0.6.md

原始字节数：2799；SHA-256：`091013b38ed0e8e27b3593b9472500b612ca3b0bfd349a72e17ff30c60afec38`。

<!-- begin:docs/packaging-1.0.6.md -->
````````markdown
# 1.0.6 双便携包验收

核对日期：2026-10-05。发布说明在 [release-1.0.6.md](release-1.0.6.md)。本记录在压缩包完成后生成，作为独立交付凭据，不写回已经验收的压缩包。

| 包 | 压缩字节数 | 解压文件数 | 清单条目数 | SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `1.0.6.7z` | 286420323 | 1063 | 1062 | `7bd0c9fca35ac4f14663af8ee30c1e9bb1701005daf058e982a51ad1abdf9894` |
| `1.0.6-Lite.7z` | 237834827 | 768 | 767 | `57a57cd6acdd9480208f87b1be6c785a93f28de94a6e8cd05ad7b77440cf1aa2` |

两包采用标准 7z / LZMA2，均附同名 `.sha256` 文件。根目录均为 `VS-Renderer-GUI-1.0.6-windows-x64`，兼容已有客户端的版本根目录识别；`release.json` 分别标记 Full / Lite。

完整包包含 LAV Filters 0.83.0、madVR 0.92.17、MKVToolNix 102.0。精简包的 `LAVFilters64`、`madVR09217`、`runtime/mkvtoolnix` 及对应 `components.json` 安装版本记录均不存在；其余核心运行时和 BD 菜单的 libVLC 保留。

## 核验

- 两个压缩包的 7z 完整性测试及独立解压通过。
- 对解压目录使用包含隐藏文件的全量枚举；逐文件 SHA-256 与内部清单一致，清单仅排除自身。
- 无个人 INI、用户 VPY、媒体索引、缓存、日志、调试符号、测试程序或 AWJ 更新状态。保留 MKVToolNix 自带 `data/portable-app` 标记及资源；它们属于组件运行文件。
- 两包各含 16 个由本轮 Player 在独立目录重新生成的内置 VPY。
- 两包的 Renderer、Player 均移除开发工具链 PATH 及 Qt 插件环境后启动，找到 Qt 主窗口并正常关闭，退出码均为 0。
- 各自便携 Python 的 VapourSynth / FFMS2 及 libVLC 动态加载通过；识别 Python 3.15.0rc1、VapourSynth R80、libVLC 3.0.23，并核对 BD 插件存在。
- 打包前后原安装目录 1181 个文件的哈希全部一致；启动验收临时抑制测试路径的文件关联重注册，并还原原图标值。
- 验收数据：`build/package-verification-1.0.6.json`；构建回归：`build/hw-queue-20261005/final-regression.txt`（30 项）及 `deployed-smoke.txt`（7 项）。

## 核心二进制

两包与已部署、已验收的本地 1.0.6 二进制一致：

| 文件 | SHA-256 |
| --- | --- |
| `vs-player.exe` | `f48eb6a815652dc5e26fa51d34cb48306f451f4ce117663c1c75dd09bc3b7fe8` |
| `VSRenderer.exe` | `cc034c8f4078f621d3f852a6591b976308cf2cf7dcd4069ddb4d3219af5807ff` |
| `FFF.Native.dll` | `f80fc559dafdf255bc3ef3129d699504f477cc44897ae1cad559f27c331e2730` |

RX 6600 同片实机、8K 软件解码和全部补帧模式的持续实时性能仍按专项记录保留未验收边界。此次线上操作范围为源码上传和无附件 Release Draft；附件上传、ModelScope 镜像及正式发布由维护者操作。

````````
<!-- end:docs/packaging-1.0.6.md -->

<a id="record-docs-performance-large-images-1-0-8-md"></a>

## docs/performance-large-images-1.0.8.md

原始字节数：3380；SHA-256：`a33ef9cf4210037811007fd6acd7455b310d982ffec83532425b5a2f7a8a4f92`。

<!-- begin:docs/performance-large-images-1.0.8.md -->
````````markdown
# 1.0.8 大图、启动与双路渲染验收

日期：2026-10-07。CPU 预缩放不作为本版本性能验收条件，保持默认关闭。版本号仍为 1.0.8；源码推送 GitHub，提供 Full/Lite 本地压缩包，正式发布及二进制上传由维护者完成。

## 大图编辑

PhotoCraft 接入 VSP 原有 SIMD/mmap 图片解码器。已解码的大图通过 32 MiB 分块交换 RGBA8、RGBA16 或 RGBA float32 与 ICC，避免重复 PNG 编解码和额外完整交换缓冲。直接在编辑器打开超大 PNG 时调用同一个 `vs-player.exe --photocraft-import` 辅助入口，不新增解码进程可执行文件或运行库。

48000×32000、1.536 Gpixel 的真实 PNG 约 13.6 秒完成原始尺寸导入、局部裁剪、像素逐字节比较及撤销恢复。超过原生 GPU 画布预算的文档主动使用 CPU 预览，避免先分配整张 GPU 纹理后显存耗尽；100% 缩放绘制可见区域的原始像素。文档像素本身仍驻留内存，本次没有实现磁盘交换，内存不足的机器不能据此保证处理任意尺寸。

32769 像素宽的 8/16/32 位分块导入回归通过，16 位整数与 ICC 往返、浮点负值和高光保留。原有源/实画面高精度截图及 HDR 浮点附件保留。

## 启动

图片查看不再提前加载 3FP、VapourSynth、字幕 DLL 或 GPU 性能计数器。视频和滤镜在真正使用时初始化。960×640 PNG 的同机测试中，窗口构造由 772 ms 降至342–359 ms，打开到图片可见由 890 ms 降至595–654 ms；图片解码约 5 ms。数值受系统负载影响，表示本次样本结果。

## 双路 4K

用同一段 3840×2160、47.95 fps 的 HEVC 视频，在 AMD Radeon 780M 上同时运行两路对比，测量稳定播放的 10 秒窗口：

| 指标 | 原默认 Lanczos4 路径 | 新默认 D3D11 原生直通 |
|---|---:|---:|
| 两路接受帧率 | 4.39 / 3.80 fps | 47.90 / 47.90 fps |
| 两路丢帧 | 433 / 428 | 0 / 0 |
| 最长 UI 定时器间隔 | 93 ms | 78 ms |

默认对比缩放复用 VSP 已有 D3D11 VideoProcessor 路径，不再默认让两路同时运行高成本 Lanczos4 shader。用户仍可选择原有精度/算法；这些算法没有因本次修复而变快。HDR、4:4:4 等不适用于原生路径的输入继续使用既有回退。

## HEVC RExt 4:4:4

在本机 RTX 4070 Laptop 上，`RebellionHEVC444p10-700MBPS-SoftwareLast.mkv` 的 3840×2160、10 位 4:4:4 HEVC RExt 实际打开、播放及跳转通过，报告硬解模式与 CUDA 像素路径。正常 D3D11VA 优先级和路径保留，NVDEC/CUDA 为不支持输入的回退。没有 NVIDIA 设备的机器不承诺 CUDA 硬解。

## 已知实验边界

CPU 预缩放仅处理软件解码后、进入渲染器前的图像；它降低后续搬运/渲染成本，不减少 AV1 熵解码和参考帧解码工作。8K AV1 样本目前仍未达到实时播放要求，因此该功能默认关闭，不宣称性能验收通过，后续单独更新。

## 回归证据

本地证据位于 `build/performance-1.0.8`。BD 实盘回归 22 项通过，覆盖模式对话框宽度、整卷和分集跳转、菜单第 8–13 集及五种字幕语言的实际字幕图像。原始/D3D11/Jinc 初始播放时钟、跳转和恢复播放回归通过；布局移动不应触发视频重载。原始媒体和手动更新的 FFmpeg 保留。

````````
<!-- end:docs/performance-large-images-1.0.8.md -->

<a id="record-docs-photocraft-editor-1-0-8-md"></a>

## docs/photocraft-editor-1.0.8.md

原始字节数：6771；SHA-256：`aaa7a8f9506ef9540c984d4996f97502021d4143425cd6d483a66a61d83ac008`。

<!-- begin:docs/photocraft-editor-1.0.8.md -->
````````markdown
# PhotoCraft 原生图像编辑器接入（1.0.8，本地）

日期：2026-10-07。上游为 [storytold/photocraft](https://github.com/storytold/photocraft)，固定提交 `7eb3b2e072aa3110da2a329bb49109744292a87b`，版本 0.2.0；源码已拉到 `.deps/photocraft`。本轮沿用 1.0.8，源码更新 GitHub 并提供 Full/Lite 压缩包，正式发布由维护者完成。

## 接入与使用

播放器图片工具栏的首项“PhotoCraft 图像编辑器”进入上游原生 Rust/egui 工作区，保留其完整菜单、工具组、参数栏、文档标签、图层、文字、滤镜、历史和保存流程。旧 `ImageEditorWindow/Canvas/Tools` 不再链接进生产播放器；旧源码仅保留历史与独立核心回归。

`PhotoCraftEditor` 启动独立 GUI 进程，通过当前用户专用的 Windows 命名管道交接绝对文件路径；跨文件夹、盘符及中文路径无需复制原文件到统一工作目录。上游原有 TCP 自动化权限规则不变，普通播放器启动不启用 TCP 控制服务。重复打开已加载或正在加载的同一路径时切换到对应标签。

编辑进程通过 `QProcess::startDetached` 独立运行。关闭 VSP 会关闭管道，但不会关闭编辑器或丢弃未保存的工作；播放器视频、音频、渲染链均不被编辑器调用。

“文件 → 另存为”会把当前文档关联到新路径，之后重新打开原路径属于另一个文档。请选择新文件名以保留原图。PhotoCraft 自身使用原子保存；具体格式与保存警告沿用上游。

VSP 图片查看栏保留旋转、镜像、回收站、桌面背景和高精度导出/格式转换，原裁剪、尺寸编辑对话框由 PhotoCraft 接管。编辑器的“自动”界面语言跟随 VSP 基本设置；PhotoCraft 内部显式选择的语言仍由其自身偏好管理。

## 高精度数据

- 支持的普通图片、PSD/PSB 和 RAW 直接交给 PhotoCraft 读取，保持原始格式、位深与图层。
- 普通图片入口等待查看器解码完成，以识别 HDR 浮点附件；PSD/RAW 不依赖查看器代理，查看器的临时旋转不会使其图层或相机数据扁平化。
- VSP 已旋转/镜像的视图、AVIF/JXL 等原生编辑器暂不支持的编码，以及带浮点附件的 PNG，通过无损高精度副本传入；不是屏幕截图或 8 位显示代理。
- 整数视图使用原有 RGBA16 PNG 写出器，自适应滤波与 libdeflate 最高无损压缩；浮点视图使用已有 `ImageHdr::exportFloatTiff`，保留 float32、ICC、负值、大于 1 的数值及非预乘透明度。
- 交换副本位于 VSP 用户缓存下的 `photocraft-imports`，文件名含 `VSP-view-copy`，需要在 PhotoCraft 中另存为；缓存和编辑器用户状态不属于发行包。
- VSP 原有源/实画面高精度截图、HDR 浮点附件和 SDR 显示回退保留。数值往返通过不代表所有显示器、HDR 显示路径或全部上游滤镜的色准已经验收。
- 超大图使用原有 VSP 解码器及 32 MiB 分块 RGBA8/16/float32 交换，支持超过 32768 像素的宽度；真实 48000×32000 PNG 完整尺寸导入、局部像素比较、裁剪和撤销通过。超预算画布主动使用 CPU 预览，100% 缩放只绘制原始像素可见区域，避免整张 GPU 纹理分配失败。
- 大图文档仍占用对应像素内存，分块交换不等于磁盘交换；当前交换入口上限为 16 GiB 像素数据。

## 构建、许可与体积

可复现入口为 `tools/build-photocraft.ps1`，固定信息在 `tools/photocraft-source.json`，改动在 `patches/photocraft-vsp-integration.patch`。脚本检查源码提交和补丁，不重置已有不同提交或冲突修改。

```powershell
./tools/build-photocraft.ps1
cmake -S . -B build/mingw-release
cmake --build build/mingw-release --target VSRenderer VSPlayer
```

构建需要 Rust MSVC 工具链（上游最低 1.95）和 Visual Studio C++ Build Tools；便携版用户无需安装 Rust。运行时仅带 GUI、所需 x64 MSVC CRT 及许可文件，不带 CLI、Cargo、编译缓存或可选 craft-fonts 字体库。中文字体使用系统字体。`vsp-release` 继承上游体积优化配置，像素处理包保留原有高优化等级，原生平台仍使用正常的 panic unwinding。

上游软件采用 MIT OR Apache-2.0，本接入选择 MIT；保留两份上游许可、NOTICE、ATTRIBUTION，以及字体、图标、词典的独立许可。PhotoCraft 应用图标自身为 MIT OR Apache-2.0；`docs/brand` 的非开源 ArtCraft 商标图形不进入运行时。MSVC CRT 是独立的 Microsoft 再分发组件，见 `RUNTIME-NOTICE.txt`。

使用更严格的十进制 **45,000,000 字节、解压后净增量** 作为预算。原生组件（GUI、CRT、许可和固定信息）合计 **40,337,255 字节**，旧 Qt 编辑器移除后，最终本地便携安装净增量约 **39.69 MB**。最终安装净增量、哈希和保护文件校验见本地 `build/photocraft-1.0.8/size-audit.json` 与 `deployment.json`。打包脚本对组件执行 45 MB 检查，Full/Lite 都携带编辑器，排除 `PhotoCraftData`。

## 验收

`TestPhotoCraft` 使用真实原生编辑进程和 VSP 图片工具栏入口，覆盖中文路径、加载中及已加载文档复用、多文档切换、RGBA16 PNG/ICC 保存、旋转后的 float32 TIFF 交换、画笔、文字、PSD 保存与重新打开、裁剪及撤销。另一个播放器测试进程退出后，编辑器仍能响应操作，验证独立生命周期。Rust 的管道路径解析及 OS 文档事件回归通过，共享 UI 的 wasm32 编译检查及工作区格式检查通过。

已检查原生 GUI 截图：菜单、左侧工具、文档标签、右侧属性/图层及简体中文显示正常。测试浮点像素为 `[-0.25, 12.5, 0.50001, 0.4]`；32 位 TIFF 往返保留 ICC，逐通道浮点误差为零，负值、高光及透明度均未截断。

24 MP（6000×4000、RGBA16）合成图片原生打开实测约 **0.807–1.24 秒**，双三次缩至 3000×2000 约 **129–339 ms**。这是本机合成样本与当前负载的结果，不能外推为所有大 PSD、RAW 或复杂滤镜的性能。

本地证据目录为 `build/photocraft-1.0.8`，包含原生 UI、16/32 位往返文件、浮点检查、Qt/Rust 回归和构建日志。旧本地 PSD 核心测试从仓库根目录运行；CTest 默认目录会让两个既有 `build/...` 证据复制断言失败。大型个人 PSD 样本回归需要 `VSR_PSD_SAMPLE`，本轮未提供该样本。

PhotoCraft 仍属早期项目，完整接入其 UI 与功能不等于承诺 Photoshop 全部兼容。未修改上游滤镜算法、RAW 解码或 PSD 引擎；已有高级格式兼容边界继续适用。

````````
<!-- end:docs/photocraft-editor-1.0.8.md -->

<a id="record-docs-player-anime-performance-1-0-12-md"></a>

## docs/player-anime-performance-1.0.12.md

原始字节数：4939；SHA-256：`4d5e0ef97020f40932fdc42225576bb42944c503ed3fa071208399e49558bd5e`。

<!-- begin:docs/player-anime-performance-1.0.12.md -->
````````markdown
# Anime 播放与手动档位续修

2026-10-17，版本保持 1.0.12。

## 改动

- VS 播放遇到短暂调度延迟时，在现有预取范围内先提交相邻帧，再追赶音频时钟；原来只有 MVTools 使用此策略，Anime4K 会直接跳过已经计算的帧。超过预取范围仍重新同步，暂停定位和关闭预取保持直接定位。
- 删除“Anime 文件超过 1GB 且未发现索引就强制回退 Jinc”的条件。该条件没有查询 SSD / HDD，不能据此判断磁盘介质。
- 手动六档不再触发八秒首帧等待回退；冷索引和 shader 初始化继续在原工作线程执行。自动 Anime 配置保留原有首帧等待策略。
- AV1 / Opus / SDR / 算法等底栏标签背景采用与解码按钮相同的 alpha 90/255；倍速输入为 180/255，倍速弹窗底色约 209/255，右键菜单底色为 220/255。保留菜单选择、动画与倍速交互。
- 菜单重绘先清除透明背景，底栏及倍速面板以 Source 合成写入底色，避免重复重绘累积不透明度。桌面实际捕获已检查，菜单可透出后方窗口，文字仍清晰。

## 连续播放验证

硬件为 RTX 4070 Laptop GPU。开启预取八帧，性能预算为 100%，保留音频时钟；定位到十秒后预热五秒，再计量。以下数字分别记录提交、实际接受、VS 跳过、渲染丢弃和合并，测试函数返回成功本身不等于实时性能通过。

| 素材 / 条件 | 计量时长 | 提交 / 呈现 | VS 跳过 / 渲染丢弃 / 合并 |
| --- | --- | --- | --- |
| Horimiya，AV1 1080p 23.976fps，CNN→4K，修复前 | 20s | 438 / 438 | 41 / 0 / 0（窗口） |
| 同素材 CNN→4K，修复前全屏 | 20s | 438 / 438 | 42 / 0 / 0 |
| 同素材 CNN→4K，调度修复后全屏 | 20s | 479 / 479 | 0 / 0 / 0 |
| 同素材 CNN+→4K，本轮最终代码全屏 | 20s | 480 / 480 | 0 / 0 / 0 |
| Bloom Into You，AV1 YUV444P12，48000/1001fps，CNN→1440p，全屏 | 30s | 1438 / 1438 | 0 / 0 / 0 |
| 同素材 CNN+→1440p，全屏 | 30s | 1439 / 1439 | 0 / 0 / 0 |

最后两行在用户重启系统后重新测试，VS 输出均为 YUV444P16，实际帧率约 47.95fps，源解码和处理没有改成 4:2:0。CNN 的请求耗时约 3～4ms（含预取命中，不是独立 GPU kernel 耗时）。`nvidia-smi` 对 CNN 运行约第 10～38 秒的 26 次采样为 17.86～37.11W，平均 29.32W；功耗采样与用户的屏幕监控可能有采样窗口差异，不用固定瓦数代替出帧验收，没有测量 CPU 瓦数。

日志位于本地 `build/anime-bloom-1440-cnn-final.*`、`build/anime-bloom-1440-cnnplus-final.*`。`*.csv` 记录每秒计数，`*.csv.frames.csv` 记录逐帧索引，`*.power.csv` 记录实际 NVIDIA 功耗、利用率与频率。较早的 4K Horimiya 配对日志为 `build/anime-fullscreen-cnn-before.*` 和 `build/anime-fullscreen-cnn-catchup.*`。

## 范围

重启前对 Y412→4K 的定位实验中，原 VS shader 吞吐约 30fps，播放器连续跳跃使预取浪费，呈现约 4fps。独立平面读回原型提高了吞吐，但播放器 4K48 仍有丢帧；该原型只保存在本地 build 实验目录，本轮没有引入新的 VS 插件或更换现有 libplacebo / VapourSynth 运行时。4K48 不属于上述通过项。

`TestPlayer::vsPrefetchSurvivesShortSchedulingDelay` 覆盖小幅延迟追赶、大幅延迟重同步、暂停和关闭预取。`manualAnimeAllowsFreshLargeFile` 使用大于 1GB 的真实媒体和全新索引目录，检查手动 CNN 启动及八秒等待后仍保持原档位。`animeCadenceMeasurement` 使用 `VSR_ANIME_PERF_SOURCE`、`VSR_PERF_STAGE`、`VSR_PERF_WIDTH`、`VSR_PERF_SECONDS`、`VSR_PERF_FULLSCREEN` 和 `VSR_PERF_CSV` 指定本地条件；没有媒体环境变量时跳过对应测试。

专项回归为 13 passed / 0 failed；覆盖大文件冷索引、手动等待、预取调度、倍速与全屏 / 菜单交互、慢 VS 丢帧统计、指定 8K SDR Jinc 像素、四档无音轨 Anime4K 激活 / 暂停定位及色彩矩阵。日志为 `build/player-anime-final-regressions.txt`。透明重绘修复后，界面交互另测 3 passed / 0 failed，桌面捕获为 `build/player-menu-translucent-desktop.png` 与 `build/player-speed-translucent-desktop.png`。Lite 回归为 7 passed / 0 failed / 1 skipped，跳过项没有提供实盘原始 FirstPlay 条件。

原菜单测试仍断言内置预设只有两项，在恢复本轮修改前的三个播放器源文件后同样复现失败（`build/menu-baseline-original.txt`）。本轮将断言更新为两项原有预设加六档手动预设，并逐项核对六档名称；没有修改菜单内容来适配旧断言。

交付仍为 `dist/VS-GUI-Full`、`dist/1.0.12-PlayerLite` 与 `dist/VS-Player.exe`。部署核对 INI / VPY / ICC / LUT 哈希，单文件检查 CRC、图标、外置 FFmpeg、配置保留以及与 Lite 播放器的一致性。Full / Lite 发布压缩包由维护者制作。

````````
<!-- end:docs/player-anime-performance-1.0.12.md -->

<a id="record-docs-player-apply-avif-1-0-3-md"></a>

## docs/player-apply-avif-1.0.3.md

原始字节数：3876；SHA-256：`5c4a96a478c196feae0d6b657d360f6a7ad164b9332d67f5f3ae837e1d41103a`。

<!-- begin:docs/player-apply-avif-1.0.3.md -->
````````markdown
# VS Player 1.0.3：应用设置与 AVIF 内容验证

日期：2026-10-02，版本保持1.0.3。

设置新增“应用 / Apply”，保存并应用当前配置，保留对话框。重新打开视频时使用原播放状态和媒体时间，暂停不会因为默认自动播放而启动；即使关闭记忆位置，也恢复本次应用前的时间。图片应用设置只更新界面和缩放设置，不重新解码巨图。

首次生成的 `build/supplied-large-grid.avif` 不是完整参考样本。其CICP为1/13/6、YUV444，不是GBR。官方avifenc读取JPEG的RGB扫描行使用 `row * rgb.rowBytes` 的32位乘法；原图31603×65278的RGB缓冲超过4 GiB，偏移在约45301行后回绕，后面的扫描行覆盖顶部，末尾区域没有写入。对解码像素直接抽样确认顶部错色和底部黑块已存在于文件，而非窗口绘制。先前只检查解码返回、尺寸和少量颜色变化，不足以证实画面完整，相关历史记录已更正。

修正样本 `build/supplied-large-yuv420.avif` 通过libjpeg-turbo直接提取原JPEG的YUV420平面，绕过有问题的RGB扫描行读取，再由avifenc/aom生成8×16网格。原图宽31603为奇数，网格YUV420要求宽度为偶数，因此仅在右边补一列边缘像素，输出31604×65278；没有缩小或裁掉图像。文件283151835字节。它仍是有损编码，不宣称像素完全相同或由zenrav1e编码。

解码采用libavif/dav1d，开启同工具链编译的libyuv1924 SIMD色彩转换，保留正常双线性色度上采样。相同有效YUV420样本解码从23675 ms降至6380 ms；没有启用GPU解码或完整原图显存缓存。Tab增加AVIF原始CICP和GBR标识，方便确认实际色彩矩阵。

实际像素对比：均匀抽样316×653，共206348个位置，对比原JPG，平均RGB通道误差5.856，底部三分之一6.037；均低于专项上限30。抽样预览人工检查完整构图及底部，没有首次样本的黑块与顶部错色。证据：`build/avif-corrected-pixels.txt`（优化前）、`build/avif-simd-pixels.txt`（优化后）、`build/jpg-original-pixel-sample.png`、`build/avif-simd-pixel-sample.png`。

正常10位GBR参考样本 `tests/fixtures/gbr-10bit.avif` 使用identity矩阵、无损渐变；红/绿/蓝分布和透明通道专项通过。用户 `TEST.avif` 的直接像素抽样呈严重错色；没有对应原始参考，当前不宣称该文件完好或可以恢复缺失像素，证据 `build/avif-test-pixel-sample.png`。

仅验证本次相关专项：暂停/播放时Apply及媒体时间、对话框不关闭、原设置页与语言、GBR颜色、网格/高位深透明，以及实际巨图内容对比。Release构建通过，4项功能加初始化/清理6/6通过：`build/player-apply-avif-simd-tests.txt`；312条语言字符串检查通过。未运行或修复无关回归。

本地便携Player已更新，交付目录相同专项6/6通过（`build/player-apply-avif-portable-tests.txt`）。EXE构建/交付SHA-256一致，12份INI/VPY哈希保留（`build/player-apply-avif-delivery-audit.json`）；临时验证EXE和Qt6Test.dll已清理。没有改版本、生成发行压缩包、提交或推送。

本轮同时更新LAV Filters 0.83.0 x64，源为用户提供的 `C:/Users/ARXChem/Downloads/LAVFilters-0.83-x64.zip`，SHA-256 `0126982f47157bb86a6dbb43c4f332f7f98beba9ad552c19b65f9db2e7d4f186`。构建与便携目录21份根级运行时/许可/说明文件哈希均与解压源一致，三个AX版本均为0.83.0，移除旧版不再使用的带 `-lav-` 名称DLL。`stage-portable.ps1` 默认使用 `.deps/lav/0.83`，完整许可保留，不注册系统组件。便携LAV打开/播放/暂停/寻址专项通过：`build/lav-0.83-portable-tests.txt`（1项功能+初始化/清理3/3）；文件审计 `build/lav-0.83-delivery-audit.json`。12份INI/VPY哈希再次核对保留。

````````
<!-- end:docs/player-apply-avif-1.0.3.md -->

<a id="record-docs-player-bd-concat-preroll-1-0-8-md"></a>

## docs/player-bd-concat-preroll-1.0.8.md

原始字节数：5347；SHA-256：`5139fa66bfc68b5666d4b3d211db0258e2c793e22bbc58bf672f1fc9054bda98`。

<!-- begin:docs/player-bd-concat-preroll-1.0.8.md -->
````````markdown
# 1.0.8 光盘分集加载开头花屏修复

日期：2026-10-06。本轮仅本地修改、构建与部署；版本保持 1.0.8，保留维护者更新的 FFmpeg，不推送、不打包、不改动在线内容。

## 现象与范围

从光盘播放列表加载单集（`PlayerPlaylist` 的“第 N 集”）时，开头约 1 秒花屏；同一集直接播放（不经播放列表、不走分段描述）正常。整卷连续播放的每一段开头同样受影响。

只有原生 3FP 直通路径（截图中的 `3FP-HW` / `3FP-SW`）受影响。VS / VPY 路径按帧号裁切，不经过拼接解复用器，未受影响。

## 原因

`BlurayCatalog::playbackInput` 为每集生成 `player.ffconcat`，把 MPLS 的 PlayItem 起止时间写成 `inpoint` / `outpoint` / `duration`。FFmpeg 的 concat 解复用器在打开每一段时执行：

~~~c
if (file->inpoint != AV_NOPTS_VALUE)
    avformat_seek_file(cat->avf, -1, INT64_MIN, file->inpoint, file->inpoint, 0);
~~~

`inpoint` 来自 PlayItem IN，而这张盘的 PlayItem IN 正好等于该 M2TS 首个 IDR 的 PTS（例如 00000–00005 为 36600.000000，00009 为 600.000000）。定位到该时间戳后，解复用器从**该 IDR 之后的包**继续，首个 IDR 被跳过，H.264 解码器在没有参考帧的情况下开始工作。

本机实测（MyGO 下卷，BRMM_10775，六集 + 花絮）：

- `inpoint 36600.000000`：段内首个视频包是源的第 24 帧，解码器报 `co located POCs unavailable` / `reference picture missing during reorder`，前约 23 帧与直接解码不一致。
- `inpoint 36599.999000`：段内首个视频包就是源的第 0 帧（首个 IDR），逐帧哈希与直接解码完全一致。

用 `-flags +output_corrupt` 保留损坏帧后逐帧对比，6 个正片片段的差异帧数均为 100/100，即整段开头窗口内没有一帧与直接解码相同。把 `inpoint` 前移极小量后，同样窗口的差异帧数降为 0。

## 修复

`BlurayCatalog::playbackInput` 写 `inpoint` 时前移一个远小于一帧的前置量（1 ms），其余字段不变：

~~~text
file 'STREAM/00003.m2ts'
inpoint 36599.999000     ; PlayItem IN 36600.000000 之前 1 ms
outpoint 38021.420000    ; 未变
duration 1421.420000     ; 未变
~~~

- `outpoint` 与 `duration` 保持原始值，因此每段的作者时长、累计起点、章节位置与音画同步不变；1 ms 只影响解复用器在片段内部从哪里开始读。
- 由于 `inpoint` 前移，拼接时间轴的呈现时间戳整体后移 1 ms（首帧由 0 变为 0.001），远小于一帧（23.976 fps 下约 41.7 ms），不增删画面，也不影响段时长与定位容差。
- `duration` 显式写入，concat 用它作为段时长，不会因为 `inpoint` 前移而产生累积漂移。
- 当 PlayItem IN 小于 1 ms（例如首段从 0 开始）时取 `start`，即退回 0，不产生负值。

## 验证

`build/bd-concat-repro/` 保存全部脚本与中间产物。

逐帧哈希对比（`verify.ps1`，每个片段取前 200 帧，与同一 M2TS 直接解码对比）：

| 片段 | PlayItem IN | 修复后 inpoint | 差异帧 |
| --- | --- | --- | --- |
| 00000–00005 | 36600.000000 | 36599.999000 | 0 / 200 |
| 00006 | 600.000000 | 599.999000 | 0 / 200 |
| 00009 | 600.000000 | 599.999000 | 0 / 200 |

对照实验（`compare.ps1`、`band.ps1`）：

- 6 个正片片段在 `inpoint == PlayItem IN` 时全部不匹配，前移 20 µs 起即全部匹配；0.00002–0.04 s 的范围内结果一致，说明修复不依赖特定数值。
- 片段中间裁剪（`trim.ps1`，IN = 36605.0 非 IDR 对齐）：修复前后输出逐帧相同，未引入多余前置画面。
- IDR 对齐的中间裁剪（`midtrim.ps1`，IN = 36610.593911）：修复后首个输出帧正是该 IDR 对应帧，修复前该段开头同样无法与直接解码对齐。
- 多段拼接（`boundary2.ps1`）：修复后每一段的段首帧出现在拼接输出的正确位置，段内内容与直接解码一致；修复前所有段首都无法对齐。
- 段帧数与时长（`count.ps1`，00009 片段 600.000→610.010）：修复前 239 帧，修复后 240 帧，与直接解码在作者窗口内的 240 帧一致，`format duration` 均为 10.010000。即修复是把原本被跳过的首帧取回，而不是新增画面。
- 两集拼接的时间轴（`timeline.ps1`）：`format duration` 修复前后都是 2842.840000（2 × 1421.42），段边界与累计起点未变。修复后第二段开头在全局 1421.421000 s 处出现关键帧，修复前该位置只有非关键帧，与“段首 IDR 被跳过”一致。

播放器回归：

- `TestPlayer::fullBluRaySeekPerformance` 使用真实下卷整卷 MPLS 通过（5 次跳转，无断言失败）。
- `TestBluray` 全部通过，其中新增 `concatPrerollKeepsOpeningKeyframe` 固定生成脚本的 `inpoint < PlayItem IN`、`outpoint` / `duration` 不变，防止该行为被回退。
- 解码日志对照（`logab.ps1`）：修复前后在软件解码前 300 帧内都没有出现 `reference picture missing`；花屏来自被跳过的 IDR，而不是解码器报错。

## 本地交付

Player 与 Renderer 由本轮源码重编译并覆盖 `dist/VS-Renderer-GUI-windows-x64`；原生 DLL、FFmpeg / FFprobe、用户配置与预设未改动，`local-resource-sha256.json` 已同步。版本保持 1.0.8。

````````
<!-- end:docs/player-bd-concat-preroll-1.0.8.md -->

<a id="record-docs-player-bd-lyrics-editor-fixes-1-0-6-md"></a>

## docs/player-bd-lyrics-editor-fixes-1.0.6.md

原始字节数：3875；SHA-256：`c3c7ee88e3f383a7bec04e43057a3e0a15fb1539213d77a55e5c0e8b519e8d00`。

<!-- begin:docs/player-bd-lyrics-editor-fixes-1.0.6.md -->
````````markdown
# BD、歌词、播放列表与图片编辑修复

日期：2026-10-05。保持 1.0.6 本地版，不推送、打包或上传。

## 操作变化

- BD 模式选择对话框加宽。原盘菜单确认节目后，依据时长与章节匹配播放列表，自动接回常规播放，恢复滤镜入口及 Tab 视频信息，并保留菜单选定的音轨、字幕和播放位置。右键仍可重新打开 BD 主菜单。
- 普通 BD 节目使用包含各片段时长及裁切时间的本地播放清单定位。VS 预设逐个读取 M2TS、按播放项裁切后拼接，避免 FFMS2 直接读取清单产生空帧。缓存不改动原盘。连续快速切换时保留最新请求，避免原生异步打开期间的 `InvalidState`。
- 鼠标移到右侧底栏不再触发播放列表。右键勾选播放列表时，非全屏窗口增宽并在右侧停靠；全屏保留覆盖显示，底栏始终在其上方。
- 音频歌词自动显示：同目录同名 `.lrc` 优先，其次内嵌歌词，再其次可转换的文本字幕轨；无歌词则不显示。支持 UTF-8 BOM、大小写歌词键、COMMENT 时间歌词、容器与流标签、MP3 USLT/TXXX、AAC 歌词及 ASS/mov_text 等字幕轨。纯文本歌词也能显示。歌词使用独立原生窗口，避免被视频输出遮盖。
- 图片编辑器工具组的下拉箭头和弹出菜单位于按钮右下角。标尺两端显示 X，拖 X 调整对应端点，拖线段平移整个标尺。
- 新添加的文字可以双击编辑：文本、字体、字号、加粗、斜体、行距；支持多行文字及撤销重做。文字在当前文档中保留编辑信息，现有 PSD 导出仍保存像素图层，不承诺重新打开 PSD 后保留可编辑文字。
- 图片向右越过末张后回到首张，左上角显示“已回到文件夹中第一张图”。打开文件夹递归收集媒体，播放列表可展开子目录，切换图片保持整棵目录列表。

## 验证范围

- 原五次 53–61 ms 的测试未充分检查后台跳转失败及暂停拖动，不能据此认定 BD 定位完成。该性能结论撤回，以后续真实鼠标、目标呈现与菜单选集 [验收记录](player-bd-seek-acceptance-1.0.6.md) 为准。
- 菜单确认播放后恢复常规 Tab 和图像处理菜单；真实 BD 自定义 VPY 输出 160×90 帧。
- 硬件、软件解码：剧集、短片、音频切换以及连续四次快速选择；PGS 字幕有非透明像素输出。
- `C:\Users\ARXChem\Downloads\kit` 的 A–P 共 16 项：每项 12 行、六秒定位第二行、歌词窗口可见；另测外部带 BOM 纯文本覆盖内嵌歌词。
- 图片递归目录、末张循环提示、底栏与固定播放列表；编辑器标尺端点与整体拖动、文字像素变化及撤销重做。
- 最终播放器专项回归 14 项通过、0 失败、0 跳过；歌词单独回归逐文件记录 A–P 的实际来源，4 项通过。
- 编辑器交互 16 项通过，2 项跳过；布局 7 项通过，1 项跳过；高级工具 12 项通过；图像文档 6 项、图层 9 项通过。跳过项为已有的可选外部样本测试。

菜单节目若不能由时长及章节唯一匹配，保留 libVLC 播放以避免切错节目；多角度/副播放路径仍保留原有 BD 后端。首次 VS 索引仍需要扫描对应视频片段，本轮快速跳转数据是原生播放结果。

测试日志保存在本地 `build/player-final-fixes.txt`、`build/player-final-lyrics.txt`、`build/player-final-bd-regression.txt`、`build/editor-final-*.txt`。已覆盖本地便携目录中的 Player 与 Renderer，旧程序备份在 `build/local-backup-bd-editor-20261005`；原生库和 VPY 哈希保持不变。INI 在初始备份后、部署前更新，保留其当前内容并单独备份，最终核对未改变。交付哈希见 `build/bd-editor-deployment-audit.json`。

````````
<!-- end:docs/player-bd-lyrics-editor-fixes-1.0.6.md -->

<a id="record-docs-player-bd-seek-acceptance-1-0-6-md"></a>

## docs/player-bd-seek-acceptance-1.0.6.md

原始字节数：3781；SHA-256：`0e3d5a500ced45b9c14868803e69951d3e546343919320d8e00aeaba79abfd61`。

<!-- begin:docs/player-bd-seek-acceptance-1.0.6.md -->
````````markdown
# BD 与进度条实际验收（1.0.6 本地版）

日期：2026-10-05。本轮只更新本地源码与便携目录，不推送、打包或上传。

## 修复原因与行为

- 进度条保持目标位置，等目标帧呈现后再恢复时间刷新；连续拖动只保留最新待处理请求，按住时呈现预览，松手定位完成后恢复原播放状态。VS 输出也等待目标呈现后恢复播放。
- 原生 3FP 的 FFmpeg 拼接清单跳转原来用视频流时间基准，后台可能失败而接口已返回成功。现在使用全局时间基准的 `avformat_seek_file`，提前一秒获取可解码 GOP，再精确解码到目标。普通视频保留原跳转路径。补丁 `patches/3fp-concat-seek.patch` 已纳入 `tools/build-3fp.ps1`，本轮重编译、交付原生 DLL；原生会话错误可由 ThreeFpPlayer 读取。
- 模式选择改用 QDialog 明确布局，实际最小宽度 560 逻辑像素，避免 QInputDialog 自动尺寸调整覆盖加宽设置。
- 菜单选择等待标题、章节及时长稳定 250 ms 后接回常规播放器，位置至少为选定章节起点，避免读取 libVLC 尚未更新的旧位置。

## 本机验收

真实样本为 Downloads 下 BRMM_10775 的 MyGO 下卷，最长清单 8538.53 秒，六集拼接；普通视频为 1280×720、24 fps、八秒长 GOP 测试视频。

真实 Qt 鼠标按下、移动、松开事件覆盖跨段点击、快速交替拖动、硬件/软件解码及播放/暂停状态；检查原生呈现计数、时间轴代次、目标位置与状态，持续采样排除滑块回弹。按住进度条时也检查目标画面呈现。以下为本机已有缓存的观察值，Qt 检查按约 50 ms 间隔采样，并非精密渲染计时。

| 素材 | 解码 | 点击至目标呈现 | 快速拖动松手至最终目标 |
| --- | --- | --- | --- |
| 普通视频 | 硬件解码 | 53–241 ms | 161–185 ms |
| 普通视频 | 软件解码 | 52–123 ms | 0–62 ms |
| BD 整卷 2:22:18.53 | 硬件解码 | 54–122 ms | 109–114 ms |
| BD 整卷 2:22:18.53 | 软件解码 | 161–287 ms | 160–226 ms |

- 原盘菜单第 8–13 集逐项点击：实际起点对应 0、1421.42、2842.84、4264.26、5685.68、7107.10 秒附近；每项随后暂停并再次跳转，确认并非只切换标签。
- 模式窗口实际宽度通过，内容截图 `build/bd-mode-width.png`。
- Jinc/D3D11 拖动、单核软件点击、VS 自定义 BD 滤镜 160×90 输出及跳转后恢复播放、Tab/处理菜单恢复、PGS 非透明像素、三段短节目、菜单右键/Tab、硬件/软件音视频切换通过。与上一份已部署原生库的像素参考比较通过。
- 菜单画面从 libVLC 输出获取，避免桌面其它窗口污染截图；原生快照不含 IG 高亮，菜单选择由实际节目和章节验证。
- 最终专项 30 项通过、0 失败、0 跳过（含初始化及清理）。日志 `build/bd-final-acceptance.txt`，原生构建日志 `build/native-bd-seek-build.txt`。首次 VS 索引扫描、其它盘结构和其它设备不包含在性能数字中。

## 本地交付

Player、Renderer 与原生 DLL 均由本轮源码重编译，覆盖 `dist/VS-Renderer-GUI-windows-x64`。新 DLL SHA-256：`6753A865A43C9C8A40008D640B573B4CCBBA1C5D6EFD1CEDF30D67EBDDA1A792`，与验收库相同。旧文件单独备份，INI 与 VPY 逐文件核对未变；交付审计 `build/bd-seek-deployment-audit.json`。版本仍为 1.0.6。

部署后再次指定加载 dist 中的原生 DLL，硬件/软件播放中拖动和菜单第 13 集抽查共 5 项通过（含初始化及清理），0 失败、0 跳过。实际加载路径与哈希保存于 build/bd-deployed-loaded-library.txt，测试日志 build/bd-deployed-dll-acceptance.txt。

````````
<!-- end:docs/player-bd-seek-acceptance-1.0.6.md -->

<a id="record-docs-player-color-vs-1-0-12-md"></a>

## docs/player-color-vs-1.0.12.md

原始字节数：5464；SHA-256：`3a96410b5a304fbd59d1a539a23ff05bb7108b6e285ea810bd343fe4dfe54b85`。

<!-- begin:docs/player-color-vs-1.0.12.md -->
````````markdown
# 1.0.12：Jinc 偏暗与无音轨 VS 播放修复

## 用户复现与对照

Windows HDR 关闭。指定素材 `D:/Video Compress Test/PlayerTest/ChouKaguya432048-NoAudio.mkv` 为 7680×4320、AV1、YUV420P10、limited、BT.709 SDR、无音轨，109.485 秒。`D:/Video Compress Test/Large-Test/1080p-2D.mkv` 为 1920×1080 H.264、无音轨、色彩标签未指定，16.058 秒。只读取原片，索引写入测试缓存。

旧内核在同一 8K 文件 10 秒、同一视口的 49 点实际 GPU 读回中：D3D11 平均亮度 0.395417，Jinc 0.162831，比例 **0.411795**，复现明显偏暗。测试 `requestedSdrJinc` 修复前失败。人工外观反馈与 GPU 像素一致，不是用控件属性或截图文件的标签推断。

## Jinc 与 HDR/SDR

`VideoRenderer.cpp` 中 Jinc 权重 `Texture1D` 和 kind-3 色彩变换 `Texture3D` 同时占用 t4。DrawWithShader 先绑定 Jinc 权重，又绑定变换 LUT，导致 Jinc 采样了错误资源。均匀中灰的 D3D11 输出约 0.5、Jinc 接近 0。将 Jinc 声明及全部绑定改为独立 **t5**；动态 HDR 曲线保留 t3、色彩变换 LUT 保留 t4。重新生成通用 ShaderBytecode.h 与三个 SDR layout 专用 shader，重建完整 IAMF 的 Native；没有只修改 HLSL 字符串而遗漏实际嵌入字节码。

视频数据路线分别为：原生 AVFrame → D3D11/Jinc 呈现；或 FFMS2/LSMASH → VS 滤镜与帧属性 → 外部帧 ABI → 原生呈现。VS 帧继续传递 range、matrix、primaries、transfer、chroma；不把 VS 后处理结果再次套源文件 HDR 标记。未标记的 1080p 输入保持 unspecified / SDR，不因转换到 16 bit 而标为 PQ/HLG。

早上 `HdrPreview.h` 的桌面参考白逻辑由 `PreviewPane::setImage` / 桌面 SDR 伴随图调用，处理 QImage 图片；直通视频和 VS 外部帧进入原生 GPU 纹理，不经过这个图片函数。本轮没有撤回截图白位修复，也没有通过曝光补偿掩盖渲染资源错误。

## 无音轨 VS 时钟

旧版 `requestedAnimeNoAudio:3` 实盘播放 2.5 秒后媒体位置已变成 16.058 秒，状态暂停且没有新处理帧。clock-only 会话跳过视频解码、又没有音频消费者来限制读包，快速把整段读到 EOF。另一个条件在 Play 时仍无条件启用视频预缓冲，时钟会话不会产生对应队列，会停在起点。

修复：无音轨 clock-only 使用 QPC 真实时间推进，到作者时长才结束；Play 不为 clock-only 启用视频预缓冲。正常带音轨播放和普通视频解码的节奏保持。独立时钟回归覆盖播放、暂停、8 秒定位与恢复，墙钟 1 秒对应媒体约 1.008 秒。

## 验收

- `jincColorMatchesNative`：8-bit 420 / 16-bit 444，SDR / PQ / HLG，自动 / 强制 SDR 输出，原生 / HDR→SDR 自动色彩路径；均匀中灰与算法一致性检查通过。Jinc / D3D11 最大 RGB 差 **0.000977516**，约一个 10-bit 码值。合成 HDR 输入只验证 GPU 处理及读回，不代替真实 HDR 显示器验收。
- 同一指定 8K SDR 文件 10 秒：D3D11 平均亮度 0.395417，Jinc **0.391150**，比例 **0.9892**。缩放算法可有局部差异，没有用完全相等作为画面要求。
- `requestedAnimeNoAudio` 四档 CNN+ / CNN / no CNN+ / no CNN 全部通过：自动播放启动完成预解码后连续出帧，保持 VS 模式；暂停后能跳到 8 秒并读回实际 GPU 像素，没有直接跳到结尾。日志合计 6 passed（含 init/cleanup），`build/anime-autoplay-final.txt`。此前暂停冷启动的 CNN+ 预取曾超时，不将其作为实时性能通过证据；本轮没有做帧率基准或宣称所有预取配置均流畅。
- 8.008 秒 BT.709 参考中心 RGB（FFmpeg，4×4 均值）：0.98407 / 0.82744 / 0.61251；四档处理后中心最大通道差约 0.0082，增强链允许有锐化差异，没有出现亮度量级变化或错误 HDR 标记。
- 桌面白位与 HDR FP16 截图读回回归 4 passed；BD 鼠标跨段 / 菜单回归 4 passed；Lite 7 passed、1 skipped（实盘原始 FirstPlay 环境未设）。未运行全量套件。

详细日志：`build/jinc-neutral-before.txt`、`build/jinc-color-after.txt`、`build/jinc-real-old-at10s.txt`、`build/jinc-real-final.txt`、`build/anime-clock-final.txt`、`build/jinc-hdr-regression.txt`、`build/jinc-bd-regression.txt`、`build/jinc-lite-regression.txt`。中途失败与最终通过分开保存，不覆盖失败证据。

版本保持 1.0.12，更新 Full / Lite 原生内核并重做单文件。固定 PR23 上游应用完整补丁后，PlayerSession.cpp / VideoRenderer.cpp / 两份 shader header 与实际构建源逐项一致。生成的两份字节码 header 使用 Git binary hunks 保存；.gitattributes 固定此补丁为 LF，避免 Windows CRLF 损坏二进制补丁。Full / Lite 部署核对 INI、VPY、ICC/ICM、LUT 哈希；FFmpeg 外置策略保持，发布压缩包仍由维护者制作。

最终直接指定已部署 Full 的 FFF.Native.dll，合并运行灰阶、指定 8K、无音轨时钟、四档 Anime4K、VS 色彩属性、桌面白位、HDR FP16 读回与 BD 定位/菜单，**14 passed、0 failed、0 skipped**（含 init/cleanup，44.656 秒）。日志 `build/jinc-color-delivered-final.txt`。部署与单文件核验结果分别在 `build/jinc-color-deploy.txt`、`build/jinc-color-single-audit.txt`，配置保持，内嵌 Native 与目录式 Lite 同源，原盘 index 哈希不变。

````````
<!-- end:docs/player-color-vs-1.0.12.md -->

<a id="record-docs-player-direct-open-1-0-4-md"></a>

## docs/player-direct-open-1.0.4.md

原始字节数：3374；SHA-256：`3038aafd4dca467ee55361187ae43cb3a3f25b6fc965fda4cadfdd3412fc7c07`。

<!-- begin:docs/player-direct-open-1.0.4.md -->
````````markdown
# Player 直通与大文件加载

## 问题与修复范围

2026-10-03 排查 NuxBox K6（主机名 LAPTOP-DSH3PNDH）机械硬盘素材：

`F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬\Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv`

文件 107,938,080,889 字节；FFprobe 返回 7680×4320 AV1、AAC 音频和内嵌 ASS 字幕，时长 8523.219 秒。这一次 FFprobe 流信息探测耗时 125 ms；此数据不是播放器首帧耗时，也不代表冷缓存磁盘性能。

排查到三处开销：

- 未选预设的原画播放仍创建 FFMS2 的 VS 源，需要帧索引。
- 固定 Jinc / D3D11 打开仍等待 VS 初始化。
- 3FP 内嵌 ASS 初始化一直 `av_read_frame` 到 EOF；内嵌 SRT 在 Qt 层先调用 FFmpeg 全量提取。这两条字幕路径都会读取整部媒体，和播放竞争机械硬盘带宽。

现在无预设原画使用原生 3FP，不创建 VS 源；固定直通档位不等待 VS 初始化。ASS / SRT 保留容器与解码器，按播放位置读取，单次最多 64 包或累计约 4 MiB（最后一个包可超过限制），时间窗口前读约 1 秒。跨位置跳转重定位，向前预留 30 秒字幕 preroll；MoreData 允许暂停时继续补齐当前窗口，避免相同位置缓存阻止后续读取。该窗口不能保证恢复在更早位置开始、持续超过 30 秒的跨越式长字幕；不等价于完整全文字幕索引。

增强、补帧、自定义 VPY 仍走 VS，首次 FFMS2 / L-SMASH 索引是这些模式的独立开销。此修改不保证所有编码、磁盘和 GPU 均能秒开或实时播放 8K48。

## 构建与验证

`tools/build-3fp.ps1` 在已有原生补丁之后应用 `patches/3fp-streaming-text-subtitles.patch`，更新后的 EXE 与 FFF.Native.dll 需配套。补丁逆向检查用于确认已应用；没有更改媒体文件。

专项测试：

本地专项通过 13/13（包含初始化与清理）：小素材约 0.96 MB，原画 / Jinc / D3D11 首帧分别 221 / 189 / 281 ms；测试日志 `build/mingw-release/fast-open-tests.txt`。这些数字仅是本地小素材回归，不是 NuxBox 108 GB 素材成绩。`indexCacheAndBitdepthResize` 同时确认增强与自定义 VPY 仍能建立和复用 FFMS2 / L-SMASH 索引，没有被直通修改绕过。

- `directOpenWithoutIndex`：原画 / Jinc / D3D11 打开不进入 deferred，不加载 VS 脚本、不生成帧索引并呈现首帧。
- `embeddedTextSubtitleSeek`：内嵌 ASS / SRT，在 2 秒、42 秒及返回 2 秒时渲染字幕，包括暂停位置继续读取。
- 原有 `subtitleOverlay` 和 `directTimelinePreview` 检查外挂字幕及四种播放 / 暂停拖动组合。

大素材测试入口为环境变量 `VSR_DIRECT_OPEN_SOURCE`；测试读取对应文件，独立临时缓存并关闭记忆位置，结束由测试套件恢复 player.ini。不能在另一窗口使用同一便携目录时运行此测试。

2026-10-03 本窗口按要求暂停传输。2026-10-04 用户确认远端部署已完成，本窗口只读核对 `C:\PortableSoft\VS-Renderer-GUI\FFF.Native.dll` 哈希与最终基线一致，没有据此补写本窗口的大素材首帧成绩。用户新报告 780M 上 1080p48→2160p 与 2160p48 原生播放大量丢帧；播放性能与加载开销分开，后续转到远端本地开发，见 [性能交接](nuxbox-performance-handoff.md)。

````````
<!-- end:docs/player-direct-open-1.0.4.md -->

<a id="record-docs-player-high-precision-screenshots-1-0-7-md"></a>

## docs/player-high-precision-screenshots-1.0.7.md

原始字节数：6976；SHA-256：`bf79c2db4cdb0051036edb772023975881d7855a6d0e42f4d8bddf1e82efc149`。

<!-- begin:docs/player-high-precision-screenshots-1.0.7.md -->
````````markdown
# Player 高精度截图 (1.0.7)

2026-10-06，1.0.7。右键 → 图像截取，源画面和实画面统一保存 RGBA 每通道 16-bit PNG (每像素 64-bit)。采用 PNG 自适应行滤波、libdeflate 最高压缩级别 12；压缩不改变整数像素。保存在线程池执行，完成后显示文件路径，同一时刻保存一张。

## 文件和取样位置

截图默认写入播放器旁的 `screenshots/`，设置 → 基本设置 → 默认截图保存路径可以修改，清空则恢复默认。右键新增“截图到指定路径…”的源画面 / 实画面入口，选择文件夹保存；单次选择不会覆盖默认路径，取消不保存。

文件名含源文件名、时间和 `source` / `VS` / `display`。每张 PNG 附 `.png.json`，记录尺寸、播放位置、源或输出位深、色彩编码、矩阵和范围等；已知色彩信息写入 PNG cICP / ICC。

| 入口 | 取样内容 |
| --- | --- |
| 原生直通源画面 | 按当前播放位置重新解码所选视频轨，转换为全范围浮点 RGB 后量化为 PNG16，保持源传递函数；不含字幕和显示变换 |
| VS 源画面 | 当前已显示的 VS 输出帧，以原有平面位深输入转换；保持 VS 的矩阵、范围、传递函数，不含字幕和显示变换 |
| 视频实画面 | 渲染器最终输出表面，含缩放、显示处理和字幕；8-bit / 10-bit / FP16 表面直接读为 float32，不再先降为 8-bit |
| 图片源画面 | 当前图片原始像素，保留已有 16-bit 精度 |
| 图片实画面 | 按当前视口、缩放、平移和旋转绘制到高精度图像；不再用 8-bit 窗口截图 |

视频矩阵未标注时，截图转换采用与原生渲染器一致的回退：HDR / BT.2020 使用 BT.2020，宽度至少 1280 使用 BT.709，其余使用 BT.601。JSON 单独记录 `conversionMatrixFallback`，不把推断写成源文件自带元数据。

## HDR 数据

HDR 每次截取生成五个文件：默认 `.png` 是 sRGB SDR 预览；`.png.json` 是预览和附件说明；`.hdr.png` 保留 HDR 编码；`.hdr.png.json` 记录 HDR 色彩信息；`.png.rgba32f` 保存原始浮点样本。默认预览已进行色调映射，在普通 SDR 图片查看器中不依赖 HDR 支持；HDR 显示器上也可直接查看另存的 HDR 图。

HDR 源画面的 `.hdr.png` 保留 PQ / HLG 编码。HDR 实画面读回 scRGB 线性数据，单位为 `1.0 = 80 cd/m²`；HDR PNG 转换为 BT.2020 / ST 2084，并标注 cICP `9 / 16 / 0 / 1`。SDR 预览先线性化 PQ / HLG 或 scRGB，再转换至 BT.709、以 203 nits 参考白作 ACES 拟合亮度压缩，转换至 sRGB；超出 SDR 色域的值在预览中裁切。HLG 预览采用 1000 nits / 系统 gamma 1.2。预览用于兼容查看，HDR 色准比较应使用原始 HDR PNG 或浮点数据。

检测到 HDR 时，另外保存 `.png.rgba32f`：从截图取样处取得的 RGBA float32 原始值，小端、像素交错、从上到下，无头部，无行填充。负数和大于 1 的值保持原样。源画面的浮点值是编码 RGB；实画面的浮点值是线性 scRGB，不能直接互相相减。

浮点图片也保存浮点附件，实画面在 float32 图像上绘制，避免先截断再保存。线性图片标记为 `linear-RGB`，保持原图相对单位，不擅自指定绝对亮度；这类附件不应乘以 scRGB 的 80 nits 单位。

PNG 的“无损”指整数像素的压缩无损。scRGB 转为 PQ PNG 必须进行色域转换和 16-bit 量化，超出可表示范围的值会截断；精确核对 HDR 渲染数据时使用浮点附件。原生后端 FP16 表面读回已修正为逐个半精度数转换，避免把 FP16 内存误当 float32。

用 NumPy 读取附件：

```python
import json
import numpy as np

path = "example-display.png"
with open(path + ".json", encoding="utf-8") as f:
    meta = json.load(f)
rgba = np.fromfile(path + ".rgba32f", dtype="<f4")
rgba = rgba.reshape(meta["height"], meta["width"], 4)
if meta["floatEncoding"] == "scRGB-linear":
    rgb_nits = rgba[:, :, :3] * meta["linearUnitNits"]
```

## 色准对比边界

测试时暂停到同一帧、隐藏 Tab 信息面板，固定输出尺寸、字幕、ICC / LUT、色彩引擎、抖动和 HDR 状态。SDR 与 HDR 分开比较，将各播放器的截图转换到相同色域与传递函数后再测误差。直接比较 PNG 字节或凭图片查看器外观，不能判断色准。

16-bit 文件不会增加渲染表面本身的位深；JSON 的 `inputBitDepthPerChannel` / `readbackBitDepth` 会记录实际精度。madVR 当前截图接口仍只提供 8-bit，写成 PNG16 也不能恢复丢失的位深，不能据此宣称完成 madVR 高精度读回。截图取样发生在显示表面读回处，不能测量操作系统后续合成、显示器校准和面板的物理色准。

原盘 BD 菜单由 libVLC 输出，当前菜单专用右键不提供此截图入口；正片交接回 3FP 后按上述视频路径截取。

## 验证

专项结果保存在 `build/screenshot-acceptance-1.0.7.txt`、`build/screenshot-color-1.0.7.txt`。覆盖 RGBA16 逐像素往返、ICC 保留、相邻 16-bit 渐变、YUV 全 / 有限范围和未标矩阵、PQ / HLG、图片源 / 实画面、原生解码源画面、字幕实画面，以及真实 FP16 渲染表面与浮点附件一致性。强制 HDR 渲染用于验证数据链，不能替代 HDR 显示器的物理测量。

最终截图专项 17 项通过，0 失败、0 跳过。`build/screenshot-regression-1.0.7.txt` 的相关回归 27 项通过，覆盖透明控制栏、固定播放列表、音频电平、HEVC / AV1 解码、输出格式、真实 BD 五种字幕与长节目跳转。语言检查 534 条通过，编译与差异检查通过。

上述为新增保存路径和 SDR 预览之前的记录。本次发行复测另存于 `build/release-1.0.7-acceptance.txt`，覆盖默认路径、中文含空格目录、单次文件夹选择、所有增强 / 补帧标识、SDR 逐像素精度、PQ / HLG 的 sRGB 预览与原始 HDR 编码、浮点扩展范围及真实 FP16 读回；相关回归为 `build/release-1.0.7-regression.txt`。

本次最终专项 **18 passed, 0 failed, 0 skipped**；相关回归 **15 passed, 0 failed, 0 skipped**，包含真实盘五种字幕、8538.53 秒节目定位、底栏 / 播放列表、音频电平、HEVC / AV1 软件解码及输出格式。语言检查 536 条通过。

既有 `TestColor::nativeDefaultAndPausedColorSwitch` 出现阻塞，旧的音频 Native DLL (2E3D6B4E…1005) 对照同样在 30 秒内无法结束，记录为未通过，不将其归入截图验收。对照日志为 `build/screenshot-color-baseline-1.0.7.txt`。

硬件解码与高级显示处理的无回读检查通过；独立 `TestColor::realVsOutputPreserved` 在同一测试进程中未能结束，不能宣称整套色彩测试通过。截图专项中的真实 VS + 字幕截图及 FP16 渲染读回已单独验收。

````````
<!-- end:docs/player-high-precision-screenshots-1.0.7.md -->

<a id="record-docs-player-image-tools-1-0-4-md"></a>

## docs/player-image-tools-1.0.4.md

原始字节数：6313；SHA-256：`419f90717c4bea0a31126e7f41579c360478dfb52c7677e84984a6a3362042b7`。

<!-- begin:docs/player-image-tools-1.0.4.md -->
````````markdown
# VS Player 图片工具栏（1.0.4 开发中）

## 第二轮改进

顶栏48→38px，按钮40→32px；尺寸窗口至少560px，转换窗口至少640px。AVIF自动位深：原始8bit输出10bit，10/12bit保持，>12bit降至12bit；手动选择仍可用。

裁剪框内部移动，四角和四边调整，图像范围内框外拖动重新选区。自由、1:1、16:9、4:3、3:4、9:16和自定义宽高比；连续编辑自定义比例不会累积缩小框。比例在整数像素上允许最多一像素舍入。默认PNG Lossless，直接复制选区像素，避免JPEG重新编码带来的额外损失。

尺寸算法默认Jinc：CPU径向EWA 2-lobe窗口，Lanczos3/4为对应3/4-tap sinc窗口；缩小时扩展源像素采样范围。后台最多四线程，可处理超出GPU纹理限制的源尺寸，内存仍受实际系统资源限制；8/16位、ICC和预乘Alpha保留。CPU实现使用16384段核查表插值。最近邻/双线性提供较快选择。

Anime4K Mode A Fast可用于放大：只在选择它并另存时启动独立VS进程，经YUV444P16进入现有placebo Shader，输出恢复RGB和ICC，Alpha单独缩放。GPU纹理/显存限制仍适用于Anime4K；遇到错误明确报告，不冒充Jinc或无损放大。图片查看不执行这条处理链。

视频Lanczos4接入原生后端与Player/Renderer菜单，保留ABI编号0–7并新增8；对应源码补丁与构建步骤保留。设置的渲染页新增四级补帧起始档，自动启用/换文件从此开始。最低档的稳定5秒观察窗口内，至少48帧且丢帧超过5%时左上角提示“补帧性能不足，建议关闭补帧。”，不再因负载自动关闭；恢复正常后隐藏。暂停、定位和重开继续保留原状态规则。

专项日志 `build/image-refinements-final-tests.txt`：图片算法（含实际Anime4K输出、Alpha/ICC/自动位深）、裁剪交互、原生Lanczos4放大/缩小、最低档提示及恢复、设置应用状态/保存起始档、真实自动降档及失败，共六项通过（含Qt初始化/清理8/8）。最终裁剪控件复查见 `build/image-refinements-crop-final.txt`；20000px超宽源和16位缩放补充检查见 `build/image-refinements-depth-final.txt`。不运行无关全量回归，不将小样本检查视作超大图片性能基准。

## 操作

打开图片时显示顶部图标，视频/音频模式隐藏。按钮从左到右：详细编辑（打开独立编辑窗口，首批工具与边界见 [图像编辑方案](image-editor-plan.md)）、左转90°、右转90°、水平镜像、删除到回收站、桌面背景、调整图像大小、方格裁剪、压缩和转换格式。悬停显示中文名称。

旋转和镜像仅改变当前显示；换图片会复位，不写回原图。调整大小、裁剪、转换按照当前旋转/镜像画面另存，不覆盖正在查看的源文件。比例尺寸可联动宽高；裁剪拖选区域，显示三分网格，支持1:1。尺寸/裁剪另存无损PNG；转换选择目标格式。删除成功后打开相邻图片，最后一张删除后清空画面。桌面背景按屏幕尺寸保留比例生成独立BMP，不依赖临时目录。

输出使用后台线程，操作期间图标禁用，底栏显示开始、完成路径或具体错误；关闭程序会取消后端进程和文件写入。原文件与已有目标在失败时保留。未改动图像的压缩转换直接读源文件，避免先复制巨大的PNG。

## AWJimage

使用未经改动的 [AWJimage 1.1.0](https://github.com/Dominic485649/AWJimage/releases/tag/1.1.0) 独立CLI，原生编码算法：AVIF/libavif+AOM、WebP/libwebp、JXL/libjxl、JPEG/JPEGli、PNG/libpng。固定质量1–100；目标视觉质量由上游自动搜索达标候选（PNG只提供固定质量）。AVIF 默认AOM、YUV而非RGB/GBR Identity、420采样、自动位深，提供420/422/444/auto和auto/8/10/12位选项。JPEGli输出兼容.jpg。WebP/JXL质量100使用上游无损语义；JPEG100仍有损，PNG100无损。Alpha可自动保留、强制保留或移除。

AVIF/WebP/JXL统一速度0–10，默认5。AWJ负责线程/内存预算与超限AVIF自动网格，禁用自动尺寸缩小。视觉质量会反复编码/解码，耗时和内存明显高于固定质量；D3D11仅加速其视觉质量指标，编码/解码仍在CPU。这个上游版本使用AOM，不把它标作zenrav1e。

Windows后端要求AVX2。图片查看、旋转、镜像、裁剪/尺寸与桌面背景不依赖该后端。`runtime/awj` 包含原始AWJ.exe/AWJ.com、LICENSE、NOTICE.txt、本软件补充通知和固定修订源码tar.gz。后端及对应源码均校验SHA-256。详细来源见 `awjimage-NOTICE.txt`。

## 专项验证

只运行本次图片操作及相关查看测试；不运行无关的导出/音频/滤镜全量回归，也不以当前高负载机器上的耗时作性能结论。验证图标顺序/提示/禁用、旋转与镜像的像素共享、实际四角画面、宽高联动、裁剪坐标、ICC/Alpha保留、后台另存、原图保护、失败时已有目标保留；五种原生格式及视觉质量实际转换后重新解码。额外保留超宽图与深度缩放专项。

真实桌面背景未在测试中更换；回收站操作由Qt Windows实现，避免对用户图片执行删除测试。

Release构建通过；图标/窗口、变换输出、后台另存、五格式原生转换、WebP视觉质量和超宽图深度缩放共10项功能用例通过（含Qt初始化/清理为12/12）。日志：`build/image-tools-tests4.txt`。截图：`build/image-tools-toolbar.png`、`build/image-tools-crop.png`、`build/image-tools-convert.png`。

最终图标/对话框与后台另存专项复查通过，见 `build/image-tools-final-ui.txt`。本地 `dist/VS-Renderer-GUI-windows-x64` 更新两份EXE、独立后端和文档，EXE与构建结果一致；当前INI保留，native与VPY共20份哈希未变。旧1.0.3归档哈希未变，未创建1.0.4归档，也未执行Git推送。部署核对见 `build/image-tools-delivery-audit.json`。

第二轮已再次同步两份EXE和新增Lanczos4的FFF.Native.dll，三份文件与Release构建结果哈希一致。当前INI、VPY和LAV文件哈希未变，旧1.0.3归档未变；未打包或推送。核对见 `build/image-refinements-delivery-audit.json`。

````````
<!-- end:docs/player-image-tools-1.0.4.md -->

<a id="record-docs-player-images-audio-1-0-3-md"></a>

## docs/player-images-audio-1.0.3.md

原始字节数：5915；SHA-256：`74a52465443e6bb65111511207b7a561a2b3a379abe79e0600330fe01f546af7`。

<!-- begin:docs/player-images-audio-1.0.3.md -->
````````markdown
# Player 图片优化与音频导入验证（1.0.3，2026-10-02）

## 图片

Tab 显示文件名、路径、大小、格式、原始像素尺寸/像素数量、原始位深与采样格式（解码器可提供时）、ICC 颜色空间、解码器/耗时、解码像素位数/Alpha/内存、拟合输出尺寸、视口和缩放、CPU/GPU/进程内存及 VS/预解码状态。未提供的数据明确标注。图片仍不经过 VS，不预解码相邻文件。

JPEG 使用 libjpeg-turbo 3.2.0 的内存映射输入和直接 BGRA 输出，减少 Qt 逐行 RGB 转换/复制；保留完整原始像素、正常 IDCT/色度上采样、ICC 与 EXIF 方向。预编译 MSYS2 静态库的无 ICC 错误分支与本项目 MinGW 运行时不兼容，已改为当前工具链 + NASM 2.16.03 从官方源码编译。源码与 NASM 下载均固定 SHA256，许可证随便携目录分发。

图片独立允许 0.25–65536 倍拟合比例，视频原有范围不变；绘制先裁切可见区域，再映射源矩形，避免巨型目标坐标导致空白。1024 倍以上继续滚轮放大及 40000 像素宽图的边缘区域已专项验证。图像依然由 CPU 解码及 Qt Raster 绘制，尚未接入 GPU 图片解码或整图显存缓存。

真实 JPG：`C:/Users/ARXChem/Downloads/にんげんまめ￤2日目東ア-31_pid112201257_アリスp0.jpg`，1,809,331,931 bytes，31603×65278，2,062,980,634 pixels。

同一次比较中，新路径解码 **12477 ms**，Qt 原路径 **23262 ms**，减少 **46.36%**；49 个分布在全图的原始像素逐一一致。两者都是完整 RGB32 图像，8,251,922,536 bytes。计时为解码阶段，不包含整个测试的启动、参考解码和截图耗时，也不承诺其他图片有相同比例。日志：`build/image-turbo-comparison-final.txt`。

本机 RTX 4070 Laptop GPU 报告显存 8188 MiB；该图单一 RGB32 缓冲区约7869.65 MiB。完整驻留的余量有限，本轮交付采用上述 CPU SIMD 路径。硬件解码和分块显存渲染尚未实现。

先前同尺寸 8×16 grid AVIF 的7767 ms记录只验证了解码、尺寸和交互，未证明画面内容正确。后来原图对比确认该样本在编码阶段已损坏；该记录不能作为完整AVIF验证结果。修正样本、原因与像素对比见 `player-apply-avif-1.0.3.md`。PNG/WebP/JPG 切换、坏 JPG、AVIF 高位深/透明、ICC 与 EXIF 旋转有独立专项检查。

## 音频与拖放

右键“音频设置”位于“字幕设置”正上方：声音轨道（含 MKV 内置音轨和外部音频）、声音同步（复位/滞后0.1s/提前0.1s）、静音、均衡器。切回内置轨道先卸载外部音频。均衡器为非模态窗口：60/170/310/600/1K/3K/6K/12K/14K/16K 十频段、±12 dB、默认/低音/人声/高音预设、复位、MST 总音量/WAV PCM 音量；设置实时作用于播放音频，不暂停视频。已建立的均衡器图用 gain 命令更新参数；初次启用或改变处理结构会更新音频图。

实际处理使用 3FP 的 FFmpeg equalizer/volume，与原有 atempo 链相接。LAV 音频在使用这些控制时转为 3FP，LAV 视频保持原选择。未声明布局的音频按声道数补标准布局；明确声明的布局保留。WASAPI 优先申请源2.0/5.1/7.1，设备不接受时申请2.0立体声，按 FFmpeg 重采样/混音。在本机实测 2→2、6→6、8→2；不代表所有设备都支持相同输出。

1 kHz 测试音设+6 dB后实际 WASAPI 峰值由0.0265137变为约0.0528626；WAV音量50%检查通过；滞后0.1s后从零定位产生4800个48 kHz静音样本，提前偏移后仍正常推进。详见 `build/audio-image-acceptance.txt` / 最终便携专项日志。

自动导入仅扫描当前视频同级目录，分别选最匹配的一个音频和主字幕：同名优先，SxxExxx/EP/第x集等明确集数优先，再比较名称有效词比例；CRF/分辨率/编码数字不作为集数。明确集数冲突不匹配，同最高分歧义候选不自动加载。示例 S01E01-crf12-xxx 与 S01E01-CN 可匹配；S01E02 不会误配。

拖放使用**视频视口**分区：音频上50%挂为外部音频，下50%关闭当前媒体并单独播放；字幕上40%挂次字幕（顶部），下60%挂主字幕（底部），始终保留当前视频。拖动尚未松手时显示居中提示，退出拖动或完成放置后消失；全屏也可见。主/次外部字幕独立保存，可与内置轨道混选、交换。

## 验证与交付

只运行本次相关专项：图片信息/深度缩放、JPEG元数据、图片打开/切换/坏图、AVIF、文件名匹配、音轨/拖放/非模态窗口及实际音频处理。没有运行无关导出或整套回归。界面截图：`build/audio-equalizer.png`、`build/image-deep-zoom.png`。

Release构建及语言包311条字符串检查通过。最终便携目录7个功能专项加初始化/清理共9/9通过（`build/audio-image-portable-final-tests2.txt`）；实际音频增益/4800静音样本和2/6/8声道协商均在交付目录验证。首次便携专项中固定350ms的启动后时间推进断言失败，已改为3s内持续推进且保持Playing的检查，复测通过；未因此改动生产播放逻辑。`build/audio-image-delivery-audit.json`确认两EXE和FFF.Native.dll与构建匹配，12份INI/VPY哈希保留。最终程序交付至 `dist/VS-Renderer-GUI-windows-x64/vs-player.exe`，版本保持1.0.3；配置及用户VPY由交付前后哈希核对保留。没有创建压缩包、推送或发布。

参考实现与依赖：[Qt JPEG源码](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/plugins/imageformats/jpeg/qjpeghandler.cpp)、[libjpeg-turbo SIMD说明](https://libjpeg-turbo.org/About/SIMDCoverage)、[TurboJPEG源码](https://github.com/libjpeg-turbo/libjpeg-turbo/tree/3.2.0)、[FFmpeg equalizer](https://ffmpeg.org/ffmpeg-filters.html#equalizer)。

````````
<!-- end:docs/player-images-audio-1.0.3.md -->

<a id="record-docs-player-images-menu-1-0-3-md"></a>

## docs/player-images-menu-1.0.3.md

原始字节数：5537；SHA-256：`dc8d9f92dd6e27d53b827391ebb6b1c3c933957b21a6808f237eb846b60f0331`。

<!-- begin:docs/player-images-menu-1.0.3.md -->
````````markdown
# VS Player 1.0.3：独立图片与右键菜单

日期：2026-10-02。只更新本地源码与程序，版本保持1.0.3。

更正：首次 `build/supplied-large-grid.avif` 样本在编码阶段已经出现顶部错色和底部黑块，下面原有“打开、显示检查通过”只代表流程及尺寸验证，不能证明像素内容完整。原图对比及新的YUV420样本见 `player-apply-avif-1.0.3.md`；不再使用首次样本证明完整AVIF支持。

## 图片路径

此前本地图片与视频共用FFMS2/VS源流程，JPG被当成视频帧序列。现在在VS初始化等待之前识别本地图片，单独通过PlayerImage后台线程解码；显示交给PreviewPane的QImage绘制模式。不会创建视频索引、执行VPY滤镜、请求视频帧或预解码相邻图片。打开视频时恢复原生表面与视频控制。

打开一张图片后，以同文件夹可识别图片的名称排序重建列表；左右键切换、滚轮缩放、放大后拖动平移，左上角还原按钮恢复默认适配。视频滚轮设置保持原值。Qt优先读取并应用EXIF方向，未提供的图片codec转交随包FFmpeg；GIF/APNG等当前只显示第一帧。解码失败显示错误，切图后旧结果不会覆盖当前文件。

AVIF直接静态链接libavif1.4.2与dav1d1.5.3。Unicode路径由QFile流式IO读取，支持grid、高位深、透明、旋转与镜像。10/12位保存为RGBA64。官方libavif1.4.2不仅CLI，API也拒绝超过268435456像素的显式上限；构建时对这一处guard受控修改，Player将像素上限设置为UINT32_MAX、边长上限设为0，保留尺寸除法、grid及AV1验证。直接向QImage转换，不生成巨大临时PNG、不上传为受边长限制的视频纹理。依赖源码与dav1d静态库的版本和SHA-256固定在tools/image-runtime.cmake，随包许可证为IMAGE-LICENSE.txt。

这是当前图像的必要解码，并非视频预解码。完整原图仍占用相应内存；本机为64GiB级内存。内存不足时解码会失败；没有通过缩小图片规避原尺寸。

## 右键菜单

PlayerMenu保留QMenu的菜单动作、键盘与子菜单机制，统一应用到主级及所有子级。显示时采用180ms三次缓出揭示，PreciseTimer每8ms更新；实际呈现频率取决于桌面刷新率和负载。透明窗口绘制8px抗锯齿圆角，不只修改stylesheet的border-radius。文字从行左48px开始，16px方框的中心位于24px处，恰在左缘与文字间；单选与布尔动作都显示边框和明确勾选，禁用项变灰。

## 验证证据

- Release构建：build/image-menu-build.txt。
- 专项：build/image-menu-targeted.txt，6项通过、0失败、0跳过（含初始化/清理）。覆盖JPG大写后缀、PNG、WebP、BMP、左右排序、默认音量滚轮被图片缩放替代、坏JPG恢复、图片→视频→图片；另覆盖TIFF、JPEG XL、GIF、JPEG2000、TGA。
- AVIF固定小样本：tests/fixtures/wide-grid.avif为40000×64、8×1 grid；alpha-10bit.avif为128×96透明10位。验证尺寸、alpha、连续切图旧结果丢弃。
- 菜单：180ms过程中可观测中间状态，主/子菜单都完成展开；实际截图build/menu-check-test.png已检查圆角、蓝色方框及居中位置。
- 用户JPG：`C:/Users/ARXChem/Downloads/にんげんまめ￤2日目東ア-31_pid112201257_アリスp0.jpg`，文件1809331931字节，31603×65278，共2062980634像素；保持完整尺寸打开、显示画面颜色变化、缩放/平移检查通过。测试进程总耗时22053ms，QImage像素缓冲8251922536字节。日志build/image-menu-large-jpg.txt。
- 同源完整AVIF：build/supplied-large-grid.avif，205270684字节，8×16 grid，4:4:4、8位，原尺寸不变。为避免原JPG奇数宽度违反4:2:0 grid限制，采用4:4:4，不裁切原图。打开、显示、缩放与平移检查通过，专项总耗时9348ms；日志build/image-menu-large-avif.txt。测试AVIF由官方avifenc/aom生成，不冒称由zenrav1e编码；zenrav1e属于编码器，解码不依赖编码器品牌。
- 用户报告部分损坏的TEST.avif：81355277字节，解码结果32768×21845，像素缓冲5726535680字节，打开、显示、缩放检查通过，专项3370ms。日志build/image-menu-damaged-avif.txt。解码器未报告结构/码流错误，这不能证明图片内容完好，也不会恢复已损坏的像素；以完整同源AVIF验证尺寸支持。

- 共同完整回归：build/image-menu-ctest.txt，八组中七组首次通过；导出组一次失败，单独明细复测8项通过（image-menu-export-retest.txt），CTest导出单组再验通过（image-menu-export-ctest.txt），未改导出代码。Player完整组36通过、0失败、7跳过；跳过为未提供环境变量的可选外部/远程素材及巨图专项，巨图已通过上列独立实测。六档Anime用例同时通过，非本轮图片功能范围。


- 本地交付：执行一次stage-portable，更新dist/VS-Renderer-GUI-windows-x64。交付目录图片/菜单/原有还原按钮专项7通过、0失败、0跳过（image-menu-portable-tests.txt）。两正式EXE与FFF.Native.dll均与构建哈希一致；INI及用户VPY不变。共同六档工作使已知旧默认builtin/Anime.vpy在首次启动定向迁移，.before-six-stage备份哈希与部署前原件一致。临时测试EXE与Qt6Test.dll已移出正式目录；完整图片许可和本说明随包。审计为image-menu-delivery-audit.json。不打包、不推送、不发布。

````````
<!-- end:docs/player-images-menu-1.0.3.md -->

<a id="record-docs-player-input-layout-1-0-6-md"></a>

## docs/player-input-layout-1.0.6.md

原始字节数：2554；SHA-256：`1c95569c4012b43d654e561d82bb2d43054da0d634d1e53c53789d07af650dd8`。

<!-- begin:docs/player-input-layout-1.0.6.md -->
````````markdown
# 1.0.6 播放器右键、工具栏和跳转

本轮只更新本地 Player 和说明文件，不打包、不推送、不上传。原生 DLL、个人配置及 VPY 保留。

## 修改

- BD 原生窗口右键在 GUI 线程读取 Qt 逻辑鼠标坐标，再弹出菜单。之前直接使用 Windows 物理坐标，在 150% DPI 下会错位。右键按下由 Qt 菜单处理，避免 libVLC 抢占鼠标捕获。
- BD 运行时延长 libVLC 指针自动隐藏超时，菜单交互时保留指针。VLC Windows 输出原本通过计时器隐藏指针，零超时会立即隐藏，故未使用零值；实现依据见 [VideoLAN Windows 鼠标事件源码](https://raw.githubusercontent.com/videolan/vlc/3.0.x/modules/video_output/win32/events.c)。
- BD 播放模式选择框最小宽度为 360 个逻辑像素，并根据当前字体的标题宽度增加空间，避免标题截断。
- 固定视口布局下，图片编辑工具栏固定在中央区域顶部，覆盖显示且保持全宽；关闭固定视口后回到顶部布局。此前移走视频与底栏后，工具栏成为布局中唯一可见控件，被垂直居中。
- 进度条左键直接跳到点击位置，按下时先建立拖动状态；拖动继续显示预览。松手目标若与最后一次预览相同，不再提交第二次跳转。打开新媒体同时清除上次拖动目标，保留播放/暂停状态恢复。

## 验证

专项回归覆盖空窗口 BD 菜单、150% DPI 原生右键坐标、原盘确认、图片顶部工具栏、Jinc/D3D11 的播放与暂停拖动预览、固定视口、HW/SW 节目切换、29 首 OST、Tab、MKS、语言与字体。日志为 `build/player-input-1.0.6-final-tests.txt`。

跳转测试使用 1280×720、24fps、8 秒 H.264 长 GOP 文件，强制软件解码；分别使用正常 CPU 亲和性及单核限制。已验证目标画面呈现，预览后松手的 timelineGeneration 只增加一次。一次实测点击中间位置出帧分别为 64 ms 和 117 ms；这是测试素材的结果，不表示低端设备上的 4K/8K 长 GOP 已达到同样速度。

本轮 Windows 已锁屏，物理鼠标事件命中 `LockScreenBackstopFrame`。因此 BD 测试向播放器原生窗口直接发送消息，确认弹出菜单矩形包含当前 Qt 鼠标位置，并能通过原生消息进入正片。指针保持显示的修改已编译，但本轮未完成解锁桌面上的视觉复测，不把锁屏指针当作 VLC 指针验收结果。

部署备份、受保护文件和资源清单哈希见 `build/player-input-1.0.6-delivery.json`。

````````
<!-- end:docs/player-input-layout-1.0.6.md -->

<a id="record-docs-player-interaction-performance-1-0-5-md"></a>

## docs/player-interaction-performance-1.0.5.md

原始字节数：7159；SHA-256：`4ea1b3bce62c71fbc0f2224ac8423b92bda623154396825524908906c3baba85`。

<!-- begin:docs/player-interaction-performance-1.0.5.md -->
````````markdown
# 1.0.5 交互与实时调度更新

日期：2026-10-04–05。基线为返回设备的 1.0.5；本轮不推送源码、不重新打包、不创建 Release / Draft。下面将功能验证与性能目标分开记录。

## 行为与实现

- 精确帧输入修改后，Enter 或点击其它位置提交跳转；跳转尚未呈现时 Enter 继续被消费，不触发全屏。普通状态仍可 Enter 全屏，F11 不变。
- 编辑器多工具组从启动即显示下拉箭头；Delete 删除当前未锁定图层，父组锁定也受保护。移动像素层时使用缓存视图代理随指针移动，松手一次提交原精度像素位置及撤销记录。复杂混合层的拖动代理不承诺与最终合成逐像素一致。
- 编辑器保留整图概览，在局部高质量预览准备期间填补可见区域；视口预览提高到最高两倍显示像素密度，仍受原生像素密度、8192 边长和 256 MiB 浮点 staging 上限约束。文档缓存按可用物理内存调整，范围 64 MiB–1 GiB。不会为超大图创建不可承受的全画布浮点副本。
- 图片查看器普通滚轮缩放，Shift 滚轮平移；视频右键选择滚轮缩放后按鼠标位置锚定源像素。超大图后台生成有界、保留像素格式的显示层级，接近 1:1 时仍读取原像素。
- 大 RGBA8 PNG 在内存足够时并行重建预测行；波前块保持 Paeth 左侧/上一行依赖。至少 128 MiB、512 行且可用内存充足才启动，最多 16 线程；内存不足使用原位单线程路径。压缩流解压仍有串行依赖，CPU 功耗不是加载速度的验收指标。
- 补帧提供 Jinc / D3D11 两个自动入口与八个固定组合。配置的预解码帧数仍是上限；GPU/CPU 利用率高不再直接关掉预取，内存/显存压力仍会限制队列。补帧使用全部可用 VS 工作线程和有界缓存；不改变模型、补帧质量、色度和源位深来伪造提速。
- 新增可选 3FP 音频/时钟接口：Player 的 VS 路径不再额外解码源视频，保留元数据、音频及播放时钟；重建解码会话会保留此标志。Renderer 源路、原生直通和 madVR 不改变原来的视频解码职责。补丁为 `patches/3fp-vs-audio-clock.patch`，构建脚本支持重复应用检查。
- 拖进度条按 16 ms 收集最新目标，等待上一张预览后继续；停止拖动精确定位并恢复播放状态。原生暂停态短距离前向跳转尽量连续解码，远跳、后退、逐帧仍走原路径。
- Tab 面板使用跟随播放器的非激活浮层，避免小窗口的视频区域裁切信息；不自动折行，最小尺寸由文本决定。右下角拖动调整尺寸并同步增大字体，下方调整背景不透明度。目标帧率包含倍速；当前帧率按接受帧减合并帧的净增量采样，不将提交 FPS 再乘倍速。该计数有一个在途呈现窗口，不等于显示器扫描测量。帧尺寸顺序为源 → VS 输出 → 当前呈现；补帧、缩放滤镜、色彩引擎独立列行。

## 大图实测

本机 `TEST.png`：48000×32000，RGBA8 原像素约 6.14 GB。Release 构建，系统文件缓存可能命中，未清空全机缓存或做机械硬盘测试。

`png-parallel-results.txt`：解析 764 ms，解压 6341 ms，预测重建 1055 ms；解码 8701 ms，编辑器就绪 **8995 ms**，通过 10000 ms 预算。单/并行 PNG 与 Qt 参考结果逐像素比较通过；144 MB 合成样本确认启用了 16 个预测线程。预测重建前一轮约 2940 ms。

最终复测 `huge-editor-final.txt`：解析 868 ms、解压 6120 ms、预测重建 851 ms，解码 8332 ms、编辑器就绪 **8797 ms**。`huge-viewer-final.txt` 确认巨图滚轮缩放，以及切换到小图后的滚轮缩放均可用；这条测试含呈现抓图/切图，不将总运行时当成解码耗时。

## 性能验收边界

设备：RTX 4070 Laptop，用户开启增强模式并说明可稳定释放 120 W；记录实际功耗，不能把 120 W 当作每种算法的实测值。源为 MyGO 1920×1080 AV1 YUV444P10 / FLAC，47.952 fps；真实 2560×1440 呈现表面。完整 RIFE 测试通过 SelectEvery 构造 23.976→47.952，半宽高/MVTools 为 47.952→95.904。使用已有索引，seek 到 120 秒，预解码上限 8。

半宽高 RIFE 稳态已接近 95.9 fps，但加载/seek 后仍观察到短暂跳帧。完整 RIFE、MVTools HQ/LQ 的实时目标尚未全部达到；不能因测试程序正常结束或 3FP 渲染丢弃为零而报告“不丢帧”，必须同时计入 VS 跳过和时间轴落后。原始 CSV 和日志位于 `build/dist-1.0.5-20261004/`。

- `timestamp-final-1/5.csv` 后 12 秒：半宽高 RIFE Jinc / D3D11 为 95.91 / 95.93 fps，均无新增 VS 跳过，末端落后约 6 ms。前段加载/seek 仍有瞬态跳过，所以不记为全过程无丢帧。
- `handoff-mv-low.csv` 后 12 秒：MVTools 低档 Jinc 为 63.59 fps，新增跳过 385 帧，末端落后约 149 ms，**未通过** 95.904 fps 目标。邻域顺序消费优于直接随机追赶，但没有解决剩余瓶颈。
- 完整 RIFE 多轮实际呈现约 5–14 fps，远低于目标；仅离线顺序取帧约 42 fps 不能替代 GUI 实时验收。GPU 队列 1/2/8 和强制源/RGB缓存试验均未解决该问题，未合入这些试验性参数或改动用户预设。
- 功耗使用 `nvidia-smi` 逐次采样并及时写盘（不是 PawnIO），`warm-final-power.csv` 峰值 94.61 W；不能从采样未到 120 W 单独认定硬件功耗限制或某个具体瓶颈。
- 真实 GBC 3840×2160 / 48 fps，10 秒连续拖动：`scrub-2160-jinc.txt` 约 33 张/秒，`scrub-2160-d3d11.txt` 约 33.9 张/秒，松手精确定位通过。D3D11 样本期间有一次小型测试目标编译，不作为严格隔离的性能上限；这是连续前向场景，未宣称任意远距离/反向拖动达到同样帧率。

## 回归与交付状态

Release GUI 与原生 DLL 已编译到 `build/dist-1.0.5-20261004/`。图片相关 10 个测试目标从项目根目录运行通过；编辑器交互为 14 passed / 2 skipped，未提供真实 PSD 的用例保持跳过。播放器最终聚焦交互/补帧切换测试为 13 passed；VS → 3FP 帧桥为 10 passed，原生补丁 reverse-check 通过。既有 `TestVpyScript::anime4kUsesShaderPathAndUniformScale` 的 shader 路径字符串断言仍失败；对应生成器本轮未修改，不删除或掩盖这条失败。

同一工作区的 BD 会话使用独立构建目录完成模块验证，并于 2026-10-05 统一部署最新 GUI、包含音频时钟及 BD 输入补丁的 Native DLL、QtConcurrent 与下载脚本到本地 dist。已核对 dist 原生 DLL SHA256 为 `FF49CFC45B22F073E079E85878E0C19BF23AC2E02263B1FCE375FD2BFC610719`；两个 EXE 均为 10 月 5 日构建。BD 会话报告 19 个 INI/VPY/color.md 哈希保持一致，备份在 `build/bd-deploy-backup-20261005/`。本轮没有新增压缩包或推送。

高分辨率、120 Mbps 与 HDR 色彩精度专项由另一桌面负责，本轮没有替代其验收；780M 未在本机复测。真实 HDR 编辑器观感、冷盘大图与完整 RIFE / MVTools 无丢帧均不能宣布完成。

````````
<!-- end:docs/player-interaction-performance-1.0.5.md -->

<a id="record-docs-player-kernel-d3d11-performance-plan-2026-10-17-md"></a>

## docs/player-kernel-d3d11-performance-plan-2026-10-17.md

原始字节数：27602；SHA-256：`93b43ce2c2dc634ed4a75ea67fd50cf43343d91f92f5d85aa0b6a4c8f1ff8eb2`。

<!-- begin:docs/player-kernel-d3d11-performance-plan-2026-10-17.md -->
````````markdown
# VS Player 播放内核性能优化设计与验证计划（2026-10-17）

> 状态：**设计与测试计划，未实施、未验证新方案**。基线以 AgentDock-PC 的 `VS-Renderer-GUI` 项目、提交 `d00741e` 和本地 1.0.12 发布目录为准。本文提供可交给下一轮开发会话实施的最小改动路线；不把历史成绩算作本轮成绩。
>
> 作用范围：`src/backend`、`src/player`、`build/native-api20/FFF.Native/3FP`（构建工作树）及对应的原生补丁/构建入口。最终实现必须更新**可重建的补丁或权威源码**，不能只修改临时 `build` 文件。本文本身不修改程序、运行目录、用户配置或任何媒体。

## 1. 目标、约束和成功定义

### 1.1 总目标

在不降低源视频位深/色度、不改变默认色彩算法、不丢帧作弊的前提下，提高高码率 2160p/4320p 软件解码播放吞吐和帧时间稳定性。优先解决 AV1 4:4:4 12-bit、HEVC 4:4:4 10-bit、大尺寸 YUV CPU→D3D11 上传时的反压。D3D11VA 硬解和普通 SDR/HDR 原生直通是必须保留的性能及质量基线。

**不要假定**增加缓冲、更多 FFmpeg 线程、CPU 亲和性或进程优先级必然提速；同一实验只改变一个变量。停止条件优先于性能收益。

### 1.2 全局硬性验收

1. 正确性：完整解码或预期有界跳帧；不重复呈现旧帧冒充新帧；视频帧时间戳单调、音画时钟连续；暂停、seek、换轨、EOF、重开、设备丢失恢复不得死锁或泄漏。
2. 精度：`yuv444p12le` 仍按 12-bit/4:4:4 解码，不能降至 4:2:0 或 8-bit；VS 输出的 range/matrix/primaries/transfer/chroma 不得被原流元数据重新覆盖。HDR scRGB FP16 与 SDR 输出保持既有格式及颜色。
3. 性能：以**相同硬件、同一时间窗口、同一渲染与音频设置**比较基线/候选，每配置至少三轮。对未触及瓶颈的 1080p、4K 硬解路径不接受稳定段吞吐退化；p95/p99 帧延迟、丢帧和音频欠载不能恶化。
4. 内存：必须记录 RSS/Private Bytes、FFmpeg 解码器在飞帧、应用队列帧和 GPU 专用/共享显存。512 MiB 是现有软件帧预算，**不是整个播放器内存的硬上限**；FFmpeg 内部参考帧/流水线、GPU 纹理、交接帧都应纳入实验账本。
5. 部署：先独立候选 DLL 和测试目录，测试通过后才能考虑替换 `dist/VS-GUI-Full`、`dist/1.0.12-PlayerLite` 和单文件版本。保留旧二进制回滚，INI/VPY/ICC/LUT 逐文件哈希不变。
6. 任何声称“8K48 通过”的结论，必须是固定完整播放链路（含音频/字幕说明）的**稳态实际提交及显示节奏**；仅解码输出、测试函数返回成功或零新增 dropped 都不够。

### 1.3 范围外

不在本轮更换 FFmpeg/dav1d/libplacebo/Qt/VS 版本；不改 HDR 算法、插帧/Anime4K 模型；不添加通用 CPU 任务调度框架、驱动、内核优先级或未经验证的 Vulkan/CUDA 零拷贝链。CPU 优先级与 CCD 调度仅作为可关闭的后续实验。

## 2. 当前已实现的能力与定位

| 模块 | 本地事实 | 对应证据 |
| --- | --- | --- |
| 解码队列 | 最多 8 帧；软件应用队列预算 512 MiB，硬解相关常量 128 MiB；目标约 150 ms 预取；当前实现还根据 decoder 在飞帧扣减队列空间，最低队列保护值为 2 | `build/native-api20/FFF.Native/3FP/Core/PlayerSession.cpp`：`VideoFrameQueueLimit`、`TargetVideoLookAhead100ns` |
| 软件解码线程 | GUI CPU 解码模式默认允许自适应，`VSR_ADAPTIVE_THREADS=0` 可关闭；普通策略初始目标 16，检测缺帧且无余量时向 24/32 攀升，重新打开解码器/seek 有代价 | `src/backend/ThreeFpPlayer.cpp`；`PlayerSession.cpp`：`SelectSoftwareDecoderThreads`、`EvaluateDecoderLoad` |
| dav1d 特化 | 重负载 4K+ 444p12、8K420p10 按不同上限设置 `thread_count` 和 `max_frame_delay`；dav1d 不参加通用重开线程阶梯 | `PlayerSession.cpp`：`OpenDecoder` |
| 独立解码 | 现有 `SoftwareVideoDecodeWorker` 及有界包输入、单个 `softwareVideoOutput_` 交接槽；正常播放下 `QueueSoftwareVideoDecode` 的入口限 `libdav1d` | `PlayerSession.cpp`：约 3485–3612 |
| 大图上传 | 大于等于 4K 的 planar/semi-planar 使用 DEFAULT texture + `UpdateSubresource`；较小平面使用 DYNAMIC + `Map(WRITE_DISCARD)` | `build/native-api20/FFF.Native/3FP/Render/VideoRenderer.cpp`：约 3595–3627、6085–6150 |
| 硬解直通 | VideoProcessor 可使用 D3D11 解码 slice；Shader/Jinc/高级颜色时保留必要 GPU 复制 | `VideoRenderer.cpp`；`docs/nuxbox-performance-session-2026-10-04.md` |
| DXGI | `FLIP_DISCARD`、`BufferCount=3`、`IDXGIDevice1::SetMaximumFrameLatency(2)`，默认 `Present(1,0)`；VRR/tearing 为可选分支 | `VideoRenderer.cpp`：约 3120–3185、7330–7355 |
| 色彩 | Full 默认 `engine=2`：普通 SDR 与可用 HDR→HDR 保留原生路径，HDR→SDR 需要时用 libplacebo；ICC/LUT、BT.2020 CL/NCL、PQ/HLG、HDR10+ 部分元数据和 FP16 scRGB 已有专用处理 | `src/player/PlayerColor.cpp`、`src/color/ColorBridge.c`、`NativeColorEngine.h` |
| CPU 拓扑/优先级 | 未找到主解码链 P/E 级别调度、先单 CCD 再跨 CCD、进程整体提权；只发现后台封面线程主动降至 `THREAD_PRIORITY_BELOW_NORMAL` | `VideoRenderer.cpp` 的 `CoverBackdropThread`；对原生播放目录的亲和性/优先级 API 搜索 |
| 构建对应 | 先前核验 Full/Lite/Release 三处 `FFF.Native.dll` 的 SHA256 相同，标识前缀 `666E5EC4C5A0E377`；实施前仍应重新核对当前时间戳/哈希 | 本轮现场对照（非持续不变的承诺） |

**特别注意**：`.deps/fff-upstream-api20` 的上游源码默认有 128 MiB/8 线程等旧常量；它不等同于本地已部署的 `build/native-api20` 合并后代码。实现分析以实际构建 DLL 的源码、补丁和二进制核对为准。

## 3. 历史实测：必须保留的正反证据

### 3.1 HEVC 444p10、4K24、高码率尖峰

`docs/hevc444-peak-diagnosis-1.0.6.md` 在 7940HX、4K 输出、强制 CPU 解码、D3D11 原生链下对 `D:/RebellionHEVC444p10-700MBPS-SoftwareLast.mkv` 记录了约 812 Mbps 峰值：

- 8 线程 + 128 MiB：峰区曾新增 5–7 个丢帧。
- 8 线程 + 512 MiB：仍出现丢帧。
- 16 线程 + 128 MiB：相同样本两次测试达到零丢帧。

因此不能只通过提高缓存预算推断能解决卡顿；FFmpeg 内部并行度和主 Pump 阻塞更关键。随后 1.0.7 的 `docs/software-decode-subtitles-1.0.7.md` 记录了改进后完整 2252 帧无丢失的同机测试，但**不能直接外推到所有 4:4:4 视频**。

### 3.2 8K48 420p10 软件解码仍失败

`docs/update-1.0.11.md`：7940HX、8K AV1 47.952fps、CPU 解码、4K 显示，完整 GUI 的 60s 窗口约 **41.03 解码 fps / 31.03 提交 fps / 599 丢帧 / 233 音频欠载**；纯 FFmpeg/libdav1d 不呈现约 **46.79 fps**。不能把完整播放链约 31 fps 与纯解码差额全部归咎于 GPU，也不能凭补丁通过推断已达到实时 8K48。

### 3.3 4K444p12 Anime 现有通过项

`docs/player-anime-performance-1.0.12.md`：4K 444p12 AV1 约 47.95fps → 1440p CNN/CNN+，指定窗口约 30s 可实现源速率零新增丢帧；**Y412→4K48 原型尚未通过**。增强链必须与普通直通分开计算。

### 3.4 旧上传基准

当前 `VideoRenderer.cpp` 注释记录 10-bit 平面上传对照：1080p Map/Update 0.582/0.546ms、4K 3.293/2.886ms、8K 22.266/15.213ms。这些数据是在旧实验条件下测得，不是当前 A/B 结果。禁止将“8K Update 较快”误写成“GPU 已足够快”。

## 4. 分阶段实现设计

### 4.1 P0-A：测量先行，不动算法

**目的**：将问题区分为读取/解复用、FFmpeg send/receive、CPU 在飞帧等待、应用单槽交接、主 Pump/音频/字幕、D3D11 提交、GPU 着色、DXGI Present 与真实上屏。

增量埋点（debug/perf build，默认关闭）：

1. `av_read_frame`、`avcodec_send_packet`、`avcodec_receive_frame` 的墙钟耗时；另记 FFmpeg 内部 frame delay、线程数和输入包 PTS。调用等待时间不得描述成“纯 CPU 解码耗时”。
2. `softwareVideoPackets_`、`softwareVideoOutput_`、`videoFrameQueue_` 的长度、帧寿命、生产者/消费者等待纳秒；增加 `frame_generation` 以区分 seek 前后；记录丢弃原因。
3. `UpdateSubresource`/`Map` 的 CPU 提交耗时、GPU timestamp 的异步查询结果；对 query 使用延迟收集，**不能为了测 GPU 在热路径 `GetData` 自旋等待**。
4. 逐秒：decoded/accepted/dropped/coalesced、实际显示 cadence（DXGI frame statistics 可用时）、audio underruns、media-clock error、CPU 核当量、工作集、VRAM、输出色深、实际 swap/dest 尺寸。
5. 从诊断日志区分首帧、seek 预热、稳定段、峰值段、EOF；每帧可按 frame-id/PTS 关联，不允许只有平均 fps。

输出建议：`build/perf-kernel-20261017/baseline/<case>/<run>.csv`、`stage.csv`、`system.csv`、`summary.json`。采集开销需用 profiling off/on A/B 核对；正式性能比较关闭重度逐帧日志，仅使用低干扰计数器。

**验收**：确认测试本身不破坏帧序/音频时钟；统计可区分“解码快、提交慢”“GPU 慢”和“时钟/音频停顿”；日志附实际 DLL、FFmpeg DLL、媒体哈希/时长、OS/驱动、电源计划、刷新率、色彩设置。未做到前不开始声称性能提升。

### 4.2 P0-B：软件解码 → 呈现的有界解耦

**现状**：`SoftwareVideoDecodeWorker` 已存在，但仅限正常 libdav1d；其 `softwareVideoOutput_` 是单槽，主会话取帧慢时解码工作者仍会阻塞。

**最小候选**（依次单独验证）：

- B1：保持原有 FFmpeg 解码器、线程数/帧预算和帧顺序，仅将单槽改为**可选的 2 帧或 3 帧有界交接环**，用引用计数持有 `AVFrame`，确保不会复制整个 4:4:4 12-bit 平面；生产者达到上限阻塞，不能无限入队。
- B2：对 HEVC CPU 解码（以及其它确实使用 `AVCodecContext` 的 FFmpeg CPU 解码器）复用同一独立工作线程机制，但禁止会话线程和工作线程无互斥地同时 `send/receive/flush/free`；seek/flush/切流/关闭先停工作者并清除旧代际。
- B3：分离读取/解码与提交的锁范围，避免持有会话或 D3D11 设备锁等待输入；避免因为字幕/音频更新造成循环性的帧交接反压。

**内存与调度约束**：

- 应用解码队列现有 512 MiB 预算、八帧上限、150ms 预取仍保留；**新交接槽必须与主队列及在飞帧一起核算**，不可变相扩成 512 MiB + 3 大帧 + 32 参考帧而不记账。
- 4K444p12 一帧约 47.46 MiB，8K444p12 一帧约 189.84 MiB。大帧模式应优先减小交接槽数而不是继续增大总缓冲。
- `av_frame_clone` 只增引用并不保证引用生命周期零成本；测试必须涵盖硬解纹理/软件 AVFrame 的引用释放时机。
- 不能因为队列更深而悄悄提高 A/V 延迟；非实时文件播放可容许少量预取，但 seek/pause 需立即失效旧代际。

**回滚**：保持原单槽/原 HEVC 线程方式可由实验开关恢复；如果 p99 延迟、内存、seek 或音频欠载恶化，即便平均 fps 提升也不默认启用。

### 4.3 P0-C：D3D11 大帧上传流水化

**现状**：4K/8K planar 当前使用 `D3D11_USAGE_DEFAULT` + `UpdateSubresource`；硬解直通在无需 Shader 时已可使用解码 surface slice，不应强制增加拷贝。

候选顺序：

1. C1：针对软件 YUV 平面，在保持输入格式和每一平面 R16/R8 texture 映射不变的前提下，对 `DEFAULT` 上传纹理做 2/3 组复用 A/B，允许 CPU 准备下帧、GPU 消费上帧。复用前需要明确的资源仍在 GPU 使用时的驱动行为；使用 fence/query 的无阻塞收割，不在热路径等待完成。
2. C2：只有 C1 的 ETW/GPU timestamps 证实同步等待时，才探索 CPU 侧紧凑排列、减少多余 `memcpy`、行对齐/上传批次以及可复用 staging 路径。不能把 `Map(WRITE_DISCARD)` 套到 `STAGING` 纹理，不能认为 `ID3D11DeviceContext` 的 deferred context 必然提高 GPU 实际吞吐。
3. C3：高级颜色路径仅在存在额外 `NativeColorEngine::draw` 的动态纹理→DEFAULT `CopyResource` 且其确实成为瓶颈时研究零重复复制或资源常驻；确保同步、安全的 resource usage、HDR metadata 完整，不要将 GPU→CPU 回传引入显示热路径。

对照 A/B：原单纹理、2 组、3 组；分别在 4K444p10、4K444p12、8K420p10、8K444p12（存在样本时）进行，记录 CPU Update 延迟 p50/p95/p99、GPU 拷贝/Shader、GPU 内存峰值、实际显示帧。若帧数未提高却额外占用数百 MiB 显存，应回退。

### 4.4 P1-A：CPU 拓扑感知，先采样后设置亲和性

**用户提出的策略**：“优先 P/E 核、先单 CCD 后双 CCD”。当前项目并未实现此策略；7940HX 不是 Intel P/E 混合核心，因此不要把 P/E 优化强行应用到它身上。

建议按 `GetSystemCpuSetInformation`、`GetLogicalProcessorInformationEx` 和 processor group/NUMA/topology 查询建立**实验用**映射，先核验物理核心/SMT/LLC/CCD 对应关系，不能直接假设 CPU 编号 0–15 属于同一个 CCD。

候选 A/B：

- A0 Windows 默认调度（强制保留）；A1 只给音频/时钟/呈现关键线程设置合适的软 CPU Sets；A2 解码工作者尽量同一 CCD；A3 在持续发生解码吞吐不足时扩展到另一 CCD/所有核。
- 混合 P/E 机器单独运行 P 核优先、系统默认、部分 E 核参与三组。FFmpeg 内部线程创建由第三方库管理；**若未可靠控制其线程集合，不得宣称已经绑定所有解码线程**。
- 单 CCD 可能丧失一半物理核心吞吐，跨 CCD 可能增加缓存/内存访问；只按实测 p99/完成帧数选择，不能因“局部性理论更好”而默认收窄亲和性。
- 禁止运行中频繁更换亲和性和逐帧迁移；固定窗口阶段调整后需稳定观察数秒再计量。

### 4.5 P1-B：调度优先级和 MMCSS

当前未见主播放线程统一设置高优先级，只有封面后台线程降低优先级。候选为：音频和视频呈现的**关键实时性线程**试用适当的 `AvSetMmThreadCharacteristics` / `AvSetMmThreadPriority`，保持 FFmpeg 解码工作者普通优先级为基线。进程级 `HIGH_PRIORITY_CLASS` 仅作为另一个独立可撤销实验，绝不使用 `REALTIME_PRIORITY_CLASS`。

验证必须在后台 CPU 压力 0/50/90%、窗口前后台状态、HAGS/VSync 状态固定时逐项对照，检查音频欠载、UI 输入延迟、前台其他进程响应和是否引入优先级反转。任何系统交互退化或音频欠载增加即撤销，不因微幅 fps 改善继续启用。

### 4.6 P2：交换链与色彩成本的隔离对照

**当前三缓冲并非缺失项**：`FLIP_DISCARD`、3 buffers、最大帧延迟 2 已实现，盲目扩大到 4–6 不在推荐范围。

- D1：保持现有模式；D2：实验性创建 `FRAME_LATENCY_WAITABLE_OBJECT` 交换链并用对应 waitable object 进行节奏控制；注意当前仅用 device-level `SetMaximumFrameLatency(2)`，不能在未打开相关 swap-chain flag 时认为 swapchain2 latency API 有效。
- D3：普通 SDR、HDR 原生、HDR→SDR libplacebo 三套处理分开计时；比较是否存在重复 GPU Copy 或额外 ICC/LUT 资源生成。处理 HDR→SDR 时不得通过关闭色调/色域映射换吞吐。
- D4：Shader/常量与 LUT 重用以现有缓存为基线，已经分开 t3 HDR 曲线、t4 transform LUT、t5 Jinc 权重；不能再引入同槽资源污染。

颜色回归同时比对 GPU 离屏高精度读回和真实显示器（如有色度仪另测 ΔE），前者证明代码路径数值一致，后者才能声称绝对色准。没有测量不宣称“比 mpv 色准高”。

## 5. 具体测试矩阵与验收办法

### 5.1 运行条件（所有测试共用）

- 优先 AgentDock-PC 上的 Ryzen 9 7940HX / RTX 4070 Laptop。CPU/GPU、供电计划、温度、系统负载、显卡驱动、Windows HDR、输出色深、刷新率、VS/字幕和解码模式都写入 JSON。硬件解码必须显式显示 D3D11VA，软件解码必须显式显示 CPU，不能依靠肉眼判断。
- 固定 3840×2160 输出测试 4K/8K 源；另对 8K 原生输出单独成组。变更窗口尺寸即作废该轮。
- 同素材取三段：简单稳定段、压缩复杂尖峰、字幕密集段；先热缓存重复三轮，再单独冷启动。峰段不可用视频总平均码率代替分秒码率。
- 每轮启动 seek 后至少预热 5 秒；稳态至少 60 秒，HEVC 尖峰样本优先覆盖完整约 94 秒；8K48 困难样本至少覆盖 60 秒并单独记录失败。
- 记录帧级帧龄/PTS，在帧率场景中核对重复帧、丢帧、合并帧、真正上屏、音频欠载、媒体时间/真实时间比例；`presentedVideoFrames` 仅代表接受帧，`swapChainPresents` 包括 OSD 重绘，**不能把二者任一个单独当作无丢帧证明**。
- 三轮结果报告中位数及最差值；改善未超过测量噪声时写“未证实收益”。优先同一 DLL 多次使用实验 gate 的 interleaved A/B，减少环境漂移。

### 5.2 测试用例

| ID | 素材与场景 | 对照变量 | 主指标及通过条件 |
| --- | --- | --- | --- |
| T01 | `D:/RebellionHEVC444p10-700MBPS-SoftwareLast.mkv`，4K24 / 约 812 Mbps 峰段 | 基线 vs HEVC 异步/交接槽 1/2/3；线程数保持一致 | 完整帧时序、无新增死锁；峰值丢帧 ≤ 基线且 p99 阻塞下降；若原基线已零丢帧，以等待时延改善为目标 |
| T02 | `D:/Video Compress Test/PlayerTest/ChouKaguya432048-Part.mkv`，8K420p10 47.95fps 软件解码→4K | baseline / B / C / B+C（最终才测交互） | decoded/accepted/displayed、drop、underrun、clock；量化当前约 31fps 不足。48fps 达标需稳定贴近源帧率且零异常，而不是单测秒数显示 0 drops |
| T03 | `D:/Video Compress Test/PlayerTest/ChouKaguya432048-NoAudio.mkv`，8K SDR 无音轨 | A/B / vs clock-only | 无音轨 QPC 时钟按墙钟推进、seek 正常、EOF 不提前；Jinc SDR 灰阶无明显偏暗 |
| T04 | `D:/Video Compress Test/Large-Test/1080p-2D.mkv`，标签 unspecified | Baseline / new upload/presenter | 不自动标 PQ/HLG，不出现 gamma/色域变化；1080p 性能无退化 |
| T05 | 实测存在时的 4K444p12 AV1 + FLAC + ASS，1440p Anime CNN/CNN+ | 当前已通过链路 / 任一 P0 调整 | 30s 连续窗口源 47.95fps、零额外 VS 跳过/渲染丢帧/合并，输出持续 444 16-bit |
| T06 | 4K/8K D3D11VA 硬解 420 8/10bit → 4K | 基线 / 纹理池/帧时延变更 | 60s 零新增 dropped/coalesced/underrun；不得增加 CPU 回传；显存不能无必要增长 |
| T07 | SDR 709 full/limited、2020 CL/NCL、PQ/HLG、HDR→SDR、Jinc+LUT | Baseline / B / C / 色彩引擎 | GPU 读回数值偏差在现有回归容差内，scRGB 203 nit 白位、t3/t4/t5 资源不冲突，浮点超白不被截断 |
| T08 | seek × 20、暂停/恢复 × 20、换轨 × 10、EOF/连续播放、设备重建 | B1/B2 | 没有死锁/旧代际帧、不访问释放的 AVFrame；在场线程退出、内存峰值与稳态资源正常 |
| T09 | 单 CCD/跨 CCD/默认，4/8/16/24/32 解码工作者 | A0/A1/A2/A3 | 以 p95/p99 与 60s 被呈现帧、音频欠载判断；不能只看 CPU 占用/频率 |
| T10 | 负载 0%、50%、90%，窗口/全屏，后台字幕和音频 | Normal/MMCSS/High | 前台可操作，系统调度不倒退，p99 音画延迟不变坏 |
| T11 | 同一显示器 VSync/VRR（如支持），60Hz/144Hz（如支持） | 3-buffer current / waitable prototype | 比较 actual display cadence、Present wait、尾延迟，不以 Present 被丢弃数量冒充视频解码丢帧 |
| T12 | mpv/MPC/PotPlayer 同素材同配置的外部参照 | 只做实际可比条件 | 固定软件解码、D3D11 输出、缩放、HDR/ICC、字幕、刷新率；记录各播放器版本和统计定义，不混合默认画质链比较 |

**样本路径仅来自项目既有记录；开始跑前必须用 `Test-Path` 与 ffprobe 检查存在性、时长、`pix_fmt`、fps 和色彩标签。缺失素材标记未执行，不能替换一个相似文件后沿用旧结论。**

### 5.3 功能回归（现有 + 拟补充）

现有 `tests/TestPlayer.cpp` 中可复用：
- `softwareCodecPipeline`：HEVC/AV1 CPU 解码，暂停、seek、音频/EOF。
- `vsPrefetchSurvivesShortSchedulingDelay`：VS 短暂延迟后的顺序追赶/大延迟重同步。
- `animeCadenceMeasurement`：通过 `VSR_ANIME_PERF_SOURCE`、`VSR_PERF_STAGE` 等环境变量记录真实 GUI 出帧。
- `jincColorMatchesNative`：Jinc/原生色彩一致性、SDR/PQ/HLG 和 16-bit 444。
- `colorSettingsPreserveVsChain` 与 `generatedColorMatrixUsesVsProps`：VS 色彩元数据与 VPY 行为。

后续实施时**新增而不是声称已存在**的测试建议：
- `softwareHandoffQueueBoundedAndOrdered`：输入 1/2/3 槽，高负载+seek generation，多帧交接有限且无乱序。
- `hevcAsyncDecodeShutdownAndFlush`：HEVC 线程阻塞时执行 seek、切轨、关闭、设备重建，确保不死锁。
- `uploadRingPreserves444HighBitDepth`：16-bit 容器中的 10/12-bit 数据与色度平面对照，宽度、stride、行对齐和 R16_UNORM 一致。
- `colorLutJincResourcesIsolated`：动态 HDR t3、颜色 LUT t4、Jinc t5 的实际资源绑定与像素回归。
- `presentModeQueueDepthUnderLoad`：DXGI 深度与真实上屏节奏对照。
- `threadAffinityNeverStarvesAudio`：CPU Sets/优先级在背景压力下不会抢占音频关键处理。

执行测试命令应以实际构建 preset 的 ctest 发现结果为准（先 `ctest --preset windows-mingw-release -N`），不要假定测试 exe 命名和工作目录。该机器本轮 PATH 没检测到 `cmake` 命令，因此这里不声称已构建/运行以上新增测试。

### 5.4 性能命令模板（仅示例，实施前检查路径与产物）

```powershell
Set-Location "C:\Users\ARXChem\Documents\Open Source Projects\VS-Renderer-GUI"
$video = "D:\RebellionHEVC444p10-700MBPS-SoftwareLast.mkv"
$dll = "dist\VS-GUI-Full\FFF.Native.dll"
$probe = "build\perf-player\perf-3fp.exe"
if (!(Test-Path $video) -or !(Test-Path $dll) -or !(Test-Path $probe)) {
    throw "Required video, DLL or probe is unavailable; do not treat as a measured run."
}
# CPU=1；推荐使用当前 Native D3D11 算法 519（7 + 双线性色度 2<<8），
# 强制输出 3840x2160，测量 93 秒。注意旧探针计量从首帧开始；
# 真正 5 秒预热/峰段/帧级埋点需在下一轮实现或分析时显式剔除。
$env:VSR_3FP_DECODE_MODE = "1"
& $probe (Resolve-Path $dll).Path $video 3840 2160 93 519
```

该模板**不是已经运行的测试结果**。候选 DLL 请输出到独立 `build/perf-kernel-20261017/candidate/` 目录，避免直接覆盖正在使用的 Full/Lite DLL；旧 `perf-3fp` 探针不自动启用 GUI 自适应线程配置（其 `FFF3FPConfiguration` 默认为 0），因此 **GUI 与独立 Native 探针的线程策略必须显式核对**。不能把它们的 fps 差算成播放器额外开销，除非配置确实相同。

## 6. 成功标准、自动回退与最终提交

### 6.1 必须通过（硬门槛）

- 原有软件、硬件、无音轨、VS、字幕、HDR/SDR、Jinc 色彩测试全部保持通过。全量回归若有历史既存失败，要出具“基线也失败”的复核证据，不能修改无关测试期待值掩盖问题。
- 关键样本无画面损坏、丢失精度、旧帧回流；连续 seek / 设备恢复无死锁；音频欠载不增加。
- 原本稳定 4K48 和 8K 硬解路径的最终显示速率、p99 与内存不明显恶化。
- 新路径有**运行时关闭开关或可替换旧 DLL**；没有回退能力禁止覆盖 Full。
- 每次改动只触及该阶段必要代码、对应测试、补丁和文档；不要借机会清理工作区旧文件或重构无关 UI。

### 6.2 优化达成（软门槛，不以此降低正确性）

- P0-B 若提升 8K 提交速率，至少应跨三轮稳定重现，p95/p99 单槽等待下降且音频欠载不增加；最佳目标是向 47.95fps 靠近，不强制以预设的“+50%”宣传。
- P0-C 以同画质的实际上屏吞吐和 GPU/CPU 同步等待为收益指标；仅毫秒级单次 Update 优化但帧吞吐不变应报告“局部优化、无端到端收益”。
- P1 CPU/优先级实验在不同负载、冷/热运行中一致有利才考虑默认开启；未证实收益则默认关闭。

### 6.3 每阶段的提交/回滚记录

每次提供：`baseline-sha256`、`candidate-sha256`、`ffmpeg-module-hashes`、改动文件列表与 diff、环境参数、三个以上原始 CSV、summary JSON、逐帧异常、回归日志、未通过项、最终是否部署。分支/补丁与 `tools/build-native-api20.ps1` 应支持从固定上游重建；禁止把仅存在 `build/native-api20` 的临时差异当作可发布修复。

## 7. 推荐实施顺序（可直接按步骤交给开发 AI）

1. **第 0 轮：测量基线。** 验证当前 Full/Lite/Release DLL、FFmpeg 和媒体参数，跑 T01/T02/T05/T06，输出分阶段等待与完整链 fps；成功条件是能指出主要时间耗在何处。不可修改色彩链。
2. **第 1 轮：单槽 → 2 槽可选环。** 仅改 libdav1d 的帧交接；T02/T03/T08 及旧软件回归通过才比较 3 槽；测内存及 seek。无提升就撤销。
3. **第 2 轮：HEVC 异步解码。** 保持同样线程数与预算，只扩展可安全异步的解码入口；测试 T01/T08；稳定后才允许与环形交接合并。
4. **第 3 轮：GPU 上传资源轮转。** 先收集 Update 等待与 GPU 时间；证实为瓶颈后单独实现 2 组 DEFAULT Texture，必要时测 3 组，跑 T02/T04/T06/T07；不触碰硬解 VideoProcessor 直通。
5. **第 4 轮：CPU Sets / MMCSS 实验。** 默认关闭，先调查 7940HX 实际 CCD/core 映射和 Windows 线程归属；再运行 T09/T10，不把不可控的 FFmpeg 内部线程当成已绑定。
6. **第 5 轮：DXGI 等待对象与高级色彩开销。** 只在 P0 后端到端性能仍由 Present/Copy 受限时执行 T07/T11，保留当前三缓冲方案为 A0。
7. **收尾：** 先做至少三次固定窗口对照、完整回归及部署一致性检查；写明每个最优参数的适用范围，禁止把 4K 通过扩大为 8K、把 SDR 准确扩大为全 HDR 面板色准。

## 8. 参考资料（项目内，可核验）

- `docs/hevc444-peak-diagnosis-1.0.6.md`：高码率 HEVC 峰段、8→16 线程与缓存对照。
- `docs/software-decode-subtitles-1.0.7.md`：512 MiB / 32 线程 / 独立软件解码交接和停顿试验。
- `docs/update-1.0.11.md`：8K48 软件解码未通过、AAC/ASS 成本、CPU/GPU 负载对照。
- `docs/player-anime-performance-1.0.12.md`：4K444p12 Anime 成功/失败边界。
- `docs/player-color-vs-1.0.12.md`：Jinc/LUT 资源槽冲突及 SDR/PQ/HLG 回归。
- `docs/color-management-1.0.4.md`：libplacebo、ICC、HDR 元数据、scRGB 203/80nit。
- `docs/nuxbox-performance-session-2026-10-04.md`：D3D11VA/VideoProcessor 及三缓冲在 780M 的历史测试。
- `src/backend/ThreeFpPlayer.cpp`、`src/backend/VapourSynthFrameServer.cpp`、`src/color/ColorBridge.c`。
- `build/native-api20/FFF.Native/3FP/Core/PlayerSession.cpp`、`Render/VideoRenderer.cpp`、`Render/NativeColorEngine.h`。
- `tools/perf-3fp.cpp`、`tools/perf-system.ps1`、`tests/TestPlayer.cpp`。

````````
<!-- end:docs/player-kernel-d3d11-performance-plan-2026-10-17.md -->

<a id="record-docs-player-kernel-performance-1-0-12-md"></a>

## docs/player-kernel-performance-1.0.12.md

原始字节数：7501；SHA-256：`83aa01d1f0795c5ab88cea4cd11dba8b30021ae1a3cd3408e61b9236f62aff6e`。

<!-- begin:docs/player-kernel-performance-1.0.12.md -->
````````markdown
# 1.0.12 软件播放内核与界面续修

2026-10-17；7940HX / RTX 4070 Laptop，Windows HDR 关闭，屏幕 2560×1600 / 165 Hz。按用户最终要求，暂停本机 8K48 的 45 fps 目标，以保留约 40 fps 的改善为本轮边界。没有执行设计文档中的测试矩阵。

## 已整合实现

- 8K420p10 AV1 默认 libdav1d 8 个工作线程、8 个 frame contexts；4K444p12 保留原有 32 contexts 策略。
- 8K 软件帧交接默认双槽，AVFrame 持有引用、有界排队；新增槽从原显示队列预算中扣除。关闭／seek／换轨先停止并 join 解码工作线程，再清空帧与 codec；FFmpeg send/receive/flush 继续串行化。
- HEVC 软件解码加入原有独立解码工作线程，主播放线程继续推进音频与字幕；硬解路径不变，通用自适应线程阶梯保留。
- 4K 及以上软件平面上传使用两组 DEFAULT 纹理和两组 CPU-write STAGING 纹理，逐行按真实 stride 拷贝，CopyResource 后轮转。不改变 R8/R16、位深、色度布局、颜色 shader 或硬解共享纹理路径。
- 实际查询 L3／物理核／进程允许 CPU 掩码。8K 的 8 个工作线程默认采用跨双 CCD 的软 ideal-processor 提示；异构 CPU 优先高 EfficiencyClass 物理核。只有 codec 打开前后新增线程数准确匹配工作者数量才应用；拓扑不匹配或多 processor groups 时交还 Windows。14700K P/E 分支没有在本机实测。
- MMCSS 为播放、呈现和 WASAPI 线程提供有生命周期的注册／恢复；decoder workers 不提升为实时线程。DXGI 等待对象使用三缓冲、最大延迟 2，等待时不持有解码／设备锁，复制句柄避免交换链重建释放竞态；普通交换链保留原方式。二者保留为默认关闭的实验开关。
- SDR／HDR→SDR／HDR 原生继续原有色彩路径及资源缓存，保留 t3 HDR、t4 颜色 LUT、t5 Jinc 槽位隔离。没有通过关闭色调映射或丢弃色度来换性能。

强制绑核、非临时缓存 SIMD memcpy、额外并行行拷贝、扩大纹理池等候选未证实端到端收益，强绑核曾降速；没有强制启用这些负收益组合，也没有提升整个进程到 High／Realtime。

## 统计与 UI

`presentedVideoFrames` 已经只计算实际提交至 Present 的新代际，原信息面板又减 `coalescedVideoFrames`，造成实时帧率缺口错误。现直接采样呈现计数；计数回退后重置采样。实际丢帧仍分别显示 VS 跳过、渲染丢弃、合并，不把单纯的解码不足／慢放伪造为丢帧。慢解码可同时出现低 fps、少量丢帧与音频欠载，必须结合时钟与欠载判断。

原始视频高度 >2160 时禁止启动内置自动 Anime 和四档 Anime4K。2160p 仍允许，Jinc／D3D11 仍可用；自定义 VPY 可用于专门测试。在打开媒体元数据完成前暂不启动内置 Anime，避免 8K 先开始索引再回退。

底部 A4KCNN／Jinc／AV1／Opus 等标签和 3FP 解码按钮共用背景样式。主题设置新增底部不透明度及右键／倍速菜单不透明度，新配置默认均为 100% 不透明；已有显式底栏透明度保留，旧 INI 的 `bottomTransparency` 仍是 0=不透明，设置界面换算为不透明度。右侧播放列表原有独立设置保留。

## 实测范围

使用完整 Qt 播放器，全屏物理 surface 2561×1601（DPI 取整），16:9 内容约 2560×1440；CPU 软解、D3D11 预设，音频和字幕保持启用。先定位 10 秒、预热 5 秒，再采样。CSV 中 `accepted` 列是 Native 的新帧呈现计数；`presents` 含字幕／OSD 重绘，不能作为视频 fps。

| 素材 | 窗口 | 呈现 fps | 新增丢帧／合并／音频欠载 |
| --- | --- | --- | --- |
| OP-yuv444p12-SWTest.mkv | 30.000 s | 47.967 | 0 / 0 / 0 |
| RebellionHEVC444p10-700MBPS-SoftwareLast.mkv | 60.004 s | 23.982 | 0 / 0 / 0 |

两项各执行一次本轮指定性能测试，开启 MMCSS／DXGI 等待对象开关以覆盖集成路径，均无测试失败。Y412 开始采样时已有 2 个启动期丢帧，窗口内没有新增。Y412 初始 32 工作者，HEVC 初始 16，既有通用阶梯在启动阶段曾将 HEVC 提升到 24；不能把该轮写成始终固定 16 线程。

此前同机 8K ChouKaguya432048-Part.mkv 的全屏对照：16/16 为 34.00 fps；8/16 为 40.58 fps；8/8 + 双槽 + 软拓扑提示 + 上传暂存组合为 40.53 fps。后者仍有 87 次音频欠载、1 个实际丢帧，属于非实时播放，不能视为 8K48 通过。与旧记录 4K 输出 31.03 fps 的环境不同，不将其直接换算为严格的 33% A/B 提升。早期误用 Jinc 的试验不列为 D3D11 对照；本轮按用户要求没有继续追逐 45 fps。

性能采样后最终代码仅追加交接槽容量配置的互斥保护，防止动态重开 codec 时和后台工作线程并发访问；随后 HEVC／AV1 生命周期回归 4 passed / 0 failed。没有重复执行两项性能采样，采样 DLL 与最终 DLL 哈希分别留存。

原始 CSV、所选回归日志及 Native／avcodec 哈希见 [证据目录](evidence/player-kernel-1.0.12/summary.json)。这些短窗口不能外推为所有内容的长时间零丢帧保证。

## 回归与构建

- 实际 burst 提交产生合并帧，验证面板直接使用呈现计数并正确处理计数重置；透明度设置和内置 Anime 限制通过。
- 生成 HEVC／AV1 软件解码的暂停、前后 seek、帧定位、EOF、关闭再打开通过，含 HEVC 异步路径。
- 4K 8／10／16-bit 平面、SDR／PQ 上传／缩放与旧 Native 的 GPU 像素探测最大 RGB 差为 0；Jinc 的 SDR／PQ／HLG 色彩回归通过。未测显示器绝对色准。
- 聚焦回归 9 passed / 0 failed / 3 skipped：两个需要单独素材环境变量的附加用例未执行，真实桌面绿色视频合成核对因前台占用跳过；窗口 alpha／全屏布局断言已执行。主题设置截图已检查可读性。
- 语言完整性脚本仍报告 9 个旧截图相关字符串缺失；以 HEAD 的代码和语言包重跑得到相同 9 项。本轮新增字符串两种语言均补齐。
- 最终默认设置回归 6 passed / 0 failed；Lite 6 passed / 0 failed；BD 目录回归 8 passed / 0 failed / 3 skipped（实盘环境变量未设置）。
- Native 完整补丁对固定上游 `ce195ebd06e9cbb8352a443f0c874cf569a60733` 的 `git apply --check` 成功，独立 checkout 使用构建脚本成功编译。补齐脚本缺少的两份色彩桥头文件拷贝，不依赖本机 ignored 目录的临时修改。

## 运行开关

设置环境变量后重启播放器；正常使用不必设置。仅用于排查或专门测试。

| 变量 | 默认 | 含义 |
| --- | --- | --- |
| FFF_DAV1D_WORKERS / FFF_DAV1D_DELAY | 按素材策略，8K420p10 为 8/8 | 1–64 调试覆盖值 |
| VSR_SW_HANDOFF | 8K 软件为 2，其余 1 | 8K 交接槽 1–3 |
| VSR_SW_UPLOAD_PIPELINE | 1 | 0 回退原 UpdateSubresource 上传 |
| VSR_SW_CCD | 2 | 0 交还 OS；2 软提示；1 强绑解码池与宿主分区（实测更慢，排查用） |
| VSR_MMCSS | 0 | 1 注册关键线程 MMCSS，失败保持普通调度 |
| VSR_DXGI_WAITABLE | 0 | 1 尝试等待对象交换链；不支持创建则回退普通交换链 |

单文件继续为 Lite 启动器，FFmpeg shared x64 组件从 EXE 同目录或 PATH 加载，配置留在启动器旁。代码上传 GitHub，1.0.12 Release 只创建草稿，包由用户上传。

````````
<!-- end:docs/player-kernel-performance-1.0.12.md -->

<a id="record-docs-player-manual-presets-1-0-3-md"></a>

## docs/player-manual-presets-1.0.3.md

原始字节数：2335；SHA-256：`a8cd26dc78e349ffbb35f1bfd0a9e94423e53ce84ee36266eedfbf085c142c2b`。

<!-- begin:docs/player-manual-presets-1.0.3.md -->
````````markdown
# VS Player 设置、图片关联与手动 Anime（1.0.3）

## 使用方式

- 设置窗口左下角：加载预设、保存预设。这两个按钮加载/保存完整 INI 配置。
- 右下角从左到右：取消(N)、确定(Y)、应用(A)，支持 Alt+N / Alt+Y / Alt+A；确定保存并关闭，应用保留窗口与当前播放/暂停状态。
- 文件关联加入常用图片：jpg/jpeg/jpe/jfif、png、webp、avif、bmp、gif、tif/tiff、heic/heif、jxl；新增“全选图片”。该按钮只选图片，“全选所有”包含视频、音频和图片。实际可解码格式取决于随附图片解码器；关联不改变解码器能力。
- 右键 → VapourSynth 预设 → 开发者内置：保留默认“Anime · 自动切换”和 Realistic，并新增六个“手动”版本：CNN + 额外增强、CNN、no CNN + 额外增强、no CNN、Jinc 直通、D3D11 原生直通。
- 手动版本固定档位：不会因丢帧或源达到 3840×2160 而自动降档；前四档仍按播放器视口目标处理（目标最大 3840×2160）。Jinc / D3D11 两个直通版本在播放器内由原生路径实现，固定使用对应缩放算法。
- Tab 信息区显示手动固定档位；设置中的 Anime 起始档位仅影响自动版。
- 六个手动 VPY 首次启动时从已有 Anime.vpy 生成，已存在的文件不覆盖；原有 INI 和用户 VPY 保留。

## 验证

- Release 构建 VSPlayer 与 vsr_player_tests 成功。
- 相关专项 17/17（含初始化、清理），没有运行无关回归：设置底部实际几何位置/助记键/中英切换、图片组选择及临时注册表键注册/取消、六个手动版本打开和寻帧、自动版六档兼容、Apply 保留暂停/播放与时间。
- 手动 CNN 测试注入 80ms 帧处理延迟，播放 9.5 秒出现明显丢帧，档位仍为 0；临时修改的测试 VPY 已恢复。
- 界面截图：build/player-settings-footer-images.png；测试记录：build/player-manual-tests.txt；构建记录：build/player-manual-build.txt。
- 测试没有修改 Windows 实际默认应用；注册测试使用临时 HKCU 测试子键并清理。
- 本轮仅更新本地便携 vs-player.exe、语言文件、新增手动 VPY 及文档，保留原有配置/VPY、Renderer、FFF.Native.dll、LAV。未打包、推送或发布。

````````
<!-- end:docs/player-manual-presets-1.0.3.md -->

<a id="record-docs-player-overlay-layout-1-0-8-md"></a>

## docs/player-overlay-layout-1.0.8.md

原始字节数：3732；SHA-256：`4a66340a082e515eb7c52021ffcd25ebfd3581824cb30d4ef742b67a4f7a601f`。

<!-- begin:docs/player-overlay-layout-1.0.8.md -->
````````markdown
# Player 浮层与窗口移动修复 (1.0.8)

日期：2026-10-06。仅本地源码、构建及运行目录更新；不推送、不打包、不修改在线版本、发行草稿或附件。保留维护者手动更新的 FFmpeg。

## 布局与交互

- 全屏顶部标题出现时，左侧 Tab 与右侧音频详细面板按标题实际高度下移，并保留 12 px 间距；标题隐藏后恢复位置。面板最大尺寸限制在视频区域内，并为底栏实际高度留位；内容较多时使用滚动区域。
- 透明度滑条、标签与下拉框按实际字体及控件尺寸布局，文字区和底部透明度栏增加约面板高度 5% 的间距。音频图与文字区都避开透明度栏；调整面板大小、切换全屏或缩小窗口时重新计算可用空间。
- 音频详细面板可见时，禁止鼠标移到右侧自动弹出播放列表；显式固定的播放列表保留，Tab 面板显示在其上方。
- 底栏与其它浮层在窗口移动事件中立即同步位置，不再等待 100 ms 周期刷新。移动事件只更新界面几何，不重新绘制音频电平数据，不操作解码器、字幕或 VS 脚本。
- 下拉框及弹出菜单处理自身键盘事件，不再被播放器的 Enter / 方向键 / Space 快捷键抢走。

这些浮层继续使用独立透明窗口覆盖原生视频，顶部标题和面板位移均不改变视频视口。真正的窗口尺寸变化或全屏切换仍会调整视频输出尺寸。

## 验证

`build/overlay-1.0.8/acceptance-final.txt`：**10 passed, 0 failed, 0 skipped**。包含原生播放和真实 VS 输出、Tab 调整、音频输入 / 输出电平、透明控制栏、全屏唤出、稳定视口、播放列表和文件夹列表。

新增真实 640×360 / 24 fps 音视频用例，分别运行原生和 VS 路径：

- 自动化实际点击左 / 右透明度滑条，以 `QApplication::widgetAt` 核对可点击的目标，鼠标移到右侧不会弹出播放列表。
- 停止原有浮层周期定时器后连续移动窗口 40 次；每次直接核对底栏全局位置，未等待下一次定时刷新。
- 移动期间视频仍持续呈现，视口尺寸、时间线 generation 不变，没有发出视频重绘 / 视图变换或脚本重载信号，渲染丢弃与音频欠载计数不增加。
- 多次唤出 / 隐藏全屏标题，核对两个 Tab 面板让位及恢复，视频视口保持不变；实际打开音频信息下拉框并按 Enter 选择，保持全屏状态。
- 暂停后反复切换顶部浮层，呈现计数和 float32 读回画面逐像素不变。

首轮测试发现下拉框的 Enter 被全局全屏快捷键截获，已修复，最终上述两条播放路径均通过。语言完整性 536 条通过，编译及 `git diff --check` 通过。面板图保存在同目录 `tab-panel.png` 与 `audio-panel.png`。

部署记录位于 `build/overlay-1.0.8/deployment-audit.json`，核对运行目录的程序版本、用户配置 / VPY / 色彩资源及 FFmpeg / FFprobe / 原生 DLL 哈希。

本地运行目录版本已更新为 1.0.8；55 个受保护文件前后哈希一致。FFmpeg 为 `d546b1cb13afab087e3e245773da230f52996f47651c32e07432cf9bc88405d2`，FFprobe 为 `cbe470e4b058d1dfe7a6cbbcc5fa230e8f434e74e59fd6c30d671bf362bdc110`，原生 DLL 为 `93510aecd9ffddd0d3de3201795043c6fddef2896d16505fe0bad5e60c817cf3`，均保留本轮开工时的内容。新版 Player 为 `94a99931538b66826460a8514160fd1e07a77e2a6c538da1fb69b970dd01eae4`。

隔离目录启动检查移除开发环境 PATH，Renderer / Player 均创建主窗口并以退出码 0 正常关闭，记录为 `startup-smoke.json`。既有播放器进程未停止，当前播放保持运行；关闭后重新启动即可使用新版。

````````
<!-- end:docs/player-overlay-layout-1.0.8.md -->

<a id="record-docs-player-performance-1-0-2-md"></a>

## docs/player-performance-1.0.2.md

原始字节数：22399；SHA-256：`7dc5dc9179700819201d05cae7168b71cdc4013133db25d7768015cd82da2b3a`。

<!-- begin:docs/player-performance-1.0.2.md -->
````````markdown
# 本轮：网络固定直通、缓存与 1080p48 性能（2026-10-01）

当前网络策略替换下面历史记录中的整文件 VS 准备：HTTP / HTTPS 始终 3FPlayer 原生直通，不执行 VPY，也不建立 VS 索引。保留重定向 / AList API / 有界 Range 代理和 Jinc 等显示缩放。网络未使用整文件缓存；首次打开速度仍受远端请求、封装头位置与线路影响。

设置扩展到基本、主题、播放、性能、解码、渲染、缓存、文件关联八页。中文默认、外置中英文语言包，新增字体 / 背景 / 透明度、自动播放、视频 / 音频位置记忆、源多线程、快捷键跨度。LAV 视频 / 音频独立选择及属性窗口，VS 源仍由 VPY 定义；网络和内置原生视频固定 3FP，倍速音频使用 atempo。音频文件不会误走已选 Anime 的视频 VS 链。

寻帧索引默认 `cache/indexes`，可自定义。身份包含绝对路径、大小、修改时间；只清除指定目录的 ffindex / lwi，不删除视频或其他文件。FFMS2 / L-SMASH 实际复用和媒体旁无新索引的回归已执行。下面的首次建立 / 已有索引打开是同一源、同进程相邻两次测量，含文件系统暖缓存效应，不代表整套播放器启动时间。[FFMS2 参数](https://github.com/FFMS/ffms2/blob/master/doc/ffms2-vapoursynth.md)、[L-SMASH 参数](https://github.com/HomeOfAviSynthPlusEvolution/L-SMASH-Works/blob/master/VapourSynth/README.md)

用户当前 **02.mkv** 实际为 1920×1080、47.952 fps、YUV420P10；不是前次实际 4K 的 03 文件。未改用户电源 / 功耗方案，无并行本轮编码 / GPU 测试。VS 8 工作线程、FFMS2 4 解码线程、96 帧、4 路异步请求暖态结果：

| 链路 | 输出 | 吞吐 | 均摊耗时 |
| --- | --- | ---: | ---: |
| AV1 软件源 | 1920×1080 | 99.56 fps | 10.04 ms |
| AV1 + 10→16 bit | 1920×1080 | 97.19 fps | 10.29 ms |
| A/M Fast | 1280×720 | 72.82 fps | 13.73 ms |
| A/M Fast | 1920×1080 | 61.45 fps | 16.27 ms |
| A/M Fast | 2560×1440 | 48.71 fps | 20.53 ms |
| GPU Sharpen edges + Enhance detail 0.3 + A/M Fast | 2560×1440 | 35.79 fps | 27.94 ms |
| VS Jinc + CPU 可读输出 | 2560×1440 | 54.40 fps | 18.38 ms |

首次 FFMS2 索引建立 **2.091 秒**，已有索引打开 **0.054 秒**。证据：`build/player-1440-performance-final.txt`、`build/player-1440-performance.json`，脚本 `build/profile-player-1440.py`。早一遍没有完成增强项，Fast 1440p 为 43.57 fps，说明暖缓存 / 运行状态也有影响；最终完整采集如上。短样本不证明长片持续帧率。

窗口与全屏差异有两个可定位来源：

1. 当前官方 A/M Fast 中 x2 M CNN 的 `WHEN` 要求目标宽高都超过源的 **1.2×**。1080p→1440p 为 1.333×，启用内部 2× CNN 后再适配目标；小窗口通常不触发这一步。处理负担并非只按最后 1440p 像素增加。
2. vs-placebo Shader 要返回 VS 可读取的 CPU 帧，并输出 YUV444P16；1440p 三个 16-bit plane 约 **21.1 MiB / 帧**。现有桥还复制 / 上传给 D3D11，隐藏 3FP 时钟也解码视频。上表不含这些呈现 / 音频 / 同步成本。47.95 fps 预算约 20.85 ms，而 Fast 单链已约 20.53 ms，几乎没有余量；叠加增强则持续不够。不是简单的“没有启用 GPU”。官方展示的 5 ms 使用不同设备 / 配置 / mpv GPU 渲染路径，不能等同本播放器整套链路。[官方 Anime4K](https://github.com/bloc97/Anime4K)、[vs-placebo](https://github.com/Lypheo/vs-placebo)

后续若要让该链在同功耗下稳定达 48 fps，主要方向是 VS / D3D11 GPU 纹理互操作、减少读回 / 重复源解码，或选更轻网络；不能用预解码掩盖长期吞吐不足。本轮保持已选 Fast 算法，没有擅自更换另一网络。

Anime 降载顺序为增强 → A/Fast → Jinc 原生直通 → D3D11 Native；Realistic 和 4K 源从 Jinc 直通开始。稳定播放恢复 **2 秒**，再观测 **5 秒**、至少 48 个呈现或丢弃帧，丢帧超过 5% 才降一级。暂停 / 恢复、seek、改速、resize、全屏、画布变化重新开始，操作停顿不用于切换。

最低档支持的 SDR / NV12 或 P010 硬解输入优先 VideoProcessor；HDR、软件源、交互放大等不适用时用轻量 Bilinear，保留原有色调映射 / 颜色与平移。已测实际 AV1 硬件路径 `videoScalingMode=1`，暂停 seek 后截取有色彩图像；不宣称所有输入都直接走 VideoProcessor，也未在老显卡 / HDR 显示器人工验证。补丁 `patches/3fp-native-scaling.patch` 叠加在现有网络字幕补丁之后。

Anti-ringing 当前实现**仅 Jinc**，放大 / 缩小都使用强度 0.5 的局部范围约束；Nearest / Bilinear 等其他菜单项不受该开关控制。其它振铃算法理论上可另写限幅，但本程序没有把该开关应用到它们，不宣称复刻 madVR 专有实现。

10-bit Resample 修复：在内置 Jinc 调用前转换成插件支持的 16-bit integer。只精确匹配带内置标记的已知旧调用，先保留 `.before-bitdepth-fix`，备份失败则不写回；其他用户 VPY 不改。新文件直接包含转换。[Resample 位深约束](https://github.com/Lypheo/vs-placebo)

文件关联只注册 HKCU 的候选应用与选定格式，默认应用交给 Windows 界面；测试用独立临时注册表分支，不在用户真实默认关联中写入。[Windows 默认应用](https://learn.microsoft.com/en-us/windows/win32/shell/default-programs)

语言包为 `assets/languages/zh_CN.json` / `en_US.json`，便携目录 `languages`；后续新增中文界面字符串同步两包，运行 `tools/check-player-languages.py`（也已加入 CTest）。所有播放器配置包含在一个 `player.ini`；视频 / 音频记忆位置按媒体时间存储，重新加载 VS 倍帧脚本按输出帧率换算，不把旧帧号套给新帧率。组件自己的 LAV / madVR 属性仍由各组件保存。


网上“Fast 5 ms”不能直接映射到本程序所用网络：上游早期 v0.9 明确采用非机器学习的梯度方法，而当前 v4.x A/M Fast 含 CNN。用户图片未给出具体版本、测试方法与完整播放成本，不能确认它与当前着色器相同；此前把 Fast 数字与 M 网络直接关联的解释应撤回。这里保持当前 v4.x A/M，并未擅自替换旧算法。[早期官方说明](https://github.com/bloc97/Anime4K/blob/v0.9/Preprint.md)、[当前官方模式](https://github.com/bloc97/Anime4K/blob/master/md/GLSL_Instructions_Advanced.md)

本轮最终验证：Release 与匹配原生 DLL 成功；完整 CTest **7/7**（126.82 秒），构建目录 Player 25 通过 / 1 个显式真实网络测试跳过；交付目录启用真实网络测试后 **26/26、无跳过**（137.97 秒），包括用户 ModelScope / AList 原生播放、故障 VPY 网络绕过、索引复用 / 清理、10-bit 先缩放、八页中英 / INI、独立音频、关联注册分支、视频 / 音频记忆与自动播放、RIFE 帧率恢复、resize / 暂停防误降级、D3D11 有色画面截图。

程序 / 用户文件验证：两正式 EXE 与原生 DLL 与构建一致；GPU helper / MVTools 色度修复仍内嵌；普通用户与旧其他内置 VPY 保留，Anime / Realistic 仅位深修复且原文件备份完整，原 `player.ini` 保留。外置语言包、README / project / changelog 同步到交付目录。移出测试 EXE / Qt6Test.dll 后，两正式程序独立启动并正常关闭；证据 `build/player-settings-delivery-audit.json`、`build/player-settings-startup-smoke.json`。

日志：`build/player-settings-final-ctest.txt`、`build/player-settings-portable-results.txt`、`build/player-settings-native-http.txt`、`build/player-settings-audio-verified.txt`。没有推送 Git、没有打压缩包，版本仍为 1.0.2。QtTest / 截图验证与人工长片播放区分；老显卡、HDR 显示器、长期稳定性仍未人工验证。

## 前一轮历史记录（其网络 VS 准备策略已替换）


# 本轮网络与自适应预设复测（2026-10-01，1.0.2）

用户确认本机使用标准性能功耗方案，本次不调整电源 / CPU / GPU 功耗。以下测试顺序执行，无并行视频编码或其他本轮 GPU 测试；结果受温度、缓存、后台程序影响，不保证长片。

真实 AV1 文件为 3840×2160、47.95 fps、yuv420p10le；文件名虽包含 1080p48F，但以实际探测为准。原 VS 路径包含源软件解码、滤镜 GPU 上传 / 读回、VS plane 拷贝，以及隐藏 3FP 时钟的重复视频解码。新 Anime / Realistic 的 4K 源直接用 3FP D3D11 硬件解码、Jinc 呈现，不走 VS 滤镜或读回。8 秒样本呈现 383 帧，丢弃与合并均 0，暂停后呈现计数不再增加。证据：`build/player-network-profile-targeted.txt`。这不能证明任意自定义 4K VPY 已具备 48 fps；自定义脚本仍按原处理链运行。

MyGO 1080p23.976 的 96 帧、4 路请求暖态配对（均 VS 输出 YUV444P16）：

| 处理方案 | 目标尺寸 | 吞吐 | 均摊耗时 |
| --- | --- | ---: | ---: |
| 原 A/VL | 3840×2160 | 24.18 fps | 41.36 ms |
| 官方 A/M Fast | 3840×2160 | 31.09 fps | 32.16 ms |
| 官方 A/M Fast | 1264×710 | 97.17 fps | 10.29 ms |

数据包含源 / Shader / VS 可读取帧成本，未包含播放器音频与显示同步；前三行没有叠加锐化，不能声称完整增强链达到同样速度。记录 `build/anime4k-fast-profile.json` / `.txt`，脚本 `build/profile-anime4k-fast.py`。官方展示图的设备是 Vega64、Fast 使用较小 M 网络，不能推导任意核显整套播放达 200 fps。[官方说明](https://github.com/bloc97/Anime4K)、[官方低端配置](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/md/Template/GLSL_Windows_Low-end/mpv.conf)

默认新内置方案采用直接目标尺寸：Anime 低于 2160p 的源先增强再 A/M，连续 5 秒丢帧 >5% 先关闭增强，再不足则原生直通；Realistic 无增强 / 降噪、直接 Jinc。强制每帧延迟的回归测试验证两级切换，不在暂停 / seek 时降级。性能页的先 Jinc 到目标再增强选项适用于 Anime 过大输入 / 小画布。画布目标在稳定调整尺寸后重建，重新开始测量，最大 3840×2160，优先使用保持源宽高比的偶数尺寸，保留 SAR 元数据；非方形像素实播尚未验证。内置 VPY 可直接编辑，已有文件不覆盖。

AList 用户地址 GET 返回 HTML、HEAD 返回 405；`/api/fs/get` 才给签名媒体直链。Qt 使用 GET / Range 验证、跟随重定向，再在独立线程代理给 FFmpeg / FFMS2。ModelScope 崩溃栈定位在字幕线程 `av_read_frame` 的空输入：网络 PGS 打开失败却保留句柄。播放器现在释放失败句柄，原生 PGS 读取检查初始化，ASS / PGS 支持 HTTP 输入。真实 ModelScope 1080p 与 AList 2160p 的分段文件头 / 原生播放测试通过，见 `build/player-supplied-links-final.txt`；真实 ModelScope 完整临时输入与 VPY 640×360 输出也通过，见 `build/player-real-network-http1.txt`；VPY 首次全帧索引仍需扫描媒体，不能承诺即时打开。标准源禁用磁盘索引缓存，现存用户索引不删除。网络 VS 使用独立连接池的四路 8 MiB Range 临时文件，显示百分比 / MiB，读完整后执行本地 VPY；临时文件在切换源释放 VS 或退出后删除。原生直通只通过 Range 代理。HTTP/1.1 与每路独立代理连接避免暂停读取占住共享连接池。


本轮最终交付验证：Release 构建成功，CTest 6/6（102.52 秒）；交付目录 Player 20/20、无跳过（69.76 秒），包括用户提供的两个真实链接、4K48 原生播放和两级自动降载。ModelScope 完整临时输入及 VPY 640×360 输出另测通过（213.16 秒，包含短 HTTP / 暂停测试）；AList 已测直链解析、Range 和原生播放，未再完整读取其 2.49 GiB 文件做 VPY 长片测试。原生直通最新 8 秒样本为 384 呈现 / 0 丢弃 / 0 合并；这是短样本，不保证长片或其他功耗条件。

移出测试 EXE / Qt6Test.dll 后，VSRenderer.exe 与 vs-player.exe 各自启动并正常关闭；交付 EXE / 原生 DLL 与构建一致，嵌入 GPU helper 和 MVTools 色度修复仍在，Fast Shader 与源码一致，原有全部 VPY 与 player.ini 哈希未变。临时网络目录在测试退出后已删除。记录：`build/player-network-profile-ctest.txt`、`build/player-network-profile-portable-results.txt`、`build/player-real-network-http1.txt`、`build/player-network-profile-delivery-audit.json`、`build/player-network-profile-startup-smoke.json`。未推送 Git、未打包，版本仍为 1.0.2。

## 前序测试记录

# VS Player 1.0.2 性能与验证记录

日期：2026-10-01。版本继续开发中，只更新本地便携目录。

## 本次 GPU 锐化与暂停修复

用户对比 madVR/LAV 和 VS 后确认原锐化链并未硬件执行：`std.Convolution`、`std.Expr`、`std.Minimum/Maximum` 属于 CPU 运算，改变强度不改变每帧运算量；3FP-HW 徽标只描述 3FP 源时钟的解码，不代表 VPY 内部源解码或滤镜。新 YUV 锐化使用随附 vs-placebo 的 Vulkan GLSL；Gray/RGB 保留原 CPU 算法。相邻锐化节点合并一次 Shader 调用，输出取 GPU 亮度与原始色度组合，仍保留局部 min/max 限幅。[vs-placebo Shader API](https://github.com/Lypheo/vs-placebo)、[libplacebo Shader Hooks](https://libplacebo.org/custom-shaders/)

与下文同一 MyGO 素材及原预设参数，48 帧、四路请求的最新配对暖态样本：CPU 13.01 fps / 76.88 ms，GPU 合并 25.76 fps / 38.82 ms，时间降低 49.5%。输出仍为 3840×2160 YUV444P16；结果受缓存、温度和 GPU 功耗影响，不能承诺长片不丢帧。记录：`build/gpu-sharpen-fused-profile.txt`，脚本 `build/profile-gpu-sharpen.py`。测试时没有并行运行其他本轮视频测试。

CPU 与 GPU 的计算精度不同，不保证逐位一致。合成灰阶纹理的三种锐化、1.0 / 0.3 / 1.5 强度复测：16-bit 亮度最大差 15–16（低于 8-bit 的一个量化级），色度逐位一致；边界采样方式也可能不同。记录：`build/gpu-sharpen-quality.txt`。不宣称复刻 madVR 的专有算法，其强度数值不能直接等同于本软件。

旧用户 VPY 文件保留；带图元数据且与旧内置代码精确匹配的 CPU 块在加载时转换，手写或修改过的未知代码不自动替换。新导出的 VPY 包含 GPU helper，可便携运行。

暂停高 GPU 的另一个来源是已选字幕每 30 ms 重新合成整幅图并上传，虽然视频时间没有改变。现在字幕线程缓存时间 / 画布 / 视频尺寸 / 显隐，切轨或样式加载使缓存失效。暂停后不新增预解码请求；已经提交给 VS 的少量异步帧仍完成，不能强制中断插件计算。通过暂停前后原生 swapChainPresents 计数验证不再重复呈现。

本次真实 MyGO 预设在四项占用阈值均为 100% 的极致设置中复测，暂停并等待已提交帧完成后，呈现计数连续 1 秒保持不变；Tab 采样 CPU 0.0%、GPU 0.3%。这是本机瞬时采样，不是对其他后台程序 GPU 占用的保证。

链接入口现在接通 3FP HTTP/HTTPS 和 FFMS2 网络源；首次索引需要扫描文件，本轮不持久写入帧索引；需要 VS 时准备临时媒体，释放源后清理。模型管理器直接传入的 URL 不再经过 QFileInfo 本地存在性检查。L-SMASH 对网络文件建索引失败时，此路径固定使用 FFMS2；原本本地选择不变。覆盖带查询参数、编码空格的本机 HTTP Range 播放、精确跳帧和 file URI。

最新验证：CTest 六组全部通过（81.62 秒），交付目录 Player 15 项通过、无跳过（34.03 秒），含极致模式真实暂停、目录树、全屏底栏、无视频右键、HTTP Range 及原 RIFE / 字幕 / madVR 回归。移出测试程序及 Qt6Test.dll 后，两正式 EXE 启动并正常关闭；三个交付文件与构建一致，两程序嵌入的 GPU helper 与当前源码相同，MVTools 色度修复保留，全部原用户 VPY 和 INI 哈希不变。日志位于 `build/player-gpu-network-*.txt` 与两个 delivery-audit / startup-smoke JSON。未做长片、跨网络服务器、HDR 显示器的人工验证。

## 素材与测试条件

用户素材 `D:\Animation Enhance\MyGO BDRemux\01.mkv`：H.264 High、1920×1080、23.976 fps、SAR 1:1、BT.709，音频 PCM 24-bit / 48 kHz / 双声道，含五条 PGS 字幕。用户保存的 `vpy/Anime4K Default.vpy` 顺序为 Sharpen edges、Crispen edges、Enhance detail、Deband、Anime4K A 2×，VS 输出 YUV444P16、3840×2160。

早期短样本测试受到后台 FFmpeg 高负载影响，不能用于判断播放器独立性能上限。用户关闭后台负载后重新采集下面的 48 帧暖态样本；采集时没有其他本轮视频测试并行运行。GPU 为 RTX 4070 Laptop，结果受功耗、温度、缓存和源帧复杂度影响，不能代表长片稳定帧率。

| 处理链 | 并发请求 | 帧数 | 吞吐 | 均摊时间 |
| --- | ---: | ---: | ---: | ---: |
| 单独 Anime4K A 2× | 1 | 48 | 26.21 fps | 38.16 ms |
| 单独 Anime4K A 2× | 4 | 48 | 25.70 fps | 38.90 ms |
| 保存的完整链 | 1 | 48 | 2.83 fps | 353.46 ms |
| 保存的完整链 | 4 | 48 | 10.30 fps | 97.05 ms |

日志：`build/anime4k-performance-idle.txt`。均摊时间是处理整批帧的时间 / 帧数，不等于某一帧的延迟；Tab 请求耗时包含冷启动及等待时间，不能直接与此列等同。

## 原因与改动

锐化和细节增强包含 CPU 卷积、表达式及局部范围运算，结果随后上传 GPU 执行 Anime4K。各阶段有依赖，单帧处理无法同时充分使用 CPU 和 GPU；任务管理器的整体占用也不能代表单个引擎、内存带宽或滤镜串行阶段的余量。关闭后台负载后的并发增益说明提前请求有帮助，但完整链仍达不到 23.976 fps。

播放器现在利用 VapourSynth 的 getFrameAsync 提前请求，保留 VS 帧引用，仅在提交当前帧时复制 CPU plane；不为每帧创建 std::async 线程。缓存回收不等待未完成的远处帧。常规阈值 50%、极致 100%，默认最多提前 8 帧，可改到 16；CPU 线程数和 VS 缓存目标同时受设置影响。CPU / GPU / RAM 为系统调度阈值，显存为本进程 DXGI budget 使用比例。达到阈值停止追加，不能强制中断当前计算或控制插件 / 其他程序的瞬时占用。预解码只能吸收短时波动，不能弥补持续吞吐不足。

发现 Sharpen edges / Crispen edges 的 Expr 不引用 z，却仍传入第二次模糊结果 `_wide`。新生成脚本只保留两路输入；Enhance detail 确实使用 z，继续保留第二次模糊。真实素材帧 100 / 220 / 350 的完整链输出，三个 plane 的 SHA-256 与旧脚本一致。

最新一组配对 48 帧 / 四路请求，保存链约 109.41 ms/帧，去掉无用卷积后约 99.37 ms/帧，时间减少约 9%；这是短样本结果，非保证值。日志 `build/sharpen-pruning-idle.txt` 保存配对数据，多次测量有波动。原 `Anime4K Default.vpy` 保留，另外生成 `Anime4K Default Optimized.vpy`；选择新文件或在 Renderer 重新保存预设即可使用修正。

## 丢帧与资源显示

之前只显示 3FP 丢帧，而 VS 来不及处理时直接跳到最新目标帧，被跳过的帧根本没有进入 3FP 队列。现在连续播放时累加实际提交帧号之间的缺口，seek / 换脚本重置连续性；Tab 分开显示 VS 跳过、渲染丢弃及合并。真实预设测试已确认显示非零跳过帧数和 GPU 数字占用。GPU 顶部显示系统中最忙引擎的占用百分比，型号另放在设备行；没有性能计数器时显示未提供。[PDH 计数器接口](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw)

## 3840×2160 变为 4096×2160

本机该素材源为 1920×1080 / SAR 1:1，Anime4K A 2× 的实际输出为 3840×2160，未复现 4096。可能方向包括像素 / 显示宽高比设置、视频源 / 目标裁切矩形，或把对齐后的存储表面尺寸当成可见尺寸；这些是待验证假设，无法在没有 PotPlayer 相应文件和渲染信息的情况下确定原因。不能因此强制裁掉 256 像素。[DirectShow 源与目标矩形](https://learn.microsoft.com/en-us/windows/win32/directshow/source-and-target-rectangles-in-video-renderers)、[FFmpeg 宽高比说明](https://ffmpeg.org/ffmpeg-filters.html#setdar_002c-setsar)

## 前一次验证与边界

- Release 与匹配原生 DLL 构建成功；CTest 六组通过，交付目录 Player 13 项通过、无跳过。涵盖原有 RIFE 精确中间帧、时间 / 帧 / 关键帧、变速、暂停、LAV、全屏恢复、INI 往返、慢 VS 跳帧、真实预设 GPU 数据。移出测试 EXE / Qt6Test.dll 后，两主程序独立启动并正常关闭；构建 / 交付三文件 SHA-256 一致，完整补帧 helper / 色度修复内嵌，两程序一致，原有三份用户 VPY 未改写。
- 真实内置 PGS、外部 SRT 优先、字幕显示开关、源 PNG 尺寸和显示 PNG 含字幕通过；madVR 彩色画面、字幕 OSD、截图、重复打开、绕过 VS 及 2× 音频时钟通过。
- 保持 VS 输出、P010 / P016 / YUV444P10 / YUV444P16 / NV12 / RGB48 / RGB24 实际提交和像素检查通过，未改变画面尺寸。其他输出格式按随附 FFmpeg 支持情况，失败会明确反馈。
- 默认 Jinc 抗振铃是强度 0.5 的局部 min/max 约束，同尺寸绕过额外缩放。使用开放的限制思路，不声称复刻 madVR 专有代码；类似的开源滤镜约束可见 [libplacebo filters](https://github.com/haasn/libplacebo/blob/master/src/filters.c)。
- VS 字幕为原生 D3D11 覆盖、madVR 字幕使用 OSD；没有另外接入 EVR。ASS 保留样式，SRT 样式不影响 ASS / SUP。HDR 实画面截图目前为 8-bit RGB 显示截取，源截图为 16-bit RGB PNG；HDR 显示器、多显示器和长片持续播放尚未人工验证。

不推送 Git、不打压缩包；保存用户原预设和便携配置。

````````
<!-- end:docs/player-performance-1.0.2.md -->

<a id="record-docs-player-resolution-threshold-1-0-3-md"></a>

## docs/player-resolution-threshold-1.0.3.md

原始字节数：7008；SHA-256：`e6d2b01272e7f50de51a455844ecf2bb9dbb163247115a3696fd441f7a042157`。

<!-- begin:docs/player-resolution-threshold-1.0.3.md -->
````````markdown
# 1080p48 输出分辨率突变分析

日期：2026-10-01。版本：1.0.3 本地开发。此次只做诊断，未修改正式算法、播放器调度或交付二进制。

## 结论

2304×1296 是 1920×1080 的精确 1.2 倍边界。当前官方 Mode A/Fast 组合的 CNN 放大阶段要求宽、高倍率均严格大于 1.2。2272×1278 和 2304×1296 不执行这些放大阶段；2368×1332 和 2560×1440 执行。因此越过边界后的计算量不能按最终输出像素增幅估算。

不超过边界时仍有 Clamp_Highlights 和 Restore_CNN_M，不能描述成完全关闭 Anime4K。超过边界会新增 Upscale_CNN_x2_M 的九个 pass（含最终重排），先生成 3840×2160 中间画面，再由 AutoDownscalePre_x2 缩到目标。后面的 S 放大器保留自身条件，不应把全部十四个条件都算成同时执行的额外 pass。

部署 shader 与 `assets/anime4k-a-fast.glsl` 的 SHA-256 一致。条件在该文件 397–657 行；655–656 行规定中间输出宽高各乘二，AutoDownscalePre_x2 在 697 行附近。上游固定版本：[Upscale_CNN_x2_M](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/glsl/Upscale/Anime4K_Upscale_CNN_x2_M.glsl)。[libplacebo WHEN 语义](https://libplacebo.org/custom-shaders/#when-expr)为条件为零时跳过该阶段。

| 输出 | 相对源倍率 | CNN 放大阶段 |
| --- | ---: | --- |
| 2272×1278 | 1.1833 | 跳过 |
| 2304×1296 | 1.2000 | 跳过 |
| 2368×1332 | 1.2333 | 执行 |
| 2560×1440 | 1.3333 | 执行 |

2368 相比 2304 只增加约 5.6% 输出像素，2560 相比 2304 增加约 23.5%；两者却都越过了上述算法分界。

## 素材与方法

素材：`D:\Animation Enhance\GBC 108048\Girls.Band.Cry.02.AV1.FLAC.1080p48F.mkv`，1920×1080 AV1、约 47.95 fps。每帧预算约 20.85 ms。本次没有并行启动其他视频测试，也未修改机器功耗方案；没有独立记录 GPU 频率、温度与功耗，不能排除这些因素。

诊断程序链接当前 Player 编译对象，使用便携目录的 Anime.vpy 和实际源文件。固定目标尺寸和质量阶段，关闭自动降载，避免中途切换预设混淆结果。增强为 sharpen_edges / enhance_detail 各 0.3；纯 A/Fast 关闭这两项。保留 Jinc、16 帧预解码和极致资源上限。仅使用构建目录的临时 INI / VPY，结束恢复构建 INI 并删除临时 VPY，未改用户便携 INI。

## 实际播放器复现

固定 1280×760 外窗口，首帧后预热三秒，每档计数六秒。画布较小，以下主要验证 VS 链的分界，不等同于用户全屏测试。丢帧率按 VS 跳过帧 /（呈现帧 + VS 跳过帧）计算。

| 模式 / VS 输出 | 呈现 | VS 跳过 | 丢帧率 |
| --- | ---: | ---: | ---: |
| 增强 + Fast / 2272×1278 | 279 | 10 | 3.5% |
| 增强 + Fast / 2304×1296 | 278 | 9 | 3.1% |
| 增强 + Fast / 2368×1332 | 187 | 101 | 35.1% |
| 增强 + Fast / 2560×1440 | 172 | 116 | 40.3% |
| 纯 Fast / 2560×1440 | 278 | 9 | 3.1% |

所有这些样本中，原生渲染器自己的 dropped / coalesced 计数均为零。实际漏帧发生在按播放时钟选择 / 提交 VS 帧的路径；不能解释为仅 Tab 显示错误，也不能仅凭这个计数认定原生呈现没有开销。

证据：`build/resolution-player-results.txt`。

## VS 吞吐与时间预算

独立 Python 诊断使用交付 Python / VS / shader，16 个 VS 线程、1 GiB VS 缓存、四路异步取帧，每档 96 帧、预热八帧。与实际 Player 的缓存预算和调度不同。

纯 Fast 的 2304、2368、2560 输出分别约 59.98、54.55、50.86 fps，即 16.67、18.33、19.66 ms/帧。增强 + Fast 在 2368、2560 时约 44.32、42.57 fps，即 22.56、23.49 ms/帧。纯 Fast 到 1440p 的孤立吞吐仅比 47.95 fps 实时要求高约 6%，不能视为整条播放器还有充足余量。

同分辨率 2304 的私有 shader 控制：强制开启原本跳过的 CNN 放大阶段，纯 Fast 短样本由 59.98 降到 51.91 fps。控制仅发生在诊断进程内，未改部署文件。其他短样本有顺序 / 暖机波动，不能把某一次 fps 差值当成固定算法成本。

证据：`build/profile-resolution-threshold.py`、`build/resolution-threshold-results.json`。

## 全屏纯 A 的边界与尚未确定部分

另测匹配大画布和实际无边框全屏。全屏 VS 输出为 2560×1440，但本机物理 surface 是 2561×1592，含宽高比适配区域；普通 2368 目标的实际 surface 也出现 2369 像素宽的取整。它们不是用户精确 2560×1440 显示器的完全等价环境。

首轮六秒全屏纯 Fast 样本丢帧约 43.6%，后续相同 Jinc / 10 ms 条件的短样本又恢复约 4.9%。延长为预热八秒、测二十秒，依次得到：

| Jinc / 调度 | 呈现 | VS 跳过 | 丢帧率 |
| --- | ---: | ---: | ---: |
| 10 ms，第一轮 | 800 | 160 | 16.7% |
| 临时 5 ms 精确定时 | 857 | 102 | 10.6% |
| 10 ms，第二轮 | 882 | 77 | 8.0% |

因此没有证据认定改为 5 ms 就能稳定修复。临时改 Bilinear 的短样本也有改善，但后续 Jinc 同样恢复，不能单凭这组顺序测试断言 Jinc 是全屏纯 A 的唯一原因。全屏的持续 30% 漏帧尚未稳定复现；需要进一步记录请求 / 取帧 / 提交 / Present 分段耗时及 GPU 频率，而非把初次的峰值当成恒定算力限制。

源码可确定的额外成本：

- VS 的 GPU shader 返回 CPU 可读帧；FrameServer 将各 plane 复制进 QByteArray，原生 renderer 再上传纹理。2560×1440 YUV444P16 的一帧有效图像约 22.1 MB，47.95 fps 时仅一份图像流就约 1.06 GB/s，实际传输 / 中间处理更多。此值不等于已经测得带宽瓶颈。
- 当前 Player 由 10 ms 定时器发起下一请求，pending 期间不发新目标；frameReady 同步提交之后仍等待下一 tick。临近 20.85 ms 预算时，复制、同步和等待可能把小幅耗时增量放大成跳帧。它是代码确认的调度结构，不是已量化的唯一因果解释。
- 原生缩放在倍率误差小于 0.00001 时走等尺寸采样，否则进入选定算法。一像素的画布差异可能开启额外 Jinc。当前 Jinc 是 4×4 邻域并计算权重，需要进一步测其真实 GPU 时间；本次不宣称已证明该一像素差异导致上述全部漏帧。

证据：`build/resolution-canvas-results.txt`、`build/resolution-scaling-results.txt`、`build/resolution-repeat-results.txt`，诊断程序 `build/resolution-player-probe.cpp`。

## 后续优化方向

先对分段耗时和实际 GPU 适配器做测量，优化请求调度与 GPU / CPU 搬运，再评估同尺寸呈现和 Jinc 的成本。若改 CNN 触发条件或替换轻量网络，应明确画质变化；仅禁用超过 1.2 倍的 CNN 放大不属于无损性能修复。预解码可吸收短暂抖动，不能解决增强链持续低于源帧率的问题。

此次新增分析记录，未替换正式程序、修改 shader、修改降载阈值或发布包。

````````
<!-- end:docs/player-resolution-threshold-1.0.3.md -->

<a id="record-docs-player-six-stage-1-0-3-md"></a>

## docs/player-six-stage-1.0.3.md

原始字节数：7485；SHA-256：`b6d8db1b06154ca55ba75b2068191d5c7613a09cdd978b8805905fa2cc5c1d5d`。

<!-- begin:docs/player-six-stage-1.0.3.md -->
````````markdown
# Anime4K 六档降载与格式转换

日期：2026-10-02；本地版本：1.0.3。

## 六档与选择入口

设置 → 渲染设置 → Anime 起始档位；写入 `player.ini` 的 `player/animeStage`，支持完整 INI 保存 / 加载。只作用于内置 Anime；自定义 VPY 不继承此参数。Tab 显示当前档位。

| 数值 | 档位 | 实际路径 |
| --- | --- | --- |
| 0 | Anime4K CNN + 额外增强 | 细节增强 shader 与 A/Fast 拼接成一次 placebo.Shader |
| 1 | Anime4K CNN | A/Fast；含 Restore CNN，放大阶段保留上游倍率条件 |
| 2 | Anime4K no CNN + 额外增强 | 细节增强与非 CNN 线条处理合并为一次 Shader |
| 3 | Anime4K no CNN | Clamp_Highlights + Thin_HQ + Darken_HQ，无神经网络 |
| 4 | Jinc 直通 | 不加载 VS，3FPlayer 原生播放；默认显示缩放为 Jinc，保留手动选择缩放算法 |
| 5 | D3D11 原生直通 | 不加载 VS，使用原生 VideoProcessor；不支持的输入按既有机制轻量 Bilinear 回退 |

保持正常播放稳定观察超过 5% 丢帧才逐级降低。暂停、seek、缩放、全屏和变速之后仍先恢复两秒，再观察五秒；未改动这些门槛。Realistic / 4K 源 / 网络源沿用原生直通策略，略过不适用的前四档。

Renderer 的 Anime4K 模式下拉增加 `anime4k-no-cnn.glsl`；CNN 版本继续使用 `anime4k-a-fast.glsl` 等已有模式。两份文件随程序携带，不需要下载。

## 非 CNN 的含义和来源

当前 A/Fast 的网络不能通过去掉卷积而保持相同画质。非 CNN 版本使用同一固定上游版本的传统线条算法组合，画质属于另一种处理方式，不宣称与 A/Fast 等价。

`assets/anime4k-no-cnn.glsl`：保留原 MIT 许可，组合以下原文件，不改系数：

1. `glsl/Restore/Anime4K_Clamp_Highlights.glsl`
2. [Thin_HQ：Sobel / 高斯 / 线条变形](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/glsl/Experimental-Effects/Anime4K_Thin_HQ.glsl)
3. [Darken_HQ：DoG 线条加深](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/glsl/Experimental-Effects/Anime4K_Darken_HQ.glsl)

此文件没有 CNN 卷积、网络权重或 2× CNN 中间图；输出缩放由 libplacebo 执行。

## 减少链路转换

原先内置增强路径：16 位转换 → 增强 Shader → CPU 帧 → ShufflePlanes → 转成 YUV420P16 → Anime4K Shader → CPU 输出。

现在：必要时只升位深、保留源 420 / 422 / 444 色度采样 → 将增强 LUMA hooks 与 Anime4K hooks 拼接 → 一次 Shader → CPU 输出。减少两次 GPU 处理之间的读回与重新上传，取消中间的色度平面重组及强制 420 转换。Renderer 的普通 Anime4K 节点也改为保留原色度采样，只做插件所需的位深转换。

融合后的增强直接在 Anime4K 前的同一 GPU 管线内执行；由于不再在两个 Shader 之间进行 RGB/YUV 整数往返与色度重采样，不承诺与旧路径逐位相同。仍保留原有增强强度 0.3 和局部范围约束。

## Y410 / Y210 与 16 位的限制

Y410 是打包 4:4:4 10 位，Y210 是打包 4:2:2 10 位；VS 使用平面格式，不能把这些名称当作同一种内存布局。[Windows 格式定义](https://learn.microsoft.com/en-us/windows/win32/medfound/10-bit-and-16-bit-yuv-video-formats)。

Anime4K 数学本身没有要求源必须先变成 Y416。当前随包的 [vs-placebo Shader 接口](https://github.com/Lypheo/vs-placebo#shader)接受 YUVxxxP16，固定输出 YUV444P16，限制来自插件接口。已用随包 Python 检查签名，没有输出位深参数。因此这轮仍保留必要的 16 位平面输入，没有声称实现直接 Y410 / Y210 输入或 GPU Y410 输出。

平面 YUV444P10 与 YUV444P16 都以 16 位容器存每个分量，无 alpha 时都是约六字节 / 像素。仅在 Shader 之后降成平面 10 位不会减少该段搬运，反而增加一次转换。打包 Y410 为四字节 / 像素，确实有降低带宽的空间；但必须扩展插件输出及原生接收路径，不能只改 GLSL / VPY。Y210 与 Y216 都约四字节 / 像素，位深降低也不等于字节数减半。

现有设置仍可选择 10 位 3FP 输出格式；这是 Shader 后的格式转换，并非改变其内部输出格式。默认保留 VS 输出，避免为了显示一个 10 位名称增加 CPU 转换。

## 迁移与验证

识别已有内置 Anime 的旧默认处理片段时，只替换该片段和阶段上限；迁移前保存 `.before-six-stage`。其他用户 VPY、内置脚本的其他编辑和 Realistic 不整体重写。

回归新增六个起始档位，实际读取 FFV1 的 10 位 420 / 422 / 444 文件，验证首帧、帧 17 精确定位、自定义 VPY 不继承档位。慢滤镜测试覆盖 CNN+增强 → CNN → no CNN+增强 → no CNN → Jinc；直接选择 D3D11 单独验证。为容纳新增观察阶段，测试片从 24 秒延长为 60 秒，不放宽降载门槛。设置测试验证六项下拉及 INI 持久化。

诊断素材：`D:\Animation Enhance\GBC 108048\Girls.Band.Cry.02.AV1.FLAC.1080p48F.mkv`，1920×1080 / 47.95 fps，目标 2560×1440。独立 VS 短样本，每档 128 帧、16 线程、1 GiB 缓存、四路请求：

| 档位 | fps | ms / 帧 |
| --- | ---: | ---: |
| 旧增强 + CNN，首轮 | 30.36 | 32.94 |
| 融合增强 + CNN | 44.08 | 22.69 |
| CNN | 36.83 | 27.15 |
| no CNN + 增强 | 52.07 | 19.20 |
| no CNN | 51.89 | 19.27 |
| 旧增强 + CNN，复测 | 41.00 | 24.39 |

顺序样本明显有暖机 / 功耗波动，不能据此宣称固定提升比例，也不能把纯 CNN 那次低于增强 CNN 解读为增强能加速。此测试不含整条音频 / 播放时钟 / 原生呈现开销，52 fps 不能保证完整 48 fps 全屏不丢帧。

证据：`build/six-stage-performance.json`、`build/profile-six-stage.py`、`build/six-stage-targeted.txt`、`build/six-stage-fallback.txt`。构建使用当前共享工作树，同时保留另一窗口的图片 / 菜单更新；不把其功能算作本轮实现。

最终共享 Release 构建后的 Player 回归：36 项通过、0 失败、7 项按条件跳过，包含本轮六档、动态降载、INI 和原生缩放测试；证据 `build/six-stage-final-player.txt`。语言包校验通过，280 项字符串的中英占位符一致。整套 CTest 首轮为 7/8；未修改导出代码，导出明细复测 8/8，再次 CTest 导出单组也通过，八个组最终均完成验证。整套首轮与单组复验分别记录，不声称一次整套全过。证据 `build/six-stage-shared-ctest.txt`、`build/image-menu-export-retest.txt` 和 `build/image-menu-export-ctest.txt`。

交付目录：`dist/VS-Renderer-GUI-windows-x64`。两正式 EXE 和 FFF.Native.dll 与最终构建 SHA-256 一致，着色器 / 语言包与源码一致。更新前 INI 全部已有值保持，12 个其他预设未变；内置 Anime 原件备份哈希匹配，新片段使用六档与融合 Shader。资源清单核验 73 项。证据 `build/six-stage-delivery-audit.json`。

两正式程序独立启动并通过自身窗口的关闭消息正常退出，退出码均为 0；隐藏启动时 Process.CloseMainWindow 找不到主窗口，复验改为只定位本次创建 PID 的应用窗口发送 WM_CLOSE，未改正式程序。记录 `build/six-stage-smoke.json`。未将启动检查称为人工长时播放验证。便携目录不含测试 EXE / Qt6Test.dll，本轮未推送 Git、打包或发布。

````````
<!-- end:docs/player-six-stage-1.0.3.md -->

<a id="record-docs-player-startup-clock-1-0-8-md"></a>

## docs/player-startup-clock-1.0.8.md

原始字节数：4631；SHA-256：`b280f4c428a8f8d1d5bd7c1b66cd2b2344461c0afb253c0ff44f48a7578a1924`。

<!-- begin:docs/player-startup-clock-1.0.8.md -->
````````markdown
# 1.0.8 无音轨视频启动时钟修复

日期：2026-10-06。本轮仅本地修改、构建与部署；版本保持 1.0.8，保留维护者更新的 FFmpeg，不推送、不打包、不改动在线内容。

## 原因

两个指定文件标称帧率均为 `7001/146`（约 47.952 fps），时间基为 `1/1000`，起点为 0。`OP-Auto33-321.mkv` 只有 AV1 视频流；`Girls.Band.Cry.04.AV1.FLAC.1080p48F_2026.10.05-22.31.36.mkv` 带 FLAC 音轨及 ASS 字幕，视频实际为 3840×2160、10-bit 4:4:4 AV1。

`PlayerSession::ArmAudioUntilVideoFrame` 曾在没有音频输出时也设置 `audioBlockedUntilVideoFrame_`。`PumpVideoPresentation` 为尽快显示音频同步首帧，允许该门控状态绕过 PTS 等待；但无音频输出不会记录和释放音频边界，后续帧也一直走这个分支。画面因此受 Present / 显示器刷新速度限制，而媒体时钟本身仍按 1 倍速推进。跳转、启动期间内部速率设置或视口调整均可能重新设定门控；暂停续播可清除无音频门控，造成交互后的表现不一致。

修复只改门控条件：存在 `audioRenderer_` 时才启用首帧音频同步。无音轨或音频输出不可用时，视频按原有 QPC 媒体时钟及 PTS 等待呈现。有音频输出的首帧同步行为保留；自动关键帧、帧率元数据和显示器设置未修改。

补丁为 `patches/3fp-silent-video-startup-clock.patch`，已纳入 `tools/build-3fp.ps1`，在此前软件解码、音频电平、HDR 读回补丁后应用。完整既有补丁源码的构建前后仅 `PlayerSession.cpp` 的门控条件及说明改变。

## 真实文件验证

新增 `TestPlayer::directStartupClock`，从首次可见帧开始记录墙钟、媒体位置、帧 PTS、接收计数与真实 Present 计数，并核对首次打开、跳转、暂停续播的播放速度。首帧等待另记毫秒数，不将冷启动初始化时间混入播放帧率。测试使用隔离的测试配置，不修改运行目录 `player.ini`。

旧 DLL 在 OP 的 D3D11 / Jinc 路径复现失败：约 3.04 秒内呈现约 500 帧，帧 PTS 已到约 10.5 秒，接近 165 fps；当前显示器为 165 Hz。记录：`build/startup-clock-1.0.8/op-baseline.txt`。

修复后 OP 的原画、D3D11、Jinc 三种直通模式：

| 解码 | 首次打开 / 跳转 / 续播实测接收帧率 | 帧 PTS 与时钟偏差 | 结果 |
| --- | --- | --- | --- |
| D3D11 硬解 | 47.80–48.17 fps | 小于 27 ms | 三种模式全部通过；测量期间无渲染丢弃或合并 |
| CPU 软解 | 46.75–48.10 fps | 小于 34 ms | 三种模式全部通过；首次打开 D3D11 缩放丢弃 4 帧、Jinc 丢弃 1 帧，跳转 / 续播无丢弃 |

硬解首帧 271–449 ms，软解首帧 218–239 ms。原画首次测量曾从 Playing 状态开始时尚未出现首帧，改为独立记录首帧时间后完成最终验收，没有为此增加生产代码改动。最终日志为 `op-hardware-final.txt` 和 `op-software-final.txt`。

带音轨的 4K 对照文件在本机回退为软解。旧 DLL 与新 DLL 都没有 165 fps 快进，但都发现启动时的软件解码丢帧 / 延迟；严格的完整实时播放用例未通过，不能据此声称该文件整体播放性能已通过验收。日志为 `audio-control-baseline.txt`、`audio-control-fixed.txt`，该项属于后续性能排查边界。

## 回归与本地部署

回归及部署记录位于 `build/startup-clock-1.0.8/`。新原生 DLL SHA256 为 `2a8d6171ea657e2a34b0ad93d9ed8693e6ce5dbec914f16778cf118fc699efce`；部署保留配置、VPY、色彩资源、语言文件及用户更新的 FFmpeg / FFprobe，并更新本地资源哈希清单。

`regression-final.txt` 中 14 项通过，涵盖合成无音轨 47.952 fps 启动、HEVC / AV1 软解及音频提前结束后的尾帧、独立音频时钟、硬软解播放 / 暂停时点击及拖动寻帧、输入电平和 HDR 浮点读回。额外两条浮层用例在鼠标命中检查处失败：`QApplication::widgetAt(QCursor::pos())` 返回空。分别用旧、新 DLL 单独复查仍同样失败（`overlays-baseline.txt`、`overlays-isolated.txt`），因此本轮不能声称浮层回归全部通过；未改动此前浮层实现或放宽这些断言。

已部署到 `dist/VS-Renderer-GUI-windows-x64`，54 个受保护文件哈希一致。随后使用运行目录的 DLL 再验收 OP 的 D3D11 直通：首次打开、跳转、续播均通过，首帧 269 ms，三段媒体时间与墙钟的差异小于 1 ms，测量期间无渲染丢弃或合并。记录为 `deployed-op-smoke.txt`；烟测后再次核对 54 个受保护文件全部未变。

````````
<!-- end:docs/player-startup-clock-1.0.8.md -->

<a id="record-docs-player-transparent-audio-info-1-0-7-md"></a>

## docs/player-transparent-audio-info-1.0.7.md

原始字节数：4427；SHA-256：`96c9fecfb20abcae4b69b4abe184897777a7540542fc3bb7c055eaa3df5d4dd4`。

<!-- begin:docs/player-transparent-audio-info-1.0.7.md -->
````````markdown
# 1.0.7 透明控制栏与音频信息 (2026-10-06)

本轮仅更新本地源码和 `dist/VS-Renderer-GUI-windows-x64`，版本保持 1.0.7。不打包、不推送、不上传、不改变线上版本；部署保留现有配置、预设和组件。

## 使用

- 设置 → 主题设置增加“底部透明度”和“右侧播放列表透明度”，分别保存。默认 50%，0% 为不透明，100% 为透明；按钮、文字仍然可读。原“窗口不透明度”保留。
- 底部栏、浮动和全屏播放列表使用透明覆盖窗口，视频继续渲染。窗口模式固定列表仍扩窗分栏，避免挡住视频，可用原有分隔条调整宽度。
- 全屏底部栏无鼠标移动或操作约 1 秒后隐藏。鼠标进入控制栏实际高度范围即可唤出；唤出边界不随隐藏而缩到最底部。编辑时间、帧号或弹出菜单期间保持控件可用。
- 全屏鼠标移到顶部可显示标题和“退出全屏”。这些浮层显示、隐藏不改变视频视口尺寸。
- Tab 首先列出当前源文件、预设和挂载的外部音频 / 字幕，再列统计信息。长路径、超出屏幕的内容可滚动查看；文字区域与底部透明度条分开。
- Tab 顶部可选“音频简略信息 / 音频详细信息”。详细信息在右侧显示，与 Tab 一起关闭；打开设置时让位于设置窗口。

## 音频电平含义

右侧显示音频后端、编解码器、WASAPI 共享 / 独占模式、实时比特率、输入 PCM 和实际协商输出格式，以及输入 / 输出各声道峰值电平。读数使用 `20 × log10(峰值)`，单位 dBFS；静音为 −∞，柱形刻度范围为 −60 至 0 dBFS。这是截图所示的声道电平，不是 EBU R128 综合响度 LUFS。

输入来自混音、均衡器和主音量之前的 PCM，随音频块提交给 WASAPI 时发布；输出来自实际提交设备的 PCM。主音量改变只影响输出。最多显示八声道，声道标签按输入布局和重采样输出布局排列；八声道输入降混为立体声时，输出只显示 L / R。

原生扩展补丁为 `patches/3fp-audio-input-meters.patch`，在软解流水线补丁之后应用。新增可选接口 `FFF3FP_GetAudioInputPeakLevels`，保留原输出电平接口及原有 ABI。PCM 扫描在音频队列锁外进行。LAV / DirectShow 没有提供该接口时显示读数不可用，避免代用其他音轨的电平。

BD 原盘菜单由 libVLC 合成时保留导航信息页；声道电平面板在节目切回 3FP 后显示。

## 验证记录

测试与截图保存在 `build/chrome-audio-1.0.7/`。专项覆盖独立透明度、全屏唤出 / 隐藏、原生视频上的实际 alpha 合成、浮层不改变视口、固定播放列表分栏及宽度调整、Tab 文字 / 滑条布局、稳定面板尺寸、设置窗口优先级、输入输出声道与音量关系。

完整回归 `acceptance-verified.txt`：20 项通过，0 失败，0 跳过。原盘菜单可加载、重开并切到节目；该项在桌面前台不可用时使用原生窗口消息测试输入坐标，本轮不据此声称重新验证了真实鼠标菜单点击。原生视频透明合成另由前台截图及像素数值验证。

全屏标题与列表布局最终调整后的界面回归 `acceptance-final-ui.txt`：9 项通过，0 失败，0 跳过。实际桌面像素测试会等待测试窗口在前台；桌面被其他窗口占用时明确跳过该视觉步骤。

测试音频为 48 kHz、32 bit float、7.1 PCM，八声道峰值依次为 0.08 至 0.64。读数误差小于 0.002；本机输出协商为双声道。主音量降至 25% 后，输入读数保持不变，输出按音量下降，暂停清空电平。

保留 HEVC / AV1 独立软解流水线、均衡器、ASS 字幕名称变体、16 个歌词样本和原盘菜单的回归。实际透明合成截图为 `transparent-native-video.png`；音频面板截图为 `audio-detail.png`。

后续高精度截图更新修复 Native FP16 读回，最终 DLL SHA-256 为 `93510AECD9FFDDD0D3DE3201795043C6FDDEF2896D16505FE0BAD5E60C817CF3`，保留本轮音频接口和软解流水线。使用该 DLL 的界面、音频电平、软解、BD 字幕 / 跳转等相关回归 27 项通过；最终部署审计在 `build/screenshot-1.0.7/deployment-audit.json`。截图格式与色彩测试边界见 [截图说明](player-high-precision-screenshots-1.0.7.md)。

````````
<!-- end:docs/player-transparent-audio-info-1.0.7.md -->

<a id="record-docs-portable-updates-md"></a>

## docs/portable-updates.md

原始字节数：4013；SHA-256：`e18bc9be8335daafe80647a06345b29ce896ec94409ba7e64eaffd325477da11`。

<!-- begin:docs/portable-updates.md -->
````````markdown
# 完整便携更新与发布

Renderer 设置和 Player 的“设置 → 基本设置”都显示当前版本与检查更新按钮。检查读取公开数据集 `ARXChem/Software-List` 的 `VS-GUI` 目录，支持分页，按数字版本选最新的 `<版本>.7z`。例如 `1.0.10` 大于 `1.0.2`，`1.0.2.0` 等于 `1.0.2`；同版本与旧包不安装。

下载使用随包 `runtime/tools/aria2-next.exe`（2.8.3），八路分段、证书验证、有限重试；哈希按小块异步验证，下载和解压也不阻塞界面。更新包需要目录接口提供有效大小与 SHA-256，下载地址使用文件 Revision 固定版本。关闭更新窗取消准备，取消或校验失败不会修改原程序目录。

为兼容已发布的 1.0.2 更新器，打包脚本仍使用单个带版本号的 `VS-Renderer-GUI-<版本>-windows-x64` 目录。本轮修订的 1.0.3 及后续更新器按内容识别：程序文件可以直接位于解压顶层，或位于顶层下任意名称的一层目录；必须找到唯一完整程序目录，拒绝更深嵌套、不完整包和多个完整候选。保留路径穿越、重复路径、符号链接与 Windows 保留名称检查。完整目录包含两个正式 EXE、匹配的原生 DLL、完整 VS / Python / 解码器 / 着色器 / Qt 运行时、语言包、更新工具与 `release.json`。`release.json` 示例：

```json
{"schema": 1, "version": "1.0.3", "platform": "windows-x64"}
```

包路径验证拒绝绝对路径、驱动器/ADS、父目录跳转、重复路径、链接、Windows 设备名及不合法尾字符。解压后检查必需组件和内部版本。下载并验证成功才出现“安装并重启”；点击后关闭本程序，独立 PowerShell 安装器等待该目录内其他程序退出（最多 120 秒），不强杀另一个正在播放的窗口。

完整更新先在原目录同级构建新目录，再把原目录移到 `.previous-<标识>` 备份，将新目录替换为原位置。保留顶层 INI、自定义 VPY、预设元数据、用户项目与截图；内置 VPY 随新版替换，旧内置和缓存仍可在完整备份查找。新目录不沿用寻帧/着色器缓存。复制失败不触碰旧目录，交换失败在原目录缺失时恢复备份；结果写入 `update.log`，安装失败会显示提示。备份不自动删除。

维护者先构建并运行回归，再运行 `tools/stage-update-tools.ps1` 部署更新工具及许可（SHA-256 固定验证 aria2-next 2.8.3）。`tools/stage-portable.ps1` 也会调用它。`tools/package-portable.ps1` 从活跃目录复制到全新暂存目录，排除缓存、个人 INI / VPY、测试产物和历史脚本，携带 Anime / Realistic 及六个手动 Anime 内置 VPY。移除 madVR 的安装/卸载/重置/调试开启脚本和本机 settings.bin，保留便携运行时与许可。活跃目录的用户文件不删除。

1.0.3 压缩采用标准 7z LZMA2：`-m0=lzma2 -mx=9 -md=256m -mfb=273 -ms=on -mmt=2`，9级极限压缩、256 MiB 字典、实心归档，不使用 Zstandard 编码，兼容支持 LZMA2 的常见解压软件。输出 `dist/<CMake 版本>.7z` 与 SHA-256 文件；包内 `local-resource-sha256.json` 在清理后重新生成，记录全部文件（不包含其自身）的校验值。

1.0.3 发布安排：推送完整源码到 GitHub，创建无附件的 Release 草稿。`1.0.3.7z` 与 `1.0.3.7z.sha256` 由维护者手动上传到 GitHub Release 及 ModelScope 数据集 `ARXChem/Software-List` 的 `VS-GUI` 目录；本轮不自动上传二进制或更新源，不发布草稿。

1.0.3 修正归档根目录后，1.0.2 可正常验证与更新，无需首次手动迁移。安装器保持已有安装目录路径，不因压缩包的版本目录名重命名目标；本地开发目录仍为 `dist/VS-Renderer-GUI-windows-x64`。旧固定根目录包的大小及 SHA-256 正确，但会在 1.0.2 路径安全校验阶段被拒绝；维护者需替换更新源中的 `1.0.3.7z` 并同步校验文件。

````````
<!-- end:docs/portable-updates.md -->

<a id="record-docs-realtime-bridge-md"></a>

## docs/realtime-bridge.md

原始字节数：2671；SHA-256：`e595b4bf0a7a2b3ed800826d8665d206a4c3b3d9c7994d1e14e2dd2e572b8f4b`。

<!-- begin:docs/realtime-bridge.md -->
````````markdown
# VapourSynth → 3FP 实时桥边界

## 已实现接口

上游 3FP 只接收本地路径，因此仓库补丁新增 API 14 的 `FFF3FP_SubmitExternalVideoFrame`。生成的 VPY 将处理后 clip 暴露为 output 0、未处理源 clip 暴露为 output 1。Qt 读取两者的总帧数与 FPS，把左路当前实际呈现时间分别换算为源绝对帧和处理后绝对帧，再对 output 0 调用 `getFrame`。这避免把 3FP 时间 seek 后的局部 PTS 索引误当成绝对帧号，也支持常见倍帧/抽帧链。plane 内存在提交返回前保持有效，3FP 在返回前完成上传/复制。

当前 CPU ABI 支持 Gray 8/16-bit，以及 YUV 4:2:0、4:2:2、4:4:4 和 planar RGB 的 8/10/12/16-bit 整数格式。VS RGB 的 R/G/B plane 在提交前重排为 FFmpeg GBR 顺序；`_ColorRange`、`_Primaries`、`_Transfer`、`_Matrix`、`_ChromaLocation` 和帧时长一并映射。

左路是唯一播放时钟和音频来源。右路不再打开同一个文件，也不建立第二个独立计时器。首帧请求把 VS 线程数设为不少于全部逻辑核心，并并发预取后续 6 帧，缩短冷启动后进入连续播放的等待；稳态播放时同时只允许一个目标 VS 帧请求，它完成并提交后，下一次直接请求左路此刻的最新目标帧，中间落后的帧自然被跳过，避免重滤镜因目标每 33 ms 改变而一直算完又丢、最终没有画面。seek 或重新载入脚本会取消旧目标，时间轴拖动期间的 3FP seek 按 timeline generation 合并。

A/B 滑块不创建第三路解码或第三次 VS 计算：原始与处理后 surface 使用相同几何尺寸，处理后 pane 仅按滑块位置设置可见区域，原始 pane 从左侧露出；并排模式则恢复两路各占一半。

不采用 Y4M named pipe 或完整临时编码：前者随机 seek 需要重启进程，后者会掩盖参数迭代延迟。当前有一次 VS→Qt CPU copy；未来只有在 4K/高帧率测量确认它成为瓶颈后，才扩展同设备 D3D11 texture ABI。

3FP 补丁位于 `patches/3fp-vsrenderer-extensions.patch`，构建入口为 `tools/build-3fp.ps1`。VapourSynth 固定源码的构建与 L-SMASH Works 安装入口为 `tools/build-vapoursynth.ps1`。

## 实验开关

- `VRR low-latency present` 对应已存在的 `FFF3FP_SetPresentConfig`。
- `VRR Pacing` 对应已存在的 `FFF3FP_SetPacingConfig`。
- 所给 3FP 头文件没有名为 `VBR low latency` 的播放 API。若此名称指可变码率音频缓冲，需给出目标分支/commit 后才能做真实开关；若实际指 VFR（可变帧率）低延迟 seek，应设计为帧桥请求策略，不能绑定到 Present。

````````
<!-- end:docs/realtime-bridge.md -->

<a id="record-docs-release-1-0-1-draft-md"></a>

## docs/release-1.0.1-draft.md

原始字节数：2657；SHA-256：`ed587ca982c46747430935ce8b693a47cb14d3879de5a327f8dd9af14e9a731f`。

<!-- begin:docs/release-1.0.1-draft.md -->
````````markdown
# VS Renderer 1.0.1

本地 Release 草稿；尚未创建、上传或发布 GitHub Release。

## 2026-09-30 新增

- 对比页顶部新增画布导出，保存 AB/ABC/ABCD/网格布局、缩放及时间偏移快照，复用 MKV 队列和 3FUI 参数，使用最大源分辨率。
- 轨道文件名自适配、明确 VRR 方框、颜色列表蓝色悬停、导航设置入口，修复参数文字旁异常竖线。
- 新增 14 个可调滤镜，随包提供 RIFE 4.26 / Heavy NCNN Vulkan 模型，AMD/NVIDIA 验证通过。
- Jinc/Super-XBR/双边核的对比导出会明确提示尚未支持；CPU VS 和 D3D 不保证像素完全相同。RIFE 实时能力依赖硬件，本机单线程 1080p 不能达到 24→48 fps。

## 新增与改进

- 新增图像分析比对页面：最多 9 路主流格式视频，常规并排/三列网格和 AB/ABC/ABCD 局部拼接滑块。
- 每路独立时间偏移，最多四行对齐轨道；源选择自动交换、音频源选择、同步缩放及 VRR 控制。
- 每格新增 × 卸载按钮，保留其余视频会话和偏移；颜色算法提示与当前项标记、加宽控件、VRR 明确边框。
- 修复滑块卡顿、ABCD RGB 提示被裁掉、窗口大小/全屏变化后缩放失效及垂直平移响应问题。
- 后台 VS 预热和进度显示，原生着色器共享及预编译缓存，降低首帧等待。
- 独立 MKV 导出与批量编码队列：顺序处理、挂起/恢复、停止保留输出、从头重做、自选输出目录与时间戳命名。
- 扩充开源色度处理、Spline36、Super-XBR 单阶段和 VS 去伪影/细节增强滤镜。

## 使用方法

下载并完整解压 `VS-Renderer-GUI-1.0.1-windows-x64.7z`，运行 `VSRenderer.exe`。请保留 runtime、shader-cache 和所有随附 DLL；无需另外安装 Qt、Python 或 VapourSynth。

对比页面可导入 MP4、MKV、MOV 等格式；导出封装仅支持 MKV。停止编码会尽量保留已封装的数据，尚未产出帧或编码失败时不保证输出可播放。

## 验证与限制

Release 构建和 CTest 五组回归通过，涵盖真实原生解码/像素取样、VS 帧桥与导出流程。Qt 布局截图不代表完整的人工视频画面验收；九路 4K 吞吐取决于硬件，未进行所有编码、VFR 长片及多显示器 DPI 的穷举验证。

Super-XBR 是开源 pass-0 单阶段实现，不等同于完整三阶段算法；不包含 NGU 或其他 madVR 专有实现。

## 待手动上传的附件

- `VS-Renderer-GUI-1.0.1-windows-x64.7z`
- `VS-Renderer-GUI-1.0.1-windows-x64.7z.sha256`

详细更新记录见仓库根目录 `changelog.md`。

````````
<!-- end:docs/release-1.0.1-draft.md -->

<a id="record-docs-release-1-0-10-md"></a>

## docs/release-1.0.10.md

原始字节数：2196；SHA-256：`c8857c2c1888eb985bfe8a049a2f79c844d3de22ea9df2657e69aa31b70b1a4e`。

<!-- begin:docs/release-1.0.10.md -->
````````markdown
# VS Renderer / VS Player 1.0.10

本版合并另一台设备返回的 1.0.9 源码与 Native API 18，并更新 HDR→SDR 默认色彩路由、软解统计/性能及 VSCap 编辑条。

- Full 新配置默认按源/输出选择色彩引擎：PQ/HLG→SDR 使用 libplacebo，SDR 和 HDR 输出保留原生路径；旧配置保留，可手动选择新的自动模式。色彩窗口“确定 / 取消 / 应用”统一中文。
- 无音频 CPU 播放按真实时间推进，丢帧与视频落后可见；8K AV1 420p10 改为 16/16 工作线程与帧上下文，减少解码内存。ASS 初始化与静态字幕合成减少重复工作。
- VSCap 工具显示选中框，左侧常驻颜色和工具参数；线工具可切直线/箭头，支持 Ctrl+Z 撤销和撤销按钮。
- VSCap 文字直接在截图中输入，虚线框可拖动，保存前可持续编辑；左侧系统字体支持输入自动补全，字号可输入数字，并可加粗 / 斜体。文字、移动和样式均可撤销，导出不包含虚线框或光标。
- VSCap 修复字号按钮、空文字框、拖动边框和工具提示，支持 Ctrl 正方形 / 圆形、虚线及数字像素笔宽。贴图默认原始像素大小，仅超屏时等比缩小，滚轮最近邻缩放；四面蓝色边框、完整编辑工具栏和右键复制 / PNG / AVIF。贴图“✓”隐藏工具栏，右键首项切换显示 / 隐藏，复制 / 保存保留原始尺寸与精度。
- Full 包含 Renderer、播放器、VSCap 及完整处理/编辑依赖；PlayerLite 为单独播放器，不含 VS、RIFE、PhotoCraft、libVLC/LAV/madVR 或高级色彩运行时。

本机 4K48 444p12 与 8K48 420p10 软件解码结果见 [验收记录](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/validation-1.0.10.md)。8K 仍未达到全程流畅：不要将更准确的计时和丢帧统计解释成吞吐已达标。测试关闭硬件解码、CPU 预缩放和位深降低。

附件由维护者手动上传，本草稿不含附件：`1.0.10.7z`（Full）与 `1.0.10-PlayerLite.7z`（独立播放器）。解压到新目录运行；保留旧配置与原始素材。包内含 `release.json` 与 SHA-256 资源清单。

````````
<!-- end:docs/release-1.0.10.md -->

<a id="record-docs-release-1-0-11-md"></a>

## docs/release-1.0.11.md

原始字节数：5447；SHA-256：`4cb4a6d16bbfc5a6f4a1d423b04bd0307d36c27f3680ac241b5cf468c15986da`。

<!-- begin:docs/release-1.0.11.md -->
````````markdown
# VS Renderer / VS Player / VS Cap 1.0.11

本版更新时间组预设、VS 后端、蓝光菜单、播放器底栏与截图交互，新增 16 位 JPEG XL 截图和 HDR 桌面白位还原，并提供精简的单文件播放器。

## VS Renderer 与预设

- 时间组支持独立滤镜链，预设携带全部时间组；全片单组预设可导入任意现有时间组，保留目标组边界。
- 保留内置脚本型预设的实际处理逻辑。本地预设与社区支持递归目录树、展开和收起，下载保留目录结构。
- 默认社区源为 [VS-GUI 公开预设社区](https://www.modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI/PresetCommunity)。
- 右侧时间与进度条同排；路径、浏览、前后帧、播放、并排及缩放控件同排，D3D11 选项完整显示。

## VS Player

- “视频 → 图像截取”和“截图到指定路径…”均支持 **PNG / AVIF / JXL**，分别可截取源画面或含字幕、显示效果的实画面。PNG / JXL 为每通道 16 位无损；AVIF 为有损压缩（Full 使用 AWJ 质量 70，独立版使用外部 FFmpeg / libaom-av1）。
- Full 与独立版内置 JXL 编解码，支持原生 PQ / HLG 色彩标记及桌面白位信息。JXL / AVIF 截取只生成单个文件；PNG 保留原有 HDR / SDR 与浮点附件流程。
- 优化 16 位 HDR PNG 解码与 SDR 预览，修正图片信息中高 DPI 视口的物理像素读数。
- 修复文件章节时间字段适配，全部章节在进度条正确分布。
- 统一蓝光右键与播放列表的返回主菜单入口，从正片返回选集页不再重新播放版权片；首次开盘保留原盘 FirstPlay。
- 非全屏底栏放到视频下方，初始窗口相应增高；全屏继续自动隐藏浮层。
- 独立播放器文件名固定为 **VS-Player.exe**，嵌入 Player 图标，自动创建并保留程序旁的 `player.ini`。单文件分发首次展开运行时缓存，后续复用。
- 单文件版不内置 FFmpeg，优先从 EXE 同目录、其次从 `PATH` 查找，缺失时明确提示。需要 **FFmpeg shared x64**：`ffmpeg.exe`、`ffprobe.exe`、`avcodec-63.dll`、`avformat-63.dll`、`avutil-61.dll`、`swresample-7.dll`、`swscale-10.dll`、`avfilter-12.dll` 及它们的依赖。
- 单文件版保留原生播放、字幕、图片与 BD 导航，不含 VapourSynth、RIFE、PhotoCraft、libVLC、LAV、madVR 或高级色彩运行时。Full 保留完整运行时。

## VS Cap

- 新配置默认保存 **JXL 无损 RGBA16**，格式顺序为 **JXL → AVIF → PNG**；保留已有明确选择的格式设置。截图工具条与贴图右键均支持三种格式，复制 JXL 文件后可用 Alt+T 贴图。
- 修正 HDR 桌面转 SDR 的亮度与颜色：捕获时查询源显示器的 Windows SDR 白位，转换时撤销系统 SDR 增益，不再固定按 203 nit 叠加 ACES。白位随选区、PNG / JXL、HDR 剪贴板和贴图传递。
- 取消三种格式的保存对话框均保留选区与标注；“✓”仍只复制剪贴板，普通 SDR 截图不自动落盘。
- 修正 PNG 压缩参数，保留 16 位四通道；3840×2040 实图 HDR PNG 从约 62.8 MB 降至 1.28 MB，像素一致。
- HDR 自动保存先显示截图层，再后台按所选格式保存；JXL / PNG 同时提供 HDR 原图和 `_SDR` 副本，不再默认生成 JSON 和裸浮点附件。SDR 副本与 VSP 共用白位转换；超出 SDR 亮度或色域的部分裁剪，完整高光保留在 HDR 原图中。
- 修复设置窗口打开时，置顶截图层被模态输入阻断、无法操作的问题；退出截图后可继续编辑未保存的设置。
- 矩形/圆形的点按钮可点击选择边框模式，与填充按钮互斥显示；滚轮仍可调节粗细。
- 填充方块仅放大内部图标，按钮高度与相邻工具保持一致。

## 后端与验证边界

- 升级 **VapourSynth R81、BM3D 11.0、BM3Dvk2 1.0、BestSource 22**，验证 Vulkan 解码与 GPU 时域降噪；默认源链不因此自动切换。
- 合入 **3FP API20 / PR23**，保留本项目原生扩展。PR23 的 Native 差异主要为接口注释命名，不宣称新增 P/E 核、CCD 亲和性或进程提权策略。
- MyGO 实盘选集及重复返回菜单、章节、时间组、外部 FFmpeg 播放和截图交互已做聚焦验证。
- 8k48在7940HX下软解仍然没有通过，当前上限仍然在30F附近。

实图新 HDR PNG 在本机加载及预览约 0.36 秒；这是文件加载采样，HDR 显示器热键端到端出框仍待实机复验。详见 [HDR 截图续修](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/vscap-hdr-1.0.11.md)。

本轮新增回归：截图交互 17 项、播放器截取与 PNG 回归 20 项、无 AWJ 的外部 FFmpeg 路径 9 项通过。3840×2034 HDR 样图转 JXL 后 RGBA16 像素逐点一致，文件由约 8.61 MiB 降至 6.12 MiB，一次本机采样编码 0.53 秒、解码 0.14 秒。色彩显示仍依赖第三方查看器的 JXL / HDR 支持；实际 HDR 屏热键、多屏和第三方查看器效果仍待实机验收。详见 [JXL 截图](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/jpeg-xl-1.0.11.md) 与 [SDR 白位修复](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/vscap-sdr-white-1.0.11.md)。

完整条件与已知限制见 [1.0.11 验证记录](https://github.com/user-Wing/VS-Renderer-GUI/blob/main/docs/update-1.0.11.md)。

````````
<!-- end:docs/release-1.0.11.md -->

<a id="record-docs-release-1-0-2-md"></a>

## docs/release-1.0.2.md

原始字节数：2750；SHA-256：`78e79e5c61dec2dd46f5599b3d3074e6483f08a369a119c075700e19fc0dbde5`。

<!-- begin:docs/release-1.0.2.md -->
````````markdown
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

````````
<!-- end:docs/release-1.0.2.md -->

<a id="record-docs-release-1-0-3-md"></a>

## docs/release-1.0.3.md

原始字节数：4316；SHA-256：`0347f25724211fd3cc6a75e256d8e815a5f560c3ccfad8ee3b9982a399d62bd4`。

<!-- begin:docs/release-1.0.3.md -->
````````markdown
# VS Renderer GUI & VS Player 1.0.3

1.0.3 改进独立图片播放、超大 AVIF/JPEG 性能、音频控制与 Renderer 的布局、参数和对比导出，并更新随包 LAV Filters 至 0.83.0 x64。

## VS Player

- 独立图片模式：修复 JPG 被当作视频帧序列的问题；支持 JPG、PNG、WebP、AVIF 等常见图片，图片不走 VS 滤镜或视频预解码。左右键按同目录名称顺序切图，滚轮缩放，Tab 显示完整输入、解码和输出信息。
- 超大图片：JPEG 使用 libjpeg-turbo SIMD，AVIF 使用 libavif/dav1d 与 libyuv SIMD；保留原始尺寸、ICC/EXIF，修复大图缩放上限和绘制问题。本机受控样本中，1.7 GiB JPEG 解码约 23.3s → 12.5s，完整巨大 YUV420 AVIF 约 23.7s → 6.4s；性能随设备和文件变化。
- 右键菜单增加平滑展开动画、圆角和明确复选框；音频设置支持 MKV 音轨切换、外部音频、±0.1s 同步、静音和非模态十频段均衡器。优先保留 2.0/5.1/7.1，设备不接受时回退到 2.0。
- 按集数及名称匹配同目录外部音频/字幕；拖入时实时提示挂载或单独播放，主次字幕独立加载。
- 设置新增应用，保留暂停/播放状态和时间；左下角加载/保存 INI 预设，右侧取消(N)、确定(Y)、应用(A)。文件关联加入常用图片和全选图片。
- Anime 默认保留自动降载；开发者内置新增六个手动固定版本：CNN + 增强、CNN、no CNN + 增强、no CNN、Jinc、D3D11。手动档不受丢帧或源分辨率触发的自动降档影响。
- 开发者内置将 Anime 自动版和 Realistic 置顶；新增右键“图像处理 → 补帧”，自动链为 RIFE 4.26 4queue、半宽高 RIFE、最高质量 MVTools、默认 MVTools、关闭，也可固定档位。补帧采用 Jinc / YUV444P16，关闭恢复 Anime 自动超分。
- 修复主题字体未应用到设置窗口和子控件的问题，中文使用所选 Han 回退字体；应用立即生效并保存。

## VS Renderer

- 新增915份去除内容重复后的 mpv GLSL / hook，覆盖指定的 hooke007 与 mpv User Shaders 来源；着色器节点可按名称搜索，支持0.5×缩小。来源、修订、哈希及许可随库保留。

- 处理链、参数与脚本、视频改为三栏；参数区使用更多高度，滤镜库更紧凑。修复 MVTools、RIFE 等参数文字遮挡，bool/下拉右对齐，标签换行。
- 改进导航图标间距、输入浏览同行、时间轴和播放组对齐；删除重复按钮，预热状态集中显示。设置改为导航中的独立页面。
- 导出栏目明确显示选中状态；对比画布限定 AB、ABC 两种 2+1、ABCD 常规/滑块 2×2，输出宽高保持最大源分辨率，其它布局提示不支持。
- 内嵌新 Renderer / Player 应用图标及视频 / 图片文件标记，保留系统缩略图和默认应用选择。

## 下载与更新

完整 Windows x64 便携包：`1.0.3.7z`，校验文件：`1.0.3.7z.sha256`。标准 7z LZMA2 9级极限压缩，不使用 Zstandard；解压得到版本目录 `VS-Renderer-GUI-1.0.3-windows-x64`，兼容 1.0.2 更新器。

包含两个程序、VS/Python/Qt/FFmpeg/滤镜运行时、LAV Filters 0.83.0、madVR、更新工具、内置 VPY 和许可证。排除缓存、个人配置、用户 VPY、截图、测试文件和日志。

维护者手动上传附件和 [ModelScope 更新源](https://www.modelscope.cn/datasets/ARXChem/Software-List/files?Root=VS-GUI)。1.0.2 可通过更新器升级，安装时保持现有目录路径并保留配置及自定义预设。

## 验证与已知边界

已完成 Release 构建、图片/音频/设置/手动 Anime、Renderer 布局与导出、图标等相关专项检查。具体样本与结果见仓库 docs；本轮没有运行无关的全量回归。打包另外验证归档完整性、压缩算法、目录排除项与内部资源 SHA-256。

图片目前使用 CPU 解码与绘制，没有 GPU 图片解码或整图显存缓存；动图显示首帧。编码端已损坏的 AVIF 无法由播放器恢复，需要从原图重新编码。受控分析已确认巨大 YUV444 本身可正常显示，早期黑底/偏绿样本来自编码工具 JPEG→RGB 扫描行的32位偏移溢出，详见 docs/avif-large-yuv444-analysis.md。

````````
<!-- end:docs/release-1.0.3.md -->

<a id="record-docs-release-1-0-4-md"></a>

## docs/release-1.0.4.md

原始字节数：3434；SHA-256：`1c7f4d4251609664b98f61240cb86260b054455fe04a482ed3ccf098597a3055`。

<!-- begin:docs/release-1.0.4.md -->
````````markdown
# VS Renderer GUI & VS Player 1.0.4

1.0.4 新增原精度图像编辑、可选的 3FP 高级色彩管理，改进原画播放、字幕加载及 Renderer 着色器配置。

## VS Player

- 新增图片工具栏：旋转、镜像、裁剪、调整尺寸、桌面背景、回收站删除，以及 AWJimage 的 AVIF、WebP、JXL、JPEGli、PNG 转换。尺寸和转换另存文件。
- “详细编辑”提供同窗口多文档、图层/组/蒙版、区域选择、画笔、修复、渐变、局部调整、栅格化文字及形状，保持 8/16bit 或 Float32 精度。支持有限 RGB PSD/PSB 图层读写和浮点 TIFF 输出。
- 图层可直接拖动与缩放；支持 Ctrl+T 自由变换、Ctrl+E 向下合并、Blend If 和独立撤销历史。图片滚轮上下移动，Shift 横移，Ctrl 缩放。
- 3FP 解码配置保留原生 709/2020 默认模式，新增 libplacebo D3D11 高级模式：HDR→SDR / scRGB、tone/gamut 映射、系统或自定义 ICC、SDR LUT、动态峰值及末端抖动。应用配置不重建 VS 链；Tab 显示实际色彩路径与回退原因。
- 修正 BT.2020 CL、RGB 通道顺序及 ICC/亮度单位；支持有效 HDR10+ knee/anchors 和不依赖增强层的 DV 元数据。必要的 VS 色彩转换根据源属性选择矩阵。
- 原画、Jinc、D3D11 直通不再等待 VS 初始化或创建索引；内嵌 ASS/SRT 字幕按播放位置增量读取。拖动直通进度条可连续预览，松手恢复原播放状态。
- 新增 Lanczos4 和自动补帧起始档位；最低补帧档仍丢帧时提示用户关闭。音频/字幕显示名称、语言与 Default/Forced 标记。

## VS Renderer 与运行时

- mpv GLSL 按功能分类，支持自定义着色器、倍率/指定宽高、等比联动；悬停显示完整路径及硬件要求。
- 使用增强 FFmpeg，增加 AVS3 编解码及其它格式支持。播放器与 CLI 共用 DLL，GLSL 库压缩存储，保留滤镜、模型、来源及许可证。
- 打包时重新生成 12 个开发者内置 VPY，不携带维护者个人预设或配置。

## 便携包与更新

完整包为 `1.0.4.7z`，校验文件为 `1.0.4.7z.sha256`。使用标准 LZMA2 9级压缩；包根目录为 `VS-Renderer-GUI-1.0.4-windows-x64`，兼容 1.0.2 更新器。本机安装目录仍使用无版本号的固定名称。

包含两个程序及 Qt、VS/Python、FFmpeg、滤镜、模型、LAV、madVR、更新工具和许可证；排除个人 INI、用户 VPY、索引/着色器缓存、截图、日志及测试程序。维护者自行上传附件与 ModelScope 更新源，本草稿不带附件。

要求 Windows 10 22H2 或更新的 x64 Windows 和 D3D11；增强 FFmpeg 要求 x86-64-v3，AWJimage 要求 AVX2，RIFE 需要 Vulkan。

## 验证与边界

Player、Renderer 与配套原生 DLL 已构建并完成相关专项；色彩 8/8、配置/语言/播放状态 6/6、VS 帧桥 10/10、便携色彩 smoke 4/4。图片编辑与直通字幕的样本、测试及限制见仓库 docs。归档另检完整性、内部资源 SHA-256、排除项和解压后的启动。

这些专项不等于全量回归：图脚本组仍有一条旧断言不匹配着色器资源读取方式。真实 HDR 显示器观感、商业 DV/mpv 对照和 108 GB 机械硬盘首帧尚未验证。未实现 DV FEL 重建、Vivid 动态处理、完整 PSD 智能对象/图层效果、DNG 或可回改矢量文字；动图目前显示首帧，重滤镜不保证实时。

````````
<!-- end:docs/release-1.0.4.md -->

<a id="record-docs-release-1-0-5-md"></a>

## docs/release-1.0.5.md

原始字节数：3042；SHA-256：`96a12088f6fcf253809c0047aa4a98344db6d71eb6017596dff70edc16b55f66`。

<!-- begin:docs/release-1.0.5.md -->
````````markdown
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

````````
<!-- end:docs/release-1.0.5.md -->

<a id="record-docs-release-1-0-6-md"></a>

## docs/release-1.0.6.md

原始字节数：3306；SHA-256：`f5d7ffcbb9c0d9461e7d8d55cf0e19b321da26ff2e5c95790c71bad3875d56ec`。

<!-- begin:docs/release-1.0.6.md -->
````````markdown
VS Renderer / VS Player 1.0.6，Windows x64 便携版。

### 下载选择

- **完整包 `1.0.6.7z`**：包含 LAV Filters 0.83.0、madVR 0.92.17、MKVToolNix 102.0。
- **精简包 `1.0.6-Lite.7z`**：不包含上述三个组件；保留 Qt、Python / VapourSynth、3FP、FFmpeg、图片处理、BD 原盘菜单和更新运行时。需要时可在设置的“组件下载”中安装可选组件。
- 两包均解压后运行 `VSRenderer.exe` 或 `vs-player.exe`，无需另装 Qt、Python 或 VapourSynth。覆盖升级前请保留自己的配置与预设；发行包不包含个人配置、媒体索引、缓存及用户自定义 VPY。
- 对照同名 `.sha256` 文件可校验下载完整性。系统要求为 Windows 10 22H2+ x64 与支持 D3D11 的显卡；增强版 FFmpeg 需要 x86-64-v3，图片转换后端需要 AVX2，Vulkan 滤镜与 RIFE 需对应驱动。

### 播放与 BD 修复

- 修复进度条点击后回弹、拖动丢失最新目标、定位后恢复播放时机；修复长 BD 拼接节目无法定位，并增加关键帧前置解码。
- 加宽 BD 播放模式对话框，修复原盘菜单选集接回旧位置、菜单节目恢复滤镜与 Tab 信息。
- 修复原盘菜单切换英语、韩语、繁中、简中仍显示日语；拼接节目保留原盘 CLPI 的轨道 PID 与语言，五种语言逐一核对字幕图像。
- BD 内容树提供整卷连续播放、剧集、花絮、独立片段和折叠音频组。优化原盘菜单节目切换，支持鼠标操作及重新进入菜单。
- 播放列表固定时在右侧停靠并增宽窗口；底部操作栏优先，鼠标移到底栏右侧不再弹出悬浮列表。
- 音频自动显示封面和歌词：同名外部歌词优先，其次内嵌歌词。兼容大小写标签、容器标签、ID3 / MP4 歌词及文本字幕轨；BOM 文件保留首行。支持外部 MKS 字幕轨和普通章节跳转。

### 图片与编辑器

- 文件夹图片列表递归识别子目录，可展开文件夹；最后一张继续向右回到第一张并显示提示。
- 工具变体下拉移到控件右下角；标尺显示端点 X，可调整起终点或整体拖动。
- 文字支持双击编辑，以及字体、字号、斜体、加粗和行距调整。

### 8K 硬解预取

- 将硬件解码表面的队列预算从 128 MiB 增至 768 MiB，软件帧预算保持 128 MiB，保留八帧上限与 150 ms 预取目标。避免 8K P010 单帧约 94.9 MiB 时仅能提前解码一帧、解码和呈现相互等待。
- 本机 AV1 8K 10-bit / 47.95 fps 的 Jinc 小窗口、Jinc 3840×2160、D3D11 3840×2160 三组 60 秒稳定段约 47.95 fps，零新增丢帧，剩余预取队列六帧。最新 BD / 定位 / 字幕 / 精度回归 30 项通过，部署后专项复核 7 项通过。
- **RX 6600 同片实机仍需复测**；本机结果不代表该显卡已验收。8K 软件解码与所有补帧模式的持续实时性能也未全部达标。

专项条件与边界见 [8K 队列记录](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.6/docs/hardware-8k-queue-1.0.6.md)、[BD 定位验收](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.6/docs/player-bd-seek-acceptance-1.0.6.md)、[菜单字幕验收](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.6/docs/bd-menu-subtitle-languages-1.0.6.md)。

````````
<!-- end:docs/release-1.0.6.md -->

<a id="record-docs-release-1-0-7-md"></a>

## docs/release-1.0.7.md

原始字节数：3946；SHA-256：`737e74ae0773edcf7d26786770bccac46ef296df2623fa2a193555a3bc9a2edf`。

<!-- begin:docs/release-1.0.7.md -->
````````markdown
VS Renderer / VS Player 1.0.7，Windows x64 便携版。

### 下载选择

- **Full `1.0.7.7z`**：包含 LAV Filters、madVR、MKVToolNix。
- **Lite `1.0.7-Lite.7z`**：不带上述三个组件；保留 Qt、Python / VapourSynth、3FP、FFmpeg、图片处理、BD 原盘菜单和更新运行时，可从设置的“组件下载”补装可选组件。
- 解压整个目录后运行 `VSRenderer.exe` 或 `vs-player.exe`，无需另装 Qt、Python、VapourSynth。发行包排除个人配置、缓存、截图和自定义 VPY，含全文件 SHA-256 清单；同名 `.sha256` 校验压缩包完整性。
- 系统要求：Windows 10 22H2+ x64、支持 D3D11 的显卡与驱动。增强版 FFmpeg 需要 x86-64-v3，图片转换后端需要 AVX2；Vulkan 滤镜与 RIFE 需要对应驱动。

### 高精度截图与保存路径

- 源画面和实画面统一保存 PNG 每通道 16-bit，采用自适应滤波与最高无损压缩级别；修复 FP16 渲染表面读回，保留源帧与图片已有精度，后台保存减少界面等待。
- 设置 → 基本设置可修改默认截图目录；右键“截图到指定路径…”可为本次源画面或实画面选择文件夹，不改变默认目录。
- HDR 默认保存经过色调映射的 sRGB SDR 预览，普通 SDR 查看器可直接查看；另存 `.hdr.png`、色彩 JSON 与未截断 `.png.rgba32f` 原始浮点数据。HDR 色准测试应使用原始 HDR 图或浮点附件。
- 浮点附件保留负值和高光；文件记录实际输入 / 读回位深，16-bit 保存不会增加原始表面的精度。madVR 当前接口读回仍为 8-bit，原盘菜单专用截图入口尚未提供。

### 底栏与信息面板

- 算法改为底栏标识框：超分显示 `A4KCNN+ / A4KCNN / A4K+ / A4K / Jinc / D3D11`，补帧显示 `Jinc / D3D11` 加 `RIFE+ / RIFE / MVT+ / MVT` 两个框，随当前实际档位更新。
- 底栏和播放列表透明度可分别设置；全屏控制栏自动隐藏，鼠标进入实际底栏范围即可唤出，顶部可唤出标题与退出全屏。
- Tab 优先显示正在使用的文件，文字可滚动，透明度条独立位于下方；新增音频简略 / 详细模式，详细面板显示真实输入 / 输出声道峰值电平。

### 软件解码与字幕

- HEVC / AV1 软件解码线程上限提升至 32，软件帧预算提升至 512 MiB；独立解码线程与会话取帧提交分开运行，避免解码尖峰阻塞已有帧呈现。硬件帧预算仍为 768 MiB，保持八帧上限和 150 ms 预取目标。
- 修复音频先结束时最后视频帧滞留；本机 HEVC 444p10 高码率样本完整播放接受 2252 帧、零丢弃 / 合并。其它硬件和素材的持续实时性能仍需实测。
- 自动匹配同目录的字幕名称变体，可识别 `xxx.sc-jp.ass` / `xxx.tc-jp.ass`，默认加载最匹配的一份，其余在首 / 次字幕菜单切换；排除明显集数冲突。

保留 1.0.6 的 BD 模式窗口、选集、长节目定位、原盘五种字幕语言与歌词等修复。RX 6600 同片实机性能仍待复测，8K 软解及所有 RIFE / MVTools 持续实时性能尚未全部验收；两项既有完整色彩测试存在阻塞，不能宣称完整色彩或显示器物理色准验收通过。

本次截图 / 路径 / 算法标识专项 18 项、相关回归 15 项均通过，0 失败、0 跳过；语言完整性检查 536 条通过。压缩包在独立目录解压后核验全文件 SHA-256、组件差异及启动，个人配置与预设保留。

格式与测试边界见 [高精度截图](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.7/docs/player-high-precision-screenshots-1.0.7.md)、[透明控制栏与音频信息](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.7/docs/player-transparent-audio-info-1.0.7.md)、[软件解码与字幕](https://github.com/user-Wing/VS-Renderer-GUI/blob/v1.0.7/docs/software-decode-subtitles-1.0.7.md)。

````````
<!-- end:docs/release-1.0.7.md -->

<a id="record-docs-release-1-0-8-md"></a>

## docs/release-1.0.8.md

原始字节数：1554；SHA-256：`57980ef005b649a8c971c42bfd0816b21580e2fdcc9b62b82a659eb36d9f9bdc`。

<!-- begin:docs/release-1.0.8.md -->
````````markdown
# VS Renderer / VS Player 1.0.8

## 更新内容

- 使用原生 PhotoCraft 编辑器替换原 Qt 图像编辑界面，保留 VSP 高精度图片与 HDR 浮点数据支持。
- 编辑器接入原有超大 PNG 解码链路，保留完整原始尺寸；大图主动使用适合内存/显存预算的预览，100% 缩放按可见区域读取原始像素。
- 图片启动延迟加载视频、滤镜和字幕后端，减少中小图片打开等待。
- Renderer 双路对比默认使用 D3D11 原生缩放，780M 双路 4K 47.95 fps 样本稳定接受约 47.90 fps，测量窗口零丢帧。
- 验证 HEVC RExt 10 位 4:4:4 的 NVDEC/CUDA 回退，保留正常 D3D11VA 路径。
- 修复 BD 整卷跳转、模式对话框宽度、原盘菜单分集和字幕语言选择；实盘回归 22 项通过。
- 修复高帧率直通初始播放时钟，以及全屏顶部栏、信息面板和播放列表的交互优先级。
- CPU 预缩放为默认关闭的实验功能；8K AV1 性能尚未验收，不作为本版加速保证。

## 压缩包

Full 包含完整便携组件。Lite 排除 LAV Filters、madVR 和 MKVToolNix；两者均包含 PhotoCraft、播放器、渲染器及必要运行库。运行编辑器无需安装 Rust。压缩包排除个人设置、缓存和编辑器用户数据，并附 SHA-256 校验文件。

手动更新的 FFmpeg 保留。正式 Release 和二进制资产由维护者自行上传；本次更新源码和提供本地压缩包。

详细验收与边界见 `performance-large-images-1.0.8.md` 和 `photocraft-editor-1.0.8.md`。

````````
<!-- end:docs/release-1.0.8.md -->

<a id="record-docs-renderer-panels-1-0-3-md"></a>

## docs/renderer-panels-1.0.3.md

原始字节数：4249；SHA-256：`466aa2d2bb55ed5bf2e31fbefe8193c2dac73b7f6c1309281b85668f7e585487`。

<!-- begin:docs/renderer-panels-1.0.3.md -->
````````markdown
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

````````
<!-- end:docs/renderer-panels-1.0.3.md -->

<a id="record-docs-renderer-ui-export-1-0-3-md"></a>

## docs/renderer-ui-export-1.0.3.md

原始字节数：4896；SHA-256：`b562596ff80e47cd50ef78501d5595dfaa4c3cec11f94ff776a68733e514a216`。

<!-- begin:docs/renderer-ui-export-1.0.3.md -->
````````markdown
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

````````
<!-- end:docs/renderer-ui-export-1.0.3.md -->

<a id="record-docs-software-decode-subtitles-1-0-7-md"></a>

## docs/software-decode-subtitles-1.0.7.md

原始字节数：6115；SHA-256：`999fb6fa74bc8b005b39e9632b798633b18fbe41b3cd75ffd364efb9b626f934`。

<!-- begin:docs/software-decode-subtitles-1.0.7.md -->
````````markdown
# 软件解码与外部字幕修复 (1.0.7 本地版)

日期：2026-10-05。本轮只修改和验证本地源码及便携运行目录；不打包、不推送、不上传，也不改变已有 GitHub / ModelScope 版本和草稿。此前尖峰诊断保留在 [1.0.6 记录](hevc444-peak-diagnosis-1.0.6.md)。

## 解码和队列

- HEVC 与 AV1 软件解码使用 `min(32, CPU 逻辑线程数)`。本机为 32 线程；AV1 优先使用现有 libdav1d，其余解码器的线程策略保持原样。
- 软件解码帧预算为 512 MiB (536870912 字节)，硬件帧仍为 768 MiB。保持最多八帧及 150 ms 预取目标。预算不会预先分配成固定内存块，也不包含 FFmpeg 内部帧线程、参考帧、GPU 工作纹理等内存。
- HEVC 444p10 队列仍引用原始平面帧。3840×2160 的估算为每帧 47.461 MiB；Y410 的打包尺寸与该 CPU 队列占用不同。未改变位深、色度采样或渲染算法。
- 独立软件视频线程执行送包 / 取帧，向会话线程交接一个有界帧引用。会话线程继续负责已解码队列、按时提交、音频、播放时钟和快照。FFmpeg 包读取与解码调用使用互斥；读取侧只尝试获取锁，解码忙时返回播放循环，继续呈现已有帧。
- 暂停保留有限预取；跳转、换视频轨、设备重建及关闭先结束 / 丢弃旧任务，再操作解码上下文。定位过程保留有界音频预读。片尾分别清空视频与音频延迟数据；音频先结束时，剩余视频以最后音频位置衔接墙钟呈现。

原生修改保存在 `patches/3fp-software-decode-pipeline.patch`，`tools/build-3fp.ps1` 在已有队列补丁后应用。完整流水线的反向检查用于跳过上下文已被覆盖的旧补丁，避免重复构建再次应用失败。没有修改 C ABI 或快照版本。

## 字幕匹配

在视频同目录扫描 ASS / SSA / SRT / SUP / VTT / MKS；按完整名称、完整文件名前缀、原有词匹配和有效名称片段排序。拉丁名称片段至少三个字符，中文等名称片段至少两个字符；语言缩写和纯数字不能单独作为模糊匹配依据，显式集数及裸集数冲突会被排除。

`xxx.mkv` 与 `xxx.sc-jp.ass`、`xxx.tc-jp.ass` 可以一起识别。默认加载排序最前的一份，另一份及其它匹配结果出现在右键“字幕设置”的首 / 次字幕选择中；相同匹配分数不再导致全部跳过。没有内置字幕的视频也适用。音频自动挂载的歧义规则保持原样。

部分名称本身无法保证语义上属于同一影片；完整名称和集数优先，低分片段匹配仍可在菜单中手动切换。

## 验证

使用 HEVC / AV1 444p10 的 96 帧音视频样本，音频短于视频，检查 32 线程、512 MiB、暂停、前后定位、近片尾定位、按帧定位、恢复播放、重新打开和播放中跳向片尾；完整播放要求 96 帧全部解码、末帧序号 95、零丢弃 / 合并、队列清空及 10-bit 输出。

字幕用例覆盖两份 ASS、短名称、包含名称片段、不同集数排除、无内置字幕自动挂载，以及通过右键切换后实际字幕图像变化。回归还包括外部音轨、内置 ASS / SRT 定位、字幕刷新、音频文件、VS 音频时钟、输出格式、8K 硬解队列和硬解 / Shader 捕获。软件像素对照的最大 RGB 差为零。

独立测试核心在软件解码调用内持锁停顿 120 ms，期间仍有 **3 帧**由会话线程提交，证明取帧提交不再随软件解码调用一起阻塞。测试停顿代码只在隔离核心中使用，未进入最终补丁。原始记录位于 `build/soft-decode-1.0.7/stall.log` 和 `stall.csv`。

最终相关回归：**27 passed, 0 failed, 0 skipped**，耗时 81.834 秒，包括普通视频 / MyGO BD 的 HW/SW 鼠标跳转、五种 PGS 字幕、外部字幕的 VS / 显示截图。日志为 `build/soft-decode-1.0.7/acceptance-final.txt`。测试不再以字幕菜单的最后一个动作代替实际选中项，并显式启用 VS 验证源帧截图。首轮曾有一个启动等待超过 5 秒的用例；功能检查改为与打开阶段一致的 15 秒等待，不作为启动性能指标。

本机 7940HX / RTX 4070 Laptop、平衡电源模式，播放原文件 `D:/RebellionHEVC444p10-700MBPS-SoftwareLast.mkv`，采用 3840×2160 输出和软件解码。97 秒采样覆盖完整 94.052 秒视频及片尾：**2252 帧解码、2252 帧接受、零丢弃、零合并、片尾队列为零**，输出持续为 10-bit。首帧约 1.278 秒；43–47 秒及 79–83 秒峰段分别接受 96 / 95 帧，均零新增丢帧，采样队列为 3–4 帧。原始记录为 `hevc32-accepted.csv`、`hevc32-accepted.log`、`peak-summary.json`，均在 `build/soft-decode-1.0.7/`。

首次完整播放检查发现最后一帧因音频先结束而滞留；已修复，并通过音频短于视频的 HEVC / AV1 回归及原文件完整播放确认。此前失败和中间诊断日志保留，最终核心不包含停顿或调试输出。其它处理器、显卡及低核心数设备未做实机验收，不能据本机样本承诺所有软解素材零丢帧。

## 本地交付

更新 `dist/VS-Renderer-GUI-windows-x64` 中的 Player、Renderer、原生 DLL、`release.json`、使用说明及本记录，资源哈希清单同步更新。最终原生 DLL SHA-256 为 `7a412e006eafb92992d05bd4bf423a0e61497af0a7b8f7cf9d707831ac6d605b`。补丁反向检查、从已有 1.0.6 补丁层重新应用构建及重复构建通过；最终重复构建产物哈希相同。

实际加载部署后的 DLL，HEVC、AV1 与字幕变体复测为 **5 passed, 0 failed, 0 skipped**；日志为 `build/soft-decode-1.0.7/deployed-smoke.txt`。部署前后 49 个 INI、VPY 和色彩文件哈希一致，审计为同目录 `deployment-audit.json`。`player.ini` 在部署前已不同于开工快照，采用部署时最新内容作为保护基线并保留，没有回滚当前设置。

没有生成压缩包、提交或推送源码，也没有修改任何线上版本、发行草稿或附件。

````````
<!-- end:docs/software-decode-subtitles-1.0.7.md -->

<a id="record-docs-tracks-size-1-0-4-md"></a>

## docs/tracks-size-1.0.4.md

原始字节数：4066；SHA-256：`8595f933dfbdd89c74fff21743e3b5d3d7d0c6773e8e13c42eeddf2f63930fd9`。

<!-- begin:docs/tracks-size-1.0.4.md -->
````````markdown
# 1.0.4 轨道信息与部署体积（2026-10-03）

## 轨道

菜单显示 `Track 0:a:0 - 名称 - 语言 - Default/Forced/Default+Forced`，字幕使用 `0:s:0`。同类编号从0递增，点击映射到真实全局流编号；编码格式、声道数和全局编号放在提示中。无标记显示None，无标题使用编码格式，无语言使用und；可用时优先显示language-ietf，不根据轨道标题猜语言。

后端原本提供metadata/default/forced，菜单却读取tags，导致标题和语言缺失。现在优先读取metadata并兼容旧tags，无需引入MKVToolNix。

多个Default选择同类全局流编号最小者，Forced不参与默认选择；无Default时音频和字幕均选择同类首条（0:a:0 / 0:s:0）。默认选择在自动播放前完成，手动切换仍有效。已挂载/自动匹配的外部轨道继续使用现有优先规则，字幕显示开关保留。

## 部署与体积

按维护者指定的空间分析区分程序文件和用户缓存，不用SFX整包展开，不把整个程序解压到内存。

- FFmpeg CLI位于程序根目录，与3FP共用同一组DLL；runtime/ffmpeg仅保留README/build-info，减少约188 MiB重复运行时。Player图片兜底解码、截图、Anime4K输出、字幕提取和Renderer导出均使用新位置；导出兼容旧隔离布局及显式绝对路径。
- 915份GLSL/HOOK原始源码在开发目录保留，发行目录使用约59 MiB的 `shaders/mpv-shaders.7z`：LZMA2、64 MiB固实块、32 MiB字典。清单和许可证仍为普通文件，菜单无需解压。VPY只通过随包7-Zip读取所选文本并传shader_s，不建立整库临时目录；VS解释器缓存最近一份文本，按归档mtime更新。自定义普通GLSL直接读取。
- 基础Anime/Jinc/D3D11的普通着色器保持原路径。手写VPY若把压缩库的旧路径直接传给placebo的shader文件参数，需提取相应文件或用Renderer重新生成VPY；当前随包/用户VPY未发现这种引用。
- 本项目VS core以enable_x86_asm=false构建，不编译自动ISA选择分支；实际确认std/zsmooth/vszip加载基线DLL，只移走当前不会使用的AVX2/ZN4副本，保留所有滤镜与RIFE模型。裁剪脚本先验证模块选择，检测到优化版正在使用则保留ISA副本。
- x64播放器加载madVR64.ax；只移走两个debug AX和32位madVR.ax，设置控制程序、对应依赖和settings.bin保留。移走文件暂留build备份，清理BAT可由维护者删除。

程序文件约596 MiB / 625 MB，现有用户索引缓存约34 MiB保留，整个便携目录约630 MiB / 661 MB。数值是普通文件长度，不是归档大小或NTFS压缩占用。没有删除有效缓存来虚报程序精简；后续缓存增长会改变目录总量。

逐帧使用的DLL、模型及Shader数学内容不变。额外读取发生在首次加载所选压缩Shader时，最坏解码所在固实块；缓存命中不再启动解压进程。未在当前高负载环境上作FPS或冷启动性能结论。

## 构建与验证

stage-ffmpeg.ps1部署共享DLL布局，stage-shaders.ps1生成并校验资源库，trim-portable.ps1验证实际模块并裁剪；stage-portable.ps1调用三者。已有压缩库时CMake不再复制原始整库；修改Shader库源码后应重新执行stage-shaders并复查。

真实MKV三音轨/三字幕轨、中文名称/语言、多Default与Forced组合的标签、自动选择、手动切换通过。压缩库实际Vulkan出帧、共享FFmpeg AVS3编解码/呈现、图片算法和Anime4K输出、外部轨道、应用设置保留状态通过；GLSL分类/脚本生成及旧导出CLI兼容仅跑相关专项。裁剪后便携Python的std、zsmooth.Median、vszip.BoxBlur实际出帧和完整Shader归档CRC检查通过。

日志：build/track-size-final-tests.txt、build/compressed-shader-final.txt、build/track-size-graph-tests.txt、build/track-size-export-tests.txt。配置、预设、大小和部署哈希见build/track-size-delivery-audit.json。保持1.0.4，不推送GitHub，不生成完整发行归档。

````````
<!-- end:docs/tracks-size-1.0.4.md -->

<a id="record-docs-ui-layout-spec-md"></a>

## docs/ui-layout-spec.md

原始字节数：4704；SHA-256：`0921ed91263cf5b086b8ecba9949821bfdb6c31079428a16634e8976ec3fce6d`。

<!-- begin:docs/ui-layout-spec.md -->
````````markdown
# C++ / Qt 主工作台 UI 模式（冻结基线）

本规格在 UI 代码之前确定。目标是 Snipaste/Windows 资源管理器式高信息密度：矩形层级、4 px 小圆角、无背景 blur、无装饰性大卡片。所有尺寸为 Qt device-independent pixel，由 Qt 的 DPI 缩放统一换算。

## 1. 窗口与网格

- 最小窗口 `1120 x 700`，推荐初始 `1440 x 900`；低于最小值禁止继续缩小。
- 根布局自上而下：命令栏 `44`、内容区 `1fr`、传输区 `76`、状态栏 `24`；四区不可相互覆盖。
- 内容区水平 splitter：左 dashboard 默认 `328`、允许 `296..460`；右工作区最小 `720`。splitter handle `1`，命中区由样式扩至 `5`。
- 基础间距：外边距 `8`，组间 `8`，控件间 `4`；文本基线按 `4` 的倍数对齐。
- 字体：Segoe UI Variable/Segoe UI，正文 `13`，辅助 `12`，区标题 `13 semibold`；不依赖固定像素字符宽度。

## 2. 顶部命令栏

- 高度严格 `44`，左右 padding `8`；按钮高 `32`，最小宽 `32`，图标 `16`。
- 从左到右：打开源、打开项目、保存项目；分隔线；撤销、重做；分隔线；重新生成/验证脚本、渲染预览；伸缩空白；运行时状态、设置。
- 按钮文字空间不足时，次要按钮先仅保留图标；主按钮“打开源”“渲染预览”不得被裁掉。

## 3. 左侧三段 dashboard

三段均有 `32` 高标题行和 chevron，可独立折叠；标题固定，内容在单一纵向滚动区内，禁止三重滚动条。

1. “输入与环境”：源文件、source filter、运行时探测、色彩/位深摘要。展开默认高度按内容，不能挤压标题。
2. “滤镜库与处理链”：上半部搜索+分类，下半部有序节点列表；列表为剩余高度的唯一伸缩项。节点行 `34`，含启用、名称、拖动柄、删除。
3. “参数与脚本”：选中节点的表单；label 列最小 `104`，editor 列伸缩，行高 `32`。底部“查看 VPY”固定在内容尾部。

折叠后只保留标题行；至少一段展开。左栏窄于 `320` 时参数表单改为上下排，不能水平遮挡。

## 4. 双路预览区

- 工作区顶部比较条 `36`；中间为水平 splitter 的两个预览 pane，默认 `1:1`，单 pane 最小 `320 x 240`。
- pane 顶部标题条 `32`：左为“源视频”，右为“处理后”；右端显示缩放倍率和格式。其下原生渲染 surface 填满余量。
- surface 背景 `#101010`；无媒体时显示居中诊断文字，但不得盖住原生 HWND；创建 3FP 会话后隐藏诊断层。
- 分隔线 `1`，不使用圆形遮罩、阴影、blur。焦点 pane 只用 `2` px accent 内描边。
- 鼠标滚轮以指针位置为锚进行 `25%` 级缩放，范围 `25%..1600%`；倍率大于 `100%` 时左键按住拖拽平移，双击回到 Fit。两路默认同步缩放/平移。
- 指针停留时像素读数放在 pane 左下角 `8` px 内边距的小型不透明浮层，保证不越过 surface。

## 5. 底部传输区

- 第一行时间轴 `36`：左时间码 `92`、中 slider 伸缩、右总时长 `92`；slider hit target 至少 `24`。
- 第二行控制 `40`：居中为上一帧、播放/暂停、下一帧；左侧比较模式，右侧缩放算法与倍率。控件高 `28..32`。
- 主窗口激活时，未带修饰键的空格由应用级事件过滤器优先处理，始终映射为播放/暂停，不能触发当前获得焦点的按钮。
- 状态栏 `24`：左侧任务/错误，右侧显示帧号、FPS、3FP/VS 状态。错误使用文字+图标，不只靠颜色。

## 6. 响应与遮挡规则

- `1120..1279`：命令栏次要动作折入菜单；dashboard 保持 `296`；两 pane 仍并排。
- `<1120` 不支持，窗口停在最小宽；不自动改成上下布局，避免比较肌肉记忆变化。
- 所有编辑器设置合理 minimum width；路径文本中间省略，完整值放 tooltip；禁止用绝对坐标摆放业务控件。
- 弹出菜单/tooltip 可覆盖内容；常驻 widget、native surface、状态浮层之间不得相互覆盖。
- DPI 切换后重新约束 splitter 比例；原生 child HWND 的 geometry 必须跟随 Qt `resizeEvent`，然后调用 `FFF3FP_Redraw`。

## 7. Fluent Design 使用边界

- 使用系统 accent、Segoe、清晰 hover/pressed/disabled 状态、键盘 focus rect 与 `32` px 命中高度。
- 圆角仅 `2..4`；不使用 Mica/Acrylic/blur，画面区保持中性黑。
- 浅色：背景 `#F3F3F3`、面板 `#FFFFFF`、边线 `#D1D1D1`；深色后续同样遵守对比度，不在 MVP 自动跟随系统。
- 动画仅允许折叠与状态切换 `<=120 ms`；首版可无动画，不能阻塞渲染线程。

````````
<!-- end:docs/ui-layout-spec.md -->

<a id="record-docs-update-1-0-11-md"></a>

## docs/update-1.0.11.md

原始字节数：18328；SHA-256：`6f8579385172994dca02d0e76be68466b753e9dd2387b0442a857d0283417df8`。

<!-- begin:docs/update-1.0.11.md -->
````````markdown
# 1.0.11 本地更新与验证（2026-10-17）

本轮只修改本地代码、编译和更新运行目录；不推送 GitHub、不创建或修改 Release、不制作发布压缩包。追加要求生成独立单文件 `dist/VS-Player.exe`，首次运行自动创建同目录 `player.ini`。Full 固定目录为 `dist/VS-GUI-Full`，独立播放器目录为 `dist/1.0.11-PlayerLite`（原 1.0.10 目录整体改名，保留配置）。现存旧压缩包不修改。

## BD 菜单与章节

MyGO 下卷 `BRMM_10775` 的 authored FirstPlay 是约 9.009 秒版权片；Top Menu 是另一入口。首次打开光盘保留原始 FirstPlay；显式点击“BD 菜单”时，改用 Top Menu 索引对象建立独立导航缓存，避免重新播放版权片。仅修改缓存入口，原盘 index/MovieObject/素材不变。libVLC 切换 title 的同步调用移至后台，Qt 继续处理其原生视频窗口消息，关闭时等待切换完成后再释放播放器。

普通文件的上游章节字段是 `startTime100ns`，GUI 原先读取 `start100ns`，所有标记因此重叠于开头。现于 3FP GUI 适配层统一字段，BD/外部 MKS 章节仍沿用统一格式。

实盘回归：MyGO 正常首播后选择第 9 集通过；显式菜单入口以及从第 9 集返回菜单后再次选择第 9 集通过，正片位置约 1421.5 秒。三章节文件的 0 / 2 / 4 秒坐标通过。日志 `build/audit-20261017/player-entry-final.txt`：5 passed（包含 init/cleanup）。

## Renderer、预设与公开社区

- 右侧控件顺序：视频路径 / 浏览 / 前一帧 / 播放 / 后一帧 / 并排 / 缩放。路径框最大 320 逻辑像素；缩放框按完整文本分配宽度，D3D11 不截断。当前时间、缩短后的进度条、总时长同排。
- 预设保存全部时间组、名称、整秒边界及滤镜链；全片单组预设可导入任意当前组，保留目标组和其它组的边界。多组预设不再静默只取第一组覆盖当前组，应使用“导入所有时间组”。
- 内置播放器 VPY 的额外处理逻辑以脚本随时间组保存，避免空 nodes 元数据导致实际处理逻辑丢失；组内脚本使用独立 Python 命名空间运行。可视化滤镜修改后使用更新后的可视化链。没有 VSR 元数据的任意外部 VPY 保留“导入全部”方式。
- 社区默认配置和显示链接始终是公开页面：<https://www.modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI/PresetCommunity>。内部目录请求与对用户显示的源地址分离。递归扫描子目录，树状展开/收起，下载保留相对路径，支持选中目录或下载全部；继续校验大小和 SHA256。
- 本地预设管理也递归扫描 `vpy` 子目录并显示目录树。

Renderer 回归：布局、时间组脚本实际帧请求（边界 0/23/24/47/48/100）、当前/全部组导入、脚本组保存恢复、公开社区 Builtin 下载及导入、预设管理、启动时预设渲染，共 9 passed（包含 init/cleanup，并补测子目录内重命名保持目录）。日志 `build/audit-20261017/renderer-complete.txt`。

## VapourSynth 后端

从用户提供的 `VapourSynth64-Portable-R81.zip` 提取并校验官方 ABI3 wheel，更新 Python 3.15 运行环境中的核心，保留原有滤镜。加入 BM3D 11.0、BM3Dvk2 1.0、BestSource 22；版本与 SHA256 固定在 `tools/update-vapoursynth-runtime.py`，运行目录内保留 `runtime-components.json` 和许可证。构建及 staging 流程同步更新，后续编译不会重新落回 R80。

已实测：R81 加载成功；CPU BM3D 时域 VBasic → VAggregate → VFinal → VAggregate 成功；BM3Dvk2 的基本/最终估计、radius=1 时域处理成功，输入及中间节点保持 GPU 常驻；BestSource `gpu=True, gpufallback=False` → Vulkan resize RGBS → BM3Dvk2 → GPUDownload 成功，未静默回退软件解码。设备为 RTX 4070 Laptop GPU，Vulkan 1.4.351。日志 `test-r81.txt` / `test-r81-decode.txt` 位于本轮 audit 目录。

R81 官方说明是导出给 CUDA/Vulkan 的输入帧每个源帧至多复制一次并共享请求，并不等于已经证明整个 Vulkan 解码 → 降噪 → CUDA 链路只拷贝一次。此次没有测量 CUDA 导出次数，也没有把 GUI 的 CPU 帧桥改成 CUDA 零拷贝桥；现有默认生成的 FFMS2/L-SMASH 源保持原有行为。新 GPU 源/滤镜可用于 VPY。

来源：[R81](https://github.com/vapoursynth/vapoursynth/releases/tag/R81)、[BM3Dvk2](https://github.com/HolyWu/VapourSynth-bm3dvk2)、[BM3D](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-BM3D)、[BestSource](https://github.com/vapoursynth/bestsource)。

## 上游性能策略核对

截至本轮读取 GitHub API 和源码，上游主线仍是 `0fe651ddab8a2bb6ddb729f314f9f4c5117fb565`（API20、PR22）；最新打开的 PR23 是颜色命名调整，不是 CPU 调度优化。本地已包含 PR22。公开源码可确认线程数阶梯、按内存约束的视频队列和帧对象复用；本地软件队列预算 512 MiB，上游主线仍为 128 MiB，均还受队列帧数上限约束。AV1 本地使用 dav1d 匹配工作线程和帧上下文深度。

未在公开主线找到 P/E 核分阶段、单 CCD 优先再扩双 CCD 的亲和性/CPU Sets 调度或提高进程优先级实现。因此不能声称这些策略已经全部集成，也无法把截图 31.20 → 46.74 fps 当作本机同条件 +50% 的测量证据。此次未凭描述自行添加亲和性策略。

## 指定 8K 软件解码复测

素材：`D:/Video Compress Test/PlayerTest/ChouKaguya432048-Part.mkv`，7680×4320 AV1、YUV420P10、7001/146 ≈ 47.952 fps，带 AAC 6 声道；Ryzen 9 7940HX，16 核 32 线程。

隔离探针 CPU=1，3840×2160 输出，SDR 映射、音频解码/播放启用并静音，从 10 秒附近开始，剔除开头 5 秒，统计约 60 秒：

| 项目 | 结果 |
| --- | --- |
| 解码输出 | 42.17 fps |
| 提交视频帧 | 32.23 fps |
| 丢弃视频帧 | 596 |
| 音频欠载增量 | 204 |
| 播放时钟推进 / 实际时间 | 0.892 |
| 冷首帧 | 5.95 秒 |

同一片段纯 FFmpeg / libdav1d、16 线程、max_frame_delay=16、无音频/字幕/呈现：2877 帧 / 61.483 秒 = **46.79 fps**，约 0.976×。单纯解码距离 47.95 fps 仍差约 2.4% 吞吐；完整播放提交帧距离目标约少 32.8%，若按所需提升计算约需 48.8%。**当前不能判定 8K 软件播放通过。**

完整 VS Player 界面另行统计 60.011 秒（CPU=1、原始直通、4K 输出、AAC 启用）：解码 **41.03 fps**、提交 **31.03 fps**、丢弃 **599** 帧、音频欠载 **233** 次，播放时钟推进比 **0.868**，末帧落后音频时钟约 **1.71 秒**。距离片源少约 35.3% 的提交帧吞吐，达到目标需要约 54.5% 提升。采样过程正常结束（诊断测试 3 passed 仅代表采样有效，**不代表流畅播放通过**）。日志为 `8k-player-software-test.txt` / `8k-player-software.csv` / `8k-player-summary.json`。隔离探针详情 `8k-summary.json` / `8k-software-api20.csv`；纯解码日志 `8k-decode-only.txt`。最初误用 GPU 模式的探针文件已明确命名 `8k-hardware-invalid.*`，排除于所有软件性能结论。

## 最终构建与本地部署

Release 构建完成，Full 的 Renderer / Player / VSCap 及独立 PlayerLite 均标记 1.0.11。聚焦验证：Renderer 9 passed、Full Player 5 passed、Blu-ray 解析及缓存入口 7 passed、Lite 首播及无 VS 软件播放 4 passed（均包含 QtTest init/cleanup）。R81 的 CPU/GPU 滤镜与 GPU 解码验证另见上述日志。没有据此宣称全量测试套件通过。

部署使用 `tools/deploy-local-runtime.py`，对 EXE/DLL/运行时文件采用临时文件原子替换并备份，校验 INI、VPY、ICC/ICM 和 LUT 哈希不变；更新 `release.json` / `resource-manifest.json`，旧压缩包不修改。实际部署审计位于本轮 `build/deployment-*/deployment.json`。

## 仍需人工验收 / 旧失败记录

旧综合测试 `bdMenuFromEmptyWindow` 的系统光标/弹出菜单位置断言在当前不可交互桌面失败；旧 `frameEditTakesPriorityAndPrefetchKeepsGpuBusy` 的 frameEditPending 超时也仍保留失败记录，不能把本轮聚焦回归当作全量套件通过。前轮长光盘跨段 seek、UIAccess 置顶覆盖与真实 HDR 的待验收项没有在本轮宣称解决。

## PR23、单文件播放器与菜单入口续修

合入 [PR23](https://github.com/Lake1059/FFF_Project/pull/23) 的固定提交 `ce195ebd06e9cbb8352a443f0c874cf569a60733`，保留本项目 Native 扩展并重新生成可复现补丁，`git apply --check` 通过。对 PR22 基线的 Native 差异均为注释/命名措辞，ABI 和执行语句没有变化；上游 VB Host 的外部色彩注入模块属于其宿主界面，本项目 Qt 宿主没有因此自动获得该 VB 功能。不把本次合入宣称为新增性能优化。完整 IAMF 构建维持启用。

实际 computer-use 从右键“打开 BD 文件夹”选择 Downloads 中的 MyGO 下卷目录，首播约 9 秒版权片后能显示 PLAY ALL、第 8～13 集和字幕设置。复现并修正遗漏的右键“BD 主菜单 / 弹出菜单”入口：先前从 3FP 正片回菜单仍调用默认 FirstPlay，导致再播版权片；现在与播放列表“BD 菜单”共用直接返回方法，暂停时也恢复播放。新版实际画面验证两次返回均直接显示选集页，点击第 9 集可播放并交回 3FP。原盘文件不改，首次开盘仍遵循原始 FirstPlay。

聚焦测试：Full 返回/选集 4 passed；Lite 全部 7 passed；Blu-ray 解析 8 passed / 3 skipped（未设置对应实盘测试环境变量）。这不代替尚未解决的历史长光盘 seek 或全量测试套件。日志位于 `build/audit-pr23-20261017/`。

独立版固定输出 `VS-Player.exe`，同时嵌入 Qt、FFF.Native、FFmpeg shared DLL/CLI、libass、原生 BD 导航及 Lite 自身依赖，不再要求用户另外放 FFmpeg DLL。运行时首次解到 `%LOCALAPPDATA%/VSPlayerLite/<payload-hash>/` 并复用缓存；这是单文件分发，不是完全不解压的静态链接程序。配置固定在启动器旁 `player.ini`，现有配置不覆盖。仍保持 Lite 功能边界，不加入 VapourSynth、libVLC、LAV、madVR 或高级色彩运行时。

## 音频 / ASS 为什么影响视频：本轮受控对照

不能把 AAC/ASS 的低压缩解码成本等同于整条播放管线零成本。Native 的主 Pump 同时处理解复用、音频补给、音画时钟和视频提交；软件视频解码虽另有线程，输出仍是单槽交接，队列满或主 Pump 不能及时收帧时会反压解码线程。ASS 则另外打开容器、读取字幕包/字体，libass 排版栅格化后还要合成/上传图层。这里的“decoded fps”也是整条管线约束后的输出计数，不能当独立解码器满速基准。这些是源码中的耦合点，尚未做逐阶段耗时追踪，不能把其中某个点说成唯一根因。

本轮以新版 Full 实际播放器、CPU 解码模式 1、原始 D3D11、固定 3840×2160 输出、媒体 10 秒附近跳转后预热 5 秒、采样 20 秒进行对照。AAC 保持解码及音频输出，字幕开关仅改变是否呈现；均不改原片。两份 8K 原片抽取的视频压缩包 SHA256 相同（`3a267183...d381360a`），排除了不同视频编码内容的影响。

| 同条件样本 | 解码输出 fps | 视频提交 fps | 丢帧增量 | 音频欠载 | 平均 CPU 核当量 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 8K 纯视频 | 41.34 | 31.99 | 187 | 0 | 16.14 |
| 同码流 8K + AAC 6ch | 40.91 | 30.95 | 200 | 87 | 16.14 |
| 4K 444p12 + FLAC，ASS 隐藏 | 47.95 | 47.95 | 0 | 0 | 7.08 |
| 同一 4K 文件，ASS 显示 | 47.93 | 47.93 | 0 | 0 | 8.15 |

8K 的 AAC 对照提交差约 1.04 fps（3.3%），纯视频本身已大量丢帧，不能把此前约 46.79 fps 的无呈现 FFmpeg 和约 31 fps 的完整播放器之差都算成 AAC 开销。有音频时墙钟推进比 0.884、无音频 1.000，欠载说明供给/时钟发生停顿。此次普通 AAC 的 `audioBinaural=0`，不能归因于 OBR/HRTF。

4K ASS 在采样内有 292 次图层更新，CPU 多约 1.07 核，但两组均跟上 47.95 fps、零丢帧；**仅此 20 秒片段**没有复现用户所述高压片段降速。当前文件缓存已热，打开到首次观察到超过两帧约 0.57～0.87 秒，这不是磁盘/着色器全冷启动数据；也不能据此宣称其它字幕字体、片段或首次加载问题全部修复。未凭这些有限对照盲改线程/时钟策略。原始 CSV、日志及摘要为 `build/audit-pr23-20261017/av-subtitle-summary.json`。

补测将原 OP 的 ASS 无重编码封装进同一 8K + AAC 视频，20 秒输出 39.58 / 提交 29.79 fps、丢帧 196、音频欠载 93、字幕更新 144 次、CPU 16.51 核当量。此字幕时间轴沿用 OP，不是 8K 影片的对白，仅用于同类 ASS 渲染负载对照；样本保留在 build/audit 中，不修改原片。与 AAC 单独组相比未出现“大幅降速”，8K 完整软件播放仍未达 47.95 fps。

## 窗口底栏与 VSCap 模态输入续修

Player 窗口模式的原生视频区域从底部扣除控制栏高度，初始窗口高度同时增加同样的高度，底栏位于视频下方。缩放窗口也维持不重叠，进出全屏不累计增高；全屏保留原有覆盖式自动隐藏底栏。信息面板仅在全屏预留浮层底部间距，避免窗口模式重复扣除。

VSCap 设置使用 `QDialog::exec()` 的应用模态输入。此前新建的无父级置顶截图层没有接管模态输入，因此设置阻断截图层，而截图层又覆盖设置，表现为整屏无法操作。现在仅在已经存在模态窗口时，将截图层设为新的应用模态窗口；Esc/完成截图后恢复原设置窗口，未保存的输入仍保留。保存文件对话框以当前模态窗口为父级，避免落到被截图层遮挡的位置。普通截图与贴图的既有模式保持。

点按钮可点击选择边框模式，默认选中；与填充按钮组成互斥组。滚轮事件仍调节线宽且不会取消当前模式，矩形/圆形共用。填充方块使用独立的 20px 图标，其中实心方块为 16px；按钮固定为相邻矩形按钮的高度，不通过增大字体撑高控件。

验证使用自动回归，不调用 computer-use：新增模态输入复现用例在修复前失败（设置占用 activeModalWidget），修复后可框选、按 Esc 并返回设置；边框按钮修复前不支持选中，修复后模式与像素验证通过。透明度/全屏专项的控件 alpha 与布局检查通过，桌面像素合成检查因前台被占用跳过（2 passed / 1 skipped），未为此操纵用户桌面。Windows VSCap 工作流 16 passed；播放器窗口/全屏/底栏回归 5 passed；Lite 模式和软件播放 4 passed（均包含 init/cleanup）。日志位于 `build/audit-controls-modal-20261017/`。离屏复现和 Windows 平台测试分开记录，不以离屏测试替代实际 Windows Qt 事件路由。没有将这些聚焦检查宣称为全量播放器测试通过。

本轮保持 1.0.11，编译并更新 Full、目录式 Lite 和 `dist/VS-Player.exe`；不上传、不制作发布压缩包、不改 Release。部署继续备份替换并校验 INI、VPY、LUT 和色彩配置哈希。

## 单文件 FFmpeg 外置（后续要求，覆盖此前内置方案）

`VS-Player.exe` 移除 FFmpeg CLI、shared DLL 和资源清单中标注的 FFmpeg 专用依赖，保留 Qt、Native、字幕/光盘组件及共同依赖。Full 和目录式 Lite 的 FFmpeg 文件不删除。启动器先查自身目录，再查进程 PATH；缺少 ffmpeg.exe、ffprobe.exe 或匹配 ABI 的 shared x64 DLL 时明确提示并退出，不静默启动一个无法播放的视频后端。静态单文件 ffmpeg.exe 不能替代 Native 所需共享库；当前 ABI 是 avcodec63、avformat63、avutil61、swresample7、swscale10、avfilter12。

启动后的子进程 PATH 包含启动器目录和原 PATH；音频元数据/封面、图片回退解码及截图转换在缓存目录没有 CLI 时改用外部 PATH 工具。INI 仍放启动器旁，更新时不覆盖。运行时缓存仍按 payload 哈希隔离，不复用旧的内置 FFmpeg 缓存。

外置后单文件约 30.18 MiB，此前 97.86 MiB，减少约 69.2%；内嵌 ZIP 扫描确认 FFmpeg CLI、shared DLL 及清单中的专用依赖均未包含。启动器测试通过“完全缺失、仅有 CLI、同目录组件齐全、PATH 组件齐全”四种情况，以及已有 INI 的重开保留。为遵守不使用 computer-use 的要求，提示/进程派发测试直接编译实际启动器源码，仅用测试包装器记录 MessageBoxW 文本和最终 CreateProcessW 参数；没有声称做了弹窗画面验收。真实解包缓存副本中删除全部 FFmpeg，采用外部 PATH 后运行 Lite 的 ffprobe 探测与软件视频播放，两项及 init/cleanup 共 4 passed。启动器的临时测试包装器和原始报告在 `build/audit-external-ffmpeg-20261017/`。

VSCap 填充按钮后续修正：内部 20px 图标绘制 16px 实心方块，点/方块控件高度固定为邻近矩形工具的高度；不使用大字体增加按钮或工具条的 Y 尺寸。相关模式、滚轮、模态截图和工具条回归 5 passed（含 init/cleanup），已重新编译 VSCap。

## 图标与发行准备（最终追加授权）

单文件启动器在链接阶段通过 windres 编译并嵌入 `assets/icons/player.ico`，资源组 ID 101；图标属于外层 EXE，不只存在于缓存中的播放器。FFmpeg 外置策略、同目录 INI 与版本 1.0.11 保持。最终授权为推送本轮源码并准备 1.0.11 无附件 Release Draft，覆盖前述本地阶段的“不上传/不改 Release”约束；Full 由维护者自行打包，所有二进制附件由维护者上传。

重新生成的单文件为 31,717,490 bytes（30.25 MiB）。通过 Windows 资源 API 读取外层 EXE 的图标组及 10 个图标资源，与原 ICO 的各尺寸图像字节逐项比对一致；ZIP CRC、payload 哈希及 Native 与目录式 Lite 同源校验通过，未内嵌 FFmpeg 或用户 INI。审计记录位于 `build/audit-icon-release-20261017/result.json`。本次部署仍确认原设置哈希不变。

````````
<!-- end:docs/update-1.0.11.md -->

<a id="record-docs-update-1-0-12-md"></a>

## docs/update-1.0.12.md

原始字节数：7619；SHA-256：`65e196271815748c2181345cfea5cad283c05b2ad66fce78cb21d7fc51bbf342`。

<!-- begin:docs/update-1.0.12.md -->
````````markdown
# 1.0.12：BD 拖动定位与菜单入口

## 实现

BD 拖动进度条期间仅更新滑块位置；松开后提交一次最终定位，避免精确 seek 与中间帧渲染积压。适用于 BD 播放列表、光盘列表中的片段及活动菜单后端；普通视频保留拖动预览。播放 / 暂停状态沿用拖动开始前状态。

打开 BD 选择光盘菜单模式时，使用已有直接 Top Menu 缓存入口，跳过版权 FirstPlay。菜单缓存引用原盘素材，不修改原始 index.bdmv 或影片。低层原始 FirstPlay 接口仍可单独测试。

## 验证与交付

本轮 Release 构建完成。MyGO 下卷实盘 BD 拖动测试覆盖暂停与播放两种状态：拖动期间时间线代次不变、没有挂起 / 排队的定位，松开后仅增加一次代次，约 110～112 ms 到达 426.426 秒并恢复原状态。普通视频鼠标点击与拖动的软件解码两种播放状态通过，合计 6 passed（含 init/cleanup）。

菜单实盘 4 passed（含 init/cleanup）：首次打开后 1.5 秒截取的原生菜单图像已显示 PLAY ALL、第 8～13 集和字幕选项，可直接点击第 9 集并交回 3FP；返回菜单、再次选集及暂停后定位通过。截图为 build/bd-1.0.12-initial-menu.png。libVLC 通过替换 FirstPlay 的缓存进入主菜单时，标题编号仍可保持 FirstPlay 编号，不能把 title==0 当作唯一选集页判断；本轮以实际菜单图像和首次直接选集验证。

Blu-ray 解析 / 缓存测试 8 passed、3 skipped（对应环境变量未设）。Lite 测试 7 passed、1 skipped（实盘原始 FirstPlay 测试未设环境变量）。日志在 build/bd-1.0.12-*.txt。未运行全量播放器套件。

上一轮只验证了默认分集的拖动，整卷约 6426 秒定位失败被错误描述为“慢例”。后续实盘复现确认它是定位失败并挂起后续请求，修复与整卷验收见下节。

版本更新为 1.0.12；Full 固定部署在 dist/VS-GUI-Full，目录式 Lite 为 dist/1.0.12-PlayerLite，单文件为 dist/VS-Player.exe。保留 INI、VPY 和色彩配置；单文件沿用 FFmpeg 外置与同目录 player.ini，包含外层 Player 图标。源码上传 GitHub，发布压缩包由维护者自行制作。


部署审计 build/deployment-20261017-164604/deployment.json 确认 Full / Lite 的 INI、VPY、ICC/ICM、LUT 哈希保持一致。单文件通过 ZIP CRC、payload 哈希、1.0.12 清单、无 FFmpeg / 用户 INI 内嵌、图标资源组 101 及 Native 同源核对；播放器二进制除 strip 生成的 PE 时间戳和校验和外与目录式 Lite 一致。复制启动器到隔离目录，使用外部 FFmpeg 路径执行真实解包及子进程图像导入成功，已有 INI 哨兵内容不变；原盘及 BACKUP 的 index.bdmv 哈希不变。单文件审计为 build/audit-bd-1.0.12-single/result.json。未制作 Full / Lite 发布压缩包，未上传二进制附件。


## 整卷点击 / 拖动无法定位续修（2026-10-17）

用户反馈整卷连续播放无法通过点击或拖动跳转。旧版 `mouseSeekRendering:BD-SW-paused` 实盘复现：目标约 6426.87 秒，`seekPending_` 始终为真、没有新帧，后续请求进入队列。原生日志显示 seek 失败；并非只是解码缓慢。直接在持续播放状态调用部分定位目标成功，不能代替鼠标先暂停后的完整路径验收。

隔离探针在相同 FFmpeg DLL、相同 ffconcat 和目标上对照：`av_seek_frame(ctx, 0, timestamp90k, BACKWARD)` 返回 -1，`av_seek_frame(ctx, -1, timestampMicroseconds, BACKWARD)` 返回 0。拼接描述包含 `exact_stream_id`；[FFmpeg concat 实现](https://github.com/FFmpeg/FFmpeg/blob/master/libavformat/concatdec.c) 的按流定位将流索引继续传给子解复用器。样本子片段的音视频顺序与输出一致，因此没有把具体 PID / 编号错配当作已证实原因；已证实的是本样本的按流定位入口失败而全局时间入口成功。

`PlayerSession::DoSeek` 对 concat 改用 stream=-1、AV_TIME_BASE_Q 的全局时间定位，保留精确定位的两秒解码前置量。失败时不再把 ffconcat 文本文件当媒体进行字节定位，避免覆盖原始错误及无效回退。普通单文件保留原来的按流定位。改动纳入固定 API20 / PR23 的可重现补丁，固定上游 `git apply --check` 通过；完整 IAMF 构建保持启用。

真实 MyGO 下卷整卷约 8538.53 秒：`mouseSeekRendering` 全部 8 行覆盖普通视频 / 整卷 BD、SW / HW、播放 / 暂停；BD 点击依次到约 6426.87、1254.74、7712.26、2547.81 秒，再反复拖动并松开到约 90%，验证实际视频帧及位置、挂起与队列清空、播放状态恢复。结合整卷 5 次双向定位、默认分集拖动与菜单回归，14 passed、0 failed、0 skipped（含 init/cleanup）。日志 `build/bd-full-seek-after.txt`，修复前失败 `build/bd-full-mouse-before.txt`，原生构建 `build/bd-full-seek-native-build.log`。Lite 回归 7 passed、1 skipped（未配置实盘原始 FirstPlay）。没有将这些聚焦测试等同全量套件通过。

版本继续为 1.0.12；重部署 Full / Lite 并重做单文件，校验配置与单文件内容后推送源码。原盘、播放列表和用户配置保持。


Jinc 偏暗与无音轨 Anime4K 时钟续修见 [色彩与 VS 记录](player-color-vs-1.0.12.md)。

VS 预取调度、手动 Anime 大文件冷索引与半透明样式续修见 [Anime 播放记录](player-anime-performance-1.0.12.md)。

## BD VS 按段索引与菜单续播（2026-10-17）

原先整卷 VS 脚本在首帧前同步创建全部 M2TS 的 Source，首次播放需要扫描整卷。现改为保留完整时间轴，按目标位置延迟创建片段节点；首帧只索引当前所选片段。首帧提交后启动独立、低 CPU 优先级的 Python 进程，顺序预建后续片段索引。后台索引成功后才从临时文件提升为正式缓存，跳转、切换影片和关闭时停止后台任务，目标片段优先。先前片段不会为播放后面的集数而扫描；跳到尚未索引的片段仍需等待该片段的首次索引。

同时修复菜单直接选择 CNN / CNN+ 时，创建 VS 时钟以零覆盖所选集续播位置，以及时钟尚未就绪就清空续播位置的问题。统一在时钟 Ready / Paused / Playing 后通过完整 seek 状态机定位。

验证记录：`build/bd-vs-paths-before.txt` 复现菜单预选 CNN、菜单内切 CNN、预选 CNN+ 三项失败；菜单交接后切 CNN 可播。`build/bd-vs-clock-after.txt` 四路径均通过。原全卷冷索引首帧约 21.6 秒；按段冷索引 CNN 约 5 秒，输出 2560×1440、整卷 204720 帧、所选集位置约 1421.5 秒。冷缓存用独立临时目录；检查前一集索引未创建、后台下一集索引落盘、继续播放和远距离跳转。最终结果见 `build/bd-lazy-final.txt`。版本仍为 1.0.12。

加强后的交界验证 `build/bd-lazy-boundary-final.txt`：四种菜单路径 6 passed、0 failed（含 init/cleanup）。每条路径使用全新索引目录，等待 VS 目标画面呈现并恢复播放后，验证整卷后段定位及第二 / 第三片段之间自然连续播放。冷索引首帧 5.54～5.68 秒。Lite 全套 `build/bd-lazy-lite.txt` 为 8 passed、0 failed；Blu-ray 解析 / 缓存测试为 8 passed、0 failed、3 skipped（未设置该套件的实盘环境变量）。其余 BD 拖动 / 返回菜单 / VPY、手动 Anime、预取与指定无音轨 Anime 四档回归通过。未运行全量 Full 播放器套件。

````````
<!-- end:docs/update-1.0.12.md -->

<a id="record-docs-update-20261016-md"></a>

## docs/update-20261016.md

原始字节数：7668；SHA-256：`9fed7621a5d781440455ef3cedce132397702708d5c6f74d54339bb554095dcb`。

<!-- begin:docs/update-20261016.md -->
````````markdown
# 2026-10-16 合并更新与验收记录

版本保持 **1.0.10**。本轮接续网页端工作区修改；更新本地 Full 固定目录和目录式 PlayerLite，源码推送；不重打压缩包、单文件 Lite 或修改 Release。

## Renderer 时间组与布局

- 左侧四格：左上滤镜库、右上高级参数、左下时间组、右下当前组滤镜；两列可分别隐藏和恢复，腾出的宽度交给视频。顶部按钮按左右列定义“功能区1/2”。
- 输入路径与浏览入口移至右侧进度条下方，仍可向窗口拖入视频。
- 时间组沿用 VideoEnhancer 分段模式：开始秒数递增、默认按整秒吸附，第一个从0开始，后组起点就是前组终点，最后一组到片尾。新建组默认复制当前滤镜链，各组随后独立修改；右键复制/粘贴整条滤镜链。
- 自动生成 VPY，按源 FPS 的精确分数将秒数转换为帧边界，分别处理再拼接，覆盖全片。超出片尾的组不产生空片段。所有组须输出相同分辨率、像素格式和 FPS，不能拼接时明确报错；不会暗中改变输出规格。
- 项目及 VSR 预设保存全部时间组；兼容旧单链预设。导入全部恢复边界及全部组；导入当前组取预设第一个组的滤镜，保留当前边界。纯外部 VPY 可作为全片脚本加载，不能拆成当前组滤镜。
- 预览下方黑色信息区复用 VS Player 的资源采样器：系统 CPU/GPU、进程内存与两路各自的丢帧数量/比例，每秒更新。

相关源码：`MainWindowTimeGroups.cpp`、`FilterTimeGroup.h`、`VpyScriptBuilder::buildGroups`、`PresetStore`。

## 预设社区

协议和下载策略依据本机 PresetCommunity / 3FUI 插件移植为 Qt：ModelScope 分页目录、最多3路下载、大小/SHA256校验、原子安装、覆盖前备份；未修改参考插件或运行中的3FUI。

设置页增加预设下载源。默认临时使用3FUI的 ModelScope 目录；本次读取到10项，并真实下载、校验、打开其中一项。该源是3FUI v6编码JSON，只展示其原始参数，不假装转换成 VS 滤镜。没有把示例预置到正式运行目录。

未来正式源可返回 ModelScope 的目录API，或 JSON 清单：

```json
{"files":[{"name":"example.vpy","url":"https://example.org/example.vpy","size":1234,"sha256":"64位SHA256小写十六进制"}]}
```

支持 VPY，或含 `nodes` / `timeGroups` 的 VSR JSON；下载后通过“导入所有时间组 / 导入当前时间组”应用。来源代码及许可见 `third_party/preset-community-LICENSE.txt`。

## VSCap 与静态 HDR

网页端的多贴图、窗口悬停框选、托盘左键截图、形状粗细/填充、贴图拖动工具、AWJ AVIF 质量70、RGBA16 PNG和HDR双图纳入本地编译。

本轮修复 SDR 伴随图仍携带浮点 `capHdr` 标记，避免错读像素/越界和错误 sidecar。HDR输出两张64bit PNG及原始浮点/JSON附件，SDR只一张PNG。标准PNG仍使用DEFLATE；LZMA2不是通用PNG的IDAT压缩方案，未私改格式。详见 [PNG标准](https://www.w3.org/TR/png-3/#10Compression)。

Player对PQ/HLG静态图保留原图，并为SDR预览做亮度与色域映射。UIAccess默认关闭，开启后下次启动经UAC授权创建自身UIAccess进程；不使用驱动或安装证书。真实HDR显示器、混合DPI以及覆盖置顶任务管理器仍交由维护者验收。

## 3FP API20 与新增 PR22

最终上游固定到 [PR22](https://github.com/Lake1059/FFF_Project/pull/22) 合并提交 `0fe651ddab8a2bb6ddb729f314f9f4c5117fb565`。保留本地VS桥、clock-only、色彩、音频和软件解码扩展，完整编译IAMF/OBR，未通过关闭功能降级。补丁及重建入口见 [构建说明](native-api20-build.md)。

**按用户追加要求，PR22仅合并、编译及补丁检查，未做运行/性能测试。下面的播放器、光盘结果均来自合入PR22之前的API20产物，不代表最终新内核已运行验收。** 新内核性能提升尚无本机测量结论。

## 光盘修复与测试边界

- Full/Lite均补鼠标移动和点击转发，Lite按画面letterbox坐标映射；Full实际选中第9集，位置1421.52秒，确认不是自动播放第8集造成的假通过。
- 保留原盘FirstPlay入口，不再改写为TopMenu；播放完FirstPlay再请求主菜单，尊重导航VM限制。Full避免将FirstPlay误识别为普通节目提前切走；Lite修正等待菜单的title边界。
- 修复Native加载器将编译依赖的libbluray3误换成libbluray4，导致公开结构读错、章节数据破坏的问题。二者可以同目录存在，但Native菜单桥必须用libbluray3。
- 菜单demux重开前停止软件解码线程；光盘视频遇到单个INVALIDDATA包时刷新解码器继续，避免菜单循环直接终止。此处理仅限光盘视频的该错误类型。
- 章节改为蓝色小三角，菜单路径也填入章节时间；播放列表默认遮罩，不扩宽窗口。
- 松开进度条恢复菜单播放；修复Lite快速暂停/继续的异步竞争与退出等待。分段流增加精确seek预读，6秒暂停定位已通过。

## 已有证据

下列数字均包含QtTest初始化/清理：

| 检查 | 结果 |
| --- | --- |
| VSCap完整Windows工作流 | 15 passed，0 failed；含取消保存、多贴图、撤销、HDR双图、真实AWJ输出 |
| PNG精度/SDR伴随图 | 5 passed，0 failed |
| BD结构、FirstPlay保持及拼接 | 6 passed，0 failed |
| API20音频时钟、直通/Jinc/D3D11、外部帧、输出格式、像素及字幕 | 9 passed，0 failed（PR22之前） |
| Lite无VS软解、设置、暂停/恢复 | 6 passed，0 failed（PR22之前） |
| 真实Lite菜单循环 | 菜单内实际180秒，整例193秒；之后鼠标选正片通过，无解码错误（PR22之前） |
| 时间组布局、预设往返、VS跨组边界取帧、社区真实下载/查看 | 5 passed，0 failed；VS测试保持101帧、24000/1001 FPS |
| 图模型完整测试 | 14 passed，1 failed；失败是既有GLSL路径测试仍断言旧版`open(绝对路径)`形态，当前脚本先赋`_shader_path`再打开；新增时间组用例通过 |
| 最终Native API20+PR22 / Full / Lite / VSCap | Release编译通过；PR22未运行测试 |

日志及截图在本机 `build/audit-20261016/`，原生编译日志在 `build/api20-*.log`。

## 明天验收及未解决项

1. **长光盘跨段拖动尚未通过**：85xx秒分段流，从约1秒跳至6426.866秒，旧API20的FFmpeg seek返回失败，界面仍有seekPending/暂停问题。短距离精确跳转和真实菜单选第9集已通过，不能据此宣称全部拖动已修复。最终PR22带上游seek失败回退改动，按要求未重测，仍列为待验收。
2. 新PR22真实性能、HDR动态元数据、IAMF/OBR听感、真实HDR/SDR显示器色彩、混合DPI、UIAccess任务管理器覆盖未验收。
3. 3FUI临时源是下载/查看演示；正式VS预设下载源及内容等待维护者替换。
4. 多时间组的跨组输出规格须一致；变帧率或分辨率不一致时会明确拒绝拼接，不能视为任意滤镜组合均通过。
5. 目录式Lite当前为190.97 MiB（含已有FFmpeg），按原清单扣除FFmpeg后为54.45 MiB；完整IAMF/OBR内核增加了体积，超过历史打包脚本50 MiB阈值。本轮不打包，也未为达标删减功能，后续压缩包制作前需重新核对该限制。

正式部署以 `dist/VS-GUI-Full` 和 `dist/1.0.10-PlayerLite` 中的 `release.json` / `resource-manifest.json` 为准；部署备份与配置哈希记录在 `build/deployment-*/deployment.json`。不更新现存7z或`dist/VSPlayerLite.exe`单文件产物。

````````
<!-- end:docs/update-20261016.md -->

<a id="record-docs-update-compatibility-1-0-3-md"></a>

## docs/update-compatibility-1.0.3.md

原始字节数：2020；SHA-256：`73ef881752ef8817cd269db54118d41286f7239ce9b49f2ace412a61e93a8d51`。

<!-- begin:docs/update-compatibility-1.0.3.md -->
````````markdown
# 1.0.3 更新包兼容1.0.2修复

## 原因

ModelScope 的 `VS-GUI/1.0.3.7z` 大小168,399,042字节，SHA-256为 `d3f9239acd66be80fa834c87040eee0050cd9de2d55d1e34386711e237d8fc9e`，与原本地包一致。

使用发布提交61b13a6中的原始1.0.2更新器源码执行实际联网下载，先通过大小与SHA-256校验，再报“更新包路径不安全或格式无效”。它只允许带版本号的根目录 `VS-Renderer-GUI-[0-9.]+-windows-x64`，原1.0.3包的固定根目录不满足此规则。不是上传损坏或哈希算法错误。

## 修复

打包脚本根目录恢复为 `VS-Renderer-GUI-1.0.3-windows-x64`。安装器仍写入已有安装目标，不重命名目录；本地开发目录继续使用固定名称，新版更新器同时支持两种归档根目录。没有放宽路径/哈希验证，没有修改两程序或原生DLL。

修正版：`dist/1.0.3.7z`，大小168,379,908字节，标准LZMA2 9级压缩。

SHA-256：`57de5cc1885958b555eb4454438a4126bd115ab6aa19bd8e689473f1d2c65aa9`。

旧包保存在 `dist/1.0.3-before-update-fix.7z`，未自动上传或删除远端附件。维护者需替换ModelScope `VS-GUI/1.0.3.7z`，同步上传 `.sha256`，并替换GitHub同名附件；旧重命名副本 `VS-Renderer&Player 1.0.3.7z` 仍是错误根目录的旧包，不要用于这次替换。

## 验证

- 原始1.0.2代码：原远端包拒绝；修正版完成下载、SHA-256、路径校验、解压、组件/版本验证，发出prepared信号。仅准备更新，没有执行安装。
- 专项4/4（含初始化/清理）；不运行无关回归。新测试 `releasedPackageCompatibility` 通过 `VSR_TEST_UPDATE_ARCHIVE` 指向真实发布包。
- 7z完整性检查通过；内部312个资源哈希一致；两EXE与FFF.Native.dll与活跃目录完全相同。
- 日志：`build/legacy-update-remote-original.txt`、`build/legacy-update-compatible-prepare.txt`、`build/update-compatible-tests.txt`、`build/update-compatibility-1.0.3-audit.json`。

````````
<!-- end:docs/update-compatibility-1.0.3.md -->

<a id="record-docs-validation-1-0-10-md"></a>

## docs/validation-1.0.10.md

原始字节数：9897；SHA-256：`89a178b229c9791a493f6aa171fd3f37a97bcb9b9abfbb5cc1c31b24d8a7d617`。

<!-- begin:docs/validation-1.0.10.md -->
````````markdown
# 1.0.10 本机验收与待处理记录

核对日期：2026-10-10。本机 Windows 11，Ryzen 9 7940HX（32 逻辑线程），RTX 4070 Laptop，165 Hz / 2560×1600 SDR 桌面。以下测试强制软件解码（decodeMode=1），D3D11 原生显示，不开启硬件解码、CPU 预缩放或位深降低。测量环境与其它设备 1.0.9 的历史记录分开。

## 1. 源码与构建

先解压维护者提供的 `VS-Renderer-GUI-1.0.9-op-vscap-fix-20261010.zip` 并同步源码；被替换的 1.0.8 文件备份在 `build/sync-backup-1.0.8`，同步清单在 `build/import-1.0.9/sync-files.json`。仅换行不同的旧文件恢复原字节，减少无关差异。提供的 ZIP、1.0.9 PlayerLite 与 Full 压缩包保持原样。

GUI 使用 Qt 6.10.2 / MinGW 13.1；Native 使用 MSVC 14.51 / v145、上游 `20bcc001b24f6b6de4575bddf9b93ec452795b0d` 和完整 `patches/3fp-api18-vsrenderer-1.0.10.patch`。已含 1.0.9 补丁的 Native 树由构建脚本识别并应用 1.0.10-update 差量补丁；干净基线使用完整补丁。完整补丁已对干净固定基线执行 `git apply --check`，对当前构建树执行反向检查。没有将纹理轮换或 SSE 动态上传的无收益实验纳入最终版本。

原生保留 IAMF 构建入口（FFF_WITH_LIBIAMF=1），本机依赖固定 libiamf `b276f4371a8ae4f370a656e914f7b07e4cd5a2fa`、OAR `ca14b54381b6e8482a72dcc65c3adb17d6751db0`。构建 `code` 子目录，`ENABLE_BUILD_CODECS=OFF`、`IAMF_ENABLE_BINAURALIZER=OFF`、`IAMF_TEST_TOOL=OFF`、`IAMF_BUILD_SHARED_LIB=OFF`，CRT 为 MultiThreadedDLL；保留 PCM 与扬声器 OAR。未验证压缩 IAMF 编解码或双耳渲染，不宣称这两项可用。FFmpeg 使用收到的 1.0.9 运行时，Native 对 libbluray 的实际依赖由递归 DLL 检查收集。

构建入口仍为返回源码的 `tools/build-native-api18.ps1`、`tools/build-1.0.9.ps1` 与 `tools/package-1.0.9.ps1`；其名称保留，版本从 CMake 读取。打包只创建 `dist/1.0.10-Full` / `dist/1.0.10-PlayerLite`，不移动整个 dist；重复输出非空会停止。工具链、NativeRoot、RuntimeDirectory 与 CRT/Dumpbin 参数需指向本机实际安装。

## 2. 色彩与截图

Full 新配置默认引擎 2：PQ/HLG 源映射 SDR 时使用 libplacebo，SDR 源及 HDR 输出保留原生路径，避免对 SDR 性能测试引入额外色彩工作。已有 engine=0/1 的显式选择保留；旧用户需要在色彩设置中选新增的自动模式。原生模式标为“原始”，不再称为默认。色彩对话框中文按钮为“确定 / 取消 / 应用”。

`automaticHdrToSdr` 验证 PQ、HLG→SDR 与自定义高级模式像素一致、值有限且在显示范围内；验证 SDR 自动模式仍为 native、HDR→SDR→SDR 切换和 requestedEngine=2 状态。`nativeDefaultAndPausedColorSwitch` 和 GUI 配置测试验证暂停切换、不重载 VPY、不改变时间轴及中文按钮。提供的过曝光对照图用于判断需求；未对原图的源媒体做色度测量，不将单张截图视作定量色准验收。

PlayerLite 没有高级色彩依赖，仍使用原生引擎；HDR→SDR 若出现原生映射偏亮，需使用 Full。缺失高级运行时时 Full 会继续原生输出并报告回退原因。此限制保留，不以删减运行时冒充完整高级映射。

VSCap 工具组独占选中，并用蓝框标识；左侧常驻颜色指示和工具参数，线工具为直线/箭头，其它绘图工具提供线宽。Ctrl+Z 与撤销按钮恢复动作前像素；历史最多 32 步 / 128 MiB（至少保留最近动作），原始截图、HDR 浮点数据和透明度保留。模拟 FP16 截图的箭头/矩形撤销逐像素一致。真实本机桌面截图通过，本机桌面为 SDR，未宣称实际 HDR 显示器捕获已验收。

2026-10-11 补充文字对象验收：取消“插入文字”弹窗，在点击位置创建透明输入框，虚线边缘拖动、框内编辑；对象保持独立到 PNG / AVIF / 复制 / 贴图交付。字体来自系统，支持可编辑下拉框自动补全、数字字号、加粗及斜体，多对象分别保留样式；长行按选区宽度换行，框限制在选区内。

`editableTextObjects` / `textAndDrawingUndo` 实际显示 Qt 窗口并输入、编辑、拖动、切换工具及导出：验证多行中文、字体补全、数字输入、加粗斜体、多个文字框重新编辑、长行换行和选区边界，以及文字 / 字体 / 移动 / 绘图的统一撤销。导出逐像素检查没有白色边框或光标，两个对象均有文字像素，RGBA32F 格式与 scRGB 标识保留，未编辑的 1.5 浮点通道值不截断。工作流合计 9 passed；本地日志和编辑 / 导出截图在 `build/acceptance-cap-text*`。本次未重新执行视频性能测试，以上性能数字仍是 2026-10-10 的测量。

维护者随后在更新后的本地 VSCap 上手动验收，明确反馈“验收通过”（2026-10-11）。截图热键和编辑交互的人工验收与上述自动工作流结果分别记录。

## 3. 软件解码性能

每行采用对应 CSV 第一与最后一秒样本之差；fps 为实际提交帧增量 / 墙钟时间，丢帧为 Native 实际丢弃计数，不把帧率缺口伪造成已丢帧。秒数是实际运行时间。4K 视频为 AV1 3840×2160 yuv444p12le，8K 为 AV1 7680×4320 yuv420p10le，均约 47.952 fps；用户所述“y210”在这两份 8K 文件中实际为平面 4:2:0 10-bit，不是 packed Y210。

| 条件 | 墙钟秒 | 提交 fps | 计时段丢帧 | 音频欠载 | 首帧 ms | 最大视频落后 ms |
|---|---:|---:|---:|---:|---:|---:|
| 1.0.9 Native / 字幕缓存优化前 GUI，4K OP FLAC+ASS | 60.004 | 47.95 | 0 | 0 | 1447 | 22.79 |
| 1.0.10 Full GUI，4K OP FLAC+ASS | 88.009 | 47.88 | 0 | 0 | 689 | 23.83 |
| 1.0.9 Native，8K 无音频，直接渲染测量 | 30.007 | 39.66 | 0 | 0 | 398 | 94.00 |
| 1.0.10 Native，8K 无音频，直接渲染测量 | 30.006 | 30.79 | 354 | 0 | 322 | 3855.90 |
| 1.0.10 Full GUI，8K 无音频 | 30.000 | 33.50 | 258 | 0 | 290 | 3483.28 |
| 1.0.9 Native / 字幕缓存优化前 GUI，8K AAC+ASS | 30.000 | 21.03 | 645 | 129 | 797 | 917.67 |
| 1.0.10 Full GUI，8K AAC+ASS | 30.003 | 33.16 | 304 | 61 | 459 | 1648.33 |

4K OP 最终构建测到 88 秒，包含高压力段，计时段零视频丢帧、零音频欠载，显示输出仍为 10-bit。此结果限于该素材、本机与当前原生显示档位，不推广到所有 4K 444p12。

旧无音频直接渲染在 30.007 秒墙钟内只推进 24.79 秒媒体时间：CPU 视频落后时 ResetClock 会让视频减速并掩盖丢帧。新版本移除该分支，直接渲染推进 30.006 秒并正确报告实际丢弃；更低的提交 fps 不能与旧减速结果直接当作同速退化比较。GUI 信息面板另显示帧率缺口和视频时间落后，计数口径保持区分。

8K 仍未达标。无音频 Full GUI 距 47.952 fps 差约 14.45 fps，需要约 43.1% 吞吐提升；AAC+ASS 差约 14.79 fps，需要约 44.6% 提升，仍有 61 次音频欠载。AAC+ASS 的媒体进度只推进 28.467 秒，不能声称音视频全程实时。完整链路视频吞吐较对照提升约 57.7%，首次加载单轮从 797 ms 到 459 ms，但没有多轮分位数数据。

纯 FFmpeg 解码 480 帧（不含显示、音频、字幕）的 8K 对照：16 workers / 16 frame contexts 约 48.17 fps，32/16 约 47.41，32/32 约 45.62。16/16 最大驻留约 6.56 GiB，32/32 约 12.32 GiB。完整播放还要 CPU→GPU 上传（约 15 ms/提交帧）及音频/字幕等，纯解码仅刚到目标，余量不足。当前选择 16/16，4K 444p12 保留 32/32。更换线程策略不是全目标验收通过。

减少字幕开销：已知字幕轨道不再重复 avformat_find_stream_info 探测高分辨率视频与 AAC；静态字幕裁剪像素及位置相同则复用整屏合成结果，动态 ASS 仍逐次渲染。未关闭字幕、音轨、降低源分辨率或减少源位深。发行默认也关闭自动上传前降分辨率；该实验仅在显式设置 VSR_ADAPTIVE_UPLOAD=1 时启用。

原始日志和逐秒 CSV 在 `build/acceptance-1.0.10`，本地汇总为 `results.json`。正式提交中保留本报告，避免提交用户媒体与依赖缓存。

## 4. 回归与交付

色彩专项 4 passed；GUI 色彩设置、静态字幕去重、裁剪字幕像素、4K AV1 暂停/跳转/续播/片尾重开通过。4K 实际跳转到 0 / 60 / 10 / 84 秒约 1.25 / 2.21 / 1.62 / 1.62 秒恢复。既有 directStartupClock 三档通过。VSCap 原生测试 5 passed、工作流 7 passed；PlayerLite 5 passed，确认没有加载 VapourSynth / libVLC。语言检查覆盖 547 字符串及占位符。

旧 API 14 的小尺寸 HEVC/AV1 合成回归与 API 18 元数据约定不同：HEVC adaptive 初始线程为 16，softwareVideoDecodeAsync 元数据只对 dav1d 为 true，旧软件队列预算字段已移除。测试按实际 API 18 约定调整；解码累计计数包含 adaptive HEVC 线程重开后的重解帧，不能等同于文件唯一帧数。HEVC/AV1 合成回归最终 4 passed，包含暂停、跳转、续播、片尾和重开。同步后发现暂停跳到片尾附近会将一次 drain 的后续帧全部当预览呈现（3.8 秒请求走到 3.958 秒），现将目标后的延迟帧保留到续播队列；暂停时间/帧号断言保持。

最后复测使用包含暂停跳转修复、默认关闭上传降分辨率的发行构建；表中 Full GUI 最终三行已更新为复测值。

包内 `resource-manifest.json` 记录每文件大小与 SHA-256；新压缩包执行 7z 完整性检测并解压核对清单。本机原部署及提供的 1.0.9 构建不覆盖。源码推送与 Release Draft 完成后再操作已打开的 3FUI 编码队列，GPU 占用按真实任务确认；该操作状态在最终交付中报告。

````````
<!-- end:docs/validation-1.0.10.md -->

<a id="record-docs-validation-1-0-9-20261010-md"></a>

## docs/validation-1.0.9-20261010.md

原始字节数：6684；SHA-256：`757aa126993fd71cf1adb1a49859a4aa7c40066feca0cc8c18c1f8b6e88678c2`。

<!-- begin:docs/validation-1.0.9-20261010.md -->
````````markdown
# 1.0.9 本机验收与复现

日期：2026-10-10。Ryzen 7 7840HS / Radeon 780M / 32 GB RAM，本地物理 3840×2160 / 60 Hz、HDR 已开启。性能测量没有并行构建。GPU 百分比来自现有资源监视器，不能替代分引擎时间戳分析。

## 素材与计数

F 盘为 12 TB USB 桥接机械硬盘。直接读取原文件，没有复制、改写或全文件哈希。根目录为 `F:\Media\4K电影+电视剧\Chou.Kaguya-hime.the.Movie 超时空辉夜姬`：

| 素材 | 文件 | 字节数 |
| --- | --- | ---: |
| 4K48 | `Chou-Kaguya 2160p48F AV1 Real-ESRGAN Ver.mkv` | 29,920,024,980 |
| 8K48 | `Chou Kaguya 4320p48F-AV1 RealESRGAN 8k收藏版.mkv` | 107,938,080,889 |

ffprobe 为 AV1、10-bit、有限范围 BT.709，`7001/146`≈47.952 fps；AAC 48 kHz 6 声道，含 ASS。两份原片标签是 SDR，不能把 Windows HDR 开启称为 HDR 视频素材验证。

`nativeRecoveryPerformance` 使用原生会话、真实全屏 HWND、D3D11 档和 4K 输出；保留 AAC，未加载字幕。硬解首帧后预热 10 秒，软解 2 秒，计时 60 秒。accepted 是 Native 接收呈现计数，presents 是交换链调用计数，都不能替代外部屏幕扫描测量；同时检查媒体/PTS/合并计数。QTest 末次快照与 CSV 可能差一帧，表格统一取 CSV 首尾差值。

| 测试 | accepted | dropped | coalesced | 音频欠载增量 | 媒体进度 | 首帧 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 4K48 硬解 | 2877 | 0 | 0 | 0 | 60.010 s | 340 ms |
| 8K48 硬解 | 2911 | 0 | 0 | 0 | 60.001 s | 266 ms |
| 8K48 软解，自适应关 | 1334 | 662 | 0 | 535 | 42.183 s | 526 ms |
| 8K48 软解，自适应开 | 1415 | 704 | 0 | 563 | 44.605 s | 340 ms |

4K 硬解约 47.94 fps，启动预热阶段另有 6 帧丢弃。8K 硬解计时开始 PTS 落后 0.7411 s，末尾落后 0.0345 s；包含追帧的 48.51 fps 平均值不能称为持续超过源帧率。这两项未证明从打开起全程无丢帧/一直同步。原始证据在 `build/session-20261010/final-hdd-*.csv/.txt`，摘要为同名 `-summary.json`。

8K 软解约 22.23→23.58 fps（+6.07%），CPU 中位数 31.77%→18.82%，GPU 40.30%→14.64%。这是同一新内核策略开关比较；自适应预缩放可能改变画质，不能当作等画质新旧提速。整体仍未实时。500 ms 音频储备试验为 23.51 fps、欠载增量 566，无改善，已撤回；保留 `reserve500-hdd-8k48-sw.*`。旧内核在 F 原片及 D 同机片段软解报解码错误，`same-ssd-old.txt` 对照未完成，不能给出可靠 FPS 提升倍数。

## 主窗口启动

每项三次，从创建进程至主窗口 HWND 出现，PATH 只有 Windows 系统目录。采样约 20 ms 精度，缓存/运行次序有影响，不能称为严格冷启动统计。未结束用户原有播放器进程。这不是视频首帧时间。

| 程序 | 第一次 | 第二次 | 第三次 | 中位数 |
| --- | ---: | ---: | ---: | ---: |
| 旧 Portable GUI | 1145 | 896 | 887 | 896 ms |
| 旧 Portable Player | 606 | 593 | 557 | 593 ms |
| 新 Full GUI | 1021 | 896 | 912 | 912 ms |
| 新 Full Player | 581 | 577 | 611 | 581 ms |
| 新 Lite Player | 581 | 583 | 564 | 581 ms |

播放器减少 12 ms（2.02%），GUI 增加 16 ms，均为小幅波动。Native/VS 延迟初始化避免图片启动强制加载内核，不代表 Qt 窗口已大幅提速。证据为 `startup-final.csv/.log` 和 `tools/measure-startup-1.0.9.ps1`。

## 功能回归

- Full 发行目录、纯系统 PATH：13 passed / 0 failed，覆盖图片延迟初始化、HDR 浮点回读、libplacebo、EQ/延迟、多声道策略、倍速/seek、VS/RIFE、字幕、独立音频时钟、Lanczos4、设置及硬解截图。日志 `shipping-vsr_player_tests.txt`。
- Cap 发行目录：5 passed / 0 failed，真实 HDR 桌面为 3840×2160、depth128、scRGB-FP16；工作流原 4 passed / 0 failed，包括原始浮点剪贴板、16-bit PQ PNG、普通顶置贴图/工具条、后台 libaom AVIF CRF18。AVIF 经 ffprobe 验证 10-bit 4:4:4、PQ、BT.2020、full。新增框选/矩形注释/✓复制测试另行运行，记录随后补充。
- Lite 发行目录：5 passed / 0 failed，默认 D3D11、仅 Jinc/D3D11、拒绝 CNN/RIFE、原生软解与暂停截图、不加载 VS/libVLC。
- HDR PNG 无损压缩保存的是 BT.2020/PQ RGBA16 的量化像素；FP16 原始 scRGB 经 RGBA32F 伴随文件保存精确值。不能把 PNG 量化称为原始 FP16 逐位无损。AVIF 是有损格式。
- 真实 GUI 首轮 4K48 视频计时段零丢帧，但 ASS 一直未显示，退出还等待全片扫描，严格用例失败。原因是上游 ASS 读取扫描全片；已移植 `3fp-streaming-text-subtitles.patch` 的有界读取，新的 GUI 含字幕验收见随后补充。
- RIFE/ncnn 在中文安装路径报 `failed to load model`；同包英文路径通过。发行目录改为 `dist/Full` 和 `dist/PlayerLite`。完整版补帧安装路径需保持纯英文；不改中文界面。
- 缺少真实 BD/IAMF 素材，未验收 HDMV/BD-J/空间音频；未运行整个 QtTest 套件，不宣称全部通过。热键冲突、文字/颜色对话框和开机自启还需要人工体验验收。

## 构建与手动回传

```powershell
cd C:\Private\VS-Renderer-GUI-1.0.9
.\tools\build-1.0.9.ps1
.\tools\measure-startup-1.0.9.ps1 -CsvPath "$PWD\build\startup.csv"
```

入口检查/应用 API18 完整补丁，v145 构建 Native，MinGW 构建四个程序，自动生成 Full/PlayerLite。旧 dist 移到 `build/dist-before-时间戳` 备份；打包从参数 RuntimeDirectory 的旧便携运行时补齐完整 FFmpeg、VS/Python/组件，不覆盖旧安装。

原始设备需准备 v145、Windows SDK、Qt 6.10.2 MinGW/QtTest；Native 根据其上游 `准备FFmpeg.ps1`、`准备Libass.ps1`、`准备Libiamf.ps1`、`准备光盘库.ps1` 准备 SDK。libiamf 固定 b276f4371a8ae4f370a656e914f7b07e4cd5a2fa，oar 固定 ca14b54。FFmpeg SDK/运行 DLL 必须与 avcodec-63 / avformat-63 / avutil-61 / avfilter-12 / swscale-10 / swresample-7 一致。

GUI/Native 固定基线见状态文档；源码包含完整 `patches/3fp-api18-vsrenderer-1.0.9.patch`，不要盲套历史补丁。helper 先反向检查已应用补丁，冲突退出，不重置工作树。构建脚本 NativeRoot、RuntimeDirectory、QtDirectory、MingwDirectory、MSBuild/CMake/Ninja 参数需按原始设备路径调整；打包 CRT/Dumpbin 默认路径也应对应工具版本。

完整日志留在本机 build，源码归档另附关键日志/CSV/manifest，不携带原视频、dist 或工具链。Lite 本体 51.96 MB（49.56 MiB），不满足严格 50 MB；含完整 FFmpeg 总包 186.07 MiB，100 MB 目标尚未达到。

````````
<!-- end:docs/validation-1.0.9-20261010.md -->

<a id="record-docs-vscap-2026-10-16-multipin-awj-hdr-md"></a>

## docs/vscap-2026-10-16-multipin-awj-hdr.md

原始字节数：8335；SHA-256：`3017501464be1b9a8e9ab5c932722816bafd7b2941eba2045457c440b929fc13`。

<!-- begin:docs/vscap-2026-10-16-multipin-awj-hdr.md -->
````````markdown
# VS Cap 多贴图、AWJ AVIF、HDR PNG 和 VS Player 静态图片颜色映射

> 第1节为功能说明；第2节之后的网页阶段构建/部署记录属于历史。当前交付与验收以文末“本地续修”和 [续修记录](update-20261016.md) 为准。

日期：2026-10-16。工程：`C:\Users\ARXChem\Documents\Open Source Projects\VS-Renderer-GUI`。

## 1. 本轮实现

### 1.1 多贴图与编辑工具栏

- 原有贴图窗口本来就使用 `QList<QPointer<PinWindow>>`，但剪贴板导入仅展开第一张文件。现每个本地图片 URL 独立建立贴图窗口，多张贴图错位展示、独立缩放与销毁；从截图工具直接连续置顶也支持多个窗口。
- 贴图启用工具栏时默认编辑画面；新增「拖动」单独工具（`CaptureOverlay::Pan`），只有选中后才能在编辑状态下移动贴图。隐藏工具栏时仍可拖动。
- 矩形和椭圆左侧增加「●」粗细控制（对其滚轮调节 1～512）和「■」纯色填充开关。维持原 Ctrl 正方形/正圆与撤销逻辑。
- 测试对象名 `capShapeWidth`、`capShapeFill`、`capPinView`。具体实现位于 `src/cap/main.cpp`。

### 1.2 窗口选区与托盘

- 左键单击托盘图标（`QSystemTrayIcon::Trigger`）触发截图；原右键托盘菜单不变。
- 截图启动前通过 Windows `EnumWindows` 获取可见窗口，优先 `DWMWA_EXTENDED_FRAME_BOUNDS`；忽略隐藏/最小化/cloaked 窗口，按当前显示器的物理原点和 DPI 换算至截图选区。未选区时悬停高亮候选窗口，单击自动选择矩形。
- 鼠标按住拖动仍按原算法自由框选；之后仍可移动矩形并通过八个控制点调整。
- 局限：窗口列表是截图启动时的快照，部分带特殊材质/权限的窗口无法精确获得可见边界；DPI、跨显示器及特殊屏幕需要在真实桌面确认。

### 1.3 AWJ AVIF

- 设置中新加保存格式 PNG/AVIF 选项、`capAvifQuality`（默认质量 70）。
- 选择工具栏「保存」时按设置选定格式输出，原来的显式 PNG / AVIF 按钮保留。
- 不再用 FFmpeg 进行 AVIF 编码；调用发布目录内 `runtime/awj/AWJ.exe`，其次是同目录 AWJ.exe，开发构建时回退 `.deps/awj/1.1.0/AWJ.exe`。符合项目现有 PlayerImageTools 对 AWJ 1.1.0 的参数形式。
- 普通图默认 AVIF 10bit 4:2:0（奇数尺寸自动 4:4:4）；HDR 带 BT.2020/PQ CICP 信息默认 AVIF 12bit。编码器 AOM，速度 6，质量 70；结果安全写入目标并清理临时图。
- 已用 `AWJ.exe` 对项目真实 PNG 做命令行试编码，得到 36.8 KiB 的 `result.avif`，成功 1 / 失败 0。参数级验证不等于编辑器真实全流程已通过。

### 1.4 PNG 16bit×4 和 HDR/SDR 双版本

- 所有正常 PNG 均通过 `Format_RGBA64` 输出，即 RGBA 每通道 16 bit，总共 64 bit。
- SDR 输出 1 张 PNG；HDR 保存 1 张原色 BT.2100 PQ 64bit PNG + 一张带 `_SDR.png` 的 sRGB 色调映射 64bit PNG。原生 HDR 全屏采集也沿用此行为。
- HDR 同时保留历史 `.scrgb-f32` 与 `.json` 科学/编辑工作流 sidecar（两张 PNG 以外的原始数据，不视为额外预览图）；FP16 捕获中转换至整数 PQ 必然有量化，原始浮点副本未丢弃。
- 现有 PNG 用 Qt `QImageWriter` `setCompression(9)` 和标准 zlib/DEFLATE 无损压缩。**未使用 LZMA2**：W3C PNG 标准规定默认 IDAT 必须使用 zlib/DEFLATE；直接改为 LZMA2，通用解码器不能打开。参见 https://www.w3.org/TR/png-3/#compression。

### 1.5 VS Player 的 HDR 静态图片 SDR 映射

- PNG 静态图片入口 `src/player/PlayerImage.cpp` 读取侧车 `.json` 的 `pngTransfer` / `pngColorSpace` 并恢复 `Bt2100Pq` 或 HLG 标记（作为插件忽略 ICC 的兜底）。
- `src/ui/PreviewPane.cpp` 保留原始 QImage 用于用户高精度导出，另为 QWidget 画面生成 SDR 映射图；应用 PQ/HLG EOTF、203 nit SDR 参考、BT.2020 或 P3 → sRGB、亮度映射及 sRGB OETF。避免把 PQ 码值当 sRGB 导致过曝、过暗或偏色。
- **边界**：这轮是 QWidget 的 SDR 兼容显示路径，即使在 HDR 显示器上也不宣称原生 HDR 高亮度显示；精确色度、HDR 全局亮度、SDR 首帧像素需要真实显示器验收。

### 1.6 UIAccess 高层级置顶

- `general/uiAccess` 默认关闭；开启后下次启动通过 UAC 管理员授权重启。本轮新增 `src/cap/UiAccess.cpp`，先检查 TokenUIAccess，已获权限时不重复重启。
- 管理员桥接仅在创建自身用户的 UIAccess token 时，使用同一桌面会话 winlogon 的 SeTcb impersonation 权限；随后 RevertToSelf、关闭所有句柄，通过 CreateProcessAsUser 启动自身。没有驱动、证书安装或修改系统进程。
- 设置开关不会立即重启而丢失贴图。失败时显示原因并继续正常置顶运行。已有实例通过 mutex 检查避免重复 UAC。
- 设计参考 [uiaccess 工具](https://github.com/killtimer0/uiaccess)，代码为本项目自行实现。标准签名清单方案的系统约束见 [微软说明](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-securityoverview)。此替代启动路径仍需用户批准 Windows UAC。
- 编译已通过；**本轮未实际发起 UAC，也未验收覆盖任务管理器始终置顶**，不将该项记作实机通过。

## 2. 代码影响与编译

直接修改：
- `CMakeLists.txt`（新增 `dwmapi`、`advapi32` 链接）
- `src/cap/main.cpp`
- `src/cap/DesktopCapture.cpp`
- `src/player/PlayerImage.cpp`
- `src/ui/PreviewPane.cpp`
- `tests/TestCap.cpp`
- `tests/TestCapWorkflow.cpp`

原工程还有此前未提交的播放器 3FP 改动和清理脚本，均没有调整。

完成 Windows/MinGW Release 编译：
- `build/mingw-release/vs-cap.exe`
- `build/mingw-release/vs-player.exe`
- `build/mingw-release/lite/vs-player.exe`
- `build/mingw-release/vsr_cap_tests.exe`
- `build/mingw-release/vsr_cap_workflow_tests.exe`

`git diff --check` 为 0（仅 CRLF 提示）。

由于 `dist/VS-GUI-Full/vs-cap.exe` 当时正在运行（旧实例可能有贴图未保存），**没有强行结束旧进程或覆盖该程序**。已经安全预置：
- `dist/VS-GUI-Full/vs-cap-vscap-next.exe`
- `dist/VS-GUI-Full/vs-player-hdr-next.exe`

两者位于原发行目录，可直接使用原有 Qt DLL 和 AWJ；但托盘截图软件采用单实例 Mutex，因此使用新版本前，应正常从旧 VSCap 托盘菜单退出，避免丢失旧贴图。待旧截图工具及播放器都退出后，可在项目根目录执行 `powershell -ExecutionPolicy Bypass -File .\tools\activate-vscap-20261016.ps1` 将这两个待激活版本复制为正式运行文件；脚本检测目标 EXE 仍在运行则拒绝覆盖。之前的单文件 Lite 也已用新的 HDR 图片映射构建重打包至 `dist/VSPlayerLite.exe`。

## 3. 验证状态与尚存边界

- 完成：Windows 编译通过、AWJ CLI 真实压缩成功、Git diff check 通过；测试代码根据 RGBA64 与 AWJ 改动完成编译。
- **未通过自动化实机 GUI 验证**：从 AgentDock 非交互会话启动 `vsr_cap_tests.exe` 与 `vs-cap.exe --self-test` 都发生阻塞超时。测试未取得断言结果，不能记为通过。实际窗口识别、图像贴图拖动与工具栏需要可交互桌面验收。
- 需要真实 HDR 显示器才能验证 HDR 截图动态范围、双版本显示的一致性和 VS Player SDR 映射校准。
- UIAccess 因缺签名正式授权，只有已授予 UIAccess 的设备才会开启新增高层级行为；其余情况下回退普通置顶提示。


## 本地续修与回归

修复 HDR preview 转成 RGBA64 后仍保留浮点 `capHdr` 标记的问题，避免保存 SDR 伴随图时错读像素和生成无效 sidecar。SDR 伴随图验证实际像素及 sRGB 色彩空间，HDR 双图测试验证 2 PNG + 原始浮点/JSON 共 4 文件。

完整 Windows 工作流 15 passed、精度测试 5 passed（均含初始化/清理），覆盖取消保存重试、多贴图、拖动、形状填充、撤销、HDR 双图和真实 AWJ AVIF 输出。正式部署和剩余验收边界见 [续修记录](update-20261016.md)。本文之前章节中的网页端编译/测试记录保留为历史，不能替代续修结果。

````````
<!-- end:docs/vscap-2026-10-16-multipin-awj-hdr.md -->

<a id="record-docs-vscap-hdr-1-0-11-md"></a>

## docs/vscap-hdr-1.0.11.md

原始字节数：4975；SHA-256：`162356450cc5a9a1aed703cbf48fdef076a06fa11157b1c7a621b9e3a2209af3`。

<!-- begin:docs/vscap-hdr-1.0.11.md -->
````````markdown
# VSCap HDR 截图体积、颜色与加载续修

> 后续 [桌面 SDR 白位修复](vscap-sdr-white-1.0.11.md) 替代本文的桌面 ACES 映射：本文“与 VSP 一致”只代表两条旧转换一致，不能证明正确还原桌面。压缩、后台保存和加载记录仍按本文保留。

日期：2026-10-17。保持 1.0.11；先完成本地编译部署，按后续授权推送源码并补充现有 1.0.11 发布说明。GitHub 该版本已由维护者发布并上传附件，本轮仅更新说明，保持其发布状态与附件。用户原始文件与设置保留，Full 打包与二进制上传由维护者完成。

## 原因与改动

用户提供 `D:\Screenshot-HDR\20261017-102016-483.png`、`_SDR.png` 和原始 scRGB 附件。图像为 3840×2040、RGBA16；旧 PNG 各约 62.8 MB，解码像素数据为 62,668,800 bytes。解码后的内存等于展开像素数据是正常现象，不能以此判断磁盘压缩是否启用。本次实际核对 IDAT，确认旧文件为不压缩的 DEFLATE 块。

`QImageWriter::setCompression(9)` 的参数是 Qt 的 0～100 刻度，Qt PNG 实现再以 `(compression * 9) / 91` 映射到 zlib，原参数恰好变为 0。本次改为 70，即 zlib level 6，继续保留 16 位四通道和 ICC，不改 PNG 格式。依据：[Qt 6.10 PNG 实现](https://github.com/qt/qtbase/blob/6.10/src/gui/image/qpnghandler.cpp#L953-L967)。

此前热键回调在创建框选窗口前同步做 PQ 转换、两张 PNG 编码、125,337,600 bytes 的 `.scrgb-f32` 及 JSON 写入。现在先创建截图层，自动保存通过 QtConcurrent 在后台运行，失败仍在托盘提示。自动输出仅保留 HDR PNG 与 `_SDR.png`，HDR 信息由 PNG 内 ICC 携带；旧附件不删除。PNG 是整数 PQ 编码，不能将其宣称为原始浮点负值及超范围样本的逐位归档。

原 SDR 伴随图使用 Reinhard，VSP 使用 203 nit 参考白的 ACES luminance 映射，两者不一致。现在共用 `src/image/HdrPreview.h`：scRGB 按 80 nit 单位进入同一映射；PQ 反变换使用完整 16 位查找表，sRGB 编码使用查表与线性插值，保持原 HDR 图片供查看和导出。

播放器原快速 PNG 仅支持 RGBA8；本次支持 RGBA16、16 位大端通道还原及受支持的 cICP。其余格式继续由 Qt 解码。图片信息栏此前把逻辑像素直接当作视口和输出尺寸；现在乘入实际设备像素比，与视频信息的单位一致。绘图原本已由 Qt 做 DPI 缩放，这次修正的是读数。

## 实图与回归证据

本机 Windows 11、Qt 6.10.2 / MinGW Release。以下是已缓存文件的单次聚焦采样，不是磁盘全冷启动指标。

| 项目 | 结果 |
| --- | --- |
| HDR PNG | 62,769,454 → 1,281,649 bytes，像素逐点一致，ICC 单独识别为 BT.2100 PQ |
| SDR PNG | 62,769,435 → 1,229,430 bytes；颜色按共用映射重新生成 |
| 原始裸浮点附件 | 125,337,600 bytes；新截图不再自动生成，原样本未删 |
| SDR 映射计算 | 相同原始浮点图，约 1,632 → 204 ms |
| SDR 伴随图与 VSP | 全行、每 13 像素采样 RGB：平均差 0.296、最大差 58 个 16 位码值；差异来自 HDR PNG 的 PQ 量化与色域矩阵往返 |
| 原旧 PNG 加载 | 快速解码 129 ms；加载至预览完成 372 ms |
| 新压缩 HDR PNG 加载 | 快速解码 43 ms，Qt 对照解码 166 ms；加载至预览完成 362 ms |
| 高 DPI 信息 | 实际 DPR=2，窗口视口 2000×1220；信息与物理像素计算一致 |
| sRGB 查表误差 | 1,000,001 个输入对照解析公式，最大差 1 个 16 位码值 |

整张 HDR PNG 的转换与编码仍约 3.64 秒，两图保存总计约 4.29 秒；这些工作现在在后台完成，不再放在框选窗口出现之前。后台保存入队小于 100 ms、500 ms 内界面计时事件可响应的回归已通过，不将其冒称为真实 HDR 桌面热键端到端耗时。

回归日志位于 `build/audit-hdr-20261017/`：

- `cap-final.txt`：HDR/PQ、SDR、PNG 实际压缩、无 sidecar、用户实图，7 passed（含 init/cleanup）。
- `cap-workflow.txt`：模态截图、选区、标注、取消保存、后台保存、文字、贴图、剪贴板，共 15 passed。Windows 剪贴板偶有被占用的初次写入警告，既有重试路径及断言通过。本次未重跑 AWJ AVIF 编码专项。
- `png-final.txt`：8/16 位、ICC/cICP、取消代次及大图 16 线程预测，像素与 Qt 对照一致，6 passed。
- `player-original.txt` / `player-compressed.txt`：实际文件加载、高 DPI、HDR 预览保留源、16 位截图，分别 5 / 3 passed。
- `gamma-lut.json`：查表数值误差；`originals.json`：用户原始四个文件 SHA-256。

当前桌面采集返回 SDR；没有实际 HDR 输出可用于本轮验证。HDR 显示器 Alt+A 的 DXGI 初始化、GPU readback、出框时间及显示器观感仍需实机复验，不能用文件夹里的 HDR 图片替代该验收。

````````
<!-- end:docs/vscap-hdr-1.0.11.md -->

<a id="record-docs-vscap-interaction-1-0-10-md"></a>

## docs/vscap-interaction-1.0.10.md

原始字节数：8066；SHA-256：`82d0577df47515af83003a193625a57903f20ccf51ece58ac8f92e10c93f983f`。

<!-- begin:docs/vscap-interaction-1.0.10.md -->
````````markdown
# VSCap 截图与贴图交互修复

## 线宽、保存取消与图片文件贴图补充

2026-10-11，保持 1.0.10。本轮编译更新 `dist/VS-GUI-Full` 并推送源码，不打包、不修改 Release。

- 线工具同时显示线型选项和数字像素粗细，与画笔共用 `penWidth_`；切换工具保持当前值，直线 / 虚线 / 箭头均使用该粗细。
- 输出回调返回是否接受保存操作，取消路径选择或 PNG 写入失败不关闭截图窗口。文字、选区及绘图保持，可以再次保存；PNG 成功保存后仍关闭截图窗口，AVIF 保留既有异步编码通知。
- Alt+T 优先读取剪贴板图像 / 内部 scRGB 数据；没有图像像素时读取本地文件 URL，跳过远程路径与不可读取项，贴出第一张可读图片。文件解码使用 `QImageReader`，按图像方向元数据自动旋转。
- `build/cap-line-save-file-workflow.txt`：14 passed，0 failed。补充验证线/画笔之间 7→9 px 粗细共享；PNG 和 AVIF 均取消后窗口与文字保留，再次 PNG 保存成功；仅有文件 URL（含中文和空格路径）时可贴图并精确复制回原始像素，远程和非图片条目跳过。既有选区、文字、HDR、最近邻及实际 AVIF 编码继续通过。

## 框选与保存流程补充

2026-10-11，版本保持 1.0.10。本轮只编译并推送源码，不打包、不修改 Release；此前压缩包不含本次更新。

- 在“移植 VS-Cap 编辑贴图功能”会话的定位基础上修复：确定选区后缺少调整分支，复制回调无条件保存 PNG，绘图 / 文字过早限制在选区内。
- 截图工具栏增加“选区”，框选完成默认进入选区模式。四角 / 四边有八个拖动点和对应鼠标指针；框内拖动只改变裁切区域，已有文字 / 图形坐标不变。选择标注工具后可在整张截图上绘制和放置文字，直到复制 / 保存时才按最终选区裁切。贴图的编辑工具与原始尺寸导出保留。
- 打勾复制不保存文件。手动 PNG / AVIF 每次打开保存对话框，记住 `save/lastDirectory`，路径生成不再创建目录。设置新增“截图默认保存路径”（`save/defaultDirectory`），只用于 HDR 原始截图的自动保存，默认是图片目录下的 `VS Cap`。
- HDR 在进入截图界面之前自动保存原图的 PQ PNG、精确 `.scrgb-f32` 和 JSON 精度说明。通用剪贴板图像 / PNG 提供 SDR 预览并清除 HDR 标记；保留应用自定义 scRGB 数据用于内部高精度贴图。SDR 捕获和复制均不自动落盘，HDR 后续复制也不再次保存。
- `build/cap-selection-save-workflow.txt`：14 passed，0 failed。`selectionHandlesAndFinalCrop` 核对八个调整位置的导出像素、选区移动时文字不动、选区外箭头 / 画笔 / 文字保留及最终裁切；`clipboardAndSaveDirectories` 在临时目录核对 SDR / 复制无文件、HDR 三个附件与浮点字节一致、两次手动保存均弹窗、设置字段及两个目录独立。既有贴图、文字撤销、最近邻和实际 AVIF 编码继续通过。
- 本机是 SDR 桌面，自动 HDR 保存使用模拟 scRGB 图像验证，不作为真实 HDR 显示器的设备验收。最终可执行文件为 `build/mingw-release/vs-cap.exe`。
- 维护者随后明确固定部署目录为 `dist/VS-GUI-Full`：编译后的 VSCap 已同步到该目录，原有配置保留，资源清单及文件大小同步更新，部署后的 PNG 自检与屏幕捕获检查通过。本地运行目录不含版本号，压缩包再按版本命名；本次不打包、不修改 Release 或版本。

2026-10-11，1.0.10 的后续更新。首轮只推送源码；随后编译部署，并按维护者追加要求修复贴图尺寸与工具栏、重制 Full / PlayerLite 压缩包。GitHub Release Draft 保持原样，正式附件由维护者上传。

## 贴图尺寸与工具栏补充

- 移除初始 960×720 显示限制，默认图片像素与屏幕物理像素 1:1；计入 Windows DPI 缩放，仅超过鼠标所在屏幕可用区域时缩小，窗口初始位置限制在该区域内。滚轮仍使用等比最近邻缩放，导出尺寸保持原样。
- 贴图工具栏“✓”只隐藏工具栏；右键首项按可见状态显示“显示工具栏”或“隐藏工具栏”，两种状态均可切换。独立复制菜单保留，截图“✓”仍复制到剪贴板。
- 四边预留 2 个逻辑像素的蓝色区域，图片视图放在边框内；避免边缘笔画裁切或覆盖。
- 最终工作流 `build/cap-pin-final-workflow.txt`：12 passed，0 failed。更新原始尺寸和滚轮检查，增加工具栏切换与剪贴板不变检查；`pinScreenFitAndBorders` 验证小图 1:1、超屏大图适配、初始窗口在屏幕内、移动前后四边每个外沿像素为蓝色、导出原图像素一致。测试在本机 150% 桌面缩放下执行；棋盘截图已查看，四边完整。
- 部署保留本地配置；两个压缩包按资源清单收集文件，排除本机新增配置。包完整性与解压后 SHA-256 检查保存在 `build/package-cap-pin-1.0.10/package-audit.json`。PlayerLite 不包含 VSCap，本轮只同步说明和重新打包。

## 修改内容

- 工具条显式使用正常鼠标指针；字号控件的上下按钮分别安排点击区域。“✓”提示“复制到剪贴板”，矩形 / 圆形提示并支持 Ctrl 正方形 / 圆形，预览和最终绘制采用相同约束。
- 线工具保留直线 / 箭头并加入虚线。画笔粗细显示默认 3 px，允许直接输入数值；切换矩形 / 圆形工具时仍显示当前自定义线宽。
- 文字框虚线宽度改为 2，边框内缩并由画布统一绘制；静态截图预览缓存，拖动更新新旧区域，避免透明子控件搬移边框造成左 / 上边丢失。空框点击其它位置会移除，同时清理对应撤销记录；有内容的文字对象保留。
- 贴图通过等比视图显示，滚轮使用最近邻缩放。原始像素、文字坐标、尺寸与精度保持在编辑画布中；复制 / 保存合成原尺寸图像，显示缩放不影响导出。
- 贴图复用截图编辑工具栏；工具栏浮动于图片旁，避免小图裁切工具条。右键菜单为“显示工具栏 / 复制 / 保存为PNG / 保存为AVIF / 销毁”。工具栏隐藏时可以拖动贴图；文字编辑中 Esc 收起工具条。贴图窗口承接 Ctrl+Z，图片视图、浮动工具条和文字编辑均可撤销。

## 验证

Qt 6.10.2 / MinGW 13.1，Windows 11，150% 桌面缩放；本机实际桌面为 SDR。

- `vsr_cap_workflow_tests`：11 passed。新增 `toolbarShapesAndTextFrames` 验证正常鼠标、提示文本、默认 / 输入笔宽、字号上下按钮、Ctrl 正方形 / 圆形、虚线间隔、空框清理，以及连续向左上拖动 6 次时四条边均完整。
- `pinAspectZoomAndEditing` 验证 1600×400 图片初始显示为 960×240，滚轮放大为 1152×288；视图横纵缩放一致，截图棋盘像素只有黑 / 白，没有插值灰色。验证五项菜单、原尺寸 PNG / AVIF 交付、剪贴板尺寸、完整编辑工具、文字导出 / 撤销、Esc 收起工具条，以及视图和浮动工具条两种焦点下 Ctrl+Z 恢复原像素。
- `vsr_cap_tests`：设置 `VSR_CAP_DEVICE_TEST=1` 后 5 passed，含实际桌面捕获、SDR PNG 与模拟 HDR 精度 / 亮度；未把模拟浮点图像视为实际 HDR 显示器验收。既有工作流继续验证浮点文字导出、绘图撤销、16-bit 剪贴板和真实 AVIF 编码。
- 本地日志：`build/cap-interaction-workflow.txt`、`build/cap-interaction-native.txt`。截图在 `build/acceptance-cap-interaction/`，文字框四边及工具条外观已查看。

构建命令为 `cmake --build build/mingw-release --target VSCap vsr_cap_workflow_tests`；本地测试程序为 `build/mingw-release/vs-cap.exe`。首次交互修复未改发布目录或压缩包；后续部署及贴图补充更新了 1.0.10 目录和压缩包，运行配置保留，未重复视频解码性能测试。

````````
<!-- end:docs/vscap-interaction-1.0.10.md -->

<a id="record-docs-vscap-sdr-white-1-0-11-md"></a>

## docs/vscap-sdr-white-1.0.11.md

原始字节数：4497；SHA-256：`8b97fac7d925d06dacbfabecc7fdb86d72c2fe531539975a32869dd7e517f518`。

<!-- begin:docs/vscap-sdr-white-1.0.11.md -->
````````markdown
# HDR 桌面截图的 SDR 白位还原

日期：2026-10-17；版本保持 1.0.11。本轮修正 SDR 导出、预览及白位传递，保留 HDR 原有像素编码，不处理 Windows 照片、QQ 缩略图的 HDR 元数据兼容性。

## 现象与原因

用户补充：`D:\Screens\过曝.png` 在 Edge 显示正常，但 QQ 缩略图偏暗、Windows 照片偏亮；`SDR仍然过曝.png` 在 Edge 仍偏亮。HDR 文件带 BT.2100 PQ ICC，另两张图片是相同 ICC 的 sRGB；这将 HDR 显示路径的差异与 SDR 像素本身错误区分开。不能仅因某个查看器显示正常，就认定所有转换正确。

用共同壁纸的多个位置反推，本次历史样图的 SDR 白位约 480 nit。旧转换固定按 203 nit，再套 ACES 曲线，使正常的 RGB 约 (154,185,210) 变成约 (200,239,255)，产生提亮和通道裁剪。显示器校准 ICC 的约 710 nit 峰值与桌面 SDR 白位不是同一个数值，不能互相替代。

## 实现与边界

- 捕获 HDR 桌面时，通过 QScreen 的 HMONITOR / GDI 设备名匹配活动 DisplayConfig 路径，查询该 target 的 `DISPLAYCONFIG_SDR_WHITE_LEVEL`，换算为 `SDRWhiteLevel × 80 / 1000` nit。生产代码没有写死 480；查询不可用时采用 Windows 标称 80 nit，图像白位缺失、无效或非正数也按 80 处理。
- 将来源白位固定为图像的 `capSdrWhiteNits`，不在异步保存时重新读取目标显示器。线性 scRGB 先乘 `80 / sourceWhiteNits`，再编码 sRGB；有白位信息的 PQ 图先反变换到绝对亮度、转换色域，再按相同白位还原。
- 桌面路径不叠加 ACES 对比度曲线。白位内的 SDR 内容保持相对亮度和颜色；超出 SDR 亮度或色域的部分裁剪，不承诺 SDR 图保留全部 HDR 高光细节。HDR 原图的 PQ 编码像素保持不变。
- QImage 裁切保留白位；新 HDR PNG 内写入普通 PNG 文本字段，快速解码读取此字段。HDR 剪贴板通过独立 MIME 项传递白位，贴图往返保留；标注的颜色也按源白位绘制。
- 普通未标记 HDR 图片的通用 VSP 映射保留。历史 PNG 没有保存源白位，软件不从图片或显示器峰值猜测 480 nit。本轮只在历史样图验收中明确注入由样图对照推断的 480 nit。

依据：[微软白位字段与单位](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_sdr_white_level)、[Windows HDR 桌面的 SDR 参考白](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range#match-your-apps-reference-white-to-the-os-sdr-reference-white-level)。

## 验证

审计目录 `build/audit-sdr-white-20261017/`：

- `before.txt`：先加入复现测试，旧逻辑在 80 / 203 / 480 nit 三组全部失败。
- `cap.txt`：修复后三组 SDR 梯度还原误差不超过 1 个 16 位码值；覆盖 PNG 白位、裁切、无效元数据、负值和超范围高光、原 HDR PQ 像素及压缩，共 11 passed（含 init/cleanup）。
- `desktopHdrFixture`：用户样图的 7 个共同壁纸采样位置，换算 8 位 RGB 后平均误差 0.150、最大误差 1.012；完整 3840×2034 图的映射约 0.23 秒。两张图的窗口内容不同、尺寸不同，未将这些采样宣称为全图逐像素相同。
- `workflow.txt`：截图模态、选区、文字、撤销、取消保存、后台保存、贴图及 HDR 剪贴板白位往返，共 15 passed。已有剪贴板占用重试可能记录初次写入警告，最终断言通过。
- `png.txt`：8/16 位快速解码及白位字段保留，4 passed。
- `player.txt`：80 / 203 / 480 nit 中性灰显示、带白位 HDR PNG 的实际加载、未标记 HDR 的原查看路径及 16 位截图，共 6 passed；样图解码约 99 ms、加载至预览约 369 ms。
- 修正后的 SDR 样图：`capture_SDR.png`。带 480 nit 验收字段的 HDR 对照副本：`capture_HDR.png`，用于播放器完整加载测试，不覆盖用户文件。原始三图和显示器 ICC 的 SHA-256 记录在 `originals.json`。
- `image-verification.json`：四个用户原文件哈希保持不变，HDR 对照副本解码为 RGBA16 后的 SHA-256 与原图完全一致。

本机当前桌面为 SDR；没有将样图、数值回归或 Win32 编译成功冒称为实际 HDR 显示器白位查询、多屏切换及 Edge / Windows 照片实机验收。HDR 屏实时截图仍待复验。旧 `D:\Screenshot-HDR` 的浮点验收素材已不可读取，本轮不以该失效路径的结果作为回归结论。

````````
<!-- end:docs/vscap-sdr-white-1.0.11.md -->

<a id="record-docs-workspace-cleanup-md"></a>

## docs/workspace-cleanup.md

原始字节数：1922；SHA-256：`d17461e47b17ea016be2d131707ffb5161d366401cfedd5d04e68a7bf246f32e`。

<!-- begin:docs/workspace-cleanup.md -->
````````markdown
# 开发工作区清理记录（2026-10-03）

## 手动清理入口

主目录 `Clean-Workspace.bat`已提供，双击执行，结束保留窗口；`Clean-Workspace.bat --preview`仅预览。BAT只调用tools/clean-workspace.ps1，由PowerShell核对边界/链接、保留INI/VPY、删除固定候选并记录结果，不跨shell拼接删除。Windows PowerShell语法与只读预览已验证，代理未执行删除模式。

最新复查原24个候选已不存在，不归因于代理删除。体积优化新生成的FFmpeg/Shader/裁剪备份和试验目录约2.09 GiB也加入候选。当前Release、便携输出、Git、源码及retained-user-files保留。实际手动结果记录于build/workspace-cleanup-result.json；下面是先前盘点的历史状态。

当前实际文件占用约11.81 GiB，其中build约8.22 GiB、.deps约1.90 GiB、dist约1.14 GiB。已识别24个历史构建、发布副本、解包验证副本、旧FFmpeg备份、重复依赖构建及生成测试素材，合计约7.79 GiB。

计划保留当前 `build/mingw-release`、`dist/VS-Renderer-GUI-windows-x64`、当前依赖源码/工具、Git与全部开发改动。历史副本里的41份INI/VPY已复制到 `build/retained-user-files`，逐文件哈希一致。当前源码、图标、着色器、文档、程序、配置和预设共1171份受保护文件检查无变化。

删除尚未执行：执行策略先后拒绝按清单变量批量删除和明确绝对路径删除，返回 `blocked by policy`，没有更具体原因；未改用其他接口绕过该限制。工作区仍约11.81 GiB，不能视为清理完成。

精确候选路径、清理前大小与受保护文件哈希见 `build/workspace-cleanup-plan.json`；当前状态见 `build/workspace-cleanup-audit.json`。候选中的旧FFmpeg备份尚在，没有失去回退文件；旧测试素材尚在。仅做盘点和小体积配置保留，没有推送或打包。

````````
<!-- end:docs/workspace-cleanup.md -->

<a id="record-readme-md"></a>

## README.md

原始字节数：22891；SHA-256：`eedbe9f9df83b7e24f77ad7c9dd2785a8d1a1aa4470a2318c294e9aa83f45c50`。

<!-- begin:README.md -->
````````markdown
# VS Renderer / VS Player

1.0.12 追加软件解码优化：8K420p10 AV1 默认 8 个工作线程、双槽帧交接、双组 DEFAULT 纹理与显式 STAGING 上传，HEVC 加入异步解码交接。内置 Anime4K 对原始高度超过 2160p 的视频禁用；专门测试仍可加载自定义预设。修正信息面板重复扣除合并帧的帧率统计，底栏标签背景统一，底栏与右键／倍速菜单不透明度可调，新配置默认 100%。性能范围与开关见 [内核优化记录](docs/player-kernel-performance-1.0.12.md)。

BD 整卷 VS 播放优先索引当前 M2TS，首帧后后台顺序准备后续片段；菜单选集续播位置保留。跳到未索引片段时仅等待目标片段，详见 [1.0.12 记录](docs/update-1.0.12.md)。

Windows 桌面视频处理与播放工具：**VS Renderer** 用图形化滤镜链生成 VapourSynth 脚本、预览和导出；**VS Player** 独立播放视频、音频与图片，并可加载 VPY 实时滤镜。

## 下载与启动

正式运行目录不保留旧的截图/播放器 `*-next.exe` 暂存构建；`runtime/tools/aria2-next.exe` 是下载工具名称，应保留。

开发机 Full 固定部署到 `dist/VS-GUI-Full`，编译更新直接同步该目录并保留配置。目录名不带版本号，最终压缩包再按版本命名（压缩包名称由发布版本决定）。

当前本地版本 **1.0.12**：修复 Jinc 直通显著偏暗及无音轨视频启用 Anime4K 后跳到结尾，见 [色彩与 VS 验证](docs/player-color-vs-1.0.12.md)。修复整卷连续播放点击 / 拖动无法跳转，BD 拖动进度条在松开时只定位最终目标；光盘菜单模式直接进入选集页，跳过版权首播片。见 [1.0.12 记录](docs/update-1.0.12.md)。

本轮续修：VS 预取帧在短暂延迟后顺序追赶；手动 Anime 档位允许大文件冷索引，不再按文件大小推测硬盘类型并强制回退。底栏编码标签、倍速与右键菜单改为半透明，见 [Anime 播放验证](docs/player-anime-performance-1.0.12.md)。

1.0.11 更新：修复 BD 右键与播放列表显式返回主菜单入口及文件章节位置，Renderer 时间与进度条同排、文件/播放/缩放控件同排，预设携带时间组并支持全片单组导入当前组。预设管理与社区递归显示目录树，默认源是 [VS-GUI 公开预设社区](https://www.modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI/PresetCommunity)。

本轮 SDR 白位修复与 JXL 截图更新继续沿用 1.0.11，详情见 [发布说明](https://github.com/user-Wing/VS-Renderer-GUI/releases/tag/v1.0.11)。

VS 后端更新为 **VapourSynth R81 + BM3D 11.0 + BM3Dvk2 1.0 + BestSource 22**，已验证 Vulkan 解码及 GPU 时域降噪。3FP 更新至 API20 / PR23（含 PR22，保留本地扩展）；指定 8K 软件播放复测仍未通过，详细条件、差距及限制见 [1.0.11 本地更新记录](docs/update-1.0.11.md)。HDR 续修继续沿用 1.0.11，源码与现有发布说明同步更新，Full 由维护者自行打包上传；另按要求生成单文件独立播放器 `dist/VS-Player.exe`，启动器已嵌入 Player 图标，首次运行自动在同目录创建 `player.ini`，保留已有配置，内置 Qt；单文件版不再内置 FFmpeg，启动时先查 EXE 同目录，再查 `PATH`，缺少时提示所需组件。需要匹配的 **FFmpeg shared x64**（含 `ffmpeg.exe`、`ffprobe.exe` 和 `avcodec-63` / `avformat-63` / `avutil-61` / `swresample-7` / `swscale-10` / `avfilter-12` DLL 及其依赖），仅静态版 `ffmpeg.exe` 不够。

VS Player 非全屏时底栏独立放在视频下方，初始窗口高度相应增加；全屏继续使用可自动隐藏的浮层。VSCap 设置窗口打开时仍可截图，截图层接管输入，Esc 退出后可继续修改未保存的设置。矩形 / 圆形的点按钮可点击选择边框模式，保留滚轮调粗；点和填充方块互斥显示选中状态；只放大方块内部图标，两个按钮保持与相邻工具相同的高度。

VSCap 框选完成后默认使用“选区”工具：拖动四角或四边调整大小，拖动框内移动选区，已有文字和图形保持截图上的原位置。切换到标注工具后可在选区外绘制或输入，复制 / 保存时才按最终选区裁切。

截图“✓”只复制到剪贴板。VSCap 新配置默认 **JXL 无损 RGBA16**，顺序为 JXL、AVIF、PNG，保留已有明确选择的格式设置。普通截图不会自动保存；HDR 先打开截图界面，再后台按所选格式保存到“截图默认保存路径”，JXL / PNG 继续附带 `_SDR` 副本，无 JSON / 裸浮点附件。JXL 原生携带 HDR 色彩信息，VSP 内置编解码，无需 FFmpeg；剪贴板仍提供 SDR 预览。SDR 副本按源屏幕的 Windows SDR 白位还原桌面颜色，超过 SDR 亮度或色域的部分裁剪，完整高光保留在 HDR 原图中。三种格式手动保存每次都选择路径，并记住上次目录；手动目录与 HDR 默认目录分别记录。见 [JXL 截图](docs/jpeg-xl-1.0.11.md) 与 [白位修复](docs/vscap-sdr-white-1.0.11.md)。

取消 JXL / AVIF / PNG 保存会保留截图窗口、选区和标注，可以继续编辑或重新保存。Alt+T 同时支持剪贴板图像与资源管理器 Ctrl+C 复制的本地图片文件（含 JXL）；文件列表中每张可读取图片分别建立贴图。

VSCap 的“文字”工具可直接点击截图选区输入多行文字。点击框内随时编辑，拖动虚线边缘移动；左侧可输入系统字体名称自动补全、直接输入字号，以及切换加粗 / 斜体。多个文字框分别保留样式，保存、复制或贴图时合成文字，虚线框和光标不会进入图片；Ctrl+Z 可撤销文字、样式、移动及绘图操作。

文字框的虚线加粗，空框点击其它位置会自动消失。工具条使用正常鼠标指针，字号上下按钮均可点击；“✓”提示“复制到剪贴板”。矩形 / 圆形工具提示并支持按住 Ctrl 画正方形 / 圆形，线工具提供直线 / 虚线 / 箭头；线与画笔均显示数字像素粗细并共用当前值，初始值为 3 px。

贴图默认按原始像素大小显示（计入 Windows 桌面缩放），仅在超出当前屏幕可用区域时等比缩小，四边均显示蓝色边框。鼠标放在图片上滚轮缩放，使用最近邻插值。右键菜单提供“显示 / 隐藏工具栏、复制、保存为JXL、保存为AVIF、保存为PNG、销毁”；贴图工具栏的“✓”收起工具栏，截图工具栏的“✓”仍复制到剪贴板。工具栏使用同一套截图编辑工具，复制和导出保留原始尺寸与精度。工具栏隐藏时可拖动贴图，文字编辑中按 Esc 收起工具栏；Ctrl+Z 在图片视图、文字和工具条中均可撤销。交互与像素验证见 [验收记录](docs/vscap-interaction-1.0.10.md)。

当前版本为 **1.0.11**，本地运行内核为 API20，Full 与独立 **PlayerLite** 的功能边界继续保留；历史压缩包不代表本轮最新构建。Full 新配置默认在 HDR→SDR 时使用高级色彩映射；SDR 保留原生路径。色彩设置按钮统一中文，VSCap 支持工具选中框、常驻颜色和参数、直线/箭头及 Ctrl+Z 撤销。历史性能结果与未完成项见 [1.0.10验收](docs/validation-1.0.10.md)，发布草稿说明见 [1.0.10](docs/release-1.0.10.md)。已有配置不会被重置：旧配置如选原生模式，可在色彩引擎中改选“自动：HDR 映射 SDR 使用高级色彩管理”；PlayerLite 不含高级色彩运行时。正式 Release 与二进制附件由维护者上传。

修复无音轨视频直通启动及跳转时按显示器刷新速度推进画面的问题：首帧音频门控仅在存在音频输出时启用，无音轨视频按媒体时间戳等待呈现。原因与真实文件验收见 [启动时钟记录](docs/player-startup-clock-1.0.8.md)。

修复从光盘播放列表加载单集时开头约 1 秒花屏：分段描述里的 `inpoint` 原等于 PlayItem IN，而该值恰好是片段首个 IDR 的时间戳，FFmpeg 拼接解复用器定位后会跳过该 IDR，解码器在没有参考帧的情况下开始工作。现在 `inpoint` 前移 1 ms，`outpoint` / `duration` 与作者时间轴不变。原因、逐帧哈希对比与段帧数见 [拼接前置记录](docs/player-bd-concat-preroll-1.0.8.md)。

VSP“视频 → 图像截取”支持 **PNG / AVIF / JXL**，分别可截取源画面或含字幕、显示效果的实画面；“截图到指定路径…”也支持三种格式。PNG / JXL 每通道 16 位无损；JXL 原生保存 HDR，无浮点附件。AVIF 质量 70（SDR 10 位、HDR 12 位），Full 使用 AWJ，独立版使用外部带 libaom-av1 的 FFmpeg。PNG 保留原有 HDR / SDR 与浮点附件流程。默认保存到程序旁 `screenshots/`，可在基本设置修改路径，或单次选择文件夹。见 [JXL 截图](docs/jpeg-xl-1.0.11.md)。

1. 从 [GitHub Releases](https://github.com/user-Wing/VS-Renderer-GUI/releases) 或 [ModelScope 更新源](https://modelscope.cn/datasets/ARXChem/Software-List/tree/master/VS-GUI) 下载完整便携包。
2. 解压整个目录，运行 `VSRenderer.exe` 或 `vs-player.exe`。无需另外安装 Qt、Python、VapourSynth。
3. 系统要求：Windows 10 22H2 或更新的 64 位 Windows，支持 Direct3D 11 的显卡与驱动；RIFE 补帧需要 Vulkan 驱动。

1.0.7 更新：HEVC / AV1 软解线程上限提升至 32，软件帧队列预算提升至 512 MiB，解码与取帧提交分开运行；自动匹配同目录的字幕名称变体，并可在右键字幕菜单切换。具体条件与验证见 [软件解码更新](docs/software-decode-subtitles-1.0.7.md)。上一轮已制作 `1.0.7.7z` Full 与 `1.0.7-Lite.7z`，推送源码及准备无附件 Release Draft，二进制附件与正式发布由维护者操作。见 [1.0.7 发行说明](docs/release-1.0.7.md)。

底栏算法框按实际档位显示 `A4KCNN+ / A4KCNN / A4K+ / A4K / Jinc / D3D11`。启用补帧时显示两个框：`Jinc / D3D11` 与 `RIFE+ / RIFE / MVT+ / MVT`，不再用单独一行文字显示常驻直通状态。

主题设置可分别调整底部控制栏和右侧播放列表的透明度。全屏控制栏约 1 秒无操作自动隐藏，鼠标进入其实际高度范围即可唤出；顶部可唤出标题和退出全屏按钮。Tab 优先列出当前使用的文件，透明度条独立置于文字下方，并可开启右侧音频详细信息，显示真实输入 / 输出声道峰值电平 (dBFS)。使用和验证见 [透明控制栏与音频信息](docs/player-transparent-audio-info-1.0.7.md)。

1.0.12 PlayerLite 是独立播放器组件，只保留 3FP、D3D11/Jinc、音视频字幕及图片查看；不包含 Renderer、VapourSynth、RIFE、PhotoCraft、libVLC、LAV、madVR 和高级色彩运行时。Full 保留完整功能与依赖。既有 1.0.6 已由维护者发布，历史说明见 [1.0.6 发行说明](docs/release-1.0.6.md)。

## 简要更新说明

1.0.6(2026.10.5 下午发布) 组件分离：本体/ff/mkvtoolnix/madvr/lav 分开下载和更新；加入 LibVLC 实现 BD 主界面字幕选择和和快速直通，Renderer 同样增加了一键 BDRemux 功能；HEVC/AV1 y410 从 8 线程改为 CPU 最大值，大幅提高软件解码效率。

1.0.5(2026.10.5 早上发布) 大幅优化 Jinc/D3D11 直通模式，780M 核显可 D3D11 直通 8K48F--4K48F 零丢帧；Jinc 直通也能够零丢帧，本人的目标是 Anime4K No CNN 左右的质量档能在核显上面跑。

1.0.4 图像编辑功能大增强，构建了类 PS 模式的编辑器，支持 RAW/PSD 高精度处理。

1.0.3 内置 RIFE / MVTools 补帧自动链 + 六档 Anime 超分链路实现

1.0.2 VSPlayer 基础实现接入(3FP目前最佳色彩实现) + 四档超分链路实现 +Renderer 详细导出支持

1.0.1(20260930) VS Renderer 预览和比对完成+Anime4K支持

1.0.0 VS Renderer基础功能实现

## 核心功能

1.0.5 **Full** 内置 MKVToolNix 102.0；Player 设置新增“组件下载”，从 ModelScope 展示软件与组件版本，支持自动下载更高版本、缓存清理及固定目录安装。Player 打开 BD 直接加入播放列表，Remux 操作位于 Renderer；Tab 透明度默认 50%，设置窗口优先。详见 [组件与播放器更新](docs/components-player-1.0.5.md) 和 [1.0.5 发行草稿](docs/release-1.0.5.md)。

2026-10-05 原生后端补充大分辨率 AV1 软件解码线程、平面上传与 D3D11 专用 shader 优化；本地 dist 已更新核心。780M 真实 8K 软件回退改善但仍未达 48fps，AV1 444 硬解限制与精度回归见 [软件回退记录](docs/av1-software-decode-1.0.5.md)。

2026-10-05 本地新增 **BD Remux** 页面和 Player 右键“打开 BD 文件夹 / 光盘镜像”：读取 MPLS 自动列出剧集候选，支持完整节目、手动 M2TS、自定义匹配模板、结构反馈和 MKVToolNix 无重编码队列；网络挂载可通过现有 ModelScope Manager 下载到本地。操作与真实验收边界见 [BD 使用说明](docs/bd-remux-1.0.5.md)。

| 程序 | 功能 |
| --- | --- |
| VS Renderer | 可排序、可开关的滤镜链；参数编辑与 VPY 预设；源画面和处理结果同步预览 |
|  | Anime4K、降噪、锐化、去伪影、反交错，以及 MVTools / RIFE 补帧 |
|  | 最多九路视频比对、时间对齐、并排/网格/滑块；单文件、批量及对比画布 MKV 导出 |
| VS Player | 视频、音频、网络链接与超大图片；倍速、逐帧、章节、播放列表、缩放与截图 |
| | VPY 实时处理，自动或手动 Anime 档位；主次字幕、音轨切换、同步与均衡器 |
| 两个程序 | 随包运行时、便携配置、检查更新 |

## VS Renderer 操作

1. **导入视频**：在“VS 实时渲染”中打开或拖入源文件。
2. **编辑处理链**：从滤镜库添加节点，调整顺序、开关与参数；在“参数与脚本”查看生成的 VPY，播放比较源画面与处理结果。
   **mpv GLSL**：按功能分类选择或加载自定义 GLSL；输出可选倍率或指定分辨率，开启“保持宽高比”后宽高联动。悬停查看完整着色器路径及硬件要求。
3. **保存预设**：使用顶部“预设管理”保存/加载处理链，或“加载 VPY”使用现有脚本。
4. **导出**：打开导出窗口，粘贴 3FUI 的 FFmpeg 命令，保留输入和输出位置；选择 MKV 输出并加入编码队列。批量文件放入“准备文件”，统一加入队列。

**多视频比对**：左侧导航切到“图像分析比对”，导入多个视频，选择并排、网格或滑块布局；用各路时间轴调整偏移，用源下拉框切换视频，用音频下拉框选择声音来源。滚轮缩放，放大后拖动画面。

**对比画布导出**：使用“导出对比画布…”。支持 AB 滑块、ABC 左右 2+1、ABCD 常规/滑块 2×2；输出保持最大源分辨率，其它布局会提示不支持。

## VS Player 操作

播放器的“设置 → 解码设置 → 打开3FP 解码配置…”提供可选 libplacebo 高级色彩管理，默认保留原生 709 / 2020 模式。支持 HDR→SDR / scRGB、ICC、输出 LUT、末端抖动及 Tab 诊断；应用配置不会重新加载 VS 处理链。实际能力与 HDR 边界见 [色彩管理说明](docs/color-management-1.0.4.md)。

打开或拖入视频、音频、图片，也可从右键菜单打开文件夹或网络链接。底栏提供逐帧、关键帧、前后文件、时间/帧号输入和倍速；画面右键菜单提供预设、音频、字幕、截图、缩放算法及设置。

D3D11 / Jinc 直通及 VS 模式支持拖动进度条时连续预览；松手精确定位并恢复原暂停或播放状态。帧号修改后按 Enter 或点击其它位置提交，跳转未完成时 Enter 不触发全屏。Tab 信息面板可从右下角调整大小，下方滑条调整背景不透明度；本轮验证与性能未达标项见 [交互与调度记录](docs/player-interaction-performance-1.0.5.md)。

停用预设的原画播放同样使用 3FP 直通，不建立 VS 帧索引；Jinc / D3D11 不等待 VS 初始化。内嵌 ASS / SRT 字幕按播放位置增量读取，不再为了提取字幕扫描整部视频。自定义 VPY 和增强 / 补帧仍可能需要首次索引。大文件加载验证边界见 [直通加载说明](docs/player-direct-open-1.0.4.md)。

**图片工具栏**：点击“PhotoCraft 图像编辑器”打开原生 Rust 编辑工作区，完整采用 PhotoCraft 的菜单、工具组、参数栏、文档标签和右侧面板。裁剪、调整尺寸、文字、图层及滤镜均在此窗口操作；同一播放器重复打开同一路径会切回已有文档，关闭播放器后编辑器仍可保存未完成的工作。

图片查看栏保留旋转、镜像、回收站、桌面背景，以及“高精度导出和格式转换”。后者继续使用 [AWJimage](https://github.com/Dominic485649/AWJimage) 的 AVIF、WebP、JXL、JPEGli、PNG 编码（该后端需要 AVX2）。PhotoCraft 的文件保存方式以自身菜单为准；“另存为”会将当前文档关联到新路径，请选择新文件名以保留原图。

普通图片、PSD/PSB 和支持的 RAW 直接打开原文件；超大图片复用 VSP 原有解码器，以 32 MiB 分带交换完整分辨率、位深、透明度和 ICC，已验证 48000×32000 PNG；旋转后的视图及 AVIF 等原生编辑器暂不支持的格式，通过 16 位 PNG 或 32 位浮点 TIFF 副本传入，保留 ICC 和 HDR 原始值。视图副本需要在编辑器中另存为。VSP 的高精度截图与 SDR 显示回退继续保留。运行无需安装 Rust，接入、体积和验收见 [PhotoCraft 编辑器记录](docs/photocraft-editor-1.0.8.md)。

| 操作 | 快捷键 / 鼠标 |
| --- | --- |
| 打开文件 / 加载 VPY | `Ctrl+O` / `Ctrl+P` |
| 播放 / 暂停 | `Space` |
| 切换全屏 / 退出全屏 | `Enter` 或 `F11` / `Esc` |
| 显示输入、输出与运行信息 | `Tab` |
| 视频前后跳转 | `←` / `→`，默认 ±1 秒，设置中可改为帧或关键帧 |
| 较大跨度跳转 | `Ctrl+←/→` 默认 ±10 秒；`Ctrl+Alt+←/→` 默认 ±60 秒 |
| 切换图片 | `←` / `→`，按同文件夹名称排序 |
| 调整音量 / 浏览图片 | 视频滚轮默认调音量，右键可改鼠标位置锚定缩放；图片滚轮缩放，Shift滚轮平移 |

- **滤镜预设**：右键 → VapourSynth 预设 → 开发者内置，选择自动 Anime、Realistic 或六个手动固定档位；用户 VPY 可通过 `Ctrl+P` 加载。Renderer 的 mpv GLSL 节点支持按名称搜索随包着色器。图片和网络视频不执行 VS 滤镜。
- **实时补帧**：右键 → 图像处理 → 补帧，选择 Jinc / D3D11 两个自动入口，或 RIFE 完整/半宽高、MVTools HQ/LQ 与两个呈现器组成的八个固定组合。保持 YUV444P16 输出；关闭恢复 Anime 自动超分。实时性能取决于素材与设备，完整 RIFE / MVTools 的本机无丢帧目标尚未全部通过。
- **音频与字幕**：右键切换音轨、调整同步、打开均衡器或选择主次字幕。同目录名称匹配的音频/字幕可自动加载；播放视频时，音频拖到画面上半部作为外部音轨，下半部单独播放；字幕上40%挂为次字幕，下60%挂为主字幕。
- **设置与关联**：设置左下角加载/保存完整 INI 配置；“确定”保存并关闭，“应用”保留窗口及暂停/播放状态。文件关联页注册所选格式后，在 Windows 默认应用中选择 VS Player。

**图层编辑**：支持 RGB PSD/PSB 像素图层、层组、蒙版及有限混合模式；选择、修复/图章、画笔、渐变/填充、局部调整、栅格化文字/形状、色阶/曲线和ICC工作空间转换保持8/16bit或Float32精度。图片共用一个编辑窗口的文档标签，普通背景层锁定，PSD保留原锁定；图层右键提供混合选项和常用操作，见 [文档与混合选项](docs/image-editor-workflows-1.0.4.md)。文件菜单可另存支持的PSD/PSB图层，或导出扁平PNG/浮点TIFF。新建Float32文档缺少Photoshop HDR色彩数据时应选TIFF；DNG和完整复杂PSD对象尚未支持。

HDR显示自动检测Windows能力并回退SDR，视图菜单可调整HDR→SDR映射；真实HDR面板观感尚未验证。操作与限制见 [第二轮编辑说明](docs/image-editor-round2-1.0.4.md)。

图片不使用 VS 或视频预解码，动图目前显示首帧；图片解码使用 CPU。滤镜实时速度取决于素材、分辨率和硬件，可切换较轻的预设或离线导出。

HEVC 444p10 高码率尖峰的本机复现、队列预算与解码线程对照见 [尖峰性能诊断](docs/hevc444-peak-diagnosis-1.0.6.md)。实验参数尚未部署为默认值。

1.0.4开发版采用增强版FFmpeg，支持AVS3编解码及更多旧格式。新增编码器可在导出自定义命令中指定；见 [FFmpeg运行时说明](docs/ffmpeg-enhanced-1.0.4.md)。音频/字幕菜单显示轨道名称、语言和Default/Forced，默认使用编号最前的Default轨道，无Default时选择同类首条音频/字幕。程序文件约625 MB，用户缓存另计；见 [轨道与体积](docs/tracks-size-1.0.4.md)。

## 文档与源码构建

- [更新日志](changelog.md)：版本改动、性能测量与验证记录。
- [依赖与构建指南](docs/dependencies.md)：Qt / CMake、VapourSynth、3FP 和完整运行时准备步骤。
- [NuxBox 性能开发交接](docs/nuxbox-performance-handoff.md)：780M 上四份本地素材、mpv 对照条件、原生构建和验收；干净开发包由 `tools/package-dev-handoff.ps1` 生成。
- [便携更新说明](docs/portable-updates.md)：配置保留、更新包与安装流程。更新包兼容 1.0.2 更新器，安装时保留已有目录路径。
- [项目地图](project.md)：实现路径与专项技术文档。
- [工作区清理记录](docs/workspace-cleanup.md)：开发副本与缓存占用；主目录 `Clean-Workspace.bat` 可手动清理，`--preview`仅预览。

源码采用 [MIT License](LICENSE)，随包第三方组件遵循各自许可证，见 [第三方声明](THIRD_PARTY_NOTICES.md)。

1.0.8 延迟加载 3FP、VS/Python、字幕库和 GPU 计数器，直接看图不初始化视频后端。Renderer 对比默认使用 D3D11 原生直通，并保留可选高精度算法。HEVC Rext 4:4:4 10 位在 D3D11VA 不支持时使用 NVDEC/CUDA；正常 D3D11VA 路径保持不变。CPU 预缩放位于解码设置，默认关闭，仅作为实验项，性能验收留待专项更新。实测条件见 [性能与大图验收](docs/performance-large-images-1.0.8.md)。

````````
<!-- end:README.md -->

<a id="record-project-md"></a>

## project.md

原始字节数：20184；SHA-256：`606c8ff1f77da128e22c72aa737ceaa1c4e8c475eac2e069340ef7aeee04d26e`。

<!-- begin:project.md -->
````````markdown
# VS Renderer / VS Player：项目开发地图

1.0.12 追加软件解码优化：8K420p10 AV1 默认 8 个工作线程、双槽帧交接、双组 DEFAULT 纹理与显式 STAGING 上传，HEVC 加入异步解码交接。内置 Anime4K 对原始高度超过 2160p 的视频禁用；专门测试仍可加载自定义预设。修正信息面板重复扣除合并帧的帧率统计，底栏标签背景统一，底栏与右键／倍速菜单不透明度可调，新配置默认 100%。性能范围与开关见 [内核优化记录](docs/player-kernel-performance-1.0.12.md)。

BD 整卷 VS 播放优先索引当前 M2TS，首帧后后台顺序准备后续片段；菜单选集续播位置保留。跳到未索引片段时仅等待目标片段，详见 [1.0.12 记录](docs/update-1.0.12.md)。

> 本文面向接手开发的维护者与 AI，负责说明架构、模块入口、约束与当前状态，不记录逐日开发流水账。用户指南见 [README](README.md)，版本变更见 [changelog](changelog.md)，专项证据见 [docs](docs/)。核对基线：2026-10-17，本地版本 1.0.12；BD 拖动只在松开时提交最终定位，原生 concat 使用全局时间定位以恢复整卷跨段点击 / 拖动；光盘菜单模式直接进入选集页。压缩包由维护者制作，验证见 [1.0.12记录](docs/update-1.0.12.md)。

Jinc 权重使用 GPU t5，与色彩变换 LUT t4 分离；修改 HLSL 后必须重生通用与 SDR 专用字节码。无音轨 VS clock-only 以墙钟推进，不等待视频队列、不提前读到 EOF。指定 SDR 与 Anime4K 验证见 [色彩与 VS 记录](docs/player-color-vs-1.0.12.md)。

播放器在 VS 预取范围内顺序追赶短暂调度延迟，避免跳过已计算帧。手动 Anime 六档不再因文件超过 1GB、缺少索引或首帧等待超过八秒而被强制切换；旧逻辑没有检测磁盘介质。底栏标签、倍速和右键菜单采用半透明背景。验证条件见 [Anime 播放续修](docs/player-anime-performance-1.0.12.md)。

本地清理约定：旧截图/播放器 `*-next.exe` 暂存构建清除后同步资源清单；`aria2-next.exe` 是有效运行依赖，不按后缀误删。

当前部署：`dist/VS-GUI-Full` / `dist/1.0.12-PlayerLite`，1.0.12 源码上传 GitHub，Full 由维护者自行打包上传；追加交付独立单文件 `dist/VS-Player.exe`（启动器嵌入 Player 图标，运行时缓存展开，同目录自动创建并保留 `player.ini`，内置 Qt，FFmpeg shared x64 从 EXE 同目录 / PATH 解析，缺失先提示）。VapourSynth 后端固定 R81，BM3D 11.0、BM3Dvk2 1.0、BestSource 22；原有插件保留，使用 `tools/update-vapoursynth-runtime.py` 维护锁定版本和哈希。社区配置使用公开页面链接，目录遍历/下载使用内部解析地址。脚本型内置预设的处理脚本随时间组序列化；章节统一由 ThreeFpPlayer 适配 `startTime100ns`。详细修改、证据及性能未通过项见 [1.0.11记录](docs/update-1.0.11.md)。

窗口模式播放器为底栏保留独立高度，视频区域不与底栏重叠；全屏继续浮层。VSCap 在已有模态窗口时打开截图层，会将截图层设为当前模态窗口，保存对话框随当前模态窗口建立父子关系，防止设置阻断置顶截图层输入。矩形/圆形的边框与填充按钮互斥，边框按钮保留滚轮调粗；填充图标单独绘制，按钮高度与相邻工具一致。

VSCap 新配置默认 JXL，顺序 JXL / AVIF / PNG，保留已有显式配置。HDR 后台自动保存跟随格式，JXL / PNG 继续附带 SDR 副本，无 JSON / 裸浮点附件。`DesktopCapture` 查询 `DISPLAYCONFIG_SDR_WHITE_LEVEL`，`capSdrWhiteNits` 经选区、PNG、JXL XML、HDR 剪贴板和贴图传递。`HdrPreview.h` 撤销桌面 SDR 增益；普通 HDR 图仍走通用映射。`ImageJxl` 使用静态 libjxl 0.12.0 无损 RGBA16 编解码、原生 PQ / HLG 标记，不依赖 FFmpeg。VSP 两个截取菜单均支持 PNG / AVIF / JXL；AVIF Full 复用 AWJ，Lite 回退外部 FFmpeg，PNG 原有 HDR 附件行为保留。图片视口按物理像素报告。见 [JXL 记录](docs/jpeg-xl-1.0.11.md) 与 [白位修复](docs/vscap-sdr-white-1.0.11.md)。

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

注意：① `FFF.Native.dll` 与 FFmpeg shared major ABI、补丁和运行时文件必须匹配；② 只复制 EXE 不是便携部署，必须随附 Qt、VS/Python 及所选后端；③ 冷启动 shader 编译、索引耗时不能当作稳定播放帧率。当前 native API20（PR23，含 PR22）构建入口见 `tools/build-native-api20.ps1` 和 [固定依赖说明](docs/native-api20-build.md)；不要混用 API 14 历史补丁。

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

**按任务找文件**：滤镜查 `FilterCatalog.cpp` / `FilterGraph` / `VpyScriptBuilder.cpp`；预览与 seek 查 `VapourSynthFrameServer.cpp` / `FrameTimeline.h`；播放控制查 `PlayerWindow.cpp`、`PlayerControls.cpp`、`PlayerSession`；字幕/菜单查 `PlayerSubtitles`、`PlayerMenus.cpp`；图片查 `PlayerImageTools.cpp`、`PlayerImageResample.cpp`；VSCap 当前主要实现于 `src/cap/main.cpp`；原生构建查 `tools/build-native-api20.ps1`；打包查 `tools/package-portable.ps1`。

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
| 3FP / FFF.Native | API20 + PR23 补丁；`tools/build-native-api20.ps1` | 原生播放/呈现不可用；不得混搭旧 ABI |
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
2. Native API20 用 `tools/build-native-api20.ps1` 与对应 patch；GUI 用 CMake preset。旧 `tools/build-3fp.ps1` 及历史版本脚本并存，调用前核对版本和补丁。
3. `tools/stage-portable.ps1` 用于本地部署；`tools/package-portable.ps1` 打包 Full/精简版，1.0.12 PlayerLite 以现行版专项发布脚本/说明核对。必须保留许可、包含正确运行时，排除个人 INI、缓存与日志。
4. 验证完整解压、启动、依赖/哈希、版本清单后再推送/发布；源码推送、Release Draft、附件上传是**不同步骤**，不能互相视为已完成。
5. **维护纪律**：架构/目录/关键决策发生变化，当天修改本文件对应节；具体实验和验收归 `docs/`，对外变更归 `changelog.md`，不要再向本文件顶部增加日期流水账。

工作区清理脚本 `tools/clean-workspace*.ps1` 与 `tools/clean-build-followup-20261011.ps1` 涉及本机产物；执行前先预览、核对保留清单，不能凭脚本存在就认定清理完成。

## 10. 当前状态与下一步

- **基线**：工作区 1.0.12；Renderer、Player、VSCap、PhotoCraft 接入。Native API20 + PR22、HDR→SDR 自动色彩映射与 Full/PlayerLite 边界见 [1.0.10 验收](docs/validation-1.0.10.md) 和 [发布说明](docs/release-1.0.10.md)。
- **近期成果**：VSCap 可编辑文字/绘图、原始尺寸贴图与工具栏切换；选区可缩放/移动，标注在整图上保留，最终导出才裁切。复制不落盘，HDR 默认目录与手动保存上次目录分开记录；工作流 14 项通过。PhotoCraft 为独立图片编辑入口；3FP 直通和 VPY 链分别处理播放与增强。各项只按相应文档注明的实测范围认可。
- **下一步优先**：基于真实素材复测 VSCap/Player HDR 像素与多屏缩放；核对 1.0.11 本地构建与源代码一致性；继续确认大分辨率软解、780M 实时播放及专项未达标项，不将短时间稳定等同全场景通过。
- **设计限制**：PlayerLite 不等于 Full；VS GPU 滤镜仍可能有 CPU 平面传输；不存在通用零丢帧、完整任意 PSD/HDR/字幕/蓝光场景保证；未实现 NGU 私有算法。发布状态以 GitHub Release 及真实附件为准，不从工作区版本推断。

## 11. 故障排查导航

| 症状 | 首查 |
| --- | --- |
| Qt/CMake 配置失败 | `QT_ROOT`、工具链、`CMakePresets.json`、[依赖指南](docs/dependencies.md) |
| 原生播放器黑屏或 ABI 错误 | `FFF.Native.dll`、FFmpeg DLL、API20 补丁、`VSR_3FP_DLL`、[验收](docs/validation-1.0.10.md) |
| VS 缺少 namespace、预览失败 | VS API4、`runtime/python`、插件 stage、`VSR_VSSCRIPT_DLL`、[帧桥](docs/realtime-bridge.md) |
| seek 后画面、声音或帧号不同步 | `PlayerSession`、`FrameTimeline.h`、VS 帧请求代次及 [启动时钟](docs/player-startup-clock-1.0.8.md) |
| HDR 色偏、截屏不准 | `src/color/`、HDR Readback patch、[色彩说明](docs/color-management-1.0.4.md)、[高精度截图](docs/player-high-precision-screenshots-1.0.7.md) |
| VSCap 工具栏/文字/贴图尺寸异常 | `src/cap/main.cpp`、[VSCap 验收](docs/vscap-interaction-1.0.10.md) |
| 图片编辑或超大图故障 | `PhotoCraftEditor.*`、`ImageDocument`、[接入说明](docs/photocraft-editor-1.0.8.md) |
| BD 首帧花屏或菜单/字幕问题 | `src/bluray/`、[拼接前置](docs/player-bd-concat-preroll-1.0.8.md)、[BD 验收](docs/bd-menu-viewport-1.0.6.md) |
| 安装包缺 DLL 或升级失败 | stage/package 脚本、包根目录、哈希、[便携更新](docs/portable-updates.md) |

历史测试结论只代表当时环境；要更改当前行为，先复现、再修复、最后留可重跑的验收记录。

## 2026-10-17 当前交接

- Renderer 使用四区工作台与 `FilterTimeGroup`，按整秒起点划分全片，各组保存独立 `FilterGraph`；`VpyScriptBuilder::buildGroups` 自动分段/拼接，`PresetStore` schema2保存时间组并兼容旧单链。所有组输出规格须一致。UI入口 `MainWindowTimeGroups.cpp`。
- `PresetCommunityDialog` 移植3FUI插件的ModelScope目录/校验/三路下载协议，设置键 `community/sourceUrl`，默认保存指定的 VS-GUI ModelScope 公开页面链接；递归目录树和下载保留子目录，内置 VPY 可导入时间组。
- 复用 `PlayerResources`，预览下方显示CPU/GPU/内存及两路丢帧。时间与进度条同排，下一排依次为缩短的文件入口/浏览/帧/播放/帧/并排/完整缩放选项；两列功能区可以分别隐藏。
- API20最终固定上游 `0fe651ddab8a2bb6ddb729f314f9f4c5117fb565`（PR22），完整IAMF/OBR编译。本轮已在此内核上完成菜单/章节/基本软件播放和指定8K复测；8K仍未达流畅目标，数据见1.0.11记录，不以此前旧内核结果替代。
- VSCap多贴图、AWJ质量70、窗口识别、HDR双PNG及UIAccess启动实现纳入；已修复伴随SDR图残留浮点标记。PNG仍标准DEFLATE。UIAccess真实权限、HDR/多DPI需人工验收。
- BD FirstPlay、菜单鼠标、章节三角、遮罩列表和快速暂停恢复已有修复；长光盘跨段拖动仍有未通过记录。本轮进一步区分原始 FirstPlay 与显式 Top Menu 入口，并兼容 startTime100ns 章节字段；详见 [本轮记录](docs/update-1.0.11.md)。
- `tools/deploy-local-runtime.py` 原子更新现存 Full / 目录式 Lite，保留并核对 INI/VPY/ICC/LUT 哈希；清单和备份同步。旧 `*-next.exe` 不作为本轮激活源。本轮更新到1.0.11，不打包、不上传、不修改Release。

````````
<!-- end:project.md -->

<a id="record-changelog-md"></a>

## changelog.md

原始字节数：43707；SHA-256：`2b9c269acdf9178b1ec54f9561439c0d6371567f744b1a5a5de8f09332b05d3e`。

<!-- begin:changelog.md -->
````````markdown
# 更新日志

按版本倒序汇总用户与开发者共同关心的变化；具体实现、性能条件和专项验证放在链接的 `docs/` 中。当前项目结构与能力边界见 [project.md](project.md)，使用说明见 [README.md](README.md)。

以下记录日期是已留存的开发 / 交付日期，不据此补写正式发布日期。同一版本号曾有多轮本地修改，归档前后的功能可能不同，具体包以发布说明和内部资源清单为准。完整旧日志、原始测试数字、哈希及交付路径保存在 [开发记录原文归档](docs/history/development-records-through-2026-10-03.md)。

## 1.0.12 — 更新（2026-10-17）

- 8K420p10 AV1 默认 8 线程；加入双槽交接、双组纹理和显式暂存上传，HEVC 扩展异步交接。双 CCD／P/E 核提示、MMCSS 和 DXGI 等待对象提供实验开关。7940HX 指定 8K48 样本约 40 fps，仍未达到实时；不再追求本轮 45 fps。见 [内核优化记录](docs/player-kernel-performance-1.0.12.md)。
- 原始高度超过 2160p 时禁用开发者内置 Anime4K；自定义 VPY 保持可用，Jinc / D3D11 直通保持可选。
- 修正信息面板对合并帧重复扣减造成的帧率缺口错误，新增输出计数与重置回归。
- 底栏算法／编解码标签与 3FP 解码按钮使用相同背景规则；主题设置支持底栏、右键／倍速菜单不透明度，新配置默认 100%，已有显式配置保留。

- BD 整卷 VS 播放改为当前 M2TS 优先索引，首帧后后台准备后续片段；跳转时目标片段优先。修复光盘菜单预选 CNN / CNN+ 后丢失选集续播位置。详见 [BD VS 记录](docs/update-1.0.12.md)。

- 修复 VS 调度短暂延迟时跳过预取帧的问题，在预取范围内顺序追赶，超过范围仍与音频时钟重新同步。
- 移除 Anime 大文件缺少索引时强制切换 Jinc 的限制；手动六档也不再因首帧等待八秒而自动回退。旧条件是文件大小判断，没有检测 SSD / HDD。
- 底栏 AV1 / Opus 等标签与解码按钮使用一致的背景，倍速输入提高不透明度以保持可读性；弹窗不透明度由主题设置控制。验证见 [Anime 播放续修](docs/player-anime-performance-1.0.12.md)。
- 修复 Jinc 权重与色彩 LUT 共用 GPU 资源槽导致的显著偏暗，重建通用与 SDR 专用 shader。指定 8K SDR 素材实测恢复；桌面截图白位逻辑保留。
- 修复无音轨视频启用 VS / Anime4K 后快速到结尾或时钟停滞；四档 Anime4K 激活、出帧、暂停定位通过。见 [色彩与 VS 验证](docs/player-color-vs-1.0.12.md)。

- 修复整卷连续播放点击 / 拖动无法跳转：concat 使用全局时间定位，避免按流定位失败后一直等待完成。实盘覆盖整卷前后跨段、软硬解和播放 / 暂停。
- BD 拖动进度条时不再连续渲染中间定位帧，松开后只跳到最终位置，保留拖动前播放 / 暂停状态。普通视频保留原有拖动预览。
- 打开 BD 的光盘菜单交互模式直接进入主菜单 / 选集页，跳过版权 FirstPlay；原盘文件不改。
- 更新 Full、PlayerLite 和单文件 `VS-Player.exe`。单文件继续外置 FFmpeg，配置保留在启动器旁。验证范围见 [1.0.12 记录](docs/update-1.0.12.md)。

## 1.0.11 — 更新（2026-10-17）

- VSCap 新配置默认 JXL，无损 RGBA16、原生 HDR 标记与内嵌白位；工具条、贴图及播放器源/实画面截取均支持 JXL / AVIF / PNG，PNG 为 VSCap 第三候选。Full / Lite 内置静态 JXL 编解码，独立版 AVIF 复用外部 FFmpeg；取消保存保留编辑状态。见 [JXL 记录](docs/jpeg-xl-1.0.11.md)。

- 修正 HDR 桌面截图转 SDR 的白位逻辑：查询截图源屏幕的 Windows SDR 白位，撤销系统亮度增益，不再固定按 203 nit 套用 ACES；选区、PNG、剪贴板和贴图保留白位，标注颜色也按该白位绘制。保留当前 HDR 像素编码，普通 HDR 文件的通用查看路径不受影响，见 [验证记录](docs/vscap-sdr-white-1.0.11.md)。

- HDR 截图续修：修正 Qt PNG 压缩参数，取消默认 JSON / 裸浮点附件；先显示截图层，再后台保存 HDR 与 SDR 两张 RGBA16 PNG。SDR 伴随图与 VSP 共用映射，优化 PQ / sRGB 转换；图片解码扩展 16 位快速路径，信息栏视口按实际 DPI 计入物理像素。3840×2040 实图 HDR PNG 由约 62.8 MB 降至 1.28 MB，像素一致，加载及预览约 0.36 秒；HDR 屏热键端到端耗时仍待实机复验，见 [验证记录](docs/vscap-hdr-1.0.11.md)。

- Player 窗口模式底栏移到视频下方，并增加初始窗口高度；全屏布局保持浮层。
- 修复 VSCap 设置打开时截图层被模态输入阻挡的问题，退出截图后保留未保存的设置。
- 矩形/圆形边框点按钮支持点击选择，与填充方块互斥；保留滚轮粗细调节，填充图标单独放大、按钮高度与相邻控件一致。

- 修正 BD 显式返回菜单入口，首次打开仍保留 authored FirstPlay；菜单切换避免阻塞 Qt 窗口线程。
- 兼容上游文件章节时间字段，恢复全部章节在进度条上的正确位置。
- Renderer 右侧时间与进度条同排，文件入口缩短并与播放、并排、完整缩放选项同排。
- 预设携带全部时间组；全片单组可导入任意当前组；保留脚本型内置预设逻辑。本地/社区预设递归目录树，社区使用指定公开页面链接。
- VS 后端升级 R81，加入 BM3D 11.0 / BM3Dvk2 1.0 / BestSource 22，验证 Vulkan 解码与 GPU 时域降噪。
- 核对 API20 / PR22 已集成；未发现所述 P/E 核、CCD 亲和性或进程提权策略。指定 8K 软件播放仍未通过，纯解码约 46.79 fps、实际 VS Player 提交约 31.03 fps；条件、聚焦回归和待验收项见 [更新记录](docs/update-1.0.11.md)。
- 合入 PR23 固定提交 ce195eb，保留本地 Native 扩展；其 Native 差异主要是接口注释命名，没有新增 CPU 调度逻辑。
- 修复右键“BD 主菜单”遗漏直接返回入口的问题；实际窗口核对选集页、点击第 9 集并两次返回。
- 独立播放器改为单文件 `VS-Player.exe`，自动创建同目录 `player.ini` 并保留现有配置；按后续要求移除 FFmpeg 及其专用依赖，从 EXE 同目录 / PATH 查找匹配的 shared x64 组件，缺少时提示。
- 单文件启动器嵌入 Player 图标；HDR 续修沿用 1.0.11，推送源码并补充现有发布说明，Full 打包与附件上传由维护者完成。

## 1.0.10 — 发布准备（2026-10-10）

- 2026-10-17：清除 Full 中旧截图/播放器的两个 next 暂存构建，保留当前正式程序、配置及 aria2-next 下载工具；同步运行清单。
- 2026-10-16：接续网页端更新；Renderer增加四区布局、整秒时间组、滤镜链整组复制/粘贴、全部/当前组预设导入，项目/预设支持多组；文件入口移至进度条下方，功能区可隐藏，预览增加CPU/GPU/内存/丢帧监控。新增预设社区和下载源设置，暂用3FUI源验证下载与参数查看。
- 2026-10-16：迁移Native API20及完整IAMF/OBR，合入上游PR22（0fe651d）并完成编译；按用户要求PR22未运行测试。修复VSCap HDR伴随图像素标记、多贴图/AWJ/PNG64bit及菜单FirstPlay/鼠标/章节/列表/暂停恢复问题。长光盘跨段拖动仍未通过，UIAccess/HDR实机待验收。更新Full固定目录及目录式Lite，保持1.0.10，不打包、不动Release；证据与边界见 [续修记录](docs/update-20261016.md)。

- 2026-10-11 编辑补充：线与画笔共用并显示数字像素粗细；取消 PNG / AVIF 保存时保留截图窗口和标注，可再次保存。Alt+T 可贴出资源管理器复制的本地图片文件，跳过不可读取项；剪贴板图像优先。14 项回归通过，编译同步到 `dist/VS-GUI-Full`，不打包、不改版本或 Release。
- 2026-10-11 本地部署：Full 运行目录固定为 `dist/VS-GUI-Full`，编译后同步更新并保留配置；版本号用于压缩包名称。将框选 / 保存修复部署到该目录，本次不重打包、不改 Release 或版本。
- 2026-10-11 框选与保存修复：新增“选区”工具，四角 / 四边可缩放，框内可移动，文字和图形保持原位置；标注可越过选区，最后裁切。打勾只复制，通用剪贴板提供 HDR 的 SDR 预览；HDR 原始截图提前保存到新增的默认路径，手动 PNG / AVIF 每次选路径并记住上次目录。工作流 14 项通过；仅编译、源码推送，不打包、不改版本或 Release。
- 2026-10-11 贴图补充：默认原始像素大小，仅超出当前屏幕可用区域时等比缩小；贴图“✓”隐藏工具栏，右键切换显示 / 隐藏，四面均保留蓝色边框。工作流 12 项通过，更新本地 1.0.10 并重制 Full / PlayerLite 压缩包；源码推送，Release 由维护者上传。
- 2026-10-11 后续：修复 VSCap 工具条鼠标、字号增减、文字框拖动边缘及空框残留；增加复制 / Ctrl 提示、Ctrl 正方形 / 圆形、虚线和数字像素笔宽。贴图改为等比最近邻缩放，复用完整编辑工具栏，右键支持工具栏 / 复制 / PNG / AVIF / 销毁，修复视图与浮动工具条的 Ctrl+Z。工作流 11 项、原生截图 5 项通过，见 [交互验收](docs/vscap-interaction-1.0.10.md)；本轮仅源码推送，草稿和压缩包由维护者处理。
- 2026-10-11：VSCap 文字取消输入弹窗，改为选区内可持续编辑、拖动的虚线框；左侧可编辑系统字体下拉框支持自动补全，字号可输入数字，并可加粗 / 斜体。多个对象保留独立内容与样式，导出时合成，文字编辑、移动、样式与绘图统一撤销；工作流 9 项通过。
- 同步返回的 1.0.9：Native API 18、PlayerLite、VSCap 及 AV1 444p12/字幕修复；旧机器的测试记录保留为历史，不当作本机验收。
- Full 新配置默认自动处理 HDR→SDR；SDR 与 HDR 输出保留原生路径，已有显式色彩选择保留。中文色彩对话框统一“确定 / 取消 / 应用”。PlayerLite 的高级色彩限制见验收文档。
- 无音频软解不再用重置时钟隐藏落后；信息面板显示帧率缺口与视频时间落后。AV1 8K 420p10 使用实测更快、更省内存的 16/16 线程与帧上下文，4K 444p12 维持 32/32。
- 修复 API 18 暂停跳转在片尾附近越过目标帧；后续延迟帧保留到续播队列。上传前降分辨率实验恢复为默认关闭。
- ASS 初始化跳过已知字幕轨道的重复媒体探测；静态字幕的裁剪像素和位置不变时复用合成结果，保留动态字幕逐次渲染。
- VSCap 工具选中时显示边框，左侧常驻颜色与参数；线工具支持直线/箭头，增加 Ctrl+Z 和撤销按钮，保留浮点截图像素。
- 分别制作 Full 与独立 PlayerLite 压缩包、推送源码并准备无附件 Release Draft；8K 软解仍未达实时。结果、差距和待处理项见 [本轮验收](docs/validation-1.0.10.md)。

## 1.0.8 — 发布准备 (2026-10-07)

- 延迟初始化 3FP 会话、VS/Python、字幕库及 GPU 计数器，移除 Renderer 启动时的双路强制预热，直接打开图片不加载视频后端。
- PhotoCraft 加入旧图片链路的超大图支持，32 MiB 分带交换完整像素、位深、透明度与 ICC；48000×32000 PNG 已通过真实裁剪、像素核对与撤销。
- Renderer 对比默认 D3D11 原生直通，并保留高精度算法选项；780M 双路 4K48 实测从约 4 fps 提升至约 47.9 fps，两路 10 秒零丢帧。
- 本机 HEVC Rext 4:4:4 10 位目标素材通过 NVDEC/CUDA 出帧和跳转，正常 D3D11VA 保持不变。
- CPU 预缩放增加默认关闭的实验开关，仅用于软解上传前降分辨率；本轮性能未通过验收，按维护者指示暂缓调优。
- 直通 Jinc/D3D11 的窗口尺寸变化不再启动 VS 预设重载，避免影响媒体时间轴。
- 本轮重编译播放器、Renderer 和原生核心，制作 1.0.8 Full/Lite，更新 GitHub 源码；正式发布及附件上传由维护者操作。见 [性能与大图记录](docs/performance-large-images-1.0.8.md)。

- 2026-10-07：图像编辑入口完整切换为 PhotoCraft 原生 Rust/egui 工作区，采用其菜单、工具组、多文档、图层、文字和滤镜 UI。旧 Qt 编辑窗口退出生产链接；图片查看、高精度截图、HDR 交换及格式导出保留。当前用户命名管道复用编辑进程，关闭播放器不终止未保存的编辑工作。16 位 PNG 与浮点 TIFF/ICC 往返及实际 GUI 操作通过，解压后体积净增量低于 45 MB；固定源码、许可与体积记录见 [接入记录](docs/photocraft-editor-1.0.8.md)。

- 修复从光盘播放列表加载单集（以及整卷连续播放的每一段）时开头约 1 秒花屏：分段描述里的 `inpoint` 原等于 PlayItem IN，而该值恰好是片段首个 IDR 的时间戳，FFmpeg 拼接解复用器定位后从该 IDR 之后的包继续，解码器因此没有参考帧。现在 `inpoint` 前移 1 ms（远小于一帧），`outpoint` / `duration` 保持不变。六集与花絮片段逐帧哈希与直接解码一致，段帧数由 239 恢复为 240，段时长与音画同步未变。见 [拼接前置记录](docs/player-bd-concat-preroll-1.0.8.md)。

- 修复无音轨视频首次直通播放及跳转后，未释放的首帧音频门控让画面绕过时间戳等待、以显示器刷新速度快进的问题。门控仅在有音频输出时启用，保留有音轨视频的首帧同步行为。见 [启动时钟记录](docs/player-startup-clock-1.0.8.md)。
- 全屏顶部标题出现时，Tab 与音频详细面板按标题实际高度下移，标题隐藏后恢复；浮层不改变视频视口或处理链。
- 按透明度滑条、标签、字体和音频选择控件的实际高度计算面板布局，文字与底部透明度栏增加约 5% 纵向间距，面板限制在视频窗口内并避开底栏。
- 音频详细面板可见时禁止鼠标自动弹出播放列表，显式固定的播放列表仍可显示，Tab 面板保持在其上方。
- 窗口移动事件立即同步底栏与浮层位置，减少原有 100 ms 更新间隔造成的滞后和残影；移动过程不刷新音频图或重载视频。
- 下拉框及弹出菜单操作不再触发播放、寻帧和全屏快捷键，修复音频信息选择中按 Enter 意外退出全屏。
- 浮层修复阶段仅本地修改与部署；后续按维护者要求制作 Full / Lite 并更新源码，保留维护者手动更新的 FFmpeg。见 [修复记录](docs/player-overlay-layout-1.0.8.md)。

## 1.0.7 — 发布准备 (2026-10-06)

- 截图新增基本设置中的默认保存目录及右键单次指定文件夹；源画面 / 实画面都支持。HDR 默认 PNG 使用 sRGB SDR 色调映射预览，另存原始 HDR PNG 与未截断浮点附件，兼顾普通 SDR 查看器与精度测试。
- 底部常驻直通文字改为算法标识：高级超分一个框显示 A4KCNN+ / A4KCNN / A4K+ / A4K / Jinc / D3D11，补帧两个框显示 Jinc / D3D11 与 RIFE+ / RIFE / MVT+ / MVT；随当前实际档位更新。
- 制作 `1.0.7.7z` Full 与 `1.0.7-Lite.7z`，Lite 不带 LAV、madVR、MKVToolNix。源码上传并准备无附件 Release Draft，附件上传和正式发布由维护者操作。见 [发行说明](docs/release-1.0.7.md)。

- 2026-10-06：源画面 / 实画面截图统一 RGBA16 PNG，自适应滤波与 libdeflate 最高无损压缩级别；HDR 另存未截断 RGBA float32 和色彩 JSON，修复 FP16 渲染表面被误当 float32 读回。补充原生直通源截图及图片高精度实画面，保存异步执行。实际输入位深、madVR 接口边界及验证见 [截图记录](docs/player-high-precision-screenshots-1.0.7.md)。

- 2026-10-06：主题设置增加独立底部 / 播放列表透明度；全屏底部栏约 1 秒无操作隐藏，以实际高度唤出，顶部可显示标题及退出全屏按钮；透明浮层不改变视频视口。
- Tab 先显示当前使用的文件，文字与透明度条分开并支持滚动；新增简略 / 详细音频信息选择，右侧显示真实输入和实际输出的声道峰值电平 (dBFS)，随 Tab 一起隐藏并让位于设置窗口。见 [专项记录](docs/player-transparent-audio-info-1.0.7.md)。
- HEVC / AV1 软件解码采用最多 32 个逻辑线程，软件帧队列预算由 128 MiB 提升至 512 MiB；独立线程处理视频解码，正常播放的取帧提交与音频不再等待同一线程完成解码。硬件帧预算仍为 768 MiB，保持八帧上限、150 ms 预取和原有精度。
- 修复多线程解码在跳转和片尾的延迟帧处理；保留定位时预读的音频，在音频先结束时继续呈现视频尾帧。
- 自动识别 `xxx.sc-jp.ass` / `xxx.tc-jp.ass` 等同目录字幕，支持有效名称片段匹配并排除不同集数；匹配多份字幕时默认选取最匹配的一份，其余可在首 / 次字幕菜单切换，不因同分全部跳过。
- 源码与本地运行版本更新为 1.0.7，保留用户配置与预设。最初仅本地验证阶段及实测边界见 [验收记录](docs/software-decode-subtitles-1.0.7.md)，本次发行包括这些已完成的修复。

## 1.0.6 — 发布准备（2026-10-05）

- 追加 HEVC 444p10 尖峰诊断：本机复现约 600 Mbps 无丢帧、800 Mbps 丢帧；单独扩大队列仍丢，128 MiB / 16 解码线程隔离对照两轮零丢帧。记录解码阻塞、队列与提交线程关系，仅更新诊断文档，不部署实验参数，见 [专项分析](docs/hevc444-peak-diagnosis-1.0.6.md)。

- 制作完整便携包 `1.0.6.7z` 与精简包 `1.0.6-Lite.7z`，精简包排除 LAV、madVR、MKVToolNix，保留 BD 菜单与核心运行时；排除个人配置、缓存和自定义预设，包含全文件 SHA-256 清单及同名压缩包哈希。源码上传，GitHub Release 保持无附件草稿；附件与正式发布由维护者处理，见 [发行说明](docs/release-1.0.6.md)。

- 修复 RX 6600 测试报告指出的 8K 硬解预取不足：硬件表面采用 768 MiB 队列预算，软件帧仍为 128 MiB，保留八帧队列上限与 150 ms 预取目标。本机 Jinc 小窗口 / 4K、D3D11 4K 三组 60 秒稳定段均约 47.95 fps，零新增丢帧；保留 BD、字幕与精度回归，更新本地原生 DLL。RX 6600 同片实机性能仍待复测，见 [队列修复与测量](docs/hardware-8k-queue-1.0.6.md)。

- 修复原盘 BD 菜单选英语、韩语、繁中、简中后仍显示日语：拼接节目保留 CLPI 中的轨道 PID 与语言。五种原盘菜单选择均与独立提取的字幕图像一致，连同跳转、六集选集、字幕和滤镜共 20 项通过；重编译更新本地 Player / Renderer，保留原生 DLL、配置与 VPY。见 [字幕语言验收](docs/bd-menu-subtitle-languages-1.0.6.md)。

- 修复进度条回弹、连续拖动丢失最新目标及恢复播放时机；修复原生 BD 拼接定位失败、菜单选集使用旧位置与模式窗口自动缩窄。Player 与原生 DLL 重编译更新；本机真实鼠标、原盘六集、滤镜/字幕等 30 项回归通过，见 [验收记录](docs/player-bd-seek-acceptance-1.0.6.md)。

- 修复 BD 模式标题裁切、菜单节目恢复滤镜/Tab、长节目跳转与快速切换；播放列表固定时停靠增宽，底栏不触发悬浮列表；歌词自动显示及外部优先；图片递归目录与循环提示；编辑器工具下拉、标尺端点/整体拖动、文字双击编辑及排版。实盘跳转、kit A–P 和编辑器验证见 [专项验收](docs/player-bd-lyrics-editor-fixes-1.0.6.md)。

- 修正 BD 右键在高 DPI 下错位并调整指针隐藏超时；加宽模式选择框，图片编辑工具栏恢复顶部。进度条点击直跳、预览后松手去重，保留拖动预览和播放状态；单核软件解码测试及锁屏验收边界见 [输入与布局修复](docs/player-input-layout-1.0.6.md)。

- 菜单后续修复：空窗口选择光盘菜单模式即显示原盘菜单，右上角“BD 菜单”可直接重新打开；移除底部菜单导航按钮，鼠标交互通过。切换节目时异步释放旧 libVLC 输出，本机花絮切换处理等待由约 1.17 s 降至 15–16 ms，剧集起播仍约 1.3 s；16 项专项回归通过，保持 1.0.6 本地版。

- 后续：增加 BD 原盘菜单交互模式与便携 libVLC；修复 `bluray:` PGS 加载 -4 和起始时间偏移、短标题过滤导致的花絮打不开。两条 IG 菜单图形流给出具体诊断。后台扫描/并行缓存与有限探测改善启动。
- 默认固定视频视口，播放列表、底栏、右键和弹窗覆盖视频；右键不强制弹底栏。修复 Tab 连续刷新尺寸增长，界面括号改为英文，默认西文字体改为 Comic Sans MS，补充 Jinc 先缩小再增强说明。真实盘及回归边界见 [菜单与稳定视口](docs/bd-menu-viewport-1.0.6.md)。

- 音频直接加载，内嵌封面作为静态图片显示，停止外部音频匹配与挂载；Tab 正确显示音频和封面信息。支持流/容器歌词标签、同名 UTF-8 LRC、BOM、COMMENT 时间歌词及文本字幕轨，kit A–P 16 样本完整行数和同步跳转通过。
- BD 完整内容列表增加整卷连续播放、花絮/其它节目、独立片段和按曲号排序的折叠音频组；真实 MyGO 下卷剧集出帧及 29 首 OST 列表验证通过。MKS 作为外挂字幕容器加载，可选择轨道、显示普通章节并从右键跳转。
- Tab 增加纵向空间，透明度区字体和几何随缩放适配，保留设置窗口优先级。此前仅更新本地源码和 dist，本轮纳入双包发行准备；详见 [验收与边界](docs/audio-bd-mks-1.0.6.md)。

## 1.0.5 — 本地构建（2026-10-04）

- 2026-10-05 Full：Player 打开 BD 直接加入播放列表；恢复空窗口 HW/SW 切换，修复 Tab 透明度、调整手柄与设置窗口遮挡。设置新增“组件下载”，内置 MKVToolNix 102.0；支持版本列表、自动选择更新、下载校验、固定目录安装、旧本体只下载与组件手动降级。7z 完整性、资源清单、独立解压启动及专项回归通过；源码和无附件 Release Draft 按维护者要求交付，详见 [本轮记录](docs/components-player-1.0.5.md)。

- 2026-10-05：大分辨率 libdav1d 线程上限 16、CPU 平面 YUV 动态纹理上传、固定 bilinear 色彩/格式专用 shader。本地 dist 更新核心，保留 BD、VS 音频时钟与个人配置；两台设备参考像素回归误差为零。780M 真实 8K 软解呈现约 18–19fps，仍未达到 48fps；不宣称 AV1 444 新增硬解支持。详情见 [专项记录](docs/av1-software-decode-1.0.5.md)。本轮未推送、打包或发布。

- 2026-10-05：Renderer 新增 BD Remux，Player 新增 BD 文件夹/镜像入口与节目播放列表。支持 MPLS 元数据与章节、自动剧集候选、JSON 匹配模板、结构反馈、手动文件及 ModelScope Manager 下载桥接。真实 Bloom 第三集 remux、直接播放和跳转通过；镜像、加密盘及复杂结构边界见 [BD 记录](docs/bd-remux-1.0.5.md)。

- 2026-10-04–05 后续本地修复：精确帧输入提交优先于 Enter 全屏；图片滚轮缩放及视频鼠标锚点；编辑器初始工具箭头、未锁定图层 Delete、缓存代理实时移动、概览填补及按内存调整预览。大 PNG 并行预测最终实测 TEST.png 约 8.80 秒进入编辑器。Tab 新增当前/目标帧率、有序尺寸、独立处理链信息及可缩放/调透明度面板。已与 BD 会话统一更新本地 dist。
- 补帧增加 Jinc/D3D11 双自动入口及八固定组合；VS Player 新增 3FP 仅音频/时钟接口，消除重复源视频解码；调整缓存、预取及进度条预览。完整 RIFE / MVTools 实时无丢帧仍未全部验收通过；后续交付与测试范围见 [交互与调度记录](docs/player-interaction-performance-1.0.5.md)。本轮没有新增推送、打包或线上发布。

- 上一轮交付范围为源码推送与本地完整便携压缩包；不创建 Release 或 Draft，不上传发行附件。打包排除个人数据目录和 AWJ 更新安全状态文件，保留运行时、许可及内置预设。

- 重新从独立 Release 构建目录编译 Renderer / Player，3FP 完整 Rebuild；已覆盖 `dist/VS-Renderer-GUI-windows-x64` 的 `VSRenderer.exe`、`vs-player.exe`、`FFF.Native.dll`，修正 `release.json` 为 1.0.5，并同步本文及说明文档。配置、VPY、缓存和色彩后端保留；旧文件备份在 `build/dist-backup-1.0.5-20261004`。本次只更新本地 dist，未发布线上版本，也未更新 PortableSoft 安装目录。

- 以返回的 1.0.5 源码继续修复：Jinc 权重查表及输入/色彩专用着色器，RTX 4070 Laptop 的 1080p48/2160p48→真实 8K 原生播放各约 60 秒稳定段无新增丢帧；不代表 780M、GUI 或 HDR 全量验收。
- 选择兼容官方适配器、SDR scRGB、SDR white 和 P3 路径，保留本地杜比核心；原生补丁及测试范围见 [本机记录](docs/local-1.0.5-performance.md)。
- 大图背景惰性载入，RGBA8 PNG 快速解码及原位 AVX2/SSE2 预测器；TEST.png 本机两次进入编辑器为 9.825 / 9.767 秒，10 秒预算通过；冷缓存、机械盘和非 AVX2 边界见专项记录。
- 选区右键支持复制/剪切图层、取消/反向选择；Ctrl+C/V 保留当前图层选区；工具变体使用不同图标。
- 大文件缺索引时 Anime 预检进入 Jinc，VS 首帧超时提供 Jinc 降级；新增 Jinc/D3D11 两个直通入口。机械盘限速及进程内索引中断边界见专项记录。

- 合入 NucBox 780M 的 D3D11 帧队列保护、硬解纹理直接呈现、字幕去重及有界三缓冲修复；本地 4K60 验收详见 [设备实测记录](docs/nuxbox-performance-session-2026-10-04.md)。
- Renderer / Player 同步升级为 1.0.5；本轮实际交付目录以本节的 dist 更新记录为准，保留个人配置与预设。
- 本地构建可显式复用现有完整 VapourSynth / Python 运行时。未执行线上发布。

## 1.0.4 — 发布准备


记录截至2026-10-03；源码与本地便携manifest为1.0.4。本轮准备干净便携包、推送源码并创建无附件 Release 草稿；正式发布与 GitHub / ModelScope 附件由维护者操作。发布内容见 [1.0.4 草稿](docs/release-1.0.4.md)。

### 新增

- 增加 NucBox 780M 的独立 3FP / mpv HWND 测量工具、GPU 引擎采样与开发环境脚本。Qt 开发组件、Player 构建及 `C:\BuildTools\2026` 的 v145 原生构建已验证；修复待呈现帧覆盖、重复字幕呈现和硬解纹理拷贝，并调整有界呈现缓冲；本地 4K60 的 D3D11 原生 / GUI 共 24 轮稳定窗口无丢帧，启动与 Jinc 边界见 [设备实测记录](docs/nuxbox-performance-session-2026-10-04.md)。

- 3FP 解码配置新增可选 libplacebo D3D11 色彩管理：HDR→SDR / FP16 scRGB、自动 / 手动 tone 与 gamut、系统 / 自定义 ICC、SDR `.cube`、动态峰值检测和末端抖动。默认仍是原生 709 / 2020，配置持久化到完整 INI；应用不重开媒体或 VS 链。解码页按钮按 3FP、LAV Video、LAV Audio 排列。
- Tab 新增实际色彩引擎、颜色字段来源、HDR 元数据、ICC / LUT、输出目标和回退原因。有效 HDR10+ knee / anchors 参与 ST2094-40；DV 支持不依赖 EL 的 decoded metadata，Vivid 仅报告底层回退，详见 [色彩管理边界与测试](docs/color-management-1.0.4.md)。

- 图像编辑器支持同窗口多文档标签及独立历史/视图，普通背景层默认锁定、PSD保留原锁标；图层面板新增锁定按钮和右键操作。混合选项增加填充、RGB通道及可拆分的Blend If，支持实时预览、复位、取消和单次撤销，PSD保存对应参数；见 [文档与混合选项](docs/image-editor-workflows-1.0.4.md)。

- 图像编辑器新增整体图层边框及四角/边缘缩放，Shift等比，Ctrl+T自由变换（Enter提交/Esc撤销）、Ctrl+E向下合并，以及编辑菜单的翻转/90°/180°旋转。变换保持文档精度，并按需读取图层区域。
- 文字改为独立、自动选中的像素图层；图层面板使用统一的当前层控制，图层树更新保留选中行。编辑器控件统一灰色背景，工具提示取消常驻，双轴标尺新增鼠标位置标记及坐标显示。见 [图层交互更新](docs/image-editor-interaction-1.0.4.md)。

- 新增原精度图像编辑工作区：Photoshop系列图标工具组、上下文选项、文档页签/标尺及颜色/属性/图层/通道/历史/路径面板；选择、画笔、修复/图章、渐变/油漆桶、局部调整、栅格化文字/形状等保持RGBA8/16/32F。
- 新增RGB PSD/PSB图层、层组、像素蒙版和有限混合读写，延迟展开通道到临时磁盘，按区域编辑；支持后台快照预览和PSD/TIFF保存。真实约520MiB、9层样本及边界见 [图像编辑第二轮](docs/image-editor-round2-1.0.4.md)。
- 新增色阶/曲线、曝光、基础颜色调整、Qt RGB ICC转换、原生Windows scRGB HDR能力检测/SDR回退及流式32bit浮点TIFF。真实HDR面板观感待验证；新建Float32文档没有源Photoshop HDR色彩数据时使用TIFF。
- AVIF在缺少ICC时识别已知CICP色彩空间，包括P3/PQ/HLG；编辑器复用原生图片解码，PQ/HLG默认启用SDR映射。PSD/PSB普通查看复用原生图层合成，避免FFmpeg不支持ZIP合并预览造成打开失败。

- 新增图片工具栏，提供旋转、镜像、回收站删除、桌面背景、调整尺寸、裁剪和压缩转换；详细编辑入口已开放。
- 新增可移动、可调整边角的裁剪框，支持常用及自定义比例，默认以PNG另存，保护原图。
- 新增图片后台Jinc、Lanczos3/4、双线性与最近邻重采样，保留8/16位、ICC和透明通道；Anime4K Mode A Fast可用于离线放大。
- 新增独立AWJimage转换后端，支持AVIF、WebP、JXL、JPEGli和PNG，以及固定质量或目标视觉质量模式。
- 新增AVIF自动位深，8bit输入默认升至10bit、10/12bit保持，保留手动位深与色度/Alpha配置。
- 新增Renderer GLSL功能分类、自定义入口与硬件要求提示，支持倍率或指定宽高输出及保持比例联动。
- 新增视频Lanczos4放大与缩小，并在设置中提供自动补帧起始档位。
- 增强版FFmpeg新增AVS3编解码及更多编码器和旧格式支持，供原生播放与自定义导出命令使用。
- MKV音频/字幕轨道显示FFmpeg同类编号、名称、语言和Default/Forced；多个Default按编号选首条，Forced仅展示。无Default时音频和字幕均选择同类首条（0:a:0 / 0:s:0）。
- 提供主目录 `Clean-Workspace.bat` 手动清理固定历史副本，支持 `--preview`，先保留历史INI/VPY。

### 修复

- 原画播放改为 3FP 原生直通，Jinc / D3D11 不再等待 VS 初始化；内嵌 ASS / SRT 字幕改为按位置限量读取，去除整视频扫描和 SRT 全量提取。远端已完成部署；大素材稳定播放性能仍待优化，见 [直通加载说明](docs/player-direct-open-1.0.4.md)。

- 修正 BT.2020 CL 被当成 NCL 处理、RGB 纹理红蓝顺序和 ICC SDR 参考亮度；内置 VPY 必要转换根据源颜色属性选择矩阵，709 默认保持一致。高级路径复用 D3D11 纹理和暂停缓存，不新增逐帧 CPU 回传；缺少依赖或色彩配置错误继续原生显示。

- 修复编辑器放大预览缺少平滑采样的问题，采用原生可见区域、HiDPI预览和视口缓存，避免缓存范围内反复合成；普通SDR显示减少浮点色彩往返。图片滚轮默认上下移动，Shift左右移动，Ctrl缩放。

- 修复D3D11 / Jinc直通模式拖动进度条时画面仅在松手后更新的问题，拖动中连续预览，松手精确定位并恢复原播放状态。
- 修复PSD详细编辑只得到扁平图而无法选择内部像素层的问题；层组树支持选择与修改子层，组移动同步后代，原文件另存保护。

### 变更

- 新增 NuxBox 780M 原生播放性能开发交接和干净源码/SDK打包脚本，3FP 构建可显式选择 v143 / v145 工具集。远端已部署基线，但 1080p48 放大和 2160p48 丢帧仍待实测优化，不写作已解决；见 [交接说明](docs/nuxbox-performance-handoff.md)。

- 图片顶栏缩至38逻辑像素，尺寸和转换窗口加宽；旋转/镜像使用显示变换，避免反复复制超大源图。
- GLSL显示短名称并通过悬停提供完整路径，避免长路径撑宽控件；NIS来源与DP4A硬件要求分别标注。
- 最低补帧档持续丢帧时保持播放并提示建议关闭，性能恢复后清除提示；加载失败与性能降档分别处理。
- 同步维护者提供的增强FFmpeg CLI、共享库和配套依赖，保留旧运行时备份，LAV与VS插件自带解码器不随之替换。
- FFmpeg CLI与播放器改为共用根目录DLL；GLSL按需读取LZMA2资源库，裁掉实际不使用的ISA副本及madVR调试/32位AX，保留滤镜和模型。程序文件约596 MiB（625 MB），现有缓存另计约34 MiB；见 [轨道与体积](docs/tracks-size-1.0.4.md)。
- 项目文档改为当前地图，更新日志按版本分类；详细流水与验证原文归档，避免历史记录遮挡开发入口。

### 验证与边界

本版各功能分别完成相关专项，不把单项通过合并成一次全量回归。图片常规解码/编码使用CPU；离线Anime4K受GPU限制，AWJ要求AVX2，增强FFmpeg为x86-64-v3。来源、许可、测量条件和交付证据见 [图片工具与细化](docs/player-image-tools-1.0.4.md)、[GLSL分类与尺寸](docs/mpv-shaders-1.0.4.md)、[增强FFmpeg](docs/ffmpeg-enhanced-1.0.4.md)。

图像编辑B–D/HDR的最终专项结果、截图与部署审计由本轮记录单独汇总。DNG、Bezier/可回改文字、语义AI及完整PSD智能对象/调整层/图层效果尚未实现，不以栅格化替代完整对象保真；当前高负载环境不用于性能倍数结论。

工作区约7.79GiB历史文件仅完成盘点，删除被执行策略阻止，未实际释放空间；已保留历史配置与审计，见 [清理记录](docs/workspace-cleanup.md)。

## 1.0.3

开发记录覆盖2026-10-01至2026-10-02，含打包、源码推送和随后兼容修正。

### 新增

- 新增独立图片查看模式，支持常见图片、AVIF grid、高位深和透明通道，以及同目录切图、滚轮缩放、拖动和还原。
- 新增图片输入/输出与解码信息，支持超大图片可见区域绘制，突破旧16倍缩放上限。
- 新增音频设置，提供内外音轨、同步、静音、十频段均衡器和PCM音量；保留设备支持的多声道输出。
- 新增同目录音频/字幕匹配及拖放分区，可独立挂载主次字幕或单独播放音频。
- 新增六个手动固定Anime档位，保留自动版；固定档位不因负载或高分辨率自动降级。
- 新增随包mpv GLSL / hook库、目录发现、名称搜索、缩小倍率和来源/许可清单。
- 新增内置RIFE / MVTools补帧自动链及固定档位，开启后采用Jinc直通输出，关闭后恢复Anime自动超分。
- 新增Renderer / Player应用图标和视频 / 图片文件标记，分类文件关联保留Windows默认应用选择。

### 修复

- 修复JPG被送入视频/VS帧序列而无法打开的问题，图片不加载VPY或预解码相邻文件。
- 修复FFMS2无法识别VVC轨道时不能继续VS预设的问题，仅对此源错误尝试L-Smash。
- 修复连续缩放重建VS链丢失继续播放意图，以及应用设置改变暂停状态或重复解码图片的问题。
- 修复字体配置未应用到子控件和设置窗口的问题，中文字体回退与持久化生效。
- 修复1.0.2更新器拒绝固定名称归档根目录的问题，重打包为版本根目录并保持已有安装目标路径。
- 更正首次巨大AVIF样本的内容完整性结论，确认编码端扫描行偏移溢出；后续安全平面输入的YUV444对照正常，不能归因于播放器不支持巨大YUV444。

### 变更

- Renderer改为处理链、参数、视频三栏，设置移入导航页面；参数说明白底、控件紧凑，播放控制对齐视频区。
- 对比画布导出限定为AB滑块、ABC左右2+1、ABCD常规/滑块2×2，输出保持最大源宽高，拒绝额外几何变换参数。
- Anime自动链细化为CNN+增强、CNN、no CNN+增强、no CNN、Jinc、D3D11六档，连续GPU增强合并执行。
- JPEG改用SIMD直接解码，AVIF启用SIMD色彩转换并补充颜色信息；性能数字保留素材和测试条件。
- Player设置增加完整INI加载/保存、应用按钮和图片关联分组；右键菜单增强展开、勾选和层级反馈。
- 本地便携目录改用固定名称，归档仍含版本号；LAV更新至0.83.0 x64，完整依赖及许可随包。
- 便携打包改用标准LZMA2并生成内部资源校验清单，排除个人配置、用户预设、缓存和测试产物。

### 安全

- 更新器按内容识别唯一完整程序目录，同时保留危险路径、重复路径、链接、不完整包和版本校验，拒绝歧义或不安全归档。

### 验证与交付

各轮有Release构建、相关专项、实际媒体输出、便携启动和配置保留记录；部分全套运行出现耗时断言或导出组失败后单组复测通过，不能简化成所有场景一次全绿。测试和性能边界见 [三栏布局](docs/renderer-panels-1.0.3.md)、[比较导出](docs/renderer-ui-export-1.0.3.md)、[图片解码](docs/player-images-menu-1.0.3.md)、[图片与音频](docs/player-images-audio-1.0.3.md)、[设置与AVIF更正](docs/player-apply-avif-1.0.3.md)、[AVIF受控分析](docs/avif-large-yuv444-analysis.md)、[六档](docs/player-six-stage-1.0.3.md)、[手动Anime](docs/player-manual-presets-1.0.3.md)、[GLSL与补帧](docs/mpv-shaders-interpolation-1.0.3.md)、[图标](docs/icons-integration.md)。

历史记录包含源码推送、完整便携包、无附件Release草稿和维护者手动上传安排。固定根目录包随后被修正版替代，早先大小/哈希只作历史证据，不能当作当前更新包校验值；见 [发布说明](docs/release-1.0.3.md)、[更新兼容修正](docs/update-compatibility-1.0.3.md)。本次整理未复查远端附件状态。

## 1.0.2

开发与交付记录覆盖2026-09-30至2026-10-01。

### 新增

- 新增独立VS Player，支持VPY实时处理、章节、逐帧/关键帧、时间输入、播放列表和不变调0.10–16倍速。
- 新增Renderer预设管理与外部VPY加载，同一处理链可覆盖输入源并在Player复用。
- 新增MVTools两倍补帧与RIFE低分辨率计算，保留原始时间点像素，按需生成中间帧。
- 新增主次字幕、SRT样式、内外音轨、源/显示PNG截图和实际LAV/madVR播放接入。
- 新增便携INI、预解码和资源调度设置、语言包、媒体位置记忆、索引缓存与文件关联。
- 新增播放列表悬停/停靠、全屏底栏自动隐藏、快捷键跨度、右键滚轮模式和画面还原。
- 新增版本检查、下载校验与完整目录更新，保留用户配置、VPY、项目和截图，并留旧目录备份。

### 修复

- 修复MVTools插值帧绿色闪烁，补充彩色色度、乱序和并发请求验证；旧保存VPY需重新生成。
- 修复网络URL被误判、AList链接解析和网络字幕打开失败导致的闪退问题。
- 修复PGS字幕缺失清屏/结束信息时不显示，以及外部字幕被后续媒体信息覆盖的问题。
- 修复暂停时字幕重复上传持续占用GPU、倍速暂停位置推进和补帧输出精确帧定位问题。
- 修复内置10bit预设位深冲突，以及真实VS跳帧统计遗漏的问题。

### 变更

- HTTP/HTTPS由早期整段准备/VS索引路径改为固定3FP原生直通，保留重定向、AList解析和有界Range代理，网络不执行VPY。
- YUV锐化合并为GPU Shader，已知旧内置代码定向升级，用户VPY不整体覆写。
- Anime降载加入Jinc和D3D11直通；暂停、定位、变速、resize和全屏后重置稳定观察窗口。
- 索引默认转存应用缓存或自定义目录，显示与输出支持更多像素格式及独立缩放/抗振铃选项。
- 音频与视频解码分别配置；LAV/madVR便携加载不要求全局注册，madVR路径绕过VS。

### 安全

- 更新下载与解压校验大小、SHA-256、版本及危险路径，完整目录交换失败时保护旧程序，不强杀正在播放的窗口。
- 发布暂存排除个人配置、缓存和用户文件，活动目录不作为清理目标。

### 验证与交付

留存真实媒体、字幕、网络、MVTools/RIFE、madVR、更新和配置保留专项，以及完整构建/便携启动记录。1.0.2曾使用Zstandard归档，1.0.3改为LZMA2；这些是不同阶段的包。性能与算法边界见 [Player性能记录](docs/player-performance-1.0.2.md)、[补帧验证脚本](tools/verify-interpolation.py)、[发布说明](docs/release-1.0.2.md) 和 [便携更新](docs/portable-updates.md)，更详细交付原文见归档。

## 1.0.1

开发与交付记录覆盖2026-09-26至2026-09-30。

### 新增

- 扩展源/处理后实时预览，新增Anime4K GLSL及3FUI参数兼容的MKV导出，处理链保持可审阅VPY。
- 新增独立分析比对页，最多九路视频、源交换/卸载、独立偏移、音频选择以及并排/网格/滑块布局。
- 新增独立导出设置、批量准备和编码队列，支持顺序执行、暂停/恢复、移除等待任务、停止保留输出和从头重做。
- 新增对比画布快照导出、教程向滤镜扩展和RIFE NCNN Vulkan模型。
- 新增独立色度重建核、Spline36、Super-XBR单阶段和去伪影/细节增强。
- 新增启动预热、共享D3D着色器缓存、异步VS初始化和全局播放/缩放拖动。

### 修复

- 修复原生窗口resize / 最大化 / 全屏后停止呈现，以及对比拖柄丢失鼠标捕获的问题。
- 修复ABC/ABCD滑块误用可变视频分格，统一为共同画布坐标上的裁切比较。
- 修复上下平移灵敏度不一致、四象限取色提示被裁掉和卸载视频后的旧索引回调问题。
- 修复部分反交错/纵向倍高模式破坏宽高比，以及参数长标签、下拉宽度和侧栏样式干扰问题。

### 变更

- 实时预览按最新目标请求VS帧，处理链改变时重建输出会话；导出保留源音频/字幕/章节映射。
- 比较滑块复用现有原生表面，画布导出采用离线合成；两者不保证逐像素等价。
- 插件部署排除R80无法加载的旧API3二进制，随包提供已验证API4插件和模型。

### 验证与边界

留存真实MKV导出/停止、双路及九路原生呈现、比较几何、VS滤镜和RIFE实际输出记录。九路合成小片通过不代表九路4K持续播放；Super-XBR为开源单阶段实现，不含NGU，部分预览核不能等价导出。发布与完整原始验证见 [1.0.1说明](docs/release-1.0.1-draft.md) 和 [原文归档](docs/history/development-records-through-2026-10-03.md)。

````````
<!-- end:changelog.md -->
