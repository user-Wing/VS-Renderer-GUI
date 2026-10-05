# VS-Player / 3FP 色彩管理与 HDR 实现升级规格

> 本文是下一轮 AI 的实现任务书。目标不是重写播放器，也不是为了“支持更多格式”堆开关，而是在现有 3FP + VapourSynth + D3D11 架构上，继续精进 FFFProject 自身的视频色彩管理，并把可合法使用的 HDR 标准与开源实现接入到授权边界。mpv `gpu-next/libplacebo` 只作为对照实现和可选高级模式，不再作为必须追随的主架构。
>
> 原则：以 FFFProject 最新上游的原生 D3D11/scRGB 色彩路径为基线，优先保留已经验证的 HDR 数值输出；新增能力不能把显示器 ICC、Windows HDR 校准等 DWM 已负责的显示端处理再次放进播放器，避免双重校色。所有新增能力必须有明确 fallback，并且不得把“能识别元数据”宣传成“完整支持某商业 HDR 标准”。

重要补充（开发者修改了来自AI处理的文章）遵照本文件所有的修改都须以**精进色彩管理/HDR/颜色输出质量这些方面**为目标，且**不能影响后续的VS处理链**。下文的很多库，当前VS似乎有很多已经带上了，能用先用本地，不拉项目。没有再拉。

目前已了解3FP的情况和当前视频色彩的模式，BT709为视频主流，HDR分支混乱，因此3FP默认的统一硬编709策略是正确的；本次增强只为了补全HDR/其他色彩的问题。UI侧-解码设置，把LAV两个移到下面，顺序如下所示：

打开3FP 解码配置----打开LAV Video/Audio

3FP解码配置：

默认3FP原生支持（当前709/2020简易模式，基本上不考虑老视频，主流标准已经足够）

自定义：Libplacebo高级调控---默认用你认为的最佳模式，允许手动调节。调节方案你来决定。





## 1. 当前状态与问题

### 1.1 当前 3FP 已经具备的正确基础

当前实现不是传统 EVR/D3D9 一类低精度链路。现有代码已经具备以下基础，应保留并回归测试，不要推倒重写：

1. VapourSynth 外部帧桥可传递 8/10/12/16-bit Gray/YUV/RGB，并保留 `_ColorRange`、`_Primaries`、`_Transfer`、`_Matrix`、chroma location 等帧属性。
2. 3FP 原生识别 YUV420/422/444 的 8/10/12/16-bit，以及 NV12、P010/P012/P016、P210/P212/P216 等格式；高位深输入不会统一先压为 8-bit。
3. SDR 高位深输出会使用 `DXGI_FORMAT_R10G10B10A2_UNORM`；HDR 输出使用 `DXGI_FORMAT_R16G16B16A16_FLOAT`。
4. HDR swapchain 使用 scRGB 语义，即 linear Rec.709 primaries、1.0 = 80 nits；这与 Windows Advanced Color 的现代推荐路径一致。
5. D3D11 swapchain 使用 flip model（`DXGI_SWAP_EFFECT_FLIP_DISCARD`）。
6. 已实现 PQ、HLG、BT.709 线性化，Rec.2020 ↔ Rec.709 基础转换，HDR→SDR 具备 BT.2390 + IPT chroma hull，而不是简单 clip。
7. 能读取 ST.2086/MaxCLL/MaxFALL 等 HDR 静态元数据，能检测显示器 HDR 状态、BitsPerColor、最大/最小/全屏亮度，并读取 Windows Advanced Color 部分亮度信息。
8. D3D11 VideoProcessor 快速路径主动关闭 driver auto-processing，避免显卡驱动偷偷叠加锐化、降噪、肤色增强等不可重复处理。
9. HDR10+、Dolby Vision、HDR Vivid 已存在基础分类/元数据识别代码，但目前主要用于诊断、峰值估计或 fallback，不能据此宣称完整动态 HDR 渲染。

关键代码位置：

- `src/backend/VapourSynthFrameServer.cpp`
- `src/backend/ThreeFpPlayer.cpp`
- `.deps/fff-player/FFF.Native/3FP/Render/VideoRenderer.cpp`
- `.deps/fff-player/FFF.Native/3FP/Hdr/HdrProcessor.cpp`
- `.deps/fff-player/FFF.Native/3FP/Api/FFF.Player.Api.h`

### 1.2 最新 FFFProject 与 mpv/libplacebo 的关系

