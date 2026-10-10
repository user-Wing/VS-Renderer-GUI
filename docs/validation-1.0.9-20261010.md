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
