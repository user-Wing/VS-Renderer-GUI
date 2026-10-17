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
