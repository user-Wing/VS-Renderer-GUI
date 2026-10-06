# Player 高精度截图 (1.0.7)

2026-10-06，1.0.7。右键 → 图像截取，源画面和实画面统一保存 RGBA 每通道 16-bit PNG (每像素 64-bit)。采用 PNG 自适应行滤波、libdeflate 最高压缩级别 12；压缩不改变整数像素。保存在线程池执行，完成后显示文件路径，同一时刻保存一张。

## 文件和取样位置

截图默认写入播放器旁的 `screenshots/`，设置 → 基本设置 → 默认截图保存路径可以修改，清空则恢复默认。右键新增“截图到指定路径…”的源画面 / 实画面入口，选择文件夹保存；单次选择不会覆盖默认路径，取消不保存。

文件名含源文件名、时间和 `source` / `VS` / `display`。每张 PNG 附 `.png.json`，记录尺寸、播放位置、源或输出位深、色彩编码、矩阵和范围等；已知色彩信息写入 PNG cICP / ICC。

| 入口 | 取样内容 |
| --- | --- |
| 原生直通源画面 | 按当前播放位置重新解码所选视频轨，转换为全范围浮点 RGB 后量化为 PNG16，保持源传递函数；不含字幕和显示变换 |
| VS 源画面 | 当前已显示的 VS 输出帧，以原有平面位深输入转换；保持 VS 的矩阵、范围、传递函数，不含字幕和显示变换 |
| 视频实画面 | 渲染器最终输出表面，含缩放、显示处理和字幕；8-bit / 10-bit / FP16 表面直接读为 float32，不再先降为 8-bit |
| 图片源画面 | 当前图片原始像素，保留已有 16-bit 精度 |
| 图片实画面 | 按当前视口、缩放、平移和旋转绘制到高精度图像；不再用 8-bit 窗口截图 |

视频矩阵未标注时，截图转换采用与原生渲染器一致的回退：HDR / BT.2020 使用 BT.2020，宽度至少 1280 使用 BT.709，其余使用 BT.601。JSON 单独记录 `conversionMatrixFallback`，不把推断写成源文件自带元数据。

## HDR 数据

HDR 每次截取生成五个文件：默认 `.png` 是 sRGB SDR 预览；`.png.json` 是预览和附件说明；`.hdr.png` 保留 HDR 编码；`.hdr.png.json` 记录 HDR 色彩信息；`.png.rgba32f` 保存原始浮点样本。默认预览已进行色调映射，在普通 SDR 图片查看器中不依赖 HDR 支持；HDR 显示器上也可直接查看另存的 HDR 图。

HDR 源画面的 `.hdr.png` 保留 PQ / HLG 编码。HDR 实画面读回 scRGB 线性数据，单位为 `1.0 = 80 cd/m²`；HDR PNG 转换为 BT.2020 / ST 2084，并标注 cICP `9 / 16 / 0 / 1`。SDR 预览先线性化 PQ / HLG 或 scRGB，再转换至 BT.709、以 203 nits 参考白作 ACES 拟合亮度压缩，转换至 sRGB；超出 SDR 色域的值在预览中裁切。HLG 预览采用 1000 nits / 系统 gamma 1.2。预览用于兼容查看，HDR 色准比较应使用原始 HDR PNG 或浮点数据。

检测到 HDR 时，另外保存 `.png.rgba32f`：从截图取样处取得的 RGBA float32 原始值，小端、像素交错、从上到下，无头部，无行填充。负数和大于 1 的值保持原样。源画面的浮点值是编码 RGB；实画面的浮点值是线性 scRGB，不能直接互相相减。

浮点图片也保存浮点附件，实画面在 float32 图像上绘制，避免先截断再保存。线性图片标记为 `linear-RGB`，保持原图相对单位，不擅自指定绝对亮度；这类附件不应乘以 scRGB 的 80 nits 单位。

