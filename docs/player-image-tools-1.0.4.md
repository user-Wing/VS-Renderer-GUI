# VS Player 图片工具栏（1.0.4 开发中）

## 第二轮改进

顶栏48→38px，按钮40→32px；尺寸窗口至少560px，转换窗口至少640px。AVIF自动位深：原始8bit输出10bit，10/12bit保持，>12bit降至12bit；手动选择仍可用。

裁剪框内部移动，四角和四边调整，图像范围内框外拖动重新选区。自由、1:1、16:9、4:3、3:4、9:16和自定义宽高比；连续编辑自定义比例不会累积缩小框。比例在整数像素上允许最多一像素舍入。默认PNG Lossless，直接复制选区像素，避免JPEG重新编码带来的额外损失。

尺寸算法默认Jinc：CPU径向EWA 2-lobe窗口，Lanczos3/4为对应3/4-tap sinc窗口；缩小时扩展源像素采样范围。后台最多四线程，可处理超出GPU纹理限制的源尺寸，内存仍受实际系统资源限制；8/16位、ICC和预乘Alpha保留。CPU实现使用16384段核查表插值。最近邻/双线性提供较快选择。

Anime4K Mode A Fast可用于放大：只在选择它并另存时启动独立VS进程，经YUV444P16进入现有placebo Shader，输出恢复RGB和ICC，Alpha单独缩放。GPU纹理/显存限制仍适用于Anime4K；遇到错误明确报告，不冒充Jinc或无损放大。图片查看不执行这条处理链。

视频Lanczos4接入原生后端与Player/Renderer菜单，保留ABI编号0–7并新增8；对应源码补丁与构建步骤保留。设置的渲染页新增四级补帧起始档，自动启用/换文件从此开始。最低档的稳定5秒观察窗口内，至少48帧且丢帧超过5%时左上角提示“补帧性能不足，建议关闭补帧。”，不再因负载自动关闭；恢复正常后隐藏。暂停、定位和重开继续保留原状态规则。

专项日志 `build/image-refinements-final-tests.txt`：图片算法（含实际Anime4K输出、Alpha/ICC/自动位深）、裁剪交互、原生Lanczos4放大/缩小、最低档提示及恢复、设置应用状态/保存起始档、真实自动降档及失败，共六项通过（含Qt初始化/清理8/8）。最终裁剪控件复查见 `build/image-refinements-crop-final.txt`；20000px超宽源和16位缩放补充检查见 `build/image-refinements-depth-final.txt`。不运行无关全量回归，不将小样本检查视作超大图片性能基准。

## 操作

打开图片时显示顶部图标，视频/音频模式隐藏。按钮从左到右：详细编辑（打开独立编辑窗口，首批工具与边界见 [图像编辑方案](image-editor-plan.md)）、左转90°、右转90°、水平镜像、删除到回收站、桌面背景、调整图像大小、方格裁剪、压缩和转换格式。悬停显示中文名称。

旋转和镜像仅改变当前显示；换图片会复位，不写回原图。调整大小、裁剪、转换按照当前旋转/镜像画面另存，不覆盖正在查看的源文件。比例尺寸可联动宽高；裁剪拖选区域，显示三分网格，支持1:1。尺寸/裁剪另存无损PNG；转换选择目标格式。删除成功后打开相邻图片，最后一张删除后清空画面。桌面背景按屏幕尺寸保留比例生成独立BMP，不依赖临时目录。

输出使用后台线程，操作期间图标禁用，底栏显示开始、完成路径或具体错误；关闭程序会取消后端进程和文件写入。原文件与已有目标在失败时保留。未改动图像的压缩转换直接读源文件，避免先复制巨大的PNG。

## AWJimage

使用未经改动的 [AWJimage 1.1.0](https://github.com/Dominic485649/AWJimage/releases/tag/1.1.0) 独立CLI，原生编码算法：AVIF/libavif+AOM、WebP/libwebp、JXL/libjxl、JPEG/JPEGli、PNG/libpng。固定质量1–100；目标视觉质量由上游自动搜索达标候选（PNG只提供固定质量）。AVIF 默认AOM、YUV而非RGB/GBR Identity、420采样、自动位深，提供420/422/444/auto和auto/8/10/12位选项。JPEGli输出兼容.jpg。WebP/JXL质量100使用上游无损语义；JPEG100仍有损，PNG100无损。Alpha可自动保留、强制保留或移除。

AVIF/WebP/JXL统一速度0–10，默认5。AWJ负责线程/内存预算与超限AVIF自动网格，禁用自动尺寸缩小。视觉质量会反复编码/解码，耗时和内存明显高于固定质量；D3D11仅加速其视觉质量指标，编码/解码仍在CPU。这个上游版本使用AOM，不把它标作zenrav1e。

Windows后端要求AVX2。图片查看、旋转、镜像、裁剪/尺寸与桌面背景不依赖该后端。`runtime/awj` 包含原始AWJ.exe/AWJ.com、LICENSE、NOTICE.txt、本软件补充通知和固定修订源码tar.gz。后端及对应源码均校验SHA-256。详细来源见 `awjimage-NOTICE.txt`。

## 专项验证

只运行本次图片操作及相关查看测试；不运行无关的导出/音频/滤镜全量回归，也不以当前高负载机器上的耗时作性能结论。验证图标顺序/提示/禁用、旋转与镜像的像素共享、实际四角画面、宽高联动、裁剪坐标、ICC/Alpha保留、后台另存、原图保护、失败时已有目标保留；五种原生格式及视觉质量实际转换后重新解码。额外保留超宽图与深度缩放专项。

真实桌面背景未在测试中更换；回收站操作由Qt Windows实现，避免对用户图片执行删除测试。

Release构建通过；图标/窗口、变换输出、后台另存、五格式原生转换、WebP视觉质量和超宽图深度缩放共10项功能用例通过（含Qt初始化/清理为12/12）。日志：`build/image-tools-tests4.txt`。截图：`build/image-tools-toolbar.png`、`build/image-tools-crop.png`、`build/image-tools-convert.png`。

最终图标/对话框与后台另存专项复查通过，见 `build/image-tools-final-ui.txt`。本地 `dist/VS-Renderer-GUI-windows-x64` 更新两份EXE、独立后端和文档，EXE与构建结果一致；当前INI保留，native与VPY共20份哈希未变。旧1.0.3归档哈希未变，未创建1.0.4归档，也未执行Git推送。部署核对见 `build/image-tools-delivery-audit.json`。

第二轮已再次同步两份EXE和新增Lanczos4的FFF.Native.dll，三份文件与Release构建结果哈希一致。当前INI、VPY和LAV文件哈希未变，旧1.0.3归档未变；未打包或推送。核对见 `build/image-refinements-delivery-audit.json`。
