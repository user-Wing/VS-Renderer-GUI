# 1.1.0 代码审查报告（发版前）

审查范围：最新版源码（HEAD `d6d4f69`，版本 1.1.0 未发布）+ 发布/打包/更新链路 + 未提交工作区状态。
审查方式：四路并行子代理分域通读 + 主审人工复核关键项。仅静态读码，未修改任何代码。
审查对象：崩溃 / 数据损坏 / 安全 / 资源泄漏 等高危项，其次是正确性、健壮性、发布就绪度。

> 严重级定义：
> - **高** = 存在可用以导致崩溃 / 数据损坏 / 越权执行 / 安全信任边界破坏的路径（含需调用方配合的条件触发）。
> - 中 = 边界逻辑弱点 / 潜在资源或正确性风险 / 需运行期确认的行为。
> - 低 = 健壮性或维护性提示，不影响正确性。

---

## 修复复核（2026-11-07）

以下是本轮逐项处置；后文保留原审查结论，不能把原报告的推测继续视为已确认缺陷。实际回归结果见 [validation.md](validation.md)。

| 项目 | 处置 |
| --- | --- |
| H1 外部 AI worker | 新增明确的外部代码执行确认、执行文件与清单 SHA-256 指纹、确认后的复核；限制清单大小、worker 类型、消息结构与时间戳。信任确认不是沙箱，仍只应使用可信的完整扩展包。 |
| H2 网络目标与代理 | HTTP 跳转逐跳校验，阻止 HTTPS 降级和跨源访问私有地址；用户明确输入的 LAN/认证地址仍支持。本机代理改为随机路径并检查响应头。DNS 校验和实际连接之间仍不构成对恶意 DNS 重绑定的完整隔离。 |
| H3 单实例 | 改为原子 CreateMutex 判定已存在，保留稳定实例名；随机实例名会破坏单实例功能。同用户主动抢占名称不作为跨权限漏洞承诺解决。 |
| M1 预热生命周期 | 依赖对象销毁时停止计时器、清空待处理工作，poll 检查运行状态。 |
| M2 VS future | 原调度已限制总在途数量，原报告“无限堆积”不成立；修复 seek 清理对未完成 future 同步等待的问题。卸载时为节点生命周期保留必要等待。 |
| M3 参数 | Choice 白名单、数值范围/有限值/整数、布尔和文件参数类型校验；兼容自定义 GLSL/HLSL 分类显示别名。 |
| M4 主题 | 原实现已有 256 MiB 解压总量限制；补强尺寸解析溢出、文件夹导入总量/数量与链接校验。 |
| M5 图片线程 | GUI 析构只断开回调并请求取消；线程持有独立图像数据，完成后自行释放，不在 GUI 析构等待。 |
| M6 cbuffer | 原报告 48 字节推断错误，实际为 32 字节；增加 sizeof 与 D3DReflect 验证，保持既有布局。 |
| L1 hook 顺序 | 核对现有 LUMA → MAIN → PREKERNEL 阶段顺序，无需调换。 |
| L2 导出 IPC | 保留 3FUI 同用户命名管道协议；外部命令标明来源，仍须手动导出，不引入破坏集成的随机端点。 |
| L3 导出参数 | 移除全命令反斜杠下划线替换，仅兼容编码器 token；找不到输出 token 时明确报错。 |
| L4 Pareto | 已有便携相对路径和内嵌图回退，未证实额外缺陷。 |
| L5–L7 | 修复缓冲乘法溢出；增强状态查询和字幕命令 ABI 增加空指针防御。 |
| 前一轮 6 项 | 更新器保留 themes/shaders/custom；关联单文件启动器；恢复多路同步；额外路 HLSL 尺寸、颜色及提交失败回退；极简模式卸载视频并释放旧 LAV；Lite 输出转换至 sRGB。 |
| 回归追加修复 | 恢复 Native 的受限本机 HTTP 代理输入，保留嵌套网络格式限制；补齐对比导出 Lanczos 4 映射并提前拒绝无法一致导出的 D3D11 模式。 |
| 追加 UI | 编码队列表头和选中项跟随统一明暗主题；修复无名称主题控件的样式选择器。 |
| R1 工作区 | 仅提交本轮相关代码、测试和文档。既有 color.md 删除、sr-pareto 等未跟踪开发资料及其既有文档修改保留在工作区。 |
| R2/R3 发布 | 仅更新草稿；附件由维护者上传。本轮不以性能数据作验收，既有原生 BT.2020 CL 等失败保留在台账，不宣称全量通过。 |

