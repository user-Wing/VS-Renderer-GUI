# API 20 原生内核构建

1.1.0 固定上游 main `72af915f8bf2567155c1f87851a6fa263f73b14f`。在该提交的独立 checkout 应用 `patches/3fp-vsp-1.1.0.patch`，再由脚本复制本库的色彩、源像素探测、Anime4K、自定义 shader 与图片重采样头文件。旧的分层补丁留作历史，不叠加到本轮基线。标准脚本已在干净 worktree 完整构建成功。

入口是 `tools/build-native-api20.ps1`，其 NativeRoot 必须固定到上述提交；DependenciesRoot 提供 ffmpeg / vcpkg_installed，LibiamfRoot 提供 IAMF 静态库。最终 Qt 和 Full/Lite 都必须使用同一份 DLL，不能复用旧 snapshot 布局。

保留本项目的外部 VS 帧输入、clock-only、输出格式、像素探测、缩放、音频 EQ/tempo、软件解码队列和可选 libplacebo 色彩桥。snapshot、字幕及音频电平结构均按 API20 版本号传递，构建时必须保持 MinGW/MSVC 的配置、snapshot、字幕命令及图层布局一致。上游20移除了旧私有 DV/RPU 测试扩展；本补丁沿用上游公开色彩和元数据路径，不恢复该私有接口。

配置仍为 112 字节，snapshot v22 为 592 字节；Qt 镜像同步新增上传耗时、字节数、次数和路径，并固定关键偏移。Jinc 权重使用纹理槽 t8，避免与上游辅助视频层 t5～t7 重叠；修改内嵌 HLSL 后必须运行上游 `tools/generate_shader_bytecode.py` 及 `--sdr-specializations` 重新生成两个字节码头。

AI字幕沿用1.0.14迁入的 #36/#37 Native 能力，来源为 #37 head `edf533155727f681eefa771082702bab275bca48`（含 #36），与VSP扩展一同在新基线上重建，不直接混用功能分支DLL。音频tap在VSP EQ/atempo前采集，避免重复采样；Qt只将有界PCM交给外置worker。Qwen3-ASR 模型及 Python 独立打包，llama-server 仅供旧 v1 配置，上游VB页面不迁入。音频块144字节、tap配置16字节、RTX状态44字节，需与Native保持ABI一致。

## 多路比对扩展

`build-native-api20.ps1` 在汇总补丁中包含比对扩展，并复制 `src/backend/SourcePixelProbe.h`。此扩展让交互重绘绕开静态图片的媒体帧率限制，并新增本地可选导出 `VSR3FP_ReadSourcePixel`；配置、snapshot 和 API20 版本不变。

源像素保留在色彩/位深转换前，YUV 直接按 FFmpeg 像素格式描述读取原码值；硬件帧在第一次取样时下载并缓存。坐标通过原生呈现矩形映射，保留旋转、留黑、缩放和 DPI 关系。缺少此扩展的旧 DLL 不提供原始 YUV，不能用 RGB 反算代替。

## Lite Anime4K Fast

Lite和本地单文件的“图像处理 → 缩放滤镜”提供与Full同名的八档：CNN＋额外增强、CNN、no CNN＋额外增强、no CNN、Jinc、D3D11、1X·Jinc、1X·D3D11。设置的渲染页可指定默认档位，`player.ini` 的 `player/animeStage` 保存0～7；旧Fast布尔选择迁移为CNN档。底栏和Tab显示实际模式。源高度≥2160p保留原有内置增强禁用规则。

1.1.0 的 Full/Lite 使用同一内核。标准构建应用 1.1.0 汇总补丁；在固定基线的独立 checkout 上执行：

```powershell
& tools/build-native-api20.ps1 `
  -NativeRoot <独立的固定提交checkout> `
  -DependenciesRoot <包含ffmpeg和vcpkg_installed的目录> `
  -LibiamfRoot build/iamf-api20-stage `
  -OutputDirectory build/mingw-release
cmake --build build/mingw-release --target VSRenderer VSPlayer VSPlayerLite VSCap
```