以 2026-09-30 上游 `Lake1059/FFF_Project` HEAD `15995b807bf8a27037d2697fcdb89d13064ee247` 为基线，FFFProject 已经不是早期“709/2020 简易模式”可以概括的状态。最新代码专门处理了 P3/BT.2020 区分、宽色域 SDR 的 scRGB 呈现、Windows SDR white、FP16 scRGB 超 1.0/负值保真、HDR/SDR 交换链切换、BT.2390 + IPT、动态 HDR 分类以及 Dolby 扩展接口。

因此后续不应把“迁移到 libplacebo”视为色准升级的前提。开发者反馈当前 HDR 色准已经优于 mpv；仅凭代码不能把“所有场景都优于 mpv”当成已证明事实，但可以确认 FFFProject 在 Windows Advanced Color/scRGB 契约上进行了大量专门实现和数值约束，必须先保留这些优势，再做同条件 A/B 验证。

当前真正需要继续补齐的是：

- HDR10+ ST 2094-40 动态 metadata 的完整利用；
- Dolby Vision RPU / EL / FEL 在授权边界内的真实处理；
- HDR Vivid 与 Ultra HDR/gain-map；
- BT.2020 constant-luminance 等少数边缘色彩空间；
- 末端量化/dithering 是否需要以及如何不破坏现有原生输出；
- VS 滤镜链改变 transfer/primaries 后的 metadata 一致性；
- 与 mpv/libplacebo 的同条件数值对照，而不是默认认为后者就是正确答案。

**显示器 ICC 不属于上述缺口。** 最新 FFFProject README 明确规定：播放器不读取 ICC，也不在 shader 中补偿显示设置；ICC、HDR 校准和 SDR 亮度映射由 DWM 在窗口合成时应用一次。这个设计应继续保留。

## 2. 总体实现策略

### 2.1 以 FFFProject 原生色彩引擎为主

下一轮修改必须把 **FFFProject 原生 D3D11/scRGB renderer 作为默认和权威路径**。不得为了“更像 mpv”而让 libplacebo 自动接管当前 HDR 主画面。

建议结构：

```text
FFmpeg / D3D11VA / VapourSynth frame
                ↓
       FFFProject 原生 metadata / HDR processor
                ↓
       3FP D3D11 native shader
      ├─ range / matrix / chroma
      ├─ Rec.709 / Rec.2020 / P3
      ├─ PQ / HLG
      ├─ BT.2390 + IPT HDR→SDR
      ├─ Dolby/HDR dynamic extension
      ├─ native scaling / overlay
      └─ BGRA8 / RGB10A2 / FP16 scRGB
                ↓
      Windows DWM / Advanced Color
      └─ 系统 ICC / HDR 校准 / SDR brightness
```

这里的关键边界是：**播放器负责把内容正确变成 Windows 约定的目标交换链数值；DWM 负责把交换链映射到实际显示器。** 两层不能重复。

libplacebo 仍可使用，但只放在用户已经指定的“自定义：Libplacebo 高级调控”中，作为实验/高级调节/对照路径。它不能成为默认 Auto backend，也不能因为启用它而绕过 FFFProject 已有的 scRGB、P3、HDR metadata、Dolby 扩展和 Windows Advanced Color 逻辑。

### 2.2 两种色彩模式

```text
Color engine:
- 3FP Native（默认）
- Libplacebo Advanced（手动选择）
```

规则：

1. 默认永远进入 3FP Native；
2. Libplacebo Advanced 只有用户显式选择时启用；
3. 高级模式失败时可回退 3FP Native，并报告原因；
4. A/B 对照必须保证同一源帧、同一目标峰值和同一 Windows 显示环境；
5. 不删除当前 `PixelShaderSource`、BT.2390/IPT、P3/scRGB 与 D3D11 VideoProcessor 路径。

## 3. 依赖引入策略

### 3.1 libplacebo：可选高级依赖

上游：

- https://github.com/haasn/libplacebo
- 官方主仓库镜像指向 VideoLAN Codeberg/GitLab；拉取时注意 recursive submodules。

用途：

- D3D11 GPU abstraction；
- FFmpeg frame interop；
- color space decode/encode；
- 高级 gamut/tone mapping 实验；
- HDR10+ / Dolby Vision 等实现对照；
- dithering / deband / custom shader 实验；
- A/B reference；
- 用户手动选择的高级调控。

**不把显示器 ICC/3DLUT 作为引入 libplacebo 的理由。** 默认 3FP Native 继续把显示器校准交给 DWM。

授权：