---

## 一、优先级摘要

| 优先级 | 问题 | 位置 |
|---|---|---|
| 发布阻断（流程） | 工作区有多类未提交/未跟踪变更，发布前须决定去留 | 工作区 |
| 高（建议尽快修） | AI 字幕 worker 信任边界（参数/输出均无校验） | PlayerAiSubtitles.cpp |
| 高 | AList 网络直链 SSRF / 本机代理面 | PlayerNetworkInput.cpp |
| 高 | VSCap 单实例互斥 TOCTOU + 本地抢注 DoS | cap/main.cpp |
| 中 | StartupWarmup 计时器与播放器销毁竞态（需确认生命周期） | StartupWarmup.cpp |
| 中 | VapourSynthFrameServer 滞留 future 堆积 | VapourSynthFrameServer.cpp |
| 中 | FilterGraph.setParameter 未校验 value（VPY 注入，需确认来源） | FilterGraph.cpp / VpyScriptBuilder.cpp |
| 中 | ThemePackage 解压总量无上限（压缩炸弹 DoS） | ThemePackage.h |
| 中 | NativeVideoSurface 析构 join 构建线程 | PreviewPane.cpp |
| 中 | ImageResample.h cbuffer 32B vs HLSL 48B 不对齐 | ImageResample.h |
| 低 | Export 命名管道 IPC 同用户可注入命令文本 | ExportWindow.cpp |
| 低 | PREKERNEL 排序与死代码守卫（需确认） | ShaderPackage.cpp |
| 低 | ImageResample cbuffer 对齐、Export 路径定位、空 handle 等 | 多个 |

---

## 二、高（建议发版前处理）

### H1. AI 字幕 worker 信任边界缺失 — PlayerAiSubtitles.cpp:20-46,47-56,79-97

- **表现**：`validatePackage()` 对扩展包里的 `worker`/`python`/`llamaServer`/`mmproj`/`model*` 只校验「是文件」并转绝对路径，不校验后缀、是否 hash 白名单或来源是否可信；`start()` 直接把这些字符串作为 `QProcess` 程序与参数拉起。`receive()` 对 worker 标准输出仅做 1MB 总量截断，逐行当 JSON 解析，`type=="final"` 时把任意 `text` 显示为字幕。
- **影响**：AI 字幕包本就是用户手动导入的外部脚本，属「运行外部推理程序」的可接受能力；但产品既支持任意模型包导入，又**在 UI 上无明确「将运行外部可执行脚本」警示**、也不锁定 worker 为已验证哈希，一旦包被替换为任意 EXE，会在当前用户权限下静默拉起。输出侧缺少 schema 校验，崩溃/被入侵的 worker 可注入任意字幕文本或刷屏。
- **建议**：最低在导入/启动时给用户明示「将运行外部脚本 worker=…」；worker 采用白名单扩展名（.py）并记录/校验 SHA-256；`receive()` 对每条 JSON 加字段类型校验、限制单条与 `text` 长度、校验时间戳合理上限。

### H2. AList 直链 SSRF / 本机代理面 — PlayerNetworkInput.cpp:103-148

- **表现**：`resolveAList()` 把用户提供的 AList 直链反查 `/api/fs/get`，返回的 `raw_url` 只校验 `scheme ∈ {http,https}` 就 `probe()`；`startProxy()` 在 `QHostAddress::LocalHost` 起 HTTP 服务器供本地播放器拉流。
- **影响**：AList 服务器返回的 `raw_url` 若指向 `127.0.0.1:端口`、内网段或云 metadata（`169.254.169.254`），播放器会以本机身份去抓取；且本机任意进程都能连 `127.0.0.1` 代理端口，播放器实际充当了本机任意 HTTP 客户端（Range 转发到任意 URL）。需要用户主动输入 AList 链路，故非一键 RCE，但属真实 SSRF 面 + 本地代理滥用。
- **建议**：对非用户主动输入的跳转目标（AList 返回的 `raw_url`、Range 重定向目标）做地址黑/白名单：拒绝 loopback、link-local、私有段与 metadata IP，除非显式允许；`RangeConnection` 对响应头/状态做防御性校验，`size<=0` 拒绝。