脚本用 `.deps/color/ucrt64/bin` 的glslc、spirv-cross及Windows SDK 10.0.26100.0的fxc转换 `anime4k-a-fast.glsl`、`anime4k-no-cnn.glsl` 和 `gpu-sharpen.py` 生成的两个原始LUMA shader，共45个pass，将字节码嵌入DLL。播放不需要构建工具、Python、VapourSynth或外部shader文件。分发保留Qt/Native各依赖的原许可，以及两个原shader的MIT/Unlicense声明。

执行链为原生色彩转换（源尺寸）→ 可选亮度提取、两段额外增强及合回RGB → MAIN → PREKERNEL → 原生RGB缩放。亮度提取使用源矩阵与量程，增强仍为原有0.3强度/2阈值的sharpen_edges、enhance_detail；浮点16位中间纹理保留CNN负数。常规Anime档按视口调度WIDTH/HEIGHT/WHEN；1X档将OUTPUT固定为源尺寸，再交给现有Jinc或D3D11 shader缩放路径。算法号为CNN 9、CNN增强10、no CNN增强11、no CNN 12、1X·Jinc 13、1X·D3D11 14。该 `Anime4KFast` 执行器仅运行已转换的内置 shader；自定义导入使用下述独立执行器。

像素回归：用MSVC将 `tests/Anime4KFastProbe.cpp` 编译并链接d3d11/d3dcompiler，包含 `src` 和 `build/anime4k-native/shaders`，输出 `build/anime4k-native/probe.exe`；安装测试用moderngl/numpy后运行 `python tools/check-anime4k-fast.py`。六变体×八比例，共48组，独立编译原GLSL对照D3D11 shader输出；探针最后统一双线性适配，不声称整个Full/libplacebo色彩/缩放链逐像素一致。`vsr_player_lite_tests nativeAnimePresets` 验证八档菜单、实际像素、额外增强差异、两种1X最终缩放差异、播放/定位和重开持久化，可用 `VSR_ANIME4K_SOURCE` 指定片段。

单文件先用 `stage-editions-1.0.9.py --edition Lite --player <本轮Lite EXE>` 整理 `build/playerlite-runtime`，再运行 `tools/package-playerlite-single.ps1 -SourceDirectory build/playerlite-runtime`。单文件封装以暂存内的 EXE 为准，自动排除用户 INI 和 FFmpeg 专用文件；体积按实际产物报告。本轮生成单文件 EXE 与 Full 运行目录，不另外创建发行压缩包。

Anime4K 的提前返回分支也必须先清空目标背景，否则缩小视频后会保留旧交换链画面。`vsr_player_lite_tests zoomOutClearsOutsideVideo` 对八档逐次缩小、放大并读取实际交换链像素，验证视频内颜色与视频外黑底。

各版本实际验收的素材、设备与颜色范围见 [测试台账](../development/validation.md)，专用 shader 像素探针不能代替 Full/libplacebo/HDR 全链验证。1.0.17 单文件目标为 `dist/VS-Player.exe`，保留旁边配置；干净 Full 目录为 `dist/VS-GUI-Full-1.0.17`。可用暂存脚本的 `--output-directory` 指定独立目录，保留正在使用的旧 Full。本轮不推送、不上传、不创建 Release。

部署目录必须使用与共享 FFmpeg 配套的 `libwinpthread-1.dll` 等传递依赖；旧 MinGW 文件可能导致入口缺失并以 `0xc06d007e` 退出。只验证测试目录能启动不能代替直接运行最终 Full 目录和 `dist/VS-Player.exe`。

## 自定义 GLSL/HLSL 扩展

标准构建在汇总补丁中保留自定义 shader 扩展，并复制 `src/render/CustomShader.h`。扩展提供可选 C 导出 `VSR3FP_SetCustomShader(handle, path, width, height)`，保留配置布局，snapshot 使用本轮 v22。Qt 通过 `ThreeFpPlayer::setCustomShader()` 安装；空路径清除，宽高为 0 时使用视频视口。加载失败保留旧执行器；已有画面时会先用候选 shader 验证当前帧，成功后才替换，设备重建后重新建立 GPU 资源。

