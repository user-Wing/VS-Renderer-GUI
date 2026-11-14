# 本轮 3FP 色彩相关更新审查（2026-11-14，静态审查，未提交 PR）

后续处置见 [两轮审查修复](color-review-fix-20261114.md)：H1/C1及已复现问题已处理；本报告下方关于CL系数、SMPTE240M公式的建议经标准与实际运行复核为误报，保留原文供追溯，不再作为待修复结论。

## 结论与范围

用户要求：审查"前一轮完整执行的后端 3fp 色彩相关代码更新"，与上游 3FP 仓库对比，排查可能存在的问题，**先不提交 PR**。

- 对比基线：上游 `db94021` 与固定基线 `248cb9bb0cef91922ab8f88ea7f6ef38ec0e311c` 在
  `FFF.Native/3FP/{Hdr,Render,Api}` 上 diff 为空 → 当前工作树 `.deps/fff-color-fix-20261114`
  相对上游的所有色彩改动 **全部是本地自定义代码**，可直接作为审查对象。
- 审查方式：静态阅读工作树代码 + 与上游 diff 对比 + 3 个并行深度子代理对应三块
  （shader 色彩数学 / 外部帧 ABI / VS 源与像素摘要），Lead 对宿主侧 `src/backend/*` 全路径直读，
  并对子代理的高风险结论逐条独立核验。本轮未重新构建/运行任何产物。

## 上游对比结论

| 项 | 结论 |
| --- | --- |
| 上游 HEAD vs 基线，Native 色彩文件 | 无差异，全部为本地产物 |
| 需审查的本地改动 | VideoRenderer.cpp/h（shader+上传）、HdrProcessor.cpp、ThreeFpApi、ThreeFpPlayer、VapourSynthFrameServer、FrameColorMetadata、NativeVsSource(新) |

## 新增问题（本轮审查发现）

### H1 — ABI v1 降级守卫遗漏 PTS/SAR/fieldBased/streamHdr10Plus（需修复）
- 位置：`src/backend/ThreeFpApi.cpp:156-158`。
- 宿主在探测到 Native 版本 < 2 时降级为 v1。守卫只查 `flags`/`sideDataBytes` + 4 种浮点格式，
  **未检查** `presentationTime100ns`、`sarNumerator/sarDenominator`、`fieldBased`、`streamHdr10Plus`。
- 而这些字段在 `VapourSynthFrameServer.cpp:462-468` 会被**无条件**写入（SAR、_AbsoluteTime→PTS），
  且此时 `flags==0`、`sideDataBytes==0` → 帧落入降级分支。
- 降级写 `legacy.size=offsetof(=112), version=1`（ThreeFpApi.cpp:160-161）→ Native v1 分支
  （PlayerSession.cpp:10156-10158）硬编码 `pts=frameIndex,duration=1`，完全不读 colorMetadata 尾巴
  → PTS/SAR/fieldBased/streamHdr10Plus 被 **静默丢弃**，仍返回 Success，宿主无法感知。
- 后果：帧桥播放位置错位（PTS 丢）、非方形像素源宽高比错误（SAR 丢）。
- 建议（最小改动）：降级守卫改为——任一非默认字段即回 `NotSupported`，
  而不是静默降级丢字段（具体条件见 detail；浮点已有同样处理）。

### C1 — Native 侧 external-frame 结构缺 static_assert 加固（低，待加固）
- 宿主 `ThreeFpApi.h` 已锁 `offsetof(colorMetadata)==112`/`sizeof==240`；Native 侧
  `FFF.Player.Api.h` 对 `FFF3FPExternalVideoFrame` 无同款锁。若该结构未来插入字段，Native 静默漂移。
  建议 Native 也加一条 `static_assert(sizeof==240)`+`offsetof==112`。

## 上轮审查 P2 遗留核对（cross-check `development/color-review-20261114.md`）

| 上轮 P2 | 本轮状态 |
| --- | --- |
| VPY 预加载破坏合法脚本 | **仍在**：`VapourSynthFrameServer.cpp:305` 仍无条件 `script.prepend(LoadPlugin)`，未处理 `from __future__` 行首语法 |
| 未知矩阵拒绝遗漏 packed CPU 路径 | **仍在**：`VideoRenderer.cpp:7975` packedYuv 仍只排除 CL/YCgCo，其余未知矩阵经 swscale 兜底 |
| 有限范围 Gray 强制转换误标全范围 | **仍存在风险**：`PlayerSession.cpp` inputFull/outputFull 对 gray 的 full 假设未改 |

（上轮 3 个 P2 在本轮代码中均未被修复，需按用户要求纳入"排查问题"清单。）

## 子代理发现与 Lead 裁决

- **GBR 通道错乱（子代理最高风险）→ 经 Lead 核验为【误报】**：子代理认为 NativeVsSource 直接搬 FFmpeg
  buffer 造成 VS plane 顺序错乱。实际 FFmpeg `comp[].plane` 字段将 GBR 物理序正确映射为 VS R,G,B；
  ThreeFpPlayer `order={1,2,0}` 再反向一次，F32 路径的硬编码平面映射亦自洽。整链两次交换互逆，通道正确。
  详见报告正文。
- 摘要/CICP 完整性、side data 越界、存活性、浮点格式全链路：均核实【正确】（ABI/VS源子代理详细结论）。

## 详细结论

### 摘要/CICP/剥离 语义（VS 源子代理 7 点 + Lead 复核）
- 正确（1a）：摘要 FNV-1a 自洽，Native 与宿主逐 plane/行/字节一致，GBR 不影响摘要匹配。
- 正确（1c/1d）：CICP 与 _Matrix 同源、只改像素只剥动态、改编码同时剥静态、格式改变(bps 变) fail-closed。
- 正确（2）：`_VsrColorMetadataValid` 只有宿主一处读取、无插件写、仅 vpy 滤镜可置 1；缺省走摘要。
  置 1 是唯一逃逸通道，需在文档明确置位义务。
