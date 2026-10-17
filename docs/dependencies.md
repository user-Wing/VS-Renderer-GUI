# 依赖准备、构建与测试

## 开发环境

Windows x64；C++20、CMake 3.25+、Ninja、Qt 6.8+（Core/Gui/Widgets/Network/Concurrent，测试另需 Qt Test）及匹配的 MinGW。当前本地记录使用 Qt 6.10.2 / GCC 13.1。Native 使用 MSVC，通过 C ABI 与 Qt 程序连接。

按实际安装目录设置环境，下列路径是示例：

```powershell
$env:QT_ROOT = 'C:/Qt/6.10.2/mingw_64'
$env:PATH = 'C:/Qt/Tools/mingw1310_64/bin;C:/Qt/Tools/Ninja;' + $env:PATH
cmake --preset windows-mingw-release
cmake --build --preset windows-mingw-release
```

预设的输出目录是 `build/mingw-release`，测试默认开启。仅配置成功不代表运行依赖完整。

## 运行依赖

| 组件 | 准备入口与核对方式 |
| --- | --- |
| VS/Python | `tools/build-vapoursynth.ps1`、`stage-vapoursynth-runtime.ps1`；当前锁定 R81 更新用 `update-vapoursynth-runtime.py`，核对 runtime-components 清单 |
| 插件/模型 | `install-analysis-plugins.ps1`、`stage-shaders.ps1`；API4，RIFE 使用 NCNN 的 param/bin，不能用 PKL 替代 |
| Native/FFmpeg | [API20 指南](native-api20-build.md)；固定上游、完整补丁、匹配 shared SDK/DLL |
| 高精度图片/色彩 | `image-runtime.cmake`、`jxl-runtime.cmake`、`color-runtime.cmake`、`awj-runtime.cmake`；依赖来源与哈希在脚本中 |
| PhotoCraft | `tools/photocraft-source.json`、`build-photocraft.ps1`；按固定源码和补丁构建 |
| BD/更新/Remux | `stage-bd-menu.ps1`、`stage-update-tools.ps1`、`stage-mkvtoolnix.ps1` |

固定运行时更新不得删除已有插件或覆盖用户配置。组件许可见 [第三方声明](../THIRD_PARTY_NOTICES.md)。

## 运行测试

先列出当前构建的测试组，再按模块选择：

```powershell
ctest --preset windows-mingw-release -N
ctest --preset windows-mingw-release -R 'graph-and-vpy|shader-parameters'
ctest --preset windows-mingw-release -R 'cap-precision|cap-workflow'
```

Player 的 `vs-player-playback`、Lite 的 `lite-native-playback`、色彩的 `d3d11-color-management`、BD 的 `bluray-metadata` 均为独立 CTest 组。CTest 一组通过不等于一个 QtTest 用例，也不等于真实媒体性能达标。

查看 QtTest 函数并只执行对应专项：

```powershell
& ./build/mingw-release/vsr_player_tests.exe -functions
& ./build/mingw-release/vsr_player_tests.exe directOpenWithoutIndex
```

需要真实媒体/桌面/BD 环境变量的测试，未配置时会跳过。函数的具体输入见 `tests/TestPlayer.cpp` 等测试源码。全量回归可运行 `ctest --preset windows-mingw-release`，但应明确记录实际运行组、失败和跳过，不能把说明中的命令当作已经执行。

修改 seek/时钟要覆盖暂停、前后定位、续播、EOF 与重开；修改 Native/色彩还需 GPU 像素及 Full/Lite 运行路径。性能和实际 UI 另外验证，见 [排查指南](performance-debugging.md)。

## 部署

构建目录不等于本地运行目录。部署前关闭相关测试程序，核对目标 Full/PlayerLite 和配置备份，按 [便携更新](portable-updates.md) 操作；不要用历史 API14/18 脚本替换当前 API20 DLL。