- LGPL-2.1-or-later。
- 如果项目继续以 MIT/其他宽松协议发布，优先动态链接 libplacebo DLL，并在 `THIRD_PARTY_NOTICES.md`、发行包中完整保留 LGPL notice、许可证与源码获取方式。
- 不要静态链接后忽略 LGPL 义务。
- 若未来上游提供特别许可，再单独评估；当前按 LGPL 处理。

构建要求：

- Windows D3D11 backend 必须开启；
- 若启用 Dolby Vision，应构建 libdovi 支持；
- 尽量使用 shared DLL，便于许可证隔离和替换。

### 3.2 LittleCMS 2 / ICC：不进入默认视频显示链

LittleCMS 2 仍可服务于图片编辑、离线转换或“源文件自带 ICC”的内容解释，但 **不负责读取当前显示器 ICC 并在视频 shader 中再校色一次**。

FFFProject 最新上游已经明确选择 Windows 原生契约：

```text
3FP 生成标准 SDR / RGB10A2 / FP16 scRGB
        ↓
DWM / Advanced Color
        ↓
系统 ICC + HDR calibration + SDR brightness
        ↓
物理显示器
```

因此默认视频播放链不需要 lcms2，也不需要建立 monitor ICC cache/3DLUT。若未来做专业软打样或离线导出，那是独立功能，不能混入普通播放。

### 3.3 libdovi / dovi_tool：Dolby Vision 开源边界

上游：

- https://github.com/quietvoid/dovi_tool
- 其中 `dolby_vision` crate 可构建 C API，即 libdovi。

用途：

- 读取/写入 Dolby Vision RPU metadata；
- profile / level / mapping metadata；
- 为 libplacebo Dolby Vision reshape 提供数据；
- 测试/诊断 RPU 与 profile。

授权：

- MIT。

**边界必须明确：**

- 可实现开源的 DV metadata 解析、RPU reshape、Profile 5→PQ/SDR 等 libplacebo/libdovi 已公开实现的能力。
- 可以把输出转换为普通 Rec.2020/PQ、scRGB 或 SDR。
- 不得把本项目标记为“Dolby Vision Certified”或宣称获得 Dolby 授权。
- 不实现、伪造或宣传依赖 Dolby 私有授权链、官方认证链、专有 HDMI/设备 DV tunnel。
- FEL 完整重建只有在 decoder 真正提供 enhancement layer 且 libplacebo 路径完成并通过样本验证后才能标记为“完整 FEL”；否则 UI 必须显示 BL/RPU fallback 或对应处理路径。
- 当前代码“检测到 DOVI metadata + HDR10 fallback”不能继续被描述为“支持 Dolby Vision 渲染”。

### 3.4 hdr10plus-rs / hdr10plus_tool：HDR10+

上游：

- https://github.com/quietvoid/hdr10plus_tool
- MSYS2 当前有 `libhdr10plus-rs` C API 包装。

用途：

- ST 2094-40 metadata 读取/校验；
- 测试动态 metadata 顺序；
- raw HEVC/MKV 诊断；
- 必要时作为 C API metadata parser。

授权：

- MIT。

运行时优先级：

1. FFmpeg 已经提供 `AV_FRAME_DATA_DYNAMIC_HDR_PLUS` 时，直接使用 FFmpeg side data；
2. libplacebo 若能从 AVFrame 映射 HDR10+ metadata，则优先使用 libplacebo；
3. hdr10plus-rs 主要作为补充 parser / validation library，不要为了“依赖多”而重复解析同一份数据；
4. `hdr10plus_tool` CLI 适合作为测试工具，不要求播放器运行时启动外部进程。

### 3.5 Google libultrahdr：Ultra HDR / gain map 图片

上游：

- https://github.com/google/libultrahdr

用途：

- Ultra HDR / JPEG gain-map 解码；
- SDR base + gain map → HDR reconstruction；
- 根据显示设备 max display boost 重建适合目标显示器的 HDR；
- Ultra HDR 编码；
- 可输出 HDR linear half-float、PQ 10-bit、HLG 10-bit 或 SDR。

授权：

- MIT / Apache-2.0 双许可。

注意：Ultra HDR 是**图片链**，不要塞进视频 `VideoRenderer.cpp`。

建议结构：

```text
PlayerImage / ImageDocument
        ↓
libultrahdr detect/decode
        ↓
SDR base + gainmap + metadata
        ↓
target display boost / HDR capability
        ↓
RGBA16F linear / PQ / HLG
        ↓
统一 CMS / HDR surface
```

