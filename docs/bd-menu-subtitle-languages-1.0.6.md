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