`src/render/ShaderPackage.*` 是 Full、Lite 与 Renderer 共用的准备接口：`.hlsl` 使用 D3DCompile 编译 `main/ps_5_0`；`.glsl/.hook` 经过 glslc → SPIRV-Cross → HLSL → D3D11 字节码。兼容的自定义 shader 无需重建 DLL。导入 GLSL 所需的工具位于 `runtime/shader-tools/`，由 `tools/stage-shader-tools.py` 复制 glslc、SPIRV-Cross、对应依赖和许可；开发目录可回退到 `.deps/color/ucrt64/bin`。必须保留转换工具配套的 MinGW DLL，不能拿 Qt 目录的同名 DLL 替换。HLSL 导入和逐帧播放不需要 Python 或 Windows SDK，运行时保留 D3DCompiler 依赖。

缓存包使用 `VSRHLSL1` 格式，当前语义键为 `VSR-HLSL-1.0.17-format1-main-prekernel-native-multipass-v7`。源内容、扩展名、GLSL 转换器二进制和 D3DCompiler 标识参与缓存键；复用前验证包结构，截断包会重新生成。包保存 pass 的 hook、纹理绑定、尺寸/条件表达式与 DXBC，逐帧播放不重新转换。导入源与缓存的位置见 [Player 指南](player.md#导入独立-glslhlsl)。

通用执行器使用 FP16 中间纹理，支持 MAIN/PREKERNEL/LUMA 的多 pass、命名纹理、尺寸表达式、条件跳过和 COMPONENTS 1～4；NATIVE 只支持原始尺寸表达式。CHROMA 像素、计算着色器、外部纹理、OFFSET、include 以及未实现的指令明确拒绝。原生 HLSL 的 t0/s0、浮点输入输出与可选 32 字节 b0 通过反射验证；完整范围见 [当前支持边界](player.md#当前支持边界)。

通用 shader 的输入处于原生色彩处理后的 RGB 阶段，仅允许受支持的普通 SDR 路径；HDR、BT.2020/P3 色域、额外颜色变换与 360 投影保守拒绝。Full VPY/Renderer 可回退原 GLSL，直接播放停用该视频的自定义 shader 并保留选择。Full 提取只覆盖可验证生成图的末尾 shader，前序 VS 仍运行，编码导出仍保留原 VPY；这不是任意 VPY 到 HLSL 的编译器，也不承诺任意 shader 的性能倍数。

`tests/TestShaderPackage.cpp` 覆盖编译、缓存复用/失效/修复、真实随包 shader 和明确拒绝项；`vsr_player_lite_tests customShaderPixelsAndFailureRecovery` 验证真实 D3D11 像素、命名多 pass、PREKERNEL、COMPONENTS 与安装失败恢复。测试应用目录同样需要媒体 DLL、MSVC runtime、Qt plugins 及 shader 转换组件；仅链接成功不足以运行这些测试。实际执行结果统一记录在测试台账。

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

## 1.1.0 资源构建补充

- `tools/build-extra-imageformats.ps1` 固定 Qt ImageFormats 6.10.2 / `076fb82c55321e42beeae62d9e3ca8c4bb71439c`，构建 TIFF 插件；打包脚本复制到 Full/Lite 并保留 LGPL/libtiff 许可。
- `tools/stage-ncnn.py --source <VideoEnhancer安装目录> --output build/mingw-release/runtime/ncnn` 整理独立 Python 3.12 与模型；当前 wheel 在进程拆卸时异常，因此每帧子进程在检查推理结果并关闭输出后直接结束，主进程不加载该扩展。21 个推理模型均完成实测，场景识别模型不用于超分。
- `vsr_shader_precompile <assets/mpv-shaders> <输出目录>` 生成 `mpv-shaders/` 配套字节码与报告，源目录不写入生成物。`tools/stage-shaders.ps1 -CompiledDirectory <输出目录/mpv-shaders>` 将其合入归档；本轮成功 231/915，不能宣称全部 mpv shader 可转换。
- 单文件采用 VSLITE03 / tar.xz，产物与配置放在独立路径验证；旧版本的 ZIP footer 校验脚本不能用于它。