### H3. VSCap 单实例互斥 TOCTOU + 本地抢注 DoS — cap/main.cpp:899-906

- **表现**：先 `OpenMutexW(SYNCHRONIZE)` 探测、再 `CreateMutexW` 持有，两段之间存在 TOCTOU 窗口；且同名互斥 `Local\VSRenderer-VSCap` 任意同用户进程都能抢占，导致主程序误判「已在运行」而退出（本地 DoS）。
- **影响**：低权限同用户进程可阻断 VSCap 启动，属本地 DoS；不涉提权。`--uiaccess-launch` 的提权重启路径等待父进程窗口期较长。
- **建议**：探测与持有合并为一个原子操作（如只用 `CreateMutexW` + 判 `ERROR_ALREADY_EXISTS`），互斥名含进程内随机 GUID 或 event+owner id，避免同名抢注。

---

## 三、中

### M1. StartupWarmup 计时器与播放器销毁竞态 — StartupWarmup.cpp:14-31,70-93
- `poll()` 由 25ms 成员 QTimer 驱动，直接调 `source_/processed_/server_` 的方法；`destroyed` 处理器只捕获 `shared_ptr<QTemporaryDir>` 保活临时目录，**不停 timer_**。若播放器先于 warmup 析构 → 下一次 poll 访问悬垂对象。
- **需确认**：主窗口生命周期是否保证 players 一定晚于 StartupWarmup 析构（若严格嵌套则降为中低）。建议 start()/析构停表，或用 QPointer 判空。

### M2. VapourSynth 滞留 future 堆积 — VapourSynthFrameServer.cpp:145-152,369-380
- `trimAhead` 仅在 `(seek || future.ready())` 时裁剪窗口外条目；快速 seek 时 ahead 可能堆积未 ready 的 promise/future，`clearScript()`（会对每个 future `get()`）与析构 `join()` 会等待它们 → 可感知卡顿、极端下内存增长。
- **需确认**：getFrameAsync 回调是否保证合理时限 ready（VapourSynth 线程池）。建议注入不 seek 时也 set desiredFrame_ 并裁剪超窗条目；滞留 future 用 `wait_for(0)` 判断后丢弃。

### M3. FilterGraph.setParameter 不校验 value — FilterGraph.cpp / VpyScriptBuilder.cpp:82-91
- `setParameter` 只校验参数 id，不校验 value 是否落在该参数定义的 choice/int/real 范围；VpyScriptBuilder 把 `kernel`/`bits` 等值直接拼进 Python 文本。
- **影响面收敛**：verifiedGraph() 重生成比对 + 恶意 .vpy 本身即是被 VS 执行的 Python（无提权，仅等价能力）+ 需用户显式打开，故实际被压到中/低。
- **建议**：setParameter 写入前按定义校验 value∈choices/数值区间，越界回退默认值；非纯数字参数一律走白名单常量映射。

### M4. ThemePackage 解压总量无上限 — ThemePackage.h:importPack
- listing 阶段按「单个存储 ≤256MiB、文件数<512、路径/符号链接」卡了 zip-slip 与条目维度，但**解压后总量不设上限**：一个 ≤256MiB 条目可解出更大内容，多条目累计可填满磁盘（DoS）。
- **建议**：解压到独立临时目录后统计落盘总量/解压倍数，超阈值（如总量>2GiB、单文件>256MiB）即中止删除报错；zip-slip 已被 listing+safeName 挡死，无需重复。

### M5. NativeVideoSurface 析构 join 构建线程 — PreviewPane.cpp:39
- GUI 析构中 `requestInterruption()+wait()` join mip 构建线程，来源可到 8M 像素级，可能阻塞主线程数帧~数十 ms；若 builder 不响应 interruption 更明显。
- 已确认 imageGeneration/levels 均在 GUI 线程读写，builder 只写 thread-local 结果经信号回 GUI，**无数据竞争**；仅卡顿。
- **建议**：join 移到后台或让线程自退；当前不构成崩溃。

