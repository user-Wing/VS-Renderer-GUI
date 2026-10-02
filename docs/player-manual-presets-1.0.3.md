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
