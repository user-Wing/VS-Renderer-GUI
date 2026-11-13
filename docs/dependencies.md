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
| Agent MCP | `tools/stage-agent-runtime.ps1`；Node 20+，锁定的官方 client v2；便携 Node/npm/生产依赖与许可位于 `runtime/agent`，缺失时可在 MCP 设置安装 |

固定运行时更新不得删除已有插件或覆盖用户配置。组件许可见 [第三方声明](../THIRD_PARTY_NOTICES.md)。

Full 当前 FFmpeg 固定为用户提供的 `09-full-shared-newdav1d.zip`，版本 `9.1-dev-3586-gfc00db20fc-full09`，dav1d 提交 `39834d7f71c85a9c555f2ddd7a7add6d671750de`；归档 SHA256 与 ABI 见 `tools/ffmpeg-source.json`，用 `tools/stage-ffmpeg.ps1` 部署。保持同一套 shared DLL，不能仅替换 CLI。

Full 不再随包携带 LAV / madVR，在“设置 → 组件下载”下载、应用并重启；原接口保留。默认 shader 库只部署完整 Anime4K，内置 MoeSR 不受影响；`tools/package-optional-shaders.ps1` 生成扩展库，可从“设置 → 组件下载”的 `mpv-shaders` 项下载、应用并重启，也可手动解压到 Full 安装目录。`package-portable.ps1` 强制排除已下载的可选组件与额外 shader，并剥离发行 EXE 的冗余符号；源码与本地调试产物保留。

## 运行测试

先列出当前构建的测试组，再按模块选择：

```powershell
ctest --preset windows-mingw-release -N
ctest --preset windows-mingw-release -R 'graph-and-vpy|shader-parameters'
ctest --preset windows-mingw-release -R 'cap-precision|cap-workflow'
```

Player 的 `vs-player-playback`、Lite 的 `lite-native-playback`、色彩的 `d3d11-color-management`、BD 的 `bluray-metadata` 均为独立 CTest 组。CTest 一组通过不等于一个 QtTest 用例，也不等于真实媒体性能达标。

Agent 专项为 `ctest --preset windows-mingw-release -R '^agent-'`。首次运行 MCP 开发测试前执行 `npm --prefix tools/agent-runtime ci --ignore-scripts`，并运行 `tools/stage-agent-runtime.ps1` 准备构建目录中的便携运行时；`npm --prefix tools/agent-runtime test` 覆盖真实 stdio / HTTP 协议。`VSR_TEST_NPM_INSTALL=1` 开启联网安装验证；`VSR_AGENT_3FUI_SETTINGS` 指向本机 3FUI 设置时会使用其中端点进行付费实测，未设置时相关用例跳过。

本地环境工具使用 Qt Core / Concurrent / Gui 与 QProcess，无额外 npm 依赖；新增 `agent-access`、`agent-local-files`、`agent-local-process`、`agent-image-messages`、`agent-local-images` 检查权限、本地操作、进程树和真实图像消息。付费 `liveDeepSeekLocalEnvironment` 会在新临时目录实际读取、编辑、运行 Node，并由只读子智能体核验；图像协议使用本地 HTTP 模拟器验证，不将文本模型当作视觉模型。

查看 QtTest 函数并只执行对应专项：

```powershell
& ./build/mingw-release/vsr_player_tests.exe -functions
& ./build/mingw-release/vsr_player_tests.exe directOpenWithoutIndex
```

需要真实媒体/桌面/BD 环境变量的测试，未配置时会跳过。函数的具体输入见 `tests/TestPlayer.cpp` 等测试源码。全量回归可运行 `ctest --preset windows-mingw-release`，但应明确记录实际运行组、失败和跳过，不能把说明中的命令当作已经执行。

修改 seek/时钟要覆盖暂停、前后定位、续播、EOF 与重开；修改 Native/色彩还需 GPU 像素及 Full/Lite 运行路径。性能和实际 UI 另外验证，见 [排查指南](performance-debugging.md)。

## 部署

准备现有 color 运行时后执行 `tools/prepare-planar-shader.ps1`，再配置 CMake；Vulkan-Headers 固定为 `73814d006638aa00bd553215b054f0f02e457e22`。依赖齐全时构建 `vsr_planar_shader`，将处理过导入名的 `vsrplacebo.dll` 放入 VS plugins；缺少时继续使用标准 placebo，性能结果不能沿用平面后端的测量。

修改源码、固定上游修订与 LGPL 许可在 `third_party/vsr-placebo`。`tools/stage-planar-shader.py` 将导入名对齐到隔离的 color DLL；不能直接部署未处理的 DLL。`tools/check-planar-shader.py --source <视频> --output <JSON>` 对照原 Shader 像素；`tools/benchmark-super-interpolation.py` 只测 VS 出帧，Native 上传与呈现另测。

构建目录不等于本地运行目录。部署前关闭相关测试程序，核对目标 Full/PlayerLite 和配置备份，按 [便携更新](portable-updates.md) 操作；不要用历史 API14/18 脚本替换当前 API20 DLL。

### 可选 shader 上传约定

上传仓库为 `ARXChem/Software-List`，分支 `master`，本次完整路径：

```text
mpv-shaders/2026.11.13/VS-GUI-mpv-shaders-optional.7z
```

只需上传现成 `.7z`，无需重新压缩、改名或手写下载清单；可附带同名 `.sha256` 供人工核对。版本从目录 `2026.11.13` 读取，后续更新使用更高的点分版本目录。客户端读取仓库接口提供的大小和SHA256，验证后识别包内 `shaders/mpv-shaders.7z` 与 `shaders/mpv-shaders/manifest.json`，整体安装到程序 `shaders` 目录并写入组件版本。保持这两个相对路径，避免再包一层 `shaders`。应用后重启使滤镜列表重新发现扩展库；Anime4K、MoeSR和自定义shader保留。
