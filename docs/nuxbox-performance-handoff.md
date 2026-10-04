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