第一阶段必须完成 JPEG Ultra HDR。HEIF/AVIF 等 gain-map 容器能力按 libultrahdr 当前 API 实际支持做 feature detection；不因为格式名存在就宣传完整支持。

长期目标应抽象到 ISO/IEC 21496-1 gain map，而不是把内部类型命名死为 “GoogleJPEG”。

### 3.6 HDR Vivid：使用 FFmpeg 公共 metadata，到官方参考代码授权边界停止

FFmpeg 已公开：

- `AV_FRAME_DATA_DYNAMIC_HDR_VIVID`
- `AVDynamicHDRVivid`
- metadata 描述对应 CUVA/UWA HDR Vivid。

因此当前项目可以合法继续做：

- 识别 HDR Vivid；
- 读取 FFmpeg 已解析的 dynamic metadata；
- 依据公开标准实现目标显示映射；
- HDR Vivid → 普通 PQ/scRGB/SDR 的兼容渲染；
- metadata 诊断。

### 3.7 其他 HDR / gain-map 标准

作为 P3 扩展考虑：

- SL-HDR1/2/3 / Advanced HDR by Technicolor：优先检测 metadata 和 base-layer fallback；没有成熟、许可证明确、易于 C/C++ 集成的参考库时，不为了凑格式数量手写大规模未验证实现。
- ISO/IEC 21496-1 gain map：作为 Ultra HDR 之后的统一 gain-map 抽象。
- AVIF/HEIF/JXL gain map：复用统一 gain-map metadata model；由各容器/codec 库负责承载解析，不把 JPEG_R 的 container 逻辑硬套到所有格式。
- HDR 静态 metadata、ambient viewing metadata：FFmpeg 已有 side data 时统一纳入 target mapping context。

## 4. 统一颜色数据模型

### 4.1 新增 FrameColorMetadata

不要再用：

```cpp
uint transfer;
uint source2020;
```

作为完整色彩语义。

增加类似：

```text
FrameColorMetadata
- pixel representation
  - RGB / YCbCr / ICtCp / XYZ / Gray
  - bit depth
  - range
  - chroma subsampling
  - chroma location
- matrix coefficients
- primaries
- transfer
- white point
- nominal peak / black
- mastering display primaries
- mastering min/max luminance
- MaxCLL / MaxFALL
- HDR10+ dynamic metadata
- Dolby Vision metadata / RPU state
- HDR Vivid dynamic metadata
- gain-map metadata
- source ICC
- metadata provenance
  - container
  - codec
  - frame side data
  - VapourSynth props
  - user override
```

每个字段必须区分：

- Unknown；
- Explicit；
- Inferred。

不能把“未知”直接改成“709”再丢掉来源。

### 4.2 推断规则集中化

当前代码存在类似：

```text
width >= 1280 → 709
otherwise → 601
```

这种兼容规则可以保留为最后 fallback，但必须集中到单一模块，并把推断标记为 inferred。

优先顺序建议：

1. frame side data；
2. codec parameters；
3. container metadata；
4. VapourSynth frame props；
5. user override；
6. legacy resolution heuristic。

## 5. SDR 与广色域视频

### 5.1 完整 primaries / transfer

至少覆盖 FFmpeg/libplacebo 能明确映射的常见空间：

- BT.709 / sRGB primaries；
- BT.601 525 / 625；
- BT.2020；
- DCI-P3；
- Display-P3 / P3-D65；
- Adobe RGB；
- ProPhoto；
- ACES AP0/AP1；
- XYZ；
- BT.1886；
- sRGB TRC；
- Gamma 2.2/2.4；
- linear；
- PQ；
- HLG。

不要继续把所有 SDR 都强制视为 BT.709 transfer。

### 5.2 修复 BT.2020 CL

当前 `BT2020_CL` 与 `BT2020_NCL` 走相同简单 Kr/Kb 矩阵。

需要：

- 3FP Native 路径必须先拆分 CL / NCL，不能把问题转交给可选后端；
- Libplacebo Advanced 同样要验证 CL / NCL 行为，但它不是默认修复手段；
- 增加测试图，确认 constant-luminance 不再走普通 NCL 逆矩阵。

## 6. 显示器色彩管理：遵循 Windows / DWM 契约

### 6.1 播放器需要识别什么

窗口跨显示器时，3FP 仍需要重新解析与**交换链选择和 HDR 数值契约**有关的信息：