- 正确（3，中风险）：`streamHdr10Plus` 剥离同时清 sideData/signature → 强制重 ConfigureStream 无残留；
  但 HdrProcessor held 帧机制下若宿主剥离/不剥离逐帧交替，流声明反复翻转 → tone map 抖动，建议剥离粘性。
- 正确（4）：`_AbsoluteTime` 负值被拦截保持 -1，PV 1060 上限 + double 舍入误差可忽略；无 PTS 的 VFR 帧轻微错位可接受。
- 正确（5）：宿主每帧新建局部 frame、Native 每帧新建 owned，无跨帧 side data 残留；唯一跨帧状态是 HdrProcessor frameState_。
- 中低（6）：仅 HDR 载荷且 valid!=1 才逐字节 FNV-1a（可 8 字节步进优化），非瓶颈。
- 正确（7）：SourcePixelProbe 与 ReadSourcePixel 逐字段对齐；YUV444 回 family=1 正确。

### shader 色彩数学（子代理系数级验证 + Lead 复核）
子代理用 numpy 反推验证矩阵/常数，结论按风险汇总：

#### 高风险
1. **CL 解码非标准**（`DecodeYuv` Kr==-1.0，行 736-741）：系数 `1.7184/0.9936`(R)、`1.9404/1.5816`(B)
   非 BT.2020 CL 标准的 `1/1.4746`、`1/1.8814`，且缺"先钳位 R/B 再恒定亮度重构 G"。对饱和红/蓝误差
   ~0.20-0.30（对照标准逆变换）。轴序（c.y=Cr 调红、c.x=Cb 调蓝）与一般矩阵分支一致，未红蓝互换——
   问题在系数/重构本身。建议改标准分段 CL 逆。CL 恒等链已被 `PlayerSession` 禁（强制转换拒绝）。
2. **SDR 输出双编码不一致**（`ToSrgb` 于行 1282/1351/1354）：8-bit `B8G8R8A8_UNORM` 不用
   `_SRGB`、不调 SetColorSpace1，DWM 按 buffer 数值直接透传 → **sRGB 与 BT.709 编码差异直接呈现为亮度差**。
   纯 709 源走 `return rgb` 直通（行 1275，BT.709 编码），转换源走 `ToSrgb`（sRGB 编码），同链混合，
   sRGB 更亮（中高光最大约 +5%@通道）。建议统一编码或换 `_SRGB` swapchain 由硬件处理 gamma。

#### 中风险
3. `Transfer==8` 公式与 `AVCOL_TRC_SMPTE240M` 语义错位（行 728）：`v/4.0 + pow((v+0.1115)/1.1115,1/0.45)`
   更似 BT.709/2.2 混合近似，非 240M 的 gamma≈2.2 解码；`v/4.0`（非 4.5）亦非标。建议修正公式或映射。
4. RGBAF32 源未像 RGBAF16 特判 transfer==3：`RGBAF32LE` untagged 且线性时，会误走 `DecodeSdr`
   （对编码值域 [0,1] 假设的 pow），对可能 >1 的线性 float 双重解码。建议核对上传端 transfer 约定。
5. GAMMA22/28 纯幂仅对"纯伽马源"正确；对实质 BT.709 编码源阴影区（v=0.05）误差 ~9x。建议按源实际曲线。

#### 低风险
6. 两套 BT.709 常数（LinearOne/BtOne 用 0.081/0.099 vs ClLinear/ClEncode 用 0.081242858/0.099296826 精确）
   并存，0.081↔0.0812428 区间 ~0.03% 差异；两套各自自洽。建议统一。

#### Lead 复核确认（正确项）
- Transfer==5/6 纯 `pow(v,2.2/2.8)` 与 GAMMA22/28 常见定义（EOTF=L'^2.2/2.8）一致，风险低。
- YCgCo 逆 `(y+cy-cx, y+cx, y-cy-cx)` 用 c.x=Cg、c.y=Co，误差 0，正确。
- 一般 RGB 矩阵（`R=Y+(2-2Kr)Cr`…）+ YuvCoefficients（2020 0.2627/0.0593、709 0.2126/0.0722、601 0.299/0.114、
  240M 0.212/0.087、FCC 0.30/0.11）数值标准正确。
- 色域矩阵全部经验证正确：`To709∘To2020=I`（<5e-7）；P3To709（Display P3 D65）逐项==numpy 反推；
  DciTo709（Bradford DCI→D65）精确匹配；SMPTE C→709 与 PAL→709（PAL G 绿点 0.29≈709 的 0.30，G 行≈[0,1,0]，可接受近似）。
- `ResolveSourceGamut`（SMPTE170M/240M→SMPTE C=4、BT470BG→PAL=5、SMPTE431→DCI-P3=3、SMPTE432→Display P3=2、BT2020→1）映射自洽；BT470M 未处理（罕见）。
- HDR→SDR（行 1284-1354）：nits 来源分支合理；`Gamut!=1 时 To2020(SourceTo709(nits))` 的"always through 709"
  链路数学单调正确（Gamut==1 跳过时恒等于原始 2020）。风险低-中。

## 边界

- 本轮为只读静态审查，已全部记录在本文件及 `development/color-fix-20261114.md`；测试计数沿用前轮的
  `evidence/color-fix-20261114/summary.json`（45/11/21/9/7 通过），非本会话重跑。未构建、未部署、未提交、未 PR。
