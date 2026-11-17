# API 20 原生内核构建

Qt构建新原生VS源时，用 `cmake -S . -B build/mingw-release -DVSR_FFMPEG_ROOT=<DependenciesRoot/ffmpeg>` 指定与Native一致的shared SDK；默认仍兼容本机既有路径。配置会检查头文件及63/63/61导入库，缺失时明确失败，不再静默省略默认源。两轮色彩审查修复及上游草稿范围见 [记录](../development/color-review-fix-20261114.md)。

2026-11-14 色彩续修沿用固定248cb9b及更新的汇总补丁，标准脚本额外复制`FrameColorMetadata.h`；通用/SDR字节码必须完整重生成。外部帧v2为240字节，v1的112字节前缀保留，通过可选查询协商版本。GUI同时构建`vsr-source.dll`，Full部署不可遗漏；验证与限制见 [色彩修复](../development/color-fix-20261114.md)。

## 当前基线：2026-11-15 上游合并

固定 `85f514be34cdbcef17ba5c44b90cd9bbe4c2526d`（已合并 #58–#62）。标准脚本应用 `patches/3fp-vsp-upstream-20261115.patch`，保留 VSP 帧桥、缩放和 AVIF 预算扩展；不再叠加旧汇总补丁或单独 AVIF 预算补丁。DependenciesRoot 除 ffmpeg/vcpkg_installed 外须提供 `libraw/include/libraw`、`libraw/lib/x64/libraw.lib`、`libraw/bin/x64/libraw.dll`，本轮由 LibRaw 0.21.4 的 `Makefile.msvc bin\libraw.dll` 构建。运行目录须携带 libraw.dll 与许可。RAW 使用上游原生开发器，VSP 可选扩展 `VSR3FP_DecodeRawImage` 将完整尺寸 RGB48 复制到图片浏览器，避免显示截图和8位中间文件。

## Native 安全与缓冲契约

`20261115` 汇总补丁同时包含安全上游 `eee98da` 的生命周期、AVIF、预取、字幕二次 IO 和时间戳边界修复，以及 VSP 的 concat seek、丢帧恢复、音频 preroll、外部帧与图集修复。普通 UNC、受控 loopback、音效和帧桥仍由 VSP 适配层维护。`.deps/fff-vsp-20261115` 的合并候选与固定 `85f514b` 加补丁是两条入口；标准脚本的 HEAD 校验只适用于后一条。

动态缓冲通过既有可选导出配置，`mediaInfo.decodeBufferPolicyVersion=2` 表示以下语义：

| bytes / frames | 行为 |
| --- | --- |
| `0 / 0` | 撤销外部配置，恢复 Native 自适应默认值 |
| `0 / 非零` | 停用可选预解码，保留解码器引用链和最小两帧按需工作路径 |
| `非零 / 2..512` | 字节预算与帧数共同限制准入；预算不是预分配目标 |

配置适用于普通 2160p 及以上 CPU 解码。GUI 保留普通 8 帧、尖峰 48 帧及用户 RAM 上限，仅在提交成功后记住配置；暂停、倍速、网络、最小模式等退出适用条件时撤销。缩容立即释放备用池并限制新帧准入，已有队列按顺序播放释放；暂停时已有驻留保留，恢复、seek 或关闭后释放，不能把设置成功解释为进程内存立即降到预算。

`decodeBufferEffectiveBytes`、`decodeBufferEffectiveQueueFrames`、`decodeBufferResidentQueueBytes` 报告当前生效预算、主队列限帧与已保留队列内存；`decodeBufferPipelineFrames` 和 `decodeBufferMinimumWorkingFrames` 说明必要工作开销。驻留计费包含 frame buffer/side data/opaque_ref，保守重复计费共享引用；不是进程全部内存或解码器内部内存的硬上限。旧 DLL 无策略标识时，零预算请求使用其最小队列；缺少可选导出返回不支持，GUI 保持可重试。

外部帧维持 v1 的 112 字节前缀、v2 的 240 字节布局及版本协商。尺寸、格式、平面跨度、512 MiB 拷贝预算、时间乘法和 side data 均在拷贝前校验；高位深格式不因预算不足而静默降精度。此接口只接受可信进程内调用方，调用方必须保证指针及按 stride 推导的平面字节可读；裸指针接口无法验证任意地址。HDR side data 仍要求精确匹配 FFmpeg avutil ABI。

## 专项回归构建