- 当前 HMONITOR；
- BitsPerColor；
- DXGI colorspace；
- HDR / Advanced Color 状态；
- min/max/full-frame luminance；
- Windows SDR white level；
- scRGB 是否可用。

这些信息用于决定 BGRA8、RGB10A2 或 FP16 scRGB，以及 HDR/SDR paper white 和 fallback。窗口跨显示器后必须重新检测。

### 6.2 显示器 ICC 不由播放器再处理

最新 FFFProject README 已明确：

> 播放器不读取 ICC，也不在 shader 中补偿显示设置；ICC、HDR 校准和 SDR 亮度映射只由 DWM 在窗口合成时应用一次。

因此默认视频播放链应保持：

```text
源视频色彩语义
→ 3FP Native 生成标准 SDR / RGB10A2 / FP16 scRGB
→ Present
→ DWM / Advanced Color
→ 系统 ICC + HDR Calibration + SDR brightness
→ 物理显示器
```

不要增加“显示器 ICC：自动/关闭/自定义”这一层，也不要读取 monitor ICC 后生成 3DLUT 再套进 shader。这样做很容易和 DWM 的系统颜色管理发生**双重校色**。

这里要区分两种 ICC：

- **显示器 ICC**：属于 Windows/DWM，播放器不碰。
- **源内容嵌入 ICC**：常见于图片，属于“这个文件本身是什么颜色”的解释，可以在图片/离线转换链处理；它不是显示器校准。

### 6.3 LUT 到底是什么，是否需要

LUT（Look-Up Table，查找表）本质上是预先计算好的映射：

- **1D LUT**：每个通道独立映射，常用于 gamma、白平衡、单通道校准。
- **3D LUT**：输入 RGB 三维坐标，输出另一组 RGB，能同时表达 gamma、色域变换和复杂颜色风格。
- 常见文件是 `.cube`。

在标准视频播放里，大部分时候**不需要额外 LUT**。BT.709、BT.2020、PQ、HLG、P3、矩阵转换、BT.2390 等都有明确数学定义，FFFProject 直接按公式/metadata 处理更透明，也更容易验证数值正确性。

LUT 只保留两个可选用途：

1. **创意/风格 LUT**：用户故意改变画面，例如电影调色 `.cube`。这是效果器，不是“色彩正确性”功能，默认关闭。
2. **特殊专业软打样/设备仿真 LUT**：仅在明确需要模拟另一目标设备时使用，必须独立于普通播放，不得和系统显示器 ICC 叠加。

所以普通播放器 UI 不增加 LUT。若以后做高级模式，只放在 Libplacebo Advanced / Developer 下，并明确标为“用户效果/软打样”，不能默认启用。

## 7. Tone mapping 与 gamut mapping

### 7.1 默认算法

默认继续使用并优化 FFFProject 原生 HDR/tone mapping。Libplacebo 的 preset 只属于用户手动选择的 Advanced 模式，不得替换 Native 默认。

建议用户层只暴露少数模式：

```text
Tone mapping:
- Auto
- Balanced
- High quality
- BT.2390 compatibility
```

普通设置页不要暴露大量算法参数；原生模式保持少量稳定策略，Libplacebo Advanced 的详细参数放进开发者/高级页。

开发者高级设置可以提供：

- spline；
- ST2094-40；
- BT.2390；
- clip/linear 等调试模式；
- peak detection；
- contrast recovery；
- inverse tone mapping。

### 7.2 Gamut mapping

默认使用 perceptual/soft gamut mapping，禁止以简单 per-channel clip 作为正常默认。

保留“Clip”仅用于 A/B 测试。

必须覆盖：

- Rec.2020 HDR → Rec.709 SDR；
- Rec.2020 HDR → P3 HDR；
- P3 SDR → Rec.709；
- Rec.709 SDR → HDR desktop；
- source gamut > target gamut；
- source gamut < target gamut 时默认不无意义扩张饱和度。

### 7.3 不再依赖 SetHDRMetaData 作为主要 tone mapper

保留 `SetHDRMetaData` 用于向系统提供合理 metadata，但不要把“最终显示映射正确性”寄托在 metadata 下传。

应用自己应根据目标显示器能力，把输出映射到目标可表示的 luminance/gamut，再交给 scRGB/DWM。

## 8. HDR10

HDR10 是基线能力，必须成为所有动态 HDR fallback 的稳定底座。

要求：

