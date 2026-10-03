# 3FP 色彩管理与 HDR（1.0.4 开发版）

本轮依据主目录 `color.md` 接入可选的 libplacebo D3D11 显示链。默认仍是 3FP 原生 709 / 2020 模式；未选择高级模式时，不加载新的色彩 DLL。版本沿用当前工作区的 1.0.4，没有打包或发布。

## 使用

播放器右键 → 设置 → 解码设置 → **打开3FP 解码配置…**。下方依次是 LAV Video 和 LAV Audio 配置按钮。

色彩引擎可以选择默认原生或自定义 libplacebo。高级模式的初始方案如下：

| 项目 | 默认行为 |
| --- | --- |
| 目标输出 | HDR 源且 Windows HDR 显示器可用时 FP16 scRGB；否则 SDR |
| 色调映射 | 普通 SDR 原值；HDR 使用 Spline；有效 HDR10+ 使用 ST2094-40 |
| 色域映射 | 普通 SDR 相对色度；HDR / 广色域使用感知映射 |
| 质量 | libplacebo 高质量路径，关闭额外 deband、sigmoid 和图像增强 |
| SDR 参考峰值 | 100 nit；HDR 峰值 0 表示使用现有显示器检测 |
| 字幕参考白 | 203 nit，沿用原生 HDR 字幕合成 |
| 峰值检测 | 开启；暂停时复用已映射帧，不重复测量 |
| 显示 ICC | 自动读取当前显示器配置；没有配置时使用标准 SDR 目标 |
| 抖动 | 整数输出末端启用；FP16 scRGB 不重复抖动 |
| 逆色调映射 / 对比恢复 | 关闭 / 0 |

允许手动选择 Spline、ST2094-40、BT.2390、Clip、Linear，设置目标峰值、ICC 或 SDR 输出 `.cube`。ICC 文件支持中文路径。高质量 ICC 使用 65³ LUT；配置和监视器未改变时复用结果。

HDR scRGB 输出跳过 SDR 显示 ICC 和 SDR `.cube`，按显示器报告的 HDR 色域映射。没有适用于 HDR 的通用 Windows SDR ICC 转换，不把它强行套到 HDR 信号。madVR 模式的颜色仍由 madVR 管理，本配置不参与其显示链。

所有配置存入已有便携 INI 的 `color/` 分组，原有加载 / 保存完整 INI 功能包含这些设置。应用色彩配置不重开媒体、不重新执行 VPY，也不改变播放位置和暂停状态。

## 处理链与颜色来源

```text
3FP 解码纹理 ─────────────┐
                         ├→ 原生显示或 libplacebo D3D11 → 原有字幕 / OSD → swap chain
VS 处理后的输出 → 上传 ──┘
```

高级分支导入现有 D3D11 YUV / RGB 纹理，映射到缓存的 GPU 目标，再通过 GPU CopyResource 进入现有合成器。不新增逐帧 GPU→CPU→GPU 回传。VS CPU 帧仍按原桥接方式上传；本分支不会改写其像素、frame props、滤镜顺序、分辨率或帧率。

仅复制解码帧的属性，不持有解码表面。解码帧缺少 CICP 时允许补充同一媒体流的参数和静态 HDR 信息；**VS 输出不补回原始流的 HDR 信息**，防止用户 VPY 已经映射到 SDR 后再次被标为 HDR。缺失字段在诊断中保留 unknown，并在显示副本中推断，不回写源帧。

内置脚本生成器的必要 RGB↔YUV 转换改用当前 clip 的 matrix / primaries，709 主流输入仍得到 709，2020 输入不会被硬编码为 709。手写或已经保存的 VPY 不自动重写；改变 transfer / primaries 的用户滤镜仍需要正确设置自身输出 props。

原生路径修正了 BT.2020 constant-luminance：矩阵 10 使用其分段色度公式，矩阵 9 仍是 NCL；709 和既有 NCL 路径不变。

## HDR 与降级边界

- HDR10 / HLG：使用原有显示器检测和 scRGB 交换链，libplacebo 执行目标感知的色调 / 色域映射。API 360 的线性 RGB 单位是 203 nit，末端转换为 Windows scRGB 的 80 nit 单位；不能把两者直接等同。
- HDR10+：使用 FFmpeg 已解析的 `AVDynamicHDRPlus`，映射 MaxSCL、统计、knee 和 Bezier anchors，自动选择 ST2094-40。库当前采用第一窗口；未宣称覆盖多窗口和全部饱和度调整语义。测试确认改变 knee / anchor 会改变输出，而非只显示识别标志。
- Dolby Vision：有效的 FFmpeg decoded metadata 可以交给 libplacebo 做不依赖增强层的 reshaping；要求仍保留原始编码分量。已转换为普通 RGB、缺失 / 不合法元数据或依赖 EL 的情况回到底层颜色。**不重建 FEL，也不宣称完整 Dolby 授权认证。**
- HDR Vivid：识别 side data 并显示提示，目前仅映射其 PQ / HLG 底层；动态 Vivid 曲线未实现。
- 错误 ICC、LUT、缺少 DLL 或设备初始化失败：继续原生显示，Tab 显示原因。360 投影和音频封面继续现有特殊合成器，并报告原生路径。
- 高级模式的缩放由 libplacebo 执行：Nearest、Bilinear、Bicubic、Lanczos3/4、Spline36、Jinc。原生 D3D11 缩放请求在该模式使用 Bilinear，Super-XBR 请求使用 Jinc；这些只在高级显示分支发生，不变更 VS 处理链。抗振铃由 libplacebo 的滤波器能力决定。

