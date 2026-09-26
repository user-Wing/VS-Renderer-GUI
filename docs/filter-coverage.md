# 滤镜覆盖矩阵

“支持”指 catalog 有明确参数、能生成可审阅 VPY；插件实际缺失时运行前必须报出 namespace。顺序由处理链决定。表达式/矩阵计算节点暂不实现。

| 类别 | 首批节点 | VS 映射 | 依赖 | 状态 |
| --- | --- | --- | --- | --- |
| 输入 | L-SMASH、FFMS2 | `lsmas.LWLibavSource`、`ffms2.Source` | 第三方 | 已建模 |
| 基础操作 | Trim、Crop、AddBorders、Transpose、AssumeFPS | `std.*` | Core | 已建模 |
| 精度/格式 | Depth、Resize | `fmtc.bitdepth` / `resize.*` | fmtc 或 Core | 已建模 |
| GPU 超分/着色器 | Anime4K GLSL | `placebo.Shader` | vs-placebo / libplacebo | 已建模；文件路径与 1×–4×等比输出可调 |
| 降噪 | RemoveGrain、Bilateral、NLMeans、Zsmooth TTempSmooth | `rgvs`、`vszip`、`nlm_ispc`、`zsmooth` | 第三方 | 已建模 |
| 去色带 | neo_f3kdb Deband、多次串联 | `neo_f3kdb.Deband` | 第三方 | 已建模 |
| 去块 | Deblock | `deblock.Deblock` | 第三方 | 已建模 |
| 锐化 | CAS、UnsharpMask | `cas.CAS`、`std.Convolution` 方案 | Core/第三方 | 首批仅 CAS |
| 加噪 | Grain Add | `grain.Add` | Core | 已建模 |
| 抗锯齿/插值 | ZNEDI3、EEDI3、SangNom | `znedi3.nnedi3`、`eedi3m.EEDI3`、`sangnom.SangNom` | 第三方 | 已建模为底层单节点；蒙版组合待实现 |
| 反交错/IVTC | Bwdif、VIVTC、QTGMC | `bwdif.Bwdif`、`vivtc.VFM` + `VDecimate`、`havsfunc.QTGMC` | 第三方 Python/插件 | Bwdif、VIVTC 已建模；QTGMC 待组合节点 |
| 字幕 | xy-VSFilter、assrender | `xyvsf.TextSub`、`assrender.TextSub` | 第三方 | 待实现 |
| 显示缩放 | Nearest、Bilinear、Bicubic、Lanczos 3、Jinc 2 | 3FP shader | Core/内核 | 已接通；默认 Nearest 放大、Lanczos 3 缩小，达到源像素密度后才转入放大重建 |
| 高级缩放 | Anime4K、NNEDI3、NGU | VS GLSL / VS 插值 / 独立 GPU 推理 | 第三方/新内核 | Anime4K 已实现；NGU 未实现，不伪映射 |

VCB 教程覆盖以公开章节 6、8、9、10 为入口。扩展节点优先选择单输入 clip 加标量/枚举参数的 API 4 原生插件。第 9 章中 mask/merge/diff、TAAmbk、QTGMC 和 BM3D 色彩空间/两阶段处理属于高阶组合节点；在基础节点稳定后按教程实例加入，而不是把任意 `Expr` 暴露成字符串框。

## 扩展插件来源

| 插件 | 上游 | 选择理由 |
| --- | --- | --- |
| Zsmooth | [zsmooth](https://github.com/adworacz/zsmooth) | 现代多架构时域平滑实现，替代已归档的 FluxSmooth/TTempSmooth 插件 |
| Deblock | [VapourSynth-Deblock](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-Deblock) | 教程明确列出的常用强力去块工具 |
| ZNEDI3 | [znedi3](https://github.com/sekrit-twc/znedi3) | 教程推荐的常用神经网络插值器 |
| EEDI3 | [VapourSynth-EEDI3](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-EEDI3) | 教程推荐的更强边缘导向插值器 |
| SangNom | [vapoursynth-sangnom](https://github.com/dubhatervapoursynth/vapoursynth-sangnom) | 教程列出的强力抗锯齿底层滤镜，参数少且风险可清楚提示 |
| Bwdif | [VapourSynth-Bwdif](https://github.com/HomeOfVapourSynthEvolution/VapourSynth-Bwdif) | 单节点、场序可枚举的运动自适应反交错 |
| VIVTC | [vivtc](https://github.com/vapoursynth/vivtc) | VFM 与 VDecimate 可封装成固定两步 IVTC 节点 |
| vs-placebo | [vs-placebo](https://github.com/Lypheo/vs-placebo) | 官方 `Shader` 接口直接执行 mpv/libplacebo GLSL，可加载 Anime4K；输入 16-bit YUV、输出 YUV444P16 |

DFTTest、Neo FFT3D 与 HQDn3D 的 VSRepo Windows 包仍使用已被 R80 拒绝的 API 3，因此没有列入可用 catalog；等上游提供 API 4 Windows 构建后再评估。
