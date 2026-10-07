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