- PQ；
- Rec.2020；
- ST.2086 mastering metadata；
- MaxCLL / MaxFALL；
- 10-bit 输入；
- SDR fallback；
- HDR scRGB output；
- 显示器 target-aware mapping；
- 缺 metadata 时 conservative fallback；
- metadata 变化时正确刷新 target state。

测试至少包含：

- 1000-nit master；
- 4000-nit master；
- MaxCLL 与 mastering peak 不一致；
- 没有 MaxCLL；
- 没有 ST2086；
- 10-bit gradient；
- 高饱和 Rec.2020 gamut boundary。

## 9. HLG

要求：

- 正确 ARIB STD-B67 transfer；
- 不把 HLG 简化为固定“1000 nit PQ”；
- 使用正确 OOTF/system gamma；
- 根据 target display peak 适配；
- HLG→SDR；
- HLG→HDR；
- Rec.2020 gamut mapping；
- overlay/subtitle 的 SDR white 与 HDR 主画面关系正确。

HLG 默认必须在 3FP Native 中按标准继续完善；Libplacebo 只用于 A/B 和高级模式，不能成为修复 Native HLG 的替代品。

## 10. HDR10+

### 10.1 当前问题

当前 3FP 能识别 `AVDynamicHDRPlus` 并估计动态 source peak，但这不是完整 HDR10+。

完整路径应读取 ST 2094-40 的：

- targeted system display max luminance；
- distribution values；
- knee point；
- Bezier anchors；
- scene/frame metadata；
- saturation mapping 等。

### 10.2 实现

优先：

```text
AVFrame HDR10+ side data
        ↓
libplacebo frame metadata
        ↓
ST2094-40 aware tone mapping
        ↓
target display
```

如果 FFmpeg→libplacebo 自动映射不足，再用 hdr10plus-rs C API 补充。

不得只把 `maxscl` 算一个峰值后标记为“完整 HDR10+”。

### 10.3 输出边界

Windows/DXGI 没有通用“把 HDR10+ 动态 metadata 原样送给所有显示器”的可靠开放链路，因此项目目标应是：

- **应用内解析 HDR10+，在 shader 中完成 display mapping，再输出 scRGB/PQ 兼容结果。**

这是“正确显示 HDR10+ 内容”，不是宣称电视收到原生 HDR10+ bitstream passthrough。

## 11. Dolby Vision

### 11.1 当前问题

当前 `HdrProcessor` 会识别 Dolby Vision、profile、level、RPU、EL/FEL/MEL，并取 Level 1/source max PQ 做峰值估计，但注释已明确：

```text
RPU is retained for diagnostics/FEL identification only.
```

也就是当前没有真正应用 DV reshape。

### 11.2 下一步

启用：

- libdovi；
- libplacebo Dolby Vision；
- FFmpeg `AV_FRAME_DATA_DOVI_METADATA` / RPU；
- Profile 5 的 IPT/reshape；
- Profile 7/8 的 BL + RPU reshape；
- 可获得 enhancement layer 时再处理 MEL/FEL。

目标输出：

- SDR：DV → target-aware SDR；
- HDR：DV → linear/scRGB 或标准 Rec.2020/PQ HDR；
- 不依赖官方 Dolby display tunnel。

### 11.3 FEL

必须严格区分：

- RPU-only；
- BL + RPU；
- BL + MEL；
- BL + FEL。

只有 enhancement layer 真正被解码、与 base layer 对齐并参与 libplacebo reshape 后，才能报告 FEL active。

否则显示：

```text
Dolby Vision P7 FEL detected · BL/RPU fallback
```

而不是“FEL supported”。

### 11.4 授权声明

UI/README 中：

- 可以写“Dolby Vision metadata detected”；
- 可以写“open-source Dolby Vision reshape path”；
- 可以写“converted to HDR10/scRGB/SDR for playback”；
- 不写“Dolby Vision Certified”；
- 不放 Dolby 官方 logo；
- 不暗示产品通过 Dolby 授权认证。

## 12. HDR Vivid

### 12.1 可实现部分

基于 FFmpeg `AVDynamicHDRVivid`：

- metadata detection；
- per-frame target luminance；
- dynamic tone/gamut parameters；
- PQ/HLG base；
- target display adaptation；
- SDR fallback。

## 13. Ultra HDR / gain-map

### 13.1 支持对象

第一目标：

- JPEG Ultra HDR / JPEG_R；
- SDR base compatibility；
- gain map；
- XMP/metadata；
- target display boost；
- HDR reconstruction。

第二目标：

