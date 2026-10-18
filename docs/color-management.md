# 色彩与 HDR 设置

## 选择输出路径

默认使用3FP原生色彩处理，包括HDR与HDR→SDR。Full 在“设置 → 解码设置 → 打开3FP 解码配置…”中可手动选择libplacebo，用于对照mpv；播放HDR素材不会自动启用它。

旧自动档（`color/engine=2`）启动时迁回3FP；原来手动选择的libplacebo（`1`）保留，ICC/LUT等参数不清空。单文件播放器仅使用3FP原生。检查Tab诊断中的实际引擎和输出模式；libplacebo仍是可选对照路径，不能把它当作所有像素格式都正确的参考。

## HDR、ICC 与 LUT

手动libplacebo模式提供 HDR→SDR/scRGB、tone/gamut 映射、系统或自定义 ICC、SDR 输出 LUT 和末端抖动。先确认 Windows HDR 开关和目标显示器，再设置源/输出；避免额外应用重复映射。

素材本身可能有意过曝，不能以降低亮度作为修复目标。HDR正确识别需要传递函数、色域和矩阵等标记；缺失标记的样本需单独核对，文件名不能替代色彩元数据。软硬解画面一致是回归证据，不等于显示器经过色度计校准。

应用色彩配置不应重载 VS 处理链。排查时先关闭自定义 ICC/LUT 并保留基线记录，再逐项恢复，区分源标记、解码、高精度交换、shader 与显示器效果。

## 截图颜色

VSCap 的 HDR 桌面来自 scRGB 高精度捕获。转 SDR 副本时使用截图源显示器的 Windows SDR 白位撤销桌面增益；高光和宽色域超出 SDR 的部分裁剪。它与普通 HDR 文件的通用查看映射用途不同。

JXL 原生携带 HDR 信息，PNG/AVIF 要由支持对应标记的查看器解释。截图格式、附件与副本规则见 [截图指南](capture-images.md)。用 8-bit 桌面截图不能证明 HDR 原始像素保真，PQ16 无损编码也不等于浮点逐位无损。

## 开发核对

修改色彩或 Jinc shader 时保持资源槽隔离：HDR t3、色彩 LUT t4、Jinc t5；HLSL 修改后同步生成通用与 SDR 专用字节码。重建当前 API20 完整补丁，不能只替换一份 header。

核对实际 GPU 像素、色彩 metadata、ICC/Alpha/位深和 HDR/SDR 两条路径。测试中的最大 RGB 差为零只代表对应输入与路径，不代表显示器绝对色准；步骤见 [性能排查](performance-debugging.md)。
