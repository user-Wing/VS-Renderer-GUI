# VSCap HDR 截图体积、颜色与加载续修

> 后续 [桌面 SDR 白位修复](vscap-sdr-white-1.0.11.md) 替代本文的桌面 ACES 映射：本文“与 VSP 一致”只代表两条旧转换一致，不能证明正确还原桌面。压缩、后台保存和加载记录仍按本文保留。

日期：2026-10-17。保持 1.0.11；先完成本地编译部署，按后续授权推送源码并补充现有 1.0.11 发布说明。GitHub 该版本已由维护者发布并上传附件，本轮仅更新说明，保持其发布状态与附件。用户原始文件与设置保留，Full 打包与二进制上传由维护者完成。

## 原因与改动

用户提供 `D:\Screenshot-HDR\20261017-102016-483.png`、`_SDR.png` 和原始 scRGB 附件。图像为 3840×2040、RGBA16；旧 PNG 各约 62.8 MB，解码像素数据为 62,668,800 bytes。解码后的内存等于展开像素数据是正常现象，不能以此判断磁盘压缩是否启用。本次实际核对 IDAT，确认旧文件为不压缩的 DEFLATE 块。

`QImageWriter::setCompression(9)` 的参数是 Qt 的 0～100 刻度，Qt PNG 实现再以 `(compression * 9) / 91` 映射到 zlib，原参数恰好变为 0。本次改为 70，即 zlib level 6，继续保留 16 位四通道和 ICC，不改 PNG 格式。依据：[Qt 6.10 PNG 实现](https://github.com/qt/qtbase/blob/6.10/src/gui/image/qpnghandler.cpp#L953-L967)。

此前热键回调在创建框选窗口前同步做 PQ 转换、两张 PNG 编码、125,337,600 bytes 的 `.scrgb-f32` 及 JSON 写入。现在先创建截图层，自动保存通过 QtConcurrent 在后台运行，失败仍在托盘提示。自动输出仅保留 HDR PNG 与 `_SDR.png`，HDR 信息由 PNG 内 ICC 携带；旧附件不删除。PNG 是整数 PQ 编码，不能将其宣称为原始浮点负值及超范围样本的逐位归档。

原 SDR 伴随图使用 Reinhard，VSP 使用 203 nit 参考白的 ACES luminance 映射，两者不一致。现在共用 `src/image/HdrPreview.h`：scRGB 按 80 nit 单位进入同一映射；PQ 反变换使用完整 16 位查找表，sRGB 编码使用查表与线性插值，保持原 HDR 图片供查看和导出。

播放器原快速 PNG 仅支持 RGBA8；本次支持 RGBA16、16 位大端通道还原及受支持的 cICP。其余格式继续由 Qt 解码。图片信息栏此前把逻辑像素直接当作视口和输出尺寸；现在乘入实际设备像素比，与视频信息的单位一致。绘图原本已由 Qt 做 DPI 缩放，这次修正的是读数。

## 实图与回归证据

本机 Windows 11、Qt 6.10.2 / MinGW Release。以下是已缓存文件的单次聚焦采样，不是磁盘全冷启动指标。

| 项目 | 结果 |
| --- | --- |
| HDR PNG | 62,769,454 → 1,281,649 bytes，像素逐点一致，ICC 单独识别为 BT.2100 PQ |
| SDR PNG | 62,769,435 → 1,229,430 bytes；颜色按共用映射重新生成 |
| 原始裸浮点附件 | 125,337,600 bytes；新截图不再自动生成，原样本未删 |
| SDR 映射计算 | 相同原始浮点图，约 1,632 → 204 ms |
| SDR 伴随图与 VSP | 全行、每 13 像素采样 RGB：平均差 0.296、最大差 58 个 16 位码值；差异来自 HDR PNG 的 PQ 量化与色域矩阵往返 |
| 原旧 PNG 加载 | 快速解码 129 ms；加载至预览完成 372 ms |
| 新压缩 HDR PNG 加载 | 快速解码 43 ms，Qt 对照解码 166 ms；加载至预览完成 362 ms |
| 高 DPI 信息 | 实际 DPR=2，窗口视口 2000×1220；信息与物理像素计算一致 |
| sRGB 查表误差 | 1,000,001 个输入对照解析公式，最大差 1 个 16 位码值 |

整张 HDR PNG 的转换与编码仍约 3.64 秒，两图保存总计约 4.29 秒；这些工作现在在后台完成，不再放在框选窗口出现之前。后台保存入队小于 100 ms、500 ms 内界面计时事件可响应的回归已通过，不将其冒称为真实 HDR 桌面热键端到端耗时。

回归日志位于 `build/audit-hdr-20261017/`：

- `cap-final.txt`：HDR/PQ、SDR、PNG 实际压缩、无 sidecar、用户实图，7 passed（含 init/cleanup）。
- `cap-workflow.txt`：模态截图、选区、标注、取消保存、后台保存、文字、贴图、剪贴板，共 15 passed。Windows 剪贴板偶有被占用的初次写入警告，既有重试路径及断言通过。本次未重跑 AWJ AVIF 编码专项。
- `png-final.txt`：8/16 位、ICC/cICP、取消代次及大图 16 线程预测，像素与 Qt 对照一致，6 passed。
- `player-original.txt` / `player-compressed.txt`：实际文件加载、高 DPI、HDR 预览保留源、16 位截图，分别 5 / 3 passed。
- `gamma-lut.json`：查表数值误差；`originals.json`：用户原始四个文件 SHA-256。

当前桌面采集返回 SDR；没有实际 HDR 输出可用于本轮验证。HDR 显示器 Alt+A 的 DXGI 初始化、GPU readback、出框时间及显示器观感仍需实机复验，不能用文件夹里的 HDR 图片替代该验收。