### M6. ImageResample.h cbuffer 32B vs HLSL 48B 不对齐 — ImageResample.h:40
- 主机 `ByteWidth=32`，HLSL cbuffer 实际打包 48B。当前 shader 只读偏移 0-20（inputSize/outputSize/algorithm），**可正常运行**；D3D debug 层报 size 不匹配，未来 shader 若读 padding 会取未定义数据。
- **建议**：主机 struct 也按打包布局对齐 `ByteWidth=48`，保留 field-offset 编译开关。

---

## 四、低

- **L1 ShaderPackage.cpp:92 PREKERNEL 排序与死代码守卫（需确认）**：`stable_sort` 把 PREKERNEL 排到 MAIN 之后（键 0=LUMA,1=MAIN,2=PREKERNEL），使 line 102「MAIN 之后 PREKERNEL 需 scheduler」的守卫成为死代码；注释却写明 Anime4K 的 PREKERNEL clamp 应在 MAIN CNN 之前。验证台账中「完整 Mode A GLSL 对照通过」用的是逐 pass 的 `glsl()`（不依赖 blocks 排序），故**不能证明**时序正确；实际渲染时序在 Native 侧，需结合 Native 链构建确认 Qt 侧此排序是否影响输出。
- **L2 Export 命名管道 IPC — ExportWindow.cpp:80-96**：`UserAccessOption` 下同用户进程可连 `VSRenderer.ExportCommand` 写入命令文本；命令仍需用户手动点「导出」且经 `QProcess::splitCommand`（无 shell）执行，故只到「替换待执行命令文本」层级，无自动执行。建议 socket 名带 PID/随机后缀或加 peer 令牌。
- **L3 ExportPipeline 路径定位（需确认）— ExportPipeline.cpp:174 / ExportWindow.cpp:326**：`\_`→`_` 替换可能破坏含 `\_` 的 Windows 路径；outputPath token 定位若 `indexOf` 取负，`insert(destination)` 会把 `-shortest/-f/-matroska` 插到 QList 头部。建议用参数流内的真实 token 索引定位，找不到即报错。
- **L4 PortableChart.h paretoImage 绝对路径缓存跨机陈旧**：仅观感，无安全/损坏。
- **L5 PlayerDecodeBuffer.cpp:25 `chosen*1048576` 可溢出**：设置项被写成超大值时提前溢出收紧缓冲；防御性。建议 `qMulOverflow` 或先除后乘。
- **L6 ThreeFpPlayer.cpp:92 videoEnhancementStatus 空 handle 传原生**：与同一文件其它方法用 `!handle_` 短路不同，此方法无条件传 `handle_`；需确认原生对空 handle 是否容忍。建议 wrapper 判空短路。
- **L7 AudioTap/Subtitle ABI 空命令指针**：`count>0 && commands==nullptr` 未防御；当前调用方保证非空，属 ABI 韧性提示。

---

## 五、已核对结论（判定安全/良好）

