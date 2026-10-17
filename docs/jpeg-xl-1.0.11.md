# JPEG XL 截图（1.0.11 续修）

VSCap 新配置的默认保存格式为 JXL，顺序为 JXL、AVIF、PNG；已有明确保存的格式设置不被重置。截图工具条、贴图右键菜单均有三种格式。取消保存仍保留截图层和标注，打勾只复制剪贴板。复制 JXL 文件后 Alt+T 支持贴图。

JXL 为无损 RGBA16，每通道 16 位，不使用 XYB 有损变换。HDR 桌面 scRGB 按 80 nit 单位转换为 BT.2020 / ST 2084 PQ，色彩信息写入 JXL 原生结构，不依赖 PNG 的 ICC TRC 兼容路径。桌面 SDR 白位嵌入标准容器 XML box，VSP 读取后继续使用白位修复逻辑。JXL 的无损指编码后的 16 位像素无损，不表示浮点 scRGB 转为整数 PQ 时完全没有量化或色域裁切。

HDR 自动保存跟随默认格式。JXL / PNG 继续同时输出 HDR 与 `_SDR` 副本，无 JSON、裸浮点附件；AVIF 使用原有 AWJ 异步保存。普通 SDR 截图不自动落盘，手动保存每次选择路径并记住目录。PNG 仍为压缩 RGBA16；AVIF 仍为质量 70 的有损格式，不声称 16 位无损。

VSP 的“视频 → 图像截取”和“截图到指定路径…”均包含 PNG、AVIF、JXL，分别支持源画面与含字幕/显示效果的实画面。JXL / AVIF 截取只输出单个文件；PNG 保留原有 HDR 附件流程。AVIF Full 优先使用 AWJ（AOM，质量 70，4:4:4；SDR 10 位、HDR 12 位）；无 AWJ 的独立版使用当前目录 / PATH 的 FFmpeg（需要 libaom-av1，CRF 19），不把 40 MB AWJ 塞回单文件。

libjxl 0.12.0 与 Brotli、Highway、Little CMS 使用同一套 MinGW 静态编译，Full / Lite 均内置 JXL 编解码，不依赖外部 FFmpeg 来保存或打开 JXL。修订和依赖固定在 `tools/jxl-runtime.cmake` 及 libjxl 子模块提交，许可全文随 `licenses/jpeg-xl-LICENSE.txt` 分发。MinGW 构建对 `stage_write.cc` 的 `MakeUnsigned` 增加 `HWY_INLINE`，绕过 GCC 13 / Win64 的未对齐 AVX 参数栈槽问题，保留 AVX2；未修改色彩计算。

色彩正确依赖文件标记与查看器的 HDR 支持。JXL 原生标记能避开此前 PNG ICC 解释分歧，但不能保证不支持 JXL / HDR 的 QQ、照片等所有版本都显示相同；SDR 兼容分发使用 `_SDR` 副本。不会把 480 nit 硬编码为新截图默认白位。

验证记录：`build/audit-jxl-20261017/`。自动测试覆盖 sRGB / PQ / HLG 的 RGBA16 逐点往返、透明像素、截断文件、嵌入白位、VSCap 自动保存与 JXL 文件贴图、取消三种保存、播放器三种格式和源画面截取。实机 HDR 热键采集、第三方查看器显示仍需维护者验收。

AVIF 外部 FFmpeg 路径同时给编码器参数与帧 `setparams` 写入色域 / 传递函数 / 矩阵；只设输出参数时本机 FFmpeg 会产生 2/2 的未指定标记。已在无 AWJ 的隔离目录验证输出为 SDR 1/13/1、HDR 9/16/9，避免将 HDR 写成色彩信息不明的 AVIF。

`D:/Screens/过曝.png`（3840×2034）转换测试：原 PNG 9,030,654 字节；JXL 6,418,884 字节。一次本机测量编码 531 ms、解码 136 ms，全部 RGBA16 像素一致，嵌入历史验证白位后 SDR 预览也完全一致。该 480 nit 值只用于历史样图测试；原文件未改动。尺寸与耗时仅代表此图和此机器，不表示所有内容均有相同比例。

本轮结果：精度 / 样图测试 15 通过、2 项可选设备测试跳过；截图交互 17 通过；播放器截取和已有 PNG 回归 20 通过；无 AWJ 隔离路径的格式 / scRGB 截取 9 通过，均无失败。初次构建暴露的 SIMD 崩溃已由内联补丁修复；新增 203 nit scRGB 测试使用直接浮点像素，避免 `QImage::fill(QColor)` 的高值裁切干扰夹具。