标准构建默认使用低优先级，`-BuildParallelism` 默认 2、最大 4，并强制全量重建及 `/p:SecurityRegressionTests=false`。发行前检查 RAW 和 DecodeBuffer 导出存在，`FFF3FP_RunSecurityRegression` 与 `FFF3FP_RejectNextDecodeBufferForTest` 不存在。完整构建之后，在独立输出目录使用 `/p:SecurityRegressionTests=true` 编译 Native，可运行归档于补丁的 `FFF.Native.Tests/SecurityRegression.cpp`：

```powershell
python tests/run-native-security.py --dll <测试版FFF.Native.dll> --runtime <FFmpeg配套运行时目录> --fixtures <隔离夹具目录>
```

夹具由 FFmpeg 生成，不需要用户媒体。测试导出只存在于显式启用的测试构建，生产 DLL 不带故障注入。GUI 专项为 `vsr_player_tests nativeConcatPreciseSeek nativeSoftwareDrainAndStep decodeBufferExitAndRetry`；最后一项需要测试版 Native，并测试真实配置失败重试、流切换及 ffprobe 取消/输出限制。其他适用回归包括 AVIF、音频路径、字幕 seek、输出格式、Lite 软件播放和 VS 帧桥。GPU/音频端点或 VS 运行时不足必须单独记未验证。

使用独立 Native 源码目录时，CMake 设置 `-DVSR_NATIVE_SOURCE_ROOT=<已应用补丁的源码根目录>`，使 AVIF 专项编译同一份实际发行源码；不要求把整个上游目录提交到本项目。

补丁维护须用新建固定基线目录检查正向应用、反向还原与再次应用，复制配套头并重新生成两类 shader 后比对候选输入，再完整编译链接。构建验证不等于安装部署；不能用单文件语法检查、源码等价或二进制哈希替代播放回归。

## 历史基线：2026-11-13 上游合并

固定 `248cb9bb0cef91922ab8f88ea7f6ef38ec0e311c`（上游 PR #57，已含 #55 CUDA 主优化及互操作续修）。`tools/build-native-api20.ps1` 仅应用 `patches/3fp-vsp-upstream-20261113.patch` 和 `patches/3fp-avif-budget.patch`，然后复制本库头文件并重新生成通用/SDR HLSL 字节码；不要叠加下文历史三层补丁。API 仍为 20，snapshot 为 **v23 / 608 字节**，Qt 根据新可选导出识别版本，旧字段偏移保持不变。

本机完整 MSVC/IAMF 构建产物来自 `.deps/fff-upstream-20261113`。OpenNGU 使用独立 `VSR3FP_SetUpscaleShader` 执行器，可位于原生直通或已有处理结果之后；旧自定义 shader 接口不变。源码补丁不包含用户提供的私有 OpenNGU 参数。 VS GLSL 导出会合并 MAIN hook 中重复的 MAIN/HOOKED 同义绑定，避免 libplacebo 重定义宏后静默禁用 hook。

上游音频直通是可选模块 seam：Native 负责轨道、WASAPI 独占、ASR 影子解码和回退，IEC 61937 打包由外部模块提供；无模块时保持 PCM。不能将接口回归通过写成 AVR 直通播放通过。当前构建/测试证据见 [本轮记录](../development/upstream-openngu-20261113.md)。下面为旧基线的历史说明。

## 2026-11-10 音频与 UNC 补丁

固定提交仍为 `72af915f8bf2567155c1f87851a6fa263f73b14f`。标准 `tools/build-native-api20.ps1` 按顺序应用原汇总补丁、原网络代理补丁、`patches/3fp-vsp-audio-unc.patch`，再复制 `src/backend/LocalMediaPath.h`、`AudioFormatPolicy.h` 等配套头文件。C ABI 和结构尺寸不变；新增实际音频状态走既有 media-info JSON。

源码包的 `native-core/FFF.Native` 是已经应用三层补丁并复制头文件的快照，不要再次对该快照 apply。可使用它复核修改；常规可复现构建仍使用上面的固定 Git checkout + 标准脚本。包内保留生成的 Anime4KFastShaders.h，但外部 FFmpeg 63/63/61、libass、IAMF 库等 SDK 不在核心源码包内。

本轮 Full/Lite GUI 和专项测试程序已完整链接；WasapiRenderer 与 PlayerSession 经 MSVC `/Zs` 编译检查，修改后的 WasapiRenderer + AsrAudioTap 已单独 MSVC 链接并实际进行硬件协商。**完整 FFF.Native.dll 未重建**，不能把 GUI 构建成功或独立探针等同于新内核完整播放器部署。本地已安装播放器未覆盖。

