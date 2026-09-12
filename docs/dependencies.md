# 依赖安装与构建

## 普通用户

下载 `VS-Renderer-GUI-1.0.0-windows-x64.7z` 后直接解压并运行 `VSRenderer.exe`。正式包已包含 Qt、MinGW runtime、Python、VapourSynth、内置 VS 插件、3FP 和匹配的 FFmpeg DLL，不读取系统中的 Python/VapourSynth 安装。

系统要求：Windows 10 22H2 或更新的 64 位 Windows，支持 Direct3D 11 的显卡与驱动。

## 源码构建依赖

- Git、CMake 3.25+、Ninja、7-Zip。
- Qt 6.8+；官方构建使用 Qt 6.10.2 MinGW 64-bit 与配套 MinGW。
- Python 3.12+。
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

VapourSynth 固定参考提交为 `5b2d5562726a91d9a75441cc4728a90e6c9f4f27`。3FP 补丁基于 `ee2bfde51a8f85ac156a2253845d5dcb4a07df09`；若上游接口变化导致补丁无法应用，请先 checkout 该提交。
