# HDR 桌面截图的 SDR 白位还原

日期：2026-10-17；版本保持 1.0.11。本轮修正 SDR 导出、预览及白位传递，保留 HDR 原有像素编码，不处理 Windows 照片、QQ 缩略图的 HDR 元数据兼容性。

## 现象与原因

用户补充：`D:\Screens\过曝.png` 在 Edge 显示正常，但 QQ 缩略图偏暗、Windows 照片偏亮；`SDR仍然过曝.png` 在 Edge 仍偏亮。HDR 文件带 BT.2100 PQ ICC，另两张图片是相同 ICC 的 sRGB；这将 HDR 显示路径的差异与 SDR 像素本身错误区分开。不能仅因某个查看器显示正常，就认定所有转换正确。

用共同壁纸的多个位置反推，本次历史样图的 SDR 白位约 480 nit。旧转换固定按 203 nit，再套 ACES 曲线，使正常的 RGB 约 (154,185,210) 变成约 (200,239,255)，产生提亮和通道裁剪。显示器校准 ICC 的约 710 nit 峰值与桌面 SDR 白位不是同一个数值，不能互相替代。

## 实现与边界

- 捕获 HDR 桌面时，通过 QScreen 的 HMONITOR / GDI 设备名匹配活动 DisplayConfig 路径，查询该 target 的 `DISPLAYCONFIG_SDR_WHITE_LEVEL`，换算为 `SDRWhiteLevel × 80 / 1000` nit。生产代码没有写死 480；查询不可用时采用 Windows 标称 80 nit，图像白位缺失、无效或非正数也按 80 处理。
- 将来源白位固定为图像的 `capSdrWhiteNits`，不在异步保存时重新读取目标显示器。线性 scRGB 先乘 `80 / sourceWhiteNits`，再编码 sRGB；有白位信息的 PQ 图先反变换到绝对亮度、转换色域，再按相同白位还原。
- 桌面路径不叠加 ACES 对比度曲线。白位内的 SDR 内容保持相对亮度和颜色；超出 SDR 亮度或色域的部分裁剪，不承诺 SDR 图保留全部 HDR 高光细节。HDR 原图的 PQ 编码像素保持不变。
- QImage 裁切保留白位；新 HDR PNG 内写入普通 PNG 文本字段，快速解码读取此字段。HDR 剪贴板通过独立 MIME 项传递白位，贴图往返保留；标注的颜色也按源白位绘制。
- 普通未标记 HDR 图片的通用 VSP 映射保留。历史 PNG 没有保存源白位，软件不从图片或显示器峰值猜测 480 nit。本轮只在历史样图验收中明确注入由样图对照推断的 480 nit。

依据：[微软白位字段与单位](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-displayconfig_sdr_white_level)、[Windows HDR 桌面的 SDR 参考白](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range#match-your-apps-reference-white-to-the-os-sdr-reference-white-level)。

## 验证

审计目录 `build/audit-sdr-white-20261017/`：

- `before.txt`：先加入复现测试，旧逻辑在 80 / 203 / 480 nit 三组全部失败。
- `cap.txt`：修复后三组 SDR 梯度还原误差不超过 1 个 16 位码值；覆盖 PNG 白位、裁切、无效元数据、负值和超范围高光、原 HDR PQ 像素及压缩，共 11 passed（含 init/cleanup）。
- `desktopHdrFixture`：用户样图的 7 个共同壁纸采样位置，换算 8 位 RGB 后平均误差 0.150、最大误差 1.012；完整 3840×2034 图的映射约 0.23 秒。两张图的窗口内容不同、尺寸不同，未将这些采样宣称为全图逐像素相同。
- `workflow.txt`：截图模态、选区、文字、撤销、取消保存、后台保存、贴图及 HDR 剪贴板白位往返，共 15 passed。已有剪贴板占用重试可能记录初次写入警告，最终断言通过。
- `png.txt`：8/16 位快速解码及白位字段保留，4 passed。
- `player.txt`：80 / 203 / 480 nit 中性灰显示、带白位 HDR PNG 的实际加载、未标记 HDR 的原查看路径及 16 位截图，共 6 passed；样图解码约 99 ms、加载至预览约 369 ms。
- 修正后的 SDR 样图：`capture_SDR.png`。带 480 nit 验收字段的 HDR 对照副本：`capture_HDR.png`，用于播放器完整加载测试，不覆盖用户文件。原始三图和显示器 ICC 的 SHA-256 记录在 `originals.json`。
- `image-verification.json`：四个用户原文件哈希保持不变，HDR 对照副本解码为 RGBA16 后的 SHA-256 与原图完全一致。

本机当前桌面为 SDR；没有将样图、数值回归或 Win32 编译成功冒称为实际 HDR 显示器白位查询、多屏切换及 Edge / Windows 照片实机验收。HDR 屏实时截图仍待复验。旧 `D:\Screenshot-HDR` 的浮点验收素材已不可读取，本轮不以该失效路径的结果作为回归结论。