PNG 的“无损”指整数像素的压缩无损。scRGB 转为 PQ PNG 必须进行色域转换和 16-bit 量化，超出可表示范围的值会截断；精确核对 HDR 渲染数据时使用浮点附件。原生后端 FP16 表面读回已修正为逐个半精度数转换，避免把 FP16 内存误当 float32。

用 NumPy 读取附件：

```python
import json
import numpy as np

path = "example-display.png"
with open(path + ".json", encoding="utf-8") as f:
    meta = json.load(f)
rgba = np.fromfile(path + ".rgba32f", dtype="<f4")
rgba = rgba.reshape(meta["height"], meta["width"], 4)
if meta["floatEncoding"] == "scRGB-linear":
    rgb_nits = rgba[:, :, :3] * meta["linearUnitNits"]
```

## 色准对比边界

测试时暂停到同一帧、隐藏 Tab 信息面板，固定输出尺寸、字幕、ICC / LUT、色彩引擎、抖动和 HDR 状态。SDR 与 HDR 分开比较，将各播放器的截图转换到相同色域与传递函数后再测误差。直接比较 PNG 字节或凭图片查看器外观，不能判断色准。

16-bit 文件不会增加渲染表面本身的位深；JSON 的 `inputBitDepthPerChannel` / `readbackBitDepth` 会记录实际精度。madVR 当前截图接口仍只提供 8-bit，写成 PNG16 也不能恢复丢失的位深，不能据此宣称完成 madVR 高精度读回。截图取样发生在显示表面读回处，不能测量操作系统后续合成、显示器校准和面板的物理色准。

原盘 BD 菜单由 libVLC 输出，当前菜单专用右键不提供此截图入口；正片交接回 3FP 后按上述视频路径截取。

## 验证

专项结果保存在 `build/screenshot-acceptance-1.0.7.txt`、`build/screenshot-color-1.0.7.txt`。覆盖 RGBA16 逐像素往返、ICC 保留、相邻 16-bit 渐变、YUV 全 / 有限范围和未标矩阵、PQ / HLG、图片源 / 实画面、原生解码源画面、字幕实画面，以及真实 FP16 渲染表面与浮点附件一致性。强制 HDR 渲染用于验证数据链，不能替代 HDR 显示器的物理测量。

最终截图专项 17 项通过，0 失败、0 跳过。`build/screenshot-regression-1.0.7.txt` 的相关回归 27 项通过，覆盖透明控制栏、固定播放列表、音频电平、HEVC / AV1 解码、输出格式、真实 BD 五种字幕与长节目跳转。语言检查 534 条通过，编译与差异检查通过。

上述为新增保存路径和 SDR 预览之前的记录。本次发行复测另存于 `build/release-1.0.7-acceptance.txt`，覆盖默认路径、中文含空格目录、单次文件夹选择、所有增强 / 补帧标识、SDR 逐像素精度、PQ / HLG 的 sRGB 预览与原始 HDR 编码、浮点扩展范围及真实 FP16 读回；相关回归为 `build/release-1.0.7-regression.txt`。

本次最终专项 **18 passed, 0 failed, 0 skipped**；相关回归 **15 passed, 0 failed, 0 skipped**，包含真实盘五种字幕、8538.53 秒节目定位、底栏 / 播放列表、音频电平、HEVC / AV1 软件解码及输出格式。语言检查 536 条通过。

既有 `TestColor::nativeDefaultAndPausedColorSwitch` 出现阻塞，旧的音频 Native DLL (2E3D6B4E…1005) 对照同样在 30 秒内无法结束，记录为未通过，不将其归入截图验收。对照日志为 `build/screenshot-color-baseline-1.0.7.txt`。

硬件解码与高级显示处理的无回读检查通过；独立 `TestColor::realVsOutputPreserved` 在同一测试进程中未能结束，不能宣称整套色彩测试通过。截图专项中的真实 VS + 字幕截图及 FP16 渲染读回已单独验收。