1.1.0 固定上游 main `72af915f8bf2567155c1f87851a6fa263f73b14f`。在该提交的独立 checkout 应用 `patches/3fp-vsp-1.1.0.patch`，再由脚本复制本库的色彩、源像素探测、Anime4K、自定义 shader 与图片重采样头文件。旧的分层补丁留作历史，不叠加到本轮基线。标准脚本已在干净 worktree 完整构建成功。

入口是 `tools/build-native-api20.ps1`，其 NativeRoot 必须固定到上述提交；DependenciesRoot 提供 ffmpeg / vcpkg_installed，LibiamfRoot 提供 IAMF 静态库。最终 Qt 和 Full/Lite 都必须使用同一份 DLL，不能复用旧 snapshot 布局。

保留本项目的外部 VS 帧输入、clock-only、输出格式、像素探测、缩放、音频 EQ/tempo、软件解码队列和可选 libplacebo 色彩桥。snapshot、字幕及音频电平结构均按 API20 版本号传递，构建时必须保持 MinGW/MSVC 的配置、snapshot、字幕命令及图层布局一致。上游20移除了旧私有 DV/RPU 测试扩展；本补丁沿用上游公开色彩和元数据路径，不恢复该私有接口。

配置仍为 112 字节，snapshot v22 为 592 字节；Qt 镜像同步新增上传耗时、字节数、次数和路径，并固定关键偏移。Jinc 权重使用纹理槽 t8，避免与上游辅助视频层 t5～t7 重叠；修改内嵌 HLSL 后必须运行上游 `tools/generate_shader_bytecode.py` 及 `--sdr-specializations` 重新生成两个字节码头。

AI字幕沿用1.0.14迁入的 #36/#37 Native 能力，来源为 #37 head `edf533155727f681eefa771082702bab275bca48`（含 #36），与VSP扩展一同在新基线上重建，不直接混用功能分支DLL。音频tap在VSP EQ/atempo前采集，避免重复采样；Qt只将有界PCM交给外置worker。Qwen3-ASR 模型及 Python 独立打包，llama-server 仅供旧 v1 配置，上游VB页面不迁入。音频块144字节、tap配置16字节、RTX状态44字节，需与Native保持ABI一致。

## 多路比对扩展

`build-native-api20.ps1` 在汇总补丁中包含比对扩展，并复制 `src/backend/SourcePixelProbe.h`。此扩展让交互重绘绕开静态图片的媒体帧率限制，并新增本地可选导出 `VSR3FP_ReadSourcePixel`；配置、snapshot 和 API20 版本不变。

源像素保留在色彩/位深转换前，YUV 直接按 FFmpeg 像素格式描述读取原码值；硬件帧在第一次取样时下载并缓存。坐标通过原生呈现矩形映射，保留旋转、留黑、缩放和 DPI 关系。缺少此扩展的旧 DLL 不提供原始 YUV，不能用 RGB 反算代替。

## Lite Anime4K Fast

Full、Lite 和本地单文件均提供六档 Anime4K：A CNN、AFast CNN、AFast 1X 各有普通与额外增强两档；MoeSR、Jinc 和 D3D11 单列。设置渲染页同步六档及悬停说明。`player/animeStage` 保留历史编号：8/9 为 A CNN 增强/普通，0/1 为 AFast CNN 增强/普通，6/7 为 AFast 1X 增强/普通；2/3 已停用。旧 no CNN 增强/普通迁移为 6/7，旧 1X·Jinc/D3D11 均迁移为普通 1X，第二上采样独立保存。源高度≥2160p 保留原有内置增强禁用规则。

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

AFast CNN 使用原生 `Anime4KFast` 执行器（算法 9/10）。A CNN 和 AFast 1X 使用内置 HLSL 包，增强档追加 sharpen_edges（0.3，阈值 2）与 enhance_detail（0.3）。1X 将 OUTPUT 固定为源尺寸，两档均交给第二上采样完成最终放大。旧 Native 算法 11～14 保留 ABI 兼容，播放器内置预设不再选择 no CNN；1X 包失败时明确提示并回退 D3D11。

历史原生 shader 像素对照使用 `tests/Anime4KFastProbe.cpp` 和 `tools/check-anime4k-fast.py`。当前 `vsr_player_tests animeSixMigration animeSixPresets` 及对应 Lite 测试验证六档菜单、设置入口、旧配置迁移、1X 增强像素差异、1X 包输出尺寸及第二上采样的实际像素效果；`nativeAnimePresets` 另验证 Lite 播放/定位及重开持久化。测试片段通过不代表所有分辨率或设备达到实时。

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
