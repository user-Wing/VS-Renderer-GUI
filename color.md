# VS-Player / 3FP 色彩管理与 HDR 实现升级规格

> 本文是下一轮 AI 的实现任务书。目标不是重写播放器，也不是为了“支持更多格式”堆开关，而是在现有 3FP + VapourSynth + D3D11 架构上，把视频色彩管理推进到接近 mpv `gpu-next/libplacebo` 的完整程度，并把可合法使用的 HDR 标准与开源实现接入到授权边界。
>
> 原则：优先复用成熟开源实现，避免继续手写重复的 tone mapping / gamut mapping / ICC / dithering 算法；保持现有 3FP D3D11、VS 外部帧桥、低延迟播放、字幕和缩放体系。所有新增能力必须有明确 fallback，并且不得把“能识别元数据”宣传成“完整支持某商业 HDR 标准”。

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

### 1.2 与 mpv gpu-next/libplacebo 的主要差距

当前差距不在“能不能输出 10-bit”，而在完整 source→display 色彩管理。

现有 3FP 核心模型仍大致是：

```text
709 / 2020
+
SDR / PQ / HLG
+
自有 HDR tone mapping
+
8/10-bit SDR 或 FP16 scRGB HDR 输出
```

mpv/libplacebo 的完整模型则更接近：

```text
源像素表示
→ range / matrix / chroma location
→ primaries / transfer / white point
→ Dolby/HDR10+/HDR metadata reshape
→ linear-light working space
→ target display primaries / transfer / luminance / black point
→ tone mapping
→ gamut mapping
→ ICC / 3DLUT
→ scaling / overlay
→ target bit depth
→ dithering
→ swapchain
```

当前需要重点补齐：

- 视频 ICC / 显示器 profile 管理；
- 完整 target display model，而不是只读取峰值后把主要 HDR tone mapping 交给 Windows/显示器；
- 更完整的 primaries / transfer / matrix 支持；
- HDR10+ ST 2094-40 动态 metadata 的完整利用；
- Dolby Vision RPU reshape，而不是只识别和 fallback；
- 更成熟的 gamut mapping；
- 8-bit/10-bit 末端量化前 dithering；
- BT.2020 constant-luminance 的正确路径；
- 统一 HDR/SDR/广色域图片与视频的颜色语义，避免播放器视频和图片编辑器各自一套逻辑。

## 2. 总体实现策略

### 2.1 不再继续手写一套“迷你 libplacebo”

下一轮修改的首选方案是把 **libplacebo 作为 3FP 的主色彩处理引擎**，优先使用其 Direct3D 11 backend，而不是把当前播放器改成 Vulkan。

截至本文编写时，libplacebo 官方已提供：

- Vulkan；
- OpenGL；
- Direct3D 11；
- FFmpeg AVFrame interop；
- high-level renderer；
- ICC；
- 3DLUT；
- HDR peak detection；
- 多种 tone mapping；
- perceptual gamut mapping；
- dithering；
- Dolby Vision reshaping；
- deband、scaling、shader hooks 等。

因此建议结构改为：

```text
FFmpeg / D3D11VA / VapourSynth frame
                ↓
       统一 FrameColorMetadata
                ↓
        libplacebo D3D11 renderer
      ├─ range / matrix / chroma
      ├─ primaries / transfer
      ├─ HDR metadata
      ├─ DV reshape
      ├─ HDR10+ metadata
      ├─ tone mapping
      ├─ gamut mapping
      ├─ ICC / 3DLUT
      ├─ dithering
      └─ scaling（可按需启用）
                ↓
  3FP 已有 D3D11 swapchain / overlay / pacing
                ↓
      Windows DWM / Advanced Color
```

允许两种集成层级：

**方案 A，优先：** libplacebo 接管视频主画面颜色处理，但 3FP 保留 swapchain、字幕、VRR/pacing、窗口、播放器状态和 overlay 合成。

**方案 B，若 A 的 D3D11 resource interop 成本或接口限制明显：** libplacebo 接管完整 video render target，再交给现有 3FP compositor 合成字幕。不要为了保留当前手写 shader 而复制 libplacebo 的 ICC/tone/gamut/dither 代码。

### 2.2 保留现有 renderer 作为 fallback

不得一次删除当前 `PixelShaderSource`、BT.2390/IPT 与 D3D11 VideoProcessor 路径。

增加运行时后端概念：

```text
Color engine:
- Auto
- libplacebo
- 3FP legacy
```

默认 Auto：

1. libplacebo D3D11 初始化成功 → 使用 libplacebo；
2. libplacebo 初始化失败或当前像素资源无法导入 → 使用 3FP legacy；
3. fallback 必须写入 snapshot / info panel，不能静默。

这样可以保证当前播放能力不因大规模色彩升级倒退。

## 3. 依赖引入策略

### 3.1 libplacebo：核心依赖

上游：

- https://github.com/haasn/libplacebo
- 官方主仓库镜像指向 VideoLAN Codeberg/GitLab；拉取时注意 recursive submodules。

用途：

- D3D11 GPU abstraction；
- FFmpeg frame interop；
- color space decode/encode；
- ICC；
- gamut mapping；
- HDR tone mapping；
- HDR10+ metadata；
- Dolby Vision；
- dithering；
- 3DLUT；
- peak detection；
- 未来可统一 Jinc/anti-ringing/deband/custom shader。