- **VapourSynthFrameServer** 结果回传：`scriptGeneration==loaded && desiredFrame_==frameIndex` 双重代次校验后才 emit，异步回调均 QueuedConnection 回 GUI；析构用 BlockingQueuedConnection 复位 impl_+quit+wait，顺序正确，无 UAF。
- **LavPlayback**：单线程 COINIT_APARTMENTTHREADED，clear() 按 setOsd→release osd→删位图→control Stop→release→FreeLibrary 顺序合理；madVR 进程级驻留是刻意设计。
- **PlayerWindow 析构** (L310)：disconnect→kill→waitForFinished(bitrateProbe)→aiSubtitles stop→removeEventFilter→timers→reset 各后端，未见 join 后访问已析构对象。
- **PlayerDecodeBuffer / PlayerCache**：缓存键含 SHA-256+元数据 hash 防错位；bitrateProbe QProcess 挂 this 下、singleShot 以 probe 为 context 自动取消，无双删/悬垂。
- **注入面收敛（Export 命令执行侧）**：`QProcess::splitCommand`（无 shell）+ 拒绝 `-filter_complex/-lavfi` + 仅 .mkv + 禁止覆盖源，注入面收敛在「文本待执行」层以内。
- **PlayerImageTools / PlayerImageResample / ImageHdr**：band 行写入 + 字节回校；AWJ raw 逐行写与 bytes 校验；raw stdin 4MB 背压 + 取消；width/height/stride/factor/algorithm 均有校验，无越界。
- **ThemePackage / PresetStore**：safeName 拒绝 `..`/前导 `/`/`:` + 符号链接校验挡 zip-slip；schema 只收 1/2 且 verifiedGraph body 比对防篡改。
- **FrameTimeline / AudioTapAbi / SubtitleAbi / SourcePixelProbe**：非负/上界校验、`llround+clamp` 到 `[0,totalFrames-1]`、static_assert 校验大小，未发现越界。
- **PlayerAssociations** 全写 HKCU，无提权面。

---

## 六、发布就绪度（流程面）

以下为「准备发布新版本」前必须处理、但**不属于代码缺陷**的事项：

### R1. 工作区存在未提交 / 未跟踪变更（发布阻断）
`git status` 显示：
- 已修改未提交：`README.md`、`changelog.md`、`project.md`（应确认这些正是发布所需的版本/说明更新并纳入提交）；
- 已删除未提交（tracked 文件）：`color.md`（982 行）——需确认无任何文档仍引用它（已抽查 README/docs 无悬空链接，但请复查 changelog/history）；
- 未跟踪：`tools/__pycache__/`（应剔除）、`tools/clean-build-followup-20261011.ps1`、`tools/clean-workspace-20261011.ps1`（一次性清理脚本，确认是否入库）、`tools/experimental/`、`tools/sr-pareto/`（评估是否纳入文档/发布）。

发布前需明确这三类内容的去留，避免把开发杂项混进发行提交。

### R2. 发布配置核对（均已确认正常）
- CMakeLists.txt `project(VSRenderer VERSION 1.1.0…)`，`VSR_VERSION` 正确传播到 VSRenderer/VSPlayer/VSPlayerLite 及测试目标。
- 打包/更新链路（`tools/package-portable.ps1`、`package-playerlite-single.ps1`、`apply-update.ps1`、`apply-components.ps1`、`ComponentDownloads`、`PortableUpdater`）路径/尺寸/SHA-256/回滚/版本不降级/进程归属等安全护栏完整，未发现缺陷。
- 单文件启动器 `playerlite-launcher.c`：reparse-point 拒绝、只清理启动器旁自有运行目录、FFmpeg 未内嵌校验、锁文件防并发，正确。

### R3. 验收边界（validation.md 明确「未通过/未验收」，非缺陷，但决定发布口径）
- 4K48 尖峰跨越、长期播放稳定性、HDR 色准、核显/4K 实时性能、完整 GLSL 库逐项、long-run AVX2 全尺寸大图等仍在「专项通过但非全量」状态；`rendererLayoutAndParameterSizing` 有未知断言失败。
- AI 字幕、网络串流（本研究项 H1/H2）可作为 1.1.0 已知限制在发布说明中声明，或作为 1.1.x 修复。

---

## 七、建议的修复优先级（若发版前修）

1. **H1、H2、H3**（安全信任边界）——若网络串流/AI 字幕属于 1.1.0 主打能力，建议修复后发；否则在发布说明中列明已知限制。
2. **M3（FilterGraph value 白名单，根因）**、**M6（cbuffer 对齐）**——低成本高收益。
3. **M1**（StartupWarmup 停表，防崩溃）——加一行 stop() 即可，建议顺手修。
4. **M2、M4、M5** 与全部低危项——可排到 1.1.x，不阻塞发版。
5. **长期**：拆 `MainWindow.cpp`（1303 行 god-file），按「源/处理链状态机」抽到独立类；此事单独 PR，回归后再并。

> 说明：本报告仅为静态代码审查结论，不含任何代码修改。所有「需确认」项建议在改动前用运行期行为/单测补证。