- ISO 21496-1 统一 gain-map model；
- AVIF/HEIF/JXL 等容器的 gain-map 扩展。

### 13.2 解码

使用 libultrahdr。

HDR 显示器：

```text
Ultra HDR JPEG
→ SDR base + gain map
→ max_display_boost
→ RGBA16F linear
→ target display CMS
→ scRGB HDR
```

SDR 显示器：

```text
Ultra HDR JPEG
→ SDR base
→ ICC
→ SDR output
```

避免先把 gain map 合成为 8-bit 再进图片播放器。

### 13.3 编码

图片编辑器导出增加 Ultra HDR：

- SDR base + HDR intent；
- 或 HDR intent 自动 tone map 生成 SDR base；
- gain map quality；
- gain map scale；
- gamma；
- content boost；
- target display peak。

UI 第一版只暴露：

- Auto；
- Quality；
- Target peak。

高级参数以后再做，不为一次功能加入大量配置。

## 14. Dithering

这是当前视频管线明确缺失的一环。

要求：

- 16F/高精度 → 10-bit：必要时 dither；
- 高精度 → 8-bit：默认 dither；
- HDR/SDR 都不能在前级提前量化；
- scaling、tone/gamut mapping、ICC 完成后再进行末端量化；
- 默认采用 libplacebo 推荐算法；
- 提供 Off 仅用于测试。

测试：

1. 10-bit gray gradient → 8-bit output；
2. HDR smooth gradient → SDR；
3. 蓝天/暗部渐变；
4. Off/On 截图统计 unique levels 和 banding；
5. 保证 dither 不改变平均亮度和明显色偏。

## 15. VapourSynth 链的特殊要求

### 15.1 不丢 frame props

任何内置 VPY 预设做 resize/format conversion 后，都要保留或显式重建：

- `_ColorRange`
- `_Primaries`
- `_Transfer`
- `_Matrix`
- `_ChromaLocation`
- HDR metadata 可保留部分

如果某滤镜改变了颜色语义，必须同步改变 props。

### 15.2 不硬编码 matrix=709

当前预设中存在 RGB→YUV444P16 时硬编码 `matrix_s='709'` 的逻辑。

对于 HDR/BT.2020/P3 等输入，这可能错误。

必须根据 source props 决定，或者保持 RGB 给 libplacebo 处理；不要为了进入 YUV444P16 而无条件转换成 709。

### 15.3 VS 后处理后的 HDR metadata

如果 VPY 做了：

- tone mapping；
- gamut conversion；
- transfer conversion；
- colorspace conversion；

则原始 HDR metadata 不能无条件继续沿用。

需要约定：

- 仅空间 resize/denoise/sharpen/interpolation：保留颜色 metadata；
- 改 transfer/primaries/luminance：重写 metadata；
- HDR→SDR：清除 HDR 标识；
- 用户脚本 metadata 不可信时在 info panel 标为 “script-provided / inferred”。

## 16. 字幕、OSD 与 HDR

字幕不是独立的 8-bit 白色贴图直接叠加即可。

目标：

- SDR output：现有 sRGB/BT.709 overlay；
- HDR scRGB：字幕按 SDR paper white 映射，例如用户设置 203 nits；
- HDR 高光不得把白字幕推成 1000+ nits；
- subtitle alpha blend 在正确的 linear space 进行；
- 字幕色彩仍按 UI sRGB 解释，再转换到 target working space；
- screenshot/capture 必须明确返回的是 display-referred SDR、HDR linear 还是 encoded target。

当前 overlay 已有 HDR 资源分支，应纳入 libplacebo/统一 color transform 后回归测试。

## 17. 截图与像素探针

### 17.1 截图模式

提供：

- Display appearance：与当前屏幕观感一致；
- Source-referred：尽量保存源色彩与 HDR；
- SDR export：tone-map 后标准 SDR；
- HDR export：16-bit/float TIFF、AVIF/JXL 等可保存 HDR 的格式。

不要默认把 HDR scRGB screenshot clamp 成 8-bit PNG 后让用户误以为 renderer 色彩错误。

### 17.2 Pixel probe

像素探针应能显示：

- 原始 YUV/RGB；
- 线性 RGB；
- nits；
- source gamut；
- mapped target RGB；
- final output value；
- 当前 color engine；
- tone/gamut mapping；
- DWM/Advanced Color contract；
- output bit depth。

显示器 ICC 不作为像素探针内部 stage，因为它发生在 Present 之后的 DWM/系统显示链。用于 A/B 对比 3FP Native、mpv gpu-next 与可选 Libplacebo Advanced。

