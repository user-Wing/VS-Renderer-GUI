# VS Renderer

Windows 上用图形化滤镜链生成 VapourSynth 脚本，并以双路画面对照源视频与处理结果的 Qt 6 桌面工具。

> 当前版本：`1.0.0`。已具备紧凑型双路 UI、滤镜链编辑、参数面板、VPY 生成，以及 VapourSynth → 3FP 同帧实时预览。

## 核心能力

- 将常用预处理步骤组织为可排序、可开关的模块化滤镜链。
- 参数修改立即反映到可审阅的 `.vpy` 脚本，不要求用户手写 Python。
- 左路 3FP 作为播放时钟；程序从 VPY 同时读取源与处理后 clip 的帧数/FPS，把左侧当前呈现时间换算成各自绝对帧号，再请求对应 VapourSynth 帧。
- 两路同步滚轮缩放、放大后左键拖拽平移与像素取色；缩放缓存随倍率恢复到源像素密度，默认 Nearest 放大、Lanczos 3 缩小，并可切换 Bilinear/Bicubic/Lanczos/Jinc；空格始终控制播放/暂停。
- 支持 3FCompare 扩展的 VRR tearing present 与 media-rate pacing 开关。
- VapourSynth、Python、源滤镜和当前 catalog 插件随程序部署，终端用户无需手动安装 VS 或 Python。
- catalog 内置 Zsmooth、Deblock、ZNEDI3、EEDI3、SangNom、Bwdif 与 VIVTC；参数面板直接说明各滤镜的收益、速度与画质代价。
- 内置运行时缺文件或插件时在启动和脚本载入阶段明确诊断，不静默生成不可运行的流程。

## 构建

要求 Windows 10 22H2+、Qt 6.8+、CMake 3.25+ 和 C++20 编译器。本机 Qt Creator 套件可直接打开根目录 `CMakeLists.txt`。

```powershell
cmake --preset windows-mingw-debug
cmake --build --preset windows-mingw-debug
ctest --preset windows-mingw-debug
```

先构建官方 Git 工作树中的 VapourSynth R80。脚本同时安装经过 R80 验证的 API 4 插件，并把便携 Python/VS 运行时自动放入程序输出目录：

```powershell
.\tools\build-vapoursynth.ps1 -SevenZip C:\path\to\7z.exe
```

再给 FFF Project 应用仓库补丁、构建 `FFF.Native.dll` 并复制到 Qt 输出目录：

```powershell
.\tools\build-3fp.ps1 -FffProject C:\path\to\FFF_Project
```

程序固定优先加载 `runtime/python/Lib/site-packages/vapoursynth/vsscript.dll`，不读取终端用户的 VS/Python 安装，也不要求调用 `vspipe.exe` 才能预览。CMake 链接程序后会重新执行运行时 staging，避免生成只有 exe、没有后端的半成品。`VSR_VSSCRIPT_DLL` 仅作为没有 bundle 时的开发覆盖项。补丁版 `FFF.Native.dll` 可放到可执行文件旁，或通过 `VSR_3FP_DLL` 指定；3FP 仍须配套其 FFmpeg shared DLL。

内部架构、依赖来源、当前限制和故障排查见 [project.md](project.md)，精确 UI 尺寸见 [docs/ui-layout-spec.md](docs/ui-layout-spec.md)。

完整依赖安装、3FP API 14 构建和 Release 构建步骤见 [docs/dependencies.md](docs/dependencies.md)。普通用户使用发布页中的 Windows x64 压缩包，无需单独安装 Qt、Python 或 VapourSynth。

## 许可证

本项目源码采用 [MIT License](LICENSE)。打包的第三方运行时遵循各自许可证，详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

## 第三方声明

本仓库中的 `third_party/vapoursynth` 是从官方 Git 仓库拉取的上游源码，其许可见该目录。3FP/FFF.Native 与 FFmpeg 不作为本项目源码提交；仓库只保存可复现的 3FP 扩展补丁，运行时二进制按各自许可提供。
