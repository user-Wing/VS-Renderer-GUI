# 图像编辑第二轮：B–D、PSD 与 HDR

日期：2026-10-03。版本：1.0.4 本地开发；本轮不推送 GitHub，不制作发布压缩包。

## 本轮行为

“详细编辑”使用 Photoshop 系列工作区：左侧42px图标工具栏，右键切换同组工具；顶部显示当前工具的大小、透明度、选区组合、容差、文字或裁剪比例；中央文档页签和双轴标尺；右侧颜色/色板、属性/调整、图层/通道/历史/路径面板。面板可从窗口菜单显隐，工具和菜单使用共享 QAction。

PSD 详细编辑直接读源文件图层，不以播放器的扁平预览作为编辑文档。图层树可选择组内子层，支持新建/复制/删除/重命名/排序、显隐、锁定、透明度、混合、组穿透和剪贴属性；移动组同时移动后代。像素与蒙版采用同一文档撤销事务，蒙版黑隐藏、白显示，支持像素层和组蒙版。

画布预览使用不可变文档快照在后台生成，旧预览结果与修订号不一致时丢弃；PSD/浮点 TIFF 另存也使用后台快照。查看器也复用原生PSD/PSB图层合成，支持ZIP合并预览在FFmpeg中不能读取的文件；普通查看输出完整QImage，详细编辑仍直接读原文件图层。编辑器“打开”复用播放器JPEG/AVIF等解码器。文档仍保留8/16bit整数和32bit浮点原始像素、256px稀疏块、受限RAM缓存及临时磁盘。

## 工具与颜色

| 功能组 | 当前实现 |
| --- | --- |
| 选择 | 矩形/椭圆、自由/多边形套索、局部边缘吸附磁性套索、颜色连通魔棒/快速选择、边框背景辅助分割、选区添加/减去/相交、反选/羽化 |
| 几何与测量 | 可移动并调整四角/边的裁剪框；自由、1:1、16:9、4:3、3:4、9:16和自定义比例；吸管、标尺、切片PNG、抓手和缩放 |
| 像素工具 | 画笔/橡皮、按颜色容差替换且保留Alpha、取样图章、无源谐和污点修复/源纹理修复、显式历史画笔源、线性渐变、颜色连通油漆桶 |
| 局部调整 | 模糊、锐化、加深、减淡、海绵去饱和；图层蒙版支持画笔/橡皮、模糊/锐化、填充与渐变 |
| 路径与内容 | 直线锚点路径，可描边、填充或转选区；按字体/字号栅格化文字，栅格化矩形/椭圆形状 |
| 颜色 | 曝光、亮度/对比度、色阶、RGB及单通道曲线、色相/饱和度、RGB颜色增益、通道混合器、反相、灰度、阈值、色调分离 |
| 滤镜与空间 | 高斯模糊、USM锐化、中值降噪、边缘检测；工作RGB ICC转换；RGB/独立RGB通道/Alpha及蒙版预览 |

颜色调整只修改当前未锁定像素图层，按选区覆盖率应用，并保持原精度与Alpha。曲线使用分段线性控制点及端点斜率外推；显示直方图来自256×128合成预览采样。ICC 转换使用 Qt QColorSpace，可选 sRGB、Linear sRGB、Display P3、Adobe RGB、ProPhoto RGB；没有把未标记相机RGB直接当作正确sRGB。

## PSD/PSB 与真实样本

自有模块支持 RGB 8/16/32bit PSD/PSB，读取RAW、RLE、ZIP与ZIP预测压缩通道，保留支持的像素图层、组、像素蒙版、显隐、锁定、透明度、剪贴和混合属性。首次需要相关图层时，压缩通道展开至临时磁盘，随后按区域读取；隐藏层不会仅因建立图层树就全部解码。首次显示仍可能解压完整通道，并未实现压缩格式内部随机区域解码或高质量预览金字塔。

PSD超过30000px应使用PSB；PSD/PSB均不超过300000px。8/16bit图层可另存PSD/PSB，并写入有效合并预览，供其它读取器查看。32bit保存需保留源Photoshop HDR色彩数据 `hdrt`；新建或由普通图片转换的Float32文档没有这项数据时明确拒绝PSD保存，应导出32bit浮点TIFF。PSD不保存本编辑器撤销历史，蒙版按目标PSD通道位深编码。

测试样本：`D:\Chem\自编讲义\OrganicARX\封面相关\封面-新.psd`，545,521,646字节（520.25 MiB）、5950×8420、16bit RGB、9层。专项检查层名称/数量、预览、局部像素编辑和撤销；原文件不得覆盖。当前机器同时执行高负载FFmpeg任务，不能据本轮等待时间给出解码性能倍数或稳定打开耗时。

## HDR 与保存

高精度编辑、显示映射与HDR输出分别处理。Float32的负值及大于1的RGB值不因预览被改写。SDR预览可调整曝光/白点并启用HDR→SDR映射；原生Windows输出检测显示器及系统HDR开关，使用D3D11 FP16 scRGB交换链（1.0为80nits），不支持时回退SDR并显示原因。

