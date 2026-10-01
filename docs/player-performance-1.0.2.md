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
