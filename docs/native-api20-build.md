# API 20 原生内核构建

当前 API20 固定上游 main `146b5e03c784a01215bcc21b5dee60c97dabb089`，再使用启动优化 [PR #48](https://github.com/Lake1059/FFF_Project/pull/48) 的 `cc545d2afc3f7b45058f47434628db6a703c7b07` 作为构建基线。包含此前 #24～#35 的内核演进、后续RTX/设备与缓冲策略、硬解试解探测和窗口重绑修复，以及PR48的无窗转换省略；Qt启动预读单独在 `src/player/main.cpp` 实现，适用边界见 [性能排查](performance-debugging.md)。

在 `cc545d2` 的独立 checkout 应用 `patches/3fp-api20-vsrenderer-1.0.10.patch`；文件名沿用历史，内容已重建，不能再用于旧 `922e4b7` 或 `ce195eb` 基线。保留VSP的VS帧桥、色彩、字幕、相对逐帧及多轨硬解修复；试解按视频包计数，缺少失败证据不排除硬解。构建脚本输出 `FFF.Native.dll` 到指定目录，现行本地部署从 `build/mingw-release/FFF.Native.dll` 取库。

保留本项目的外部 VS 帧输入、clock-only、输出格式、像素探测、缩放、音频 EQ/tempo、软件解码队列和可选 libplacebo 色彩桥。snapshot、字幕及音频电平结构均按 API20 版本号传递，构建时必须保持 MinGW/MSVC 的配置、snapshot、字幕命令及图层布局一致。上游20移除了旧私有 DV/RPU 测试扩展；本补丁沿用上游公开色彩和元数据路径，不恢复该私有接口。

配置仍为 112 字节，snapshot 增至 464 字节；Qt 镜像同步新增上传耗时、字节数、次数和路径，并固定关键偏移。Jinc 权重使用纹理槽 t8，避免与上游辅助视频层 t5～t7 重叠；修改内嵌 HLSL 后必须运行上游 `tools/generate_shader_bytecode.py` 及 `--sdr-specializations` 重新生成两个字节码头。

AI字幕沿用1.0.14迁入的 #36/#37 Native 能力，来源为 #37 head `edf533155727f681eefa771082702bab275bca48`（含 #36），与VSP扩展一同在新基线上重建，不直接混用功能分支DLL。音频tap在VSP EQ/atempo前采集，避免重复采样；Qt只将有界PCM交给外置worker。模型和llama-server不下载、不打包，上游VB页面不迁入。音频块144字节、tap配置16字节、RTX状态44字节，需与Native保持ABI一致。

## 多路比对扩展

`build-native-api20.ps1` 在完整 VSP 补丁后应用 `patches/3fp-api20-analysis-probe.patch`，并复制 `src/backend/SourcePixelProbe.h`。此扩展让交互重绘绕开静态图片的媒体帧率限制，并新增本地可选导出 `VSR3FP_ReadSourcePixel`；配置、snapshot 和 API20 版本不变。

源像素保留在色彩/位深转换前，YUV 直接按 FFmpeg 像素格式描述读取原码值；硬件帧在第一次取样时下载并缓存。坐标通过原生呈现矩形映射，保留旋转、留黑、缩放和 DPI 关系。缺少此扩展的旧 DLL 不提供原始 YUV，不能用 RGB 反算代替。

## Lite Anime4K Fast

Lite和本地单文件的“图像处理 → 缩放滤镜”提供与Full同名的八档：CNN＋额外增强、CNN、no CNN＋额外增强、no CNN、Jinc、D3D11、1X·Jinc、1X·D3D11。设置的渲染页可指定默认档位，`player.ini` 的 `player/animeStage` 保存0～7；旧Fast布尔选择迁移为CNN档。底栏和Tab显示实际模式。源高度≥2160p保留原有内置增强禁用规则。

1.0.16 的 Full/Lite 使用同一内核。标准构建依次应用 VSP、比对取样与 Anime4K 补丁；在固定基线的独立 checkout 上执行：

```powershell
& tools/build-native-api20.ps1 `
  -NativeRoot <独立的固定提交checkout> `
  -DependenciesRoot <包含ffmpeg和vcpkg_installed的目录> `
  -LibiamfRoot build/iamf-api20-stage `
  -OutputDirectory build/mingw-release
cmake --build build/mingw-release --target VSRenderer VSPlayer VSPlayerLite VSCap
```

脚本追加 `patches/3fp-api20-anime4k-fast.patch`，用 `.deps/color/ucrt64/bin` 的glslc、spirv-cross及Windows SDK 10.0.26100.0的fxc转换 `anime4k-a-fast.glsl`、`anime4k-no-cnn.glsl` 和 `gpu-sharpen.py` 生成的两个原始LUMA shader，共45个pass，将字节码嵌入DLL。播放不需要构建工具、Python、VapourSynth或外部shader文件。分发保留Qt/Native各依赖的原许可，以及两个原shader的MIT/Unlicense声明。

执行链为原生色彩转换（源尺寸）→ 可选亮度提取、两段额外增强及合回RGB → MAIN → PREKERNEL → 原生RGB缩放。亮度提取使用源矩阵与量程，增强仍为原有0.3强度/2阈值的sharpen_edges、enhance_detail；浮点16位中间纹理保留CNN负数。常规Anime档按视口调度WIDTH/HEIGHT/WHEN；1X档将OUTPUT固定为源尺寸，再交给现有Jinc或D3D11 shader缩放路径。算法号为CNN 9、CNN增强10、no CNN增强11、no CNN 12、1X·Jinc 13、1X·D3D11 14。它仅运行已转换的内置shader。

像素回归：用MSVC将 `tests/Anime4KFastProbe.cpp` 编译并链接d3d11/d3dcompiler，包含 `src` 和 `build/anime4k-native/shaders`，输出 `build/anime4k-native/probe.exe`；安装测试用moderngl/numpy后运行 `python tools/check-anime4k-fast.py`。六变体×八比例，共48组，独立编译原GLSL对照D3D11 shader输出；探针最后统一双线性适配，不声称整个Full/libplacebo色彩/缩放链逐像素一致。`vsr_player_lite_tests nativeAnimePresets` 验证八档菜单、实际像素、额外增强差异、两种1X最终缩放差异、播放/定位和重开持久化，可用 `VSR_ANIME4K_SOURCE` 指定片段。

单文件先用 `stage-editions-1.0.9.py --edition Lite --player <本轮Lite EXE>` 整理 `build/playerlite-runtime`，再运行 `tools/package-playerlite-single.ps1 -SourceDirectory build/playerlite-runtime`。打包以暂存内的EXE为准，不再强制取另一构建目录的旧EXE；自动排除用户INI和FFmpeg专用文件。原有60.9 MiB解压体积已经超过旧脚本50 MiB门槛，现改为如实报告体积并验证单文件压缩结果。

Anime4K 的提前返回分支也必须先清空目标背景，否则缩小视频后会保留旧交换链画面。`vsr_player_lite_tests zoomOutClearsOutsideVideo` 对八档逐次缩小、放大并读取实际交换链像素，验证视频内颜色与视频外黑底。

本轮验收SDR短片、八档输出及RTX4070 Laptop上的实际处理。尚未验收HDR色准、长时间稳定性、核显/4K实时性能。1.0.16 单文件替换 `dist/VS-Player.exe` 并保留旁边配置；干净Full目录为 `dist/VS-GUI-Full-1.0.16`。可用暂存脚本的 `--output-directory` 指定空目录，保留正在使用的旧Full；Release只创建草稿，附件由维护者上传。

部署目录必须使用与共享 FFmpeg 配套的 `libwinpthread-1.dll` 等传递依赖；旧 MinGW 文件可能导致入口缺失并以 `0xc06d007e` 退出。只验证测试目录能启动不能代替直接运行最终 `lite/vs-player.exe`。

## IAMF/OBR

先取得以下固定源，初始化 libiamf 的 OAR 子模块。不要下载 libiamf 仓库的媒体测试集合；可用 sparse checkout 仅取 `code/`、`LICENSE`、`.gitmodules`。

| 组件 | 固定源 |
| --- | --- |
| libiamf | [AOMediaCodec/libiamf](https://github.com/AOMediaCodec/libiamf/tree/b276f4371a8ae4f370a656e914f7b07e4cd5a2fa)，`b276f4371a8ae4f370a656e914f7b07e4cd5a2fa` |
| OAR/OBR | libiamf 的 `code/dep_external/src/oar` 子模块，`ca14b54381b6e8482a72dcc65c3adb17d6751db0` |
| Eigen 3.4 | [同 SHA 镜像](https://github.com/eigen-mirror/eigen/tree/3147391d946bb4b6c68edd901f2add6ac1f31f8c)，`3147391d946bb4b6c68edd901f2add6ac1f31f8c` |
| PFFFT | [官方快照](https://bitbucket.org/jpommier/pffft/get/0aec0327a6912e1a0ec5326eef737c2ce19bc836.zip)，ZIP SHA256 `53e81a31839a28c393a07e7de0ce041de94922caa71e2ce32fe37fa839b37ea0` |
| Abseil / GoogleTest | OAR 的 FetchContent 固定为 `20250512.1` / `v1.17.0`；只编译 iamf 目标，不编译测试目标 |

使用 MSVC v145、CMake 3.30、Ninja。libiamf 的 CMake 入口是 **code**，不在仓库根。使用 `/MD` 静态库，开启 binauralizer。脚本同时收集 IAMF、OAR、OBR、PFFFT、FLAC、Opus、FDK-AAC 和 Abseil 链接库，避免“关闭 IAMF 才能构建成功”的降级。

```powershell
& tools/build-iamf-api20.ps1 `
  -SourceRoot .deps/iamf-api20-src -EigenRoot .deps/eigen-api20 -PffftRoot .deps/pffft-api20 `
  -BuildDirectory build/iamf-api20 -OutputDirectory build/iamf-api20-stage `
  -VcVars 'C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/Build/vcvarsall.bat' `
  -CMake 'C:/Private/Soft/QT/Tools/CMake_64/bin/cmake.exe' `
  -Ninja 'C:/Private/Soft/QT/Tools/Ninja/ninja.exe'
```

首次构建需要联网下载 libiamf/OAR 固定的其他依赖。若直连 GitHub 不可用，可在调用脚本前为 Git 配置可用镜像；脚本本身不改变全局 Git 配置。

## Native

在一个独立、固定提交的 Git checkout 内构建；`DependenciesRoot` 指已有 `ffmpeg` 和 `vcpkg_installed` 的父目录。脚本只为缺少的目录建立 junction，不修改原依赖。原生 bridge 使用 libbluray 1.4.1 的 `bluray-3.dll`；FFmpeg 自己可同时加载 `bluray-4.dll`，但二者的公开结构不兼容，不能按文件名选择最高版本替代 bridge 依赖。

```powershell
& tools/build-native-api20.ps1 `
  -NativeRoot <独立的固定提交checkout> `
  -DependenciesRoot build/native-1.0.10/third_party `
  -LibiamfRoot build/iamf-api20-stage `
  -OutputDirectory build/mingw-release
```

脚本强制 `FFF_FORCE_LIBIAMF=true`。vcxproj 补齐上游遗漏的 `fdk-aac.lib`。运行目录需保留 MSVC runtime、FFmpeg ABI63/61/7/10/12、libass、匹配的 libbluray3 及它们的依赖。第三方静态许可位于 `third_party/native-api20/`，分发时一起复制。

修改后先检查补丁可应用，再独立构建并核对 ABI、依赖与实际媒体路径。IAMF/OBR 成功链接不能替代真实空间音频听感验收。测试结论写入源码工作区的 `development/validation.md`，不在本指南追加逐轮日志。