AVIF保留ICC优先；没有有效ICC时识别已知CICP RGB原色及sRGB/线性/PQ/HLG传递函数，不把未知色彩空间伪标记。PQ/HLG输入自动启用SDR显示映射；Qt 6.10将PQ的10000nit端点归一为64线性单位，原生输出据此换算到1.0=80nit的scRGB；相对线性/SDR使用203nit纸白，HLG还未实现基于场景元数据的完整OOTF适配。依据：[Qt PQ/HLG归一实现](https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/gui/painting/qcolortransfergeneric_p.h)。

32bit浮点TIFF按条带输出、非预乘Alpha及ICC，保留负值与高光，超过经典TIFF偏移范围时使用BigTIFF。PNG只导出8/16bit扁平结果，当前像素数据限256 MiB；Float32拒绝PNG隐式降精度。扁平TIFF/PNG不含图层与历史，另存保护源文件。

当前没有HDR显示器：可以验证shader编译、能力检测、SDR回退、原始数值及导出数据，不能宣称已验证真实HDR面板的亮度或观感。

## 明确限制

- 尚无Bezier控制柄、可回改文字图层、完整矢量对象、路径工程持久化或语义AI对象选择；框选分割以边框背景颜色连通为依据。
- 复杂PSD对象有可用RGB像素时栅格化并提示，没有像素则明确提示不渲染；不承诺完整智能对象、文字、矢量蒙版、调整层、图层效果和全部混合模式保真。CMYK/Lab等需先转RGB。
- 几何选区栅格化范围限64M像素；颜色连通、框选辅助分割、整幅填充/渐变/反选/羽化等当前限16M像素；超限报错，快速选择可在较大图上操作局部区域。
- 无源污点修复半径≤128px，较大区域需取样源修复；图章源缓存2048²，历史画笔源≤64 MiB原精度像素。
- 原生PSD图层读取当前不导入只含合并图像、没有可编辑图层记录的PSD；普通TIFF/EXR等查看仍走已有解码路径，不承诺所有外部高精度格式的Float32导入往返。浮点TIFF导出数值已独立验证。
- 尚未实现DNG/LibRaw、LittleCMS专业印刷/软打样、多通道/专色、ACES/LUT及无损保存所有未知PSD资源；本轮不引入巨型AI模型，不复制GIMP GPL或提取的闭源Photopea代码。

## 验证与交付

只运行本轮相关专项，不执行无关全量回归。40个实际测试槽通过（QtTest初始化/清理另计，日志合计54 passed），0失败；PSD压缩测试槽内部遍历8/16/32bit × RAW/RLE/ZIP/ZIP预测共12组输入。可选真实样本已启用。

| 项目 | 结果与证据 |
| --- | --- |
| Release构建 | `build/image-round2-final-build.txt`、`build/image-round2-sample-viewer-build.txt`；无新增运行时DLL |
| 核心 | 4项，`build/image-round2-core-final.txt`；高精度、缓存、蒙版/选区、混合极值与柔光暗部 |
| PSD | 6项，`build/image-round2-psd-final.txt`；12组独立压缩fixture、组/蒙版往返、Lazy精度链、快照生命周期、损坏输入、真实520MiB源文件 |
| 工具和画布 | 5+9项，`build/image-round2-advanced-final.txt`、`build/image-round2-editor-final.txt`；原精度、连通填充、组移动、蒙版、撤销和真实鼠标手势 |
| 工作区 | 6项，`build/image-round2-layout-final.txt`；组内子层编辑、对话框、CICP P3/PQ AVIF打开、后台保存快照/继续编辑/源文件保护，以及9层真实PSD窗口笔划和精确撤销 |
| 颜色/HDR | 7项，`build/image-round2-adjustments-final.txt`；父组锁定、ICC/曝光、滤镜跨块边界、负值/高光浮点TIFF、shader编译/SDR回退、Qt PQ归一常量 |
| Player | 3项，`build/image-round2-player-final.txt`；轨道默认选择（无Default字幕取首条）、图片工具栏、原生AVIF编码/解码入口 |
| 独立PSD读取器 | psd-tools 1.23.0确认组/子层、RGB16及蒙版123低位、合并Alpha；`build/image-round2-psd-independent.json`。FFmpeg尚未实现ZIP合并预览，未把这一解码器限制误判为PSD输出损坏 |
| 实际大PSD普通查看 | `build/image-round2-sample-viewer.txt`；5950×8420完整RGBA16、原生PSD解码路径、与图层合成像素一致；不作为性能基准 |
| 本地部署 | 已更新 `dist/VS-Renderer-GUI-windows-x64/vs-player.exe` 与本轮文档/许可；`build/image-round2-delivery-audit.json`记录构建/便携exe一致、Renderer/native/player.ini及样本哈希保持。仅本地1.0.4，未推送/打包 |

开发工作区截图：`build/image-editor-large-psd.png`（不随便携包分发）。源文件SHA-256：`6da69581d0ba8e8ec901328a0132ed3c31dfbf76012345ddb34643f178304795`，本轮未覆盖原文件。

真实HDR面板亮度、色彩观感和稳定呈现未验证；保留SDR回退与数值验收结论，不以编译或普通屏截图替代HDR实屏验证。
