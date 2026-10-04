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
