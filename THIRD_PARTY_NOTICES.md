# 第三方组件声明

本项目源码采用 MIT License。正式 Windows 包还包含以下独立第三方组件；它们分别遵循各自许可证，本项目许可证不会取代其条款。

| 组件 | 用途 | 来源 / 许可证 |
| --- | --- | --- |
| Qt 6.10.2 | GUI 与 Win32 平台插件 | [Qt](https://www.qt.io/)；LGPL-3.0/GPL-3.0/商业多重许可 |
| VapourSynth R80 | VPY 执行与逐帧处理 | [VapourSynth](https://github.com/vapoursynth/vapoursynth)；LGPL-2.1 |
| Python Embedded | VapourSynth Python 环境 | [Python](https://www.python.org/)；PSF License |
| FFF Project / 3FP | 左右视频呈现 | [FFF Project](https://github.com/Lake1059/FFF_Project)；MIT |
| FFmpeg shared libraries | 3FP 解码依赖 | [FFmpeg](https://ffmpeg.org/)；以发布包所含构建的配置为准，通常为 LGPL/GPL |
| L-SMASH Works、FFMS2 | VapourSynth 源滤镜 | 通过 VSRepo 获取；许可证及源码链接见各 VSRepo 包元数据 |
| fmtconv、RemoveGrain、AddGrain、VSZip、nlm-ispc、CAS、Zsmooth、Deblock、ZNEDI3、EEDI3、SangNom、Bwdif、VIVTC | VapourSynth 处理插件 | 通过 PyPI/VSRepo 获取；许可证及源码链接见各项目包元数据 |
| vs-placebo 2.0.4 / libplacebo | VapourSynth 中执行 Anime4K 等 mpv GLSL | [vs-placebo](https://github.com/Lypheo/vs-placebo)、[libplacebo](https://github.com/haasn/libplacebo)；LGPL-2.1+ |
| AWJimage 1.1.0 | 独立原生图片压缩转换CLI | [AWJimage](https://github.com/Dominic485649/AWJimage/releases/tag/1.1.0)；AGPL-3.0，随包保留许可证、完整上游NOTICE和固定修订对应源码；不链接进本项目 |

发布包内同时保留 Qt、Python、VapourSynth 等组件随附的许可证文件。重新分发前请核对你所替换的 FFmpeg 和 VapourSynth 插件构建配置。

## 可选 D3D11 色彩运行时

`color/` 使用 MSYS2 UCRT64 二进制包的 libplacebo 7.360.1-2（LGPL-2.1+）、LittleCMS 2.19.1（MIT）及其 shaderc / glslang / SPIRV-Cross / SPIRV-Tools / libdovi / MinGW runtime 等依赖。完整包 URL、版本和 SHA-256 在 `third_party/color/packages.json`，实际部署 DLL 闭包与原始 / 修改后哈希在 `color/runtime.json`；许可证保留于 `color/licenses/`。

源码可从 [libplacebo](https://github.com/haasn/libplacebo/tree/v7.360.1)、[LittleCMS](https://github.com/mm2/Little-CMS)、[MSYS2 包工程](https://github.com/msys2/MINGW-packages) 获取，其他包来源见各固定包的 MSYS2 元数据。为避免与 Qt / VS 的 GCC runtime 冲突，仅修改 PE 导入名和部署文件名为 `v3_` 前缀，不修改可执行代码。第三方 DLL 动态链接并保持可替换，不改变其独立许可证；重新分发者应保留修改说明与对应源码获取路径。


## Super-XBR 单阶段色度 / 放大核

Hyllian, Copyright (c) 2015. MIT License. Adapted from the pass-0 diagonal kernel in [MPDN Extensions](https://github.com/zachsaw/MPDN_Extensions/blob/master/Extensions/RenderScripts/Super-xBR/super-xbr.hlsl). The original source and full license are retained in `third_party/shaders/super-xbr-upstream.hlsl` and deployed under `shader-licenses`. This implementation is the single-stage variant, not the complete three-pass scaler.


- RIFE Vulkan：styler00dollar/VapourSynth-RIFE-ncnn-Vulkan r9_mod_v33，MIT；基于 Practical-RIFE 与 Tencent NCNN，对应 MIT/BSD 声明附在 `rife-licenses`。两套转换模型来自用户已安装的 VideoEnhancer RIFE 目录，精确哈希记录于 third_party/rife/runtime-sha256.json。以 NCNN param/bin 形式部署，不包含 PyTorch PKL 或 CUDA/TensorRT。
- MVTools v29：dubhatervapoursynth/vapoursynth-mvtools，GPL-2.0；通过 `vapoursynth-mvtools==29` Windows API 4 wheel 部署，完整许可随运行时 `MVTools-LICENSE.txt` 提供；源码见 https://github.com/dubhatervapoursynth/vapoursynth-mvtools/tree/17250aa979616ac48dfb0e18abfdcf2bd4e3afc0 。
- Descale r11：Irrational-Encoding-Wizardry/descale，MIT；来源与使用参数见 README 和依赖说明。

## 1.0.2 本机便携资源

- `shaders/*.glsl`：从本机 FFmpegFreeUI libplacebo 目录复制的 Anime4K 系列文件，保留文件内上游版权与许可。来源为用户现有资源，未修改 shader 算法。
- `LAVFilters64`：LAV Filters 0.83.0 x64，采用用户提供的 `LAVFilters-0.83-x64.zip`（SHA-256 `0126982f47157bb86a6dbb43c4f332f7f98beba9ad552c19b65f9db2e7d4f186`）。仅按独立 DLL/AX 运行时加载；上游 https://github.com/Nevcairiel/LAVFilters ，对应源码为0.83版本，GPL许可 `COPYING` 随运行时保留。
- `madVR09217`：从用户现有 PotPlayer madVR 目录复制，专有软件，保留目录内 license.txt；不是本项目开源代码。播放器通过 DirectShow 加载随附 madVR64.ax，不注册系统。
- 早期仅在本地携带；本轮完整便携包包含上述现有运行时与许可，源码仓库仍不包含这些二进制。清理后的资源 SHA-256 随包记录。

- `anime4k-a-fast.glsl`：官方 Anime4K Windows Low-end Mode A (Fast) 的六段组合，算法未修改，保留上游版权 / MIT 等原文件许可；commit `7684e9586f8dcc738af08a1cdceb024cc184f426`。文件顺序和来源见 `docs/anime4k-fast-source.md`。
- `anime4k-no-cnn.glsl`：同一 Anime4K commit 的 Clamp_Highlights + Thin_HQ + Darken_HQ 组合，保留原 MIT 许可与系数；传统梯度 / DoG 线条处理，不含神经网络。来源见 `docs/player-six-stage-1.0.3.md`。

## mpv 用户着色器库

`assets/mpv-shaders` 收录维护者指定的 mpv User Shaders 与 hooke007 列表中的 GLSL / hook 文件，按上游来源保留目录、版权头和许可证。`manifest.json` 固定每个源文件的 URL、修订、字节数和 SHA-256；相同内容只携带一份，其他来源列入 aliases。这些着色器不是本项目 MIT 许可的原创代码，遵循各文件/上游目录的许可。使用前请查看对应许可；上游未给单独许可的 gist 不另行宣称 MIT。来源与执行兼容说明见 `docs/mpv-shaders-interpolation-1.0.3.md`。

## 便携更新工具

- aria2-next 2.8.3：独立下载进程，GPL-2.0-or-later；[上游版本与对应源码](https://github.com/AnInsomniacy/aria2-next/tree/v2.8.3)，完整许可随包 `runtime/tools/aria2-COPYING.txt`。Windows x86_64 官方资产 SHA-256：`08afaf2a44811d38e7ce538da719ab06d6925bcaad1231ee7b92c497f58e5aac`。
- 7-Zip Zstandard 25.01 ZS v1.5.7 R4：本机 `C:/PortableSoft/7-Zip-Zstandard` 的 7z.exe / 7z.dll，用于 Zstandard Ultra 归档与更新解压；[对应源码](https://github.com/mcmilk/7-Zip-zstd/tree/v25.01-v1.5.7-R4)，完整许可随包 `runtime/tools/License.txt`，包含 LGPL / BSD 与 unRAR 限制。

## 1.0.4 本地增强版 FFmpeg

维护者提供的 `ffmpeg-autokey444-full.7z`（2026-10-03，SHA-256 `b56bcecdb5de1e768f14b2e18564dd052446b73e36af8903b043561c86684811`）包含FFmpeg及新增编解码依赖。实际构建启用 `--enable-gpl --enable-version3 --enable-nonfree`，不能将这一构建标作纯LGPL或自由可再分发版本。保留上游README、源码提交清单、构建脚本、补丁和配置于 `runtime/ffmpeg/build-info`；这些记录不等于完整对应源码。此轮仅更新本地运行时，未制作或发布该二进制包。

## 静态图片解码组件

- libdeflate 1.25：MIT，https://github.com/ebiggers/libdeflate 。RGBA8 PNG 快速路径静态链接，版本及 SHA-256 固定于 `tools/image-runtime.cmake`；许可全文随 `IMAGE-LICENSE.txt` 分发。不增加用户需安装的 DLL。

- 图层 PSD/PSB 模块使用 zlib 1.3.2，Jean-loup Gailly / Mark Adler，zlib 许可；未修改的最小压缩/解压源文件位于 `third_party/image-zlib`，来源为 libjpeg-turbo 3.2.0 的 spng 内含副本。静态链接，无新增运行时 DLL；完整许可随 `docs/image-zlib-LICENSE.txt` 分发。PSD 解析、工具与编辑核心为本项目 MIT 自有实现，未复制 GIMP GPL 或闭源 Photopea 代码。

- libavif 1.4.2：BSD-2-Clause，https://github.com/AOMediaCodec/libavif 。通过官方源码与 dav1d 静态链接；构建时仅扩大显式图像像素上限，保留网格、格式、尺寸算术与码流检查。依赖版本和 SHA-256 固定于 `tools/image-runtime.cmake`。
- dav1d 1.5.3：BSD-2-Clause，https://code.videolan.org/videolan/dav1d 。Windows MinGW 静态库来自 MSYS2；不增加用户需安装的解码器。
- libyuv 1924：BSD-3-Clause，Chromium libyuv commit `644251f252a84bf8ce91ff0aca86a9b16b069ab8`（libavif 1.4.2推荐修订），使用当前MinGW静态编译SIMD色彩转换。源码镜像及SHA-256固定于 `tools/image-runtime.cmake`；完整许可在 `IMAGE-LICENSE.txt`。
- 完整许可证见随包 `IMAGE-LICENSE.txt`（源文件 `assets/image-runtime-LICENSE.txt`）。libavif 的内置 libyuv 通用转换代码许可也包含于其中。

- 图片 JPEG 直解路径静态链接 libjpeg-turbo 3.2.0（官方源码 SHA256 `980dd81f425082aa6d7c9e47fef27554ce7a9ffc8e2f6e863b97d263c5c50858`，使用当前 MinGW 和 NASM 2.16.03 编译）；BSD/IJG/zlib 许可全文随 `image-runtime-LICENSE.txt` 分发。使用精确 IDCT 和正常色度上采样，不启用 FASTDCT/FASTUPSAMPLE。
