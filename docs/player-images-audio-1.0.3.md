# Player 图片优化与音频导入验证（1.0.3，2026-10-02）

## 图片

Tab 显示文件名、路径、大小、格式、原始像素尺寸/像素数量、原始位深与采样格式（解码器可提供时）、ICC 颜色空间、解码器/耗时、解码像素位数/Alpha/内存、拟合输出尺寸、视口和缩放、CPU/GPU/进程内存及 VS/预解码状态。未提供的数据明确标注。图片仍不经过 VS，不预解码相邻文件。

JPEG 使用 libjpeg-turbo 3.2.0 的内存映射输入和直接 BGRA 输出，减少 Qt 逐行 RGB 转换/复制；保留完整原始像素、正常 IDCT/色度上采样、ICC 与 EXIF 方向。预编译 MSYS2 静态库的无 ICC 错误分支与本项目 MinGW 运行时不兼容，已改为当前工具链 + NASM 2.16.03 从官方源码编译。源码与 NASM 下载均固定 SHA256，许可证随便携目录分发。

图片独立允许 0.25–65536 倍拟合比例，视频原有范围不变；绘制先裁切可见区域，再映射源矩形，避免巨型目标坐标导致空白。1024 倍以上继续滚轮放大及 40000 像素宽图的边缘区域已专项验证。图像依然由 CPU 解码及 Qt Raster 绘制，尚未接入 GPU 图片解码或整图显存缓存。

真实 JPG：`C:/Users/ARXChem/Downloads/にんげんまめ￤2日目東ア-31_pid112201257_アリスp0.jpg`，1,809,331,931 bytes，31603×65278，2,062,980,634 pixels。

同一次比较中，新路径解码 **12477 ms**，Qt 原路径 **23262 ms**，减少 **46.36%**；49 个分布在全图的原始像素逐一一致。两者都是完整 RGB32 图像，8,251,922,536 bytes。计时为解码阶段，不包含整个测试的启动、参考解码和截图耗时，也不承诺其他图片有相同比例。日志：`build/image-turbo-comparison-final.txt`。

本机 RTX 4070 Laptop GPU 报告显存 8188 MiB；该图单一 RGB32 缓冲区约7869.65 MiB。完整驻留的余量有限，本轮交付采用上述 CPU SIMD 路径。硬件解码和分块显存渲染尚未实现。

先前同尺寸 8×16 grid AVIF 的7767 ms记录只验证了解码、尺寸和交互，未证明画面内容正确。后来原图对比确认该样本在编码阶段已损坏；该记录不能作为完整AVIF验证结果。修正样本、原因与像素对比见 `player-apply-avif-1.0.3.md`。PNG/WebP/JPG 切换、坏 JPG、AVIF 高位深/透明、ICC 与 EXIF 旋转有独立专项检查。

## 音频与拖放

右键“音频设置”位于“字幕设置”正上方：声音轨道（含 MKV 内置音轨和外部音频）、声音同步（复位/滞后0.1s/提前0.1s）、静音、均衡器。切回内置轨道先卸载外部音频。均衡器为非模态窗口：60/170/310/600/1K/3K/6K/12K/14K/16K 十频段、±12 dB、默认/低音/人声/高音预设、复位、MST 总音量/WAV PCM 音量；设置实时作用于播放音频，不暂停视频。已建立的均衡器图用 gain 命令更新参数；初次启用或改变处理结构会更新音频图。

实际处理使用 3FP 的 FFmpeg equalizer/volume，与原有 atempo 链相接。LAV 音频在使用这些控制时转为 3FP，LAV 视频保持原选择。未声明布局的音频按声道数补标准布局；明确声明的布局保留。WASAPI 优先申请源2.0/5.1/7.1，设备不接受时申请2.0立体声，按 FFmpeg 重采样/混音。在本机实测 2→2、6→6、8→2；不代表所有设备都支持相同输出。

1 kHz 测试音设+6 dB后实际 WASAPI 峰值由0.0265137变为约0.0528626；WAV音量50%检查通过；滞后0.1s后从零定位产生4800个48 kHz静音样本，提前偏移后仍正常推进。详见 `build/audio-image-acceptance.txt` / 最终便携专项日志。

自动导入仅扫描当前视频同级目录，分别选最匹配的一个音频和主字幕：同名优先，SxxExxx/EP/第x集等明确集数优先，再比较名称有效词比例；CRF/分辨率/编码数字不作为集数。明确集数冲突不匹配，同最高分歧义候选不自动加载。示例 S01E01-crf12-xxx 与 S01E01-CN 可匹配；S01E02 不会误配。

拖放使用**视频视口**分区：音频上50%挂为外部音频，下50%关闭当前媒体并单独播放；字幕上40%挂次字幕（顶部），下60%挂主字幕（底部），始终保留当前视频。拖动尚未松手时显示居中提示，退出拖动或完成放置后消失；全屏也可见。主/次外部字幕独立保存，可与内置轨道混选、交换。

## 验证与交付

只运行本次相关专项：图片信息/深度缩放、JPEG元数据、图片打开/切换/坏图、AVIF、文件名匹配、音轨/拖放/非模态窗口及实际音频处理。没有运行无关导出或整套回归。界面截图：`build/audio-equalizer.png`、`build/image-deep-zoom.png`。

Release构建及语言包311条字符串检查通过。最终便携目录7个功能专项加初始化/清理共9/9通过（`build/audio-image-portable-final-tests2.txt`）；实际音频增益/4800静音样本和2/6/8声道协商均在交付目录验证。首次便携专项中固定350ms的启动后时间推进断言失败，已改为3s内持续推进且保持Playing的检查，复测通过；未因此改动生产播放逻辑。`build/audio-image-delivery-audit.json`确认两EXE和FFF.Native.dll与构建匹配，12份INI/VPY哈希保留。最终程序交付至 `dist/VS-Renderer-GUI-windows-x64/vs-player.exe`，版本保持1.0.3；配置及用户VPY由交付前后哈希核对保留。没有创建压缩包、推送或发布。

参考实现与依赖：[Qt JPEG源码](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/plugins/imageformats/jpeg/qjpeghandler.cpp)、[libjpeg-turbo SIMD说明](https://libjpeg-turbo.org/About/SIMDCoverage)、[TurboJPEG源码](https://github.com/libjpeg-turbo/libjpeg-turbo/tree/3.2.0)、[FFmpeg equalizer](https://ffmpeg.org/ffmpeg-filters.html#equalizer)。
