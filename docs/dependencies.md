# 依赖安装与构建

## 普通用户

下载 `VS-Renderer-GUI-1.0.1-windows-x64.7z` 后直接解压并运行 `VSRenderer.exe`。正式包已包含 Qt、MinGW runtime、Python、VapourSynth、vs-placebo、内置 VS 插件、3FP，以及隔离部署在 `runtime\ffmpeg` 的导出 CLI，不读取系统中的 Python/VapourSynth 安装。

系统要求：Windows 10 22H2 或更新的 64 位 Windows，支持 Direct3D 11 的显卡与驱动。

## 源码构建依赖

补帧新增 `vapoursynth-mvtools==29`，由 `build-vapoursynth.ps1` 固定安装，staging 携带 `plugins/mvtools.dll` 与 GPL-2.0 许可。不要使用 VSRepo 的旧 v24/API 3 版本；R80 已不能加载。现有开发环境可先运行 `.deps\vs-python\Scripts\python.exe -m pip install vapoursynth-mvtools==29`，再重跑 staging 或链接 GUI。

- Git、CMake 3.25+、Ninja、7-Zip。
- Qt 6.8+；官方构建使用 Qt 6.10.2 MinGW 64-bit 与配套 MinGW。
- Python 3.12+。
- 带所需编码器的 FFmpeg CLI；发布脚本通过 `-FfmpegExecutable` 指定，并把其 DLL 隔离复制到 `runtime\ffmpeg`。
- Visual Studio 2022 的“使用 C++ 的桌面开发”工作负载，用于构建 VapourSynth 和 3FP。

先设置 Qt 与编译器环境；路径按本机 Qt 安装位置调整：

```powershell
$env:QT_ROOT = "C:\Qt\6.10.2\mingw_64"
$env:PATH = "C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;$env:PATH"
```

构建 VapourSynth 及 GUI 使用的 VS 插件。脚本会在本地创建 `.deps`、自动克隆固定的 VapourSynth 源码目录并安装插件；这些依赖不会进入 Git：

```powershell
.\tools\build-vapoursynth.ps1 -PythonVersion 3.12 -SevenZip "C:\Program Files\7-Zip\7z.exe"
```

脚本固定安装 `vs-placebo==2.0.4`，其 Windows wheel 提供 `libvs_placebo.dll`。Anime4K shader 文件本身不复制进仓库或便携运行时；GUI 默认读取 `C:\PortableSoft\FFmpegFreeUI ReadyToRun x64\libplacebo`，也可在节点参数中浏览任意兼容 `.glsl`。

构建 API 14 版 3FP：

```powershell
git clone https://github.com/Lake1059/FFF_Project.git .deps\FFF_Project
git -C .deps\FFF_Project checkout ee2bfde51a8f85ac156a2253845d5dcb4a07df09
.\tools\build-3fp.ps1 -FffProject .deps\FFF_Project -OutputDirectory build\mingw-release
```

最后构建和测试 GUI：

```powershell
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
ctest --preset windows-mingw-release
```

构建正式包时显式提供与其 DLL 同目录的 FFmpeg CLI，避免把导出进程依赖与根目录的 3FP FFmpeg ABI 混用：

```powershell
.\tools\package-release.ps1 -Version 1.0.1 `
  -FfmpegExecutable "C:\path\to\ffmpeg.exe" `
  -SevenZip "C:\path\to\7z.exe" `
  -WinDeployQt "$env:QT_ROOT\bin\windeployqt.exe"
```

VapourSynth 固定参考提交为 `5b2d5562726a91d9a75441cc4728a90e6c9f4f27`。3FP 补丁基于 `ee2bfde51a8f85ac156a2253845d5dcb4a07df09`；若上游接口变化导致补丁无法应用，请先 checkout 该提交。


### 多路分析构建补丁与着色器缓存

`tools/build-3fp.ps1` 还应用 `3fp-performance-chroma.patch`：共享 D3D 字节码、非阻塞状态读取/首帧重绘、独立色度核与 Spline36/Super-XBR 单阶段。它与当前 GUI 的缩放参数编码配套，不可只替换 EXE。

Release 构建后运行 `vsr_frame_bridge_tests.exe startupWarmsDecodeAndBothRenderers`，生成输出目录中的 `shader-cache/*.cso`。编译缓存缺失时首次测试会较慢，后续新进程直接读取字节码。发布必须一起复制 `shader-cache`，打包脚本会检查此目录存在；Super-XBR 原始源文件/许可证从 `third_party/shaders` 部署到 `shader-licenses`。


### RIFE / 分析导出扩展（2026-09-30）

`tools/install-analysis-plugins.ps1` 安装 Descale 并下载固定 r9_mod_v33 RIFE Vulkan DLL，校验 SHA-256，再从 `-ModelDirectory` 拷贝已转换的 `rife-v4.26` 与 `rife-v4.26-heavy` 目录（每个包含 flownet.param / flownet.bin）。默认采用用户的 FFmpegFreeUI VideoEnhancer 模型目录；模型清单见 third_party/rife/runtime-sha256.json。stage 脚本随后将这些文件放入便携运行时，发布脚本附带许可证。PKL 不能替代 NCNN 文件。

14 个新增节点其余使用已固定的 Zsmooth / VSZip；不部署 VSRepo 返回的 API 3 旧二进制。源码依赖安装需要模型目录，最终便携使用无需下载。RIFE Vulkan 不要求 CUDA/TensorRT，需支持 Vulkan 的显卡驱动。

## 1.0.2 本地 Player

CMake 生成 VSRenderer.exe 与 vs-player.exe；播放器额外链接 Windows DirectShow / COM / DXGI / PSAPI / PDH / GDI 系统库，无新增 Qt 模块。3FP 构建增加 patches/3fp-player-rate.patch、patches/3fp-player-output.patch，API 14 版本兼容，新增 optional SetPlaybackRate / SetExternalOutputFormat 导出，字幕和位图读取复用已有 C ABI。tools/stage-1.0.2.ps1 只更新本地目录，不调用归档或 Git 工具，首次复制旧便携依赖，重复执行保留用户 VPY / INI。默认复制用户指定的 Anime4K / LAV / madVR 本机资源；madVR 通过随附 ax / DLL 实例化，不要求注册系统。VPY 位于应用旁 vpy；播放器所有设置位于 player.ini。PNG 源截图及嵌入 SRT 提取使用 runtime/ffmpeg/ffmpeg.exe，源码测试目录也需部署此 CLI。