授权：

- LGPL-2.1-or-later。
- 如果项目继续以 MIT/其他宽松协议发布，优先动态链接 libplacebo DLL，并在 `THIRD_PARTY_NOTICES.md`、发行包中完整保留 LGPL notice、许可证与源码获取方式。
- 不要静态链接后忽略 LGPL 义务。
- 若未来上游提供特别许可，再单独评估；当前按 LGPL 处理。

构建要求：

- Windows D3D11 backend 必须开启；
- 若启用 ICC，应构建 lcms2 支持；
- 若启用 Dolby Vision，应构建 libdovi 支持；
- 尽量使用 shared DLL，便于许可证隔离和替换。

### 3.2 LittleCMS 2：ICC

上游：

- https://github.com/mm2/Little-CMS

用途：

- ICC v2/v4 profile；
- 显示器 ICC；
- source ICC；
- profile transform / 3DLUT generation。

授权：

- MIT。

优先让 libplacebo 通过 lcms2 使用 ICC，不要在 3FP 再造一套与 libplacebo 并行的 CMS。图片编辑器现有 Qt QColorSpace/ICC 可暂时保留，但长期应建立统一的颜色元数据表示。

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

- 优先让 libplacebo 根据 FFmpeg colorspace 处理；
- legacy path 也必须拆分 CL / NCL；
- 增加测试图，确认 constant-luminance 不再走普通 NCL 逆矩阵。

## 6. 显示器色彩管理

### 6.1 自动目标显示器识别

窗口跨显示器时，重新解析：

- 当前 HMONITOR；
- BitsPerColor；
- DXGI colorspace；
- HDR/Advanced Color 状态；
- min/max/full-frame luminance；
- Windows HDR calibration 信息；
- 当前显示器 ICC profile。

必须支持从一个显示器拖到另一个显示器后动态重建 color target。

### 6.2 ICC

目标：

```text
Source color space
→ linear working space
→ target display ICC / 3DLUT
→ target framebuffer
```

规则：

- Auto：使用当前显示器系统 ICC；
- Off：禁用 ICC，仅使用标准 target primaries；
- Custom：指定 ICC 文件；
- 允许 cache 生成后的 3DLUT，避免每帧 CPU CMS。

不能把图片编辑器中的 Qt QColorSpace 当成视频 ICC 已完成；视频要进入同一 target profile 体系。

### 6.3 HDR 下的 ICC

不要机械地把 SDR ICC LUT 直接套到 HDR scRGB 上。

需要依据 libplacebo 的 target color model 和 Windows Advanced Color 路径处理；HDR 下 profile/target primaries 的适用方式必须通过真实 HDR 显示器验证。

## 7. Tone mapping 与 gamut mapping

### 7.1 默认算法

优先直接使用 libplacebo 默认/高质量 preset。

建议用户层只暴露少数模式：

```text
Tone mapping:
- Auto
- Balanced
- High quality
- BT.2390 compatibility
```

不要把 libplacebo 内部十几个参数全部暴露到普通设置页。

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

优先让 libplacebo 处理扩展 HLG gamma，不继续维护一套简化常量公式作为默认。

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
- ICC；
- output bit depth。

用于 A/B 对比 libplacebo 与 legacy。

## 18. UI

普通用户只需要：

```text
色彩管理：自动 / 关闭
HDR：自动 / 映射到 SDR / 强制 HDR（开发者）
Tone mapping：自动 / 高质量
显示器 ICC：自动 / 关闭 / 自定义
Dithering：自动 / 关闭
```

开发者信息页显示：

```text
Source:
YUV420P10 · limited · BT.2020 NCL · PQ
Mastering: P3-D65 / 1000 nits
MaxCLL 812 · MaxFALL 240
HDR10+ / DV / Vivid metadata state

Pipeline:
D3D11VA → libplacebo D3D11
DV reshape: active / fallback
Tone map: spline / ST2094-40 / ...
Gamut map: perceptual
ICC: <monitor profile>
Dither: ...
Target:
scRGB FP16 / RGB10A2 / BGRA8
Display peak / black / BPC
```

## 19. 自动 fallback 规则

### 19.1 HDR source → HDR display

优先：

```text
metadata-aware source decode
→ target-aware mapping
→ FP16 scRGB
```

失败：

```text
libplacebo HDR
→ 3FP legacy HDR
→ 3FP HDR→SDR
```

不得黑屏。

### 19.2 HDR source → SDR display

```text
dynamic metadata
→ tone map
→ gamut map
→ ICC
→ dither
→ SDR
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
- libplacebo D3D11 resource interop；
- VS CPU frame允许 upload；
- 只有无法导入的特殊 format 才 swscale fallback；
- 记录：
  - decode time；
  - hw transfer；
  - upload；
  - color mapping GPU；
  - present；
- 不允许引入“为了 ICC 每帧重新生成 3DLUT”之类明显错误实现。

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
- monitor A/B ICC；
- 窗口跨显示器。

### 21.4 A/B reference

建立与 mpv `gpu-next` 的自动对照。

对同一帧：

```text
VS-Player libplacebo
mpv gpu-next
VS-Player legacy
```

在相同：

- target primaries；
- target transfer；
- target peak；
- tone mapper；
- ICC off/on；

条件下截图/浮点读回。

允许少量 rounding，不允许系统性色偏、range 错、gamma 错和明显 banding。


