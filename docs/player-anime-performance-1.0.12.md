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