`color.md` 的后续扩展尚未全部完成：JPEG Ultra HDR / gain-map 编解码、完整 HDR10+ 多窗口、Vivid 动态曲线、FEL、全部 source/display/HDR 截图格式与增强像素探针，以及与 mpv 的逐像素自动对照。本轮未修改图片编辑器的 gain-map 或 HDR 导出功能。

## 信息与验证

Tab 显示请求 / 实际引擎、源 matrix / primaries / transfer / range / chroma / 位深、来自解码还是 VS、显式字段和流补充字段、实际 tone / gamut / dither、HDR 元数据状态、ICC / LUT、输出位深 / HDR、目标峰值与回退原因。`renderSubmitMs` 是 CPU 提交耗时，不能当作 GPU 执行时间或完整帧时延。

独立构建目录为 `build/color-1.0.4`。Qt 色彩专项、离屏 HDR 元数据专项和播放器配置专项分别验证：

- 默认原生、暂停切换、暂停色彩缓存与尺寸改变后重新映射。
- BT.2020 CL 与 NCL 不混淆；PQ / HLG 输出有限且可读取。
- sRGB ICC、中文文件名、identity `.cube`、RGB 红蓝通道顺序；错误配置回退后可恢复。
- 硬解模式实际启用，既有硬件回传和软件转换统计均为 0。
- 真实 VS 输出矩阵 / transfer 和像素保持不变；配置应用不重新加载 VS 脚本。
- 203 nit PQ 白色经 FP16 离屏输出为 scRGB **2.5352**，期望 **2.5375**；只证明单位和离屏处理正确，不代替真实 HDR 面板观感验证。
- 畸形 HDR10+ / 截断 DV 元数据不使处理崩溃，不改写源 side data。
- 合成残差关闭的 DV metadata 实际进入 reshaping shader；残差依赖打开后回退。该测试不替代商用 DV 样片颜色验收。
- 16bit 常量量化到 8bit，抖动关闭得到 1 个级别、平均 124；打开得到 2 个级别、平均 123.5156，理论 123.5175，无明显均值漂移。

已有脚本测试的一项旧断言仍失败：`anime4kUsesShaderPathAndUniformScale` 期待 `open("固定路径")`，但当前着色器归档机制改为 `_shader_path` 变量再读取，与本轮矩阵更改无关。本轮保留这个不匹配并记录，未以“全量测试通过”替代专项结果。VS 桥接专项为 10/10，通过实际脚本执行另验证了生成器的 709 / 2020 矩阵选择与 PQ props 保留。

详细结果与 UI 截图保存在本机构建目录。真实 HDR 面板、跨显示器 / HDR 开关、商用 DV / Vivid 样片及长期性能尚需实机验证；本轮不报告 mpv 等效质量或性能倍数。

## 依赖与重建

本地已有 vs-placebo 是 Vulkan VS 插件，无法导出 D3D11 入口，因此新增固定版本二进制 SDK：libplacebo **7.360.1-2**、LittleCMS **2.19.1**，其余运行时依赖按导入表取闭包。下载地址和 SHA-256 在 `third_party/color/packages.json`，没有克隆上游工程。

```powershell
./tools/prepare-color.ps1
./tools/build-3fp.ps1 -FffProject .deps/fff-player -OutputDirectory build/mingw-release
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
```

`patches/3fp-color-management.patch` 只包含本轮 3FP 修改；`build-3fp.ps1` 在旧补丁之后应用，并复制权威 C ABI / loader 头文件。没有 SDK 时 GUI 和原生回退仍可构建，高级模式报告依赖缺失。

`color/` 为可替换的独立 DLL 目录。MSYS2 库使用较新 GCC，与 Qt 的旧 MinGW runtime 不能混放；staging 将本闭包 DLL 文件名 / 导入名隔离为 `v3_` 前缀，未修改可执行代码。原始 / 部署哈希和修改说明在 `color/runtime.json`，上游许可证在 `color/licenses/`，本轮不会覆盖现有 FFmpeg、VS 或 Qt runtime。

上游说明：[libplacebo](https://github.com/haasn/libplacebo)、[renderer](https://libplacebo.org/renderer/)、[MSYS2 固定包来源](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-libplacebo)。

## 本机交付

2026-10-03 更新到 `dist/VS-Renderer-GUI-windows-x64/`，沿用 1.0.4 开发版。合并使用“开发 VapourSynth 实时渲染器”窗口完成字幕回归后的最终 3FP DLL；色彩 8/8、配置 / 语言 / 播放状态 6/6 和离屏 HDR 专项再次通过。部署后指定便携 DLL 的切换 / 回退 smoke 为 4/4。

部署更新 37 个文件，其余 509 个既有文件保持 SHA-256；INI、VPY 和缓存未被部署替换。备份在 `build/color-backup-20261003-220118`，更新哈希和边界记录在 `build/color-deployment-audit.json`。本轮未打包、推送 Git 或发布。