## 18. UI

普通用户只需要：

```text
色彩引擎：3FP Native（默认） / Libplacebo Advanced
HDR：自动 / 映射到 SDR / 强制 HDR（开发者）
Tone mapping：自动 / 高质量
Dithering：自动 / 关闭

显示器 ICC 不提供播放器内开关，由 Windows/DWM 统一管理。
LUT 不放普通设置页；仅 Advanced/Developer 可选加载创意 .cube LUT，默认关闭。
```

开发者信息页显示：

```text
Source:
YUV420P10 · limited · BT.2020 NCL · PQ
Mastering: P3-D65 / 1000 nits
MaxCLL 812 · MaxFALL 240
HDR10+ / DV / Vivid metadata state

Pipeline:
D3D11VA → 3FP Native / Libplacebo Advanced
DV reshape: active / fallback
Tone map: native BT.2390/IPT / advanced ...
Gamut: Rec.709 / Rec.2020 / P3
Dither: ...
Target:
scRGB FP16 / RGB10A2 / BGRA8
Display peak / SDR white / black / BPC
Display color management: Windows DWM / system ICC
```

## 19. 自动 fallback 规则

### 19.1 HDR source → HDR display

优先：

```text
metadata-aware source decode
→ target-aware mapping
→ FP16 scRGB
```

默认 Native 路径失败：

```text
3FP Native HDR
→ 3FP Native HDR→SDR fallback
```

用户手动选择 Libplacebo Advanced 时：

```text
Libplacebo Advanced
→ 3FP Native
```

不得因为高级后端失败而黑屏，也不得自动把 Libplacebo 置于 Native 之前。

### 19.2 HDR source → SDR display

```text
dynamic metadata
→ tone map
→ gamut map
→ dither
→ SDR swapchain
→ DWM/system ICC
```

### 19.3 未知 colorspace

- 不崩溃；
- 标记 Unknown；
- 走 legacy heuristic；
- info panel 报告 inferred；
- 不把 heuristic 写回 source metadata。

## 20. 性能与零拷贝

色彩正确性优先，但不能把 D3D11VA 变回每帧 GPU→CPU→GPU。

要求：

- D3D11 hardware frame 优先 zero-copy / GPU-to-GPU；
- 3FP Native 保持当前 GPU-to-GPU 路径；
- Libplacebo Advanced 若启用，再要求 D3D11 resource interop；
- VS CPU frame允许 upload；
- 只有无法导入的特殊 format 才 swscale fallback；
- 记录：
  - decode time；
  - hw transfer；
  - upload；
  - color mapping GPU；
  - present；
- 不允许在默认视频链加入 monitor ICC/3DLUT；显示器校准留给 DWM。

## 21. 测试矩阵

至少新增自动/半自动测试素材：

### 21.1 SDR

- BT.601 limited/full；
- BT.709 limited/full；
- BT.2020 SDR；
- P3-D65；
- 8-bit / 10-bit / 12-bit；
- 420 / 422 / 444；
- BT.2020 CL / NCL；
- RGB source。

### 21.2 HDR

- HDR10 PQ 1000；
- HDR10 PQ 4000；
- HLG；
- HDR10+；
- Dolby Vision P5；
- Dolby Vision P7/P8；
- HDR Vivid；
- Ultra HDR JPEG；
- metadata missing/malformed；
- SDR base fallback。

### 21.3 Display target

- SDR 8-bit；
- SDR 10-bit；
- Windows HDR desktop；
- HDR display with zero/invalid luminance report；
- 窗口跨显示器；
- 系统 ICC/HDR 校准变化不应改变 3FP 后缓冲原始数值，只应影响 DWM 后的实际显示结果；
- 更换显示器后正确重探测 HDR capability / SDR white / BPC。

### 21.4 A/B reference

建立与 mpv `gpu-next` 的自动对照。

对同一帧：

```text
VS-Player 3FP Native
mpv gpu-next
VS-Player Libplacebo Advanced（若启用）
```

在相同：

- source metadata；
- target primaries / transfer；
- target peak / SDR white；
- tone-mapping 目标；
- 同一 Windows HDR / DWM 环境；

条件下优先比较**后缓冲原始数值/浮点读回**。系统 ICC 是 Present 之后的显示层变量，不作为播放器内部 A/B 开关。

允许少量 rounding，不允许系统性色偏、range 错、gamma 错和明显 banding。


