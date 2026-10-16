# 3FP API 20 上游核对与 VSPlayerLite 单文件交付（2026-10-16）

## 1. 已确认上游

- 上游仓库：`Lake1059/FFF_Project`；主分支本次取得的 commit：`856b320`（2026-10-15，合并 PR #21）。
- [PR #9](https://github.com/Lake1059/FFF_Project/pull/9)：D3D11 DXGI 适配器选择、批量像素读取、渲染目标状态、视图变换快速路径；已合并。
- [PR #19](https://github.com/Lake1059/FFF_Project/pull/19)：HDR10+/Vivid 动态元数据注入、IAMF OBR 双耳空间音频、旋转感知 RTX VSR、API 20 和音频延时；已合并。
- [PR #21](https://github.com/Lake1059/FFF_Project/pull/21)：刷新节奏锁定、kind-3 色彩变换、图片缩放几何、RTX VSR 宿主设置，以及渲染修复；已合并。
- 直连 GitHub 曾失败，本机使用 `https://tvv.tw/https://github.com/Lake1059/FFF_Project.git` 成功取得代码，保留于 `.deps/fff-upstream-api20`。
- 本工程自身 `origin/main` 与本地 `17b51f9` 完全同步（领先/落后：0/0），因此新功能位于外部 3FP 原生仓库，并非本项目漏拉 `main`。

## 2. API20 迁移进展（本地续修）

已在固定上游 `856b3200c2130db75a2ab06730e32fb5c65f0bd1` 重新移植 VS 外部帧、clock-only、色彩桥、缩放和音频扩展，得到 `patches/3fp-api20-vsrenderer-1.0.10.patch`。MSVC v145 成功构建完整 IAMF/OBR 内核，未通过关闭 IAMF 降级；补齐 FDK-AAC 链接项。构建入口和固定依赖见 [API20 构建说明](native-api20-build.md)。

GUI 的 snapshot、字幕 renderer/command/layer、音频电平结构按 API20 传版本号，同时保留 API18 兼容分支；MinGW 与 MSVC 的结构大小及关键偏移已核对一致。真实回归已覆盖音频时钟、原始/Jinc/D3D11 播放、外部 VS 帧、输出格式切换、像素探测、字幕叠加和 Lite 无 VS 软解。

原网页阶段“API20 未部署”“Lite 测试超时”的记录由本地续修替代；最终部署/验证以 [续修记录](update-20261016.md) 为准。上游20不再提供旧私有 DV/RPU 测试扩展，本轮不恢复该接口；公开 HDR 元数据和 kind-3 色彩路径继续使用上游实现。IAMF/OBR 已编译，但缺少对应空间音频素材，不能宣称听感验收已通过。

## 3. 已完成的播放器修改

- `src/player/PlayerControls.cpp`：只删除了控制栏右下角重复的 `☰` 菜单按钮和连接的动作；操作仍在 `src/player/PlayerMenus.cpp` 右键菜单中，保留其入口。
- `src/player/PlayerWindow.cpp`：Lite 优先从 `VSR_PLAYER_CONFIG` 读取 INI 路径，否则使用 EXE 所在目录 `player.ini`；首次执行应用 Lite 默认设置，已有配置保留所有可用设置与字体，不再无条件重置合法选项；如配置要求 Lite 根本不提供的 LAV/madVR 等后端，则只将这些无效选项回退至 Lite 支持的 3FP/VS。
- `src/backend/ThreeFpApi.h`：API20 snapshot 布局及调用版本已配套迁移，详见上节。
- 完成 `VSPlayer` 与 `VSPlayerLite` 的 MinGW CMake Release 编译（无编译错误）。

## 4. 网页阶段单文件 Lite（历史产物，本轮不重打包）

- 输出：`dist/VSPlayerLite.exe`，约 **31.72 MiB**。
- 构建入口：`tools/package-playerlite-single.ps1`；启动器：`tools/playerlite-launcher.c`。
- 这是**单文件分发、首次解压、以后复用缓存**的便携实现，而非把 Qt/FFF Native 静态链接进一个原生 PE。EXE 附带 ZIP，首次运行展开至 `%LOCALAPPDATA%/VSPlayerLite/<payload-hash>/`；之后识别 `.ready` 不再解压，减少热启动前置开销。
- 包内没有 `ffmpeg.exe`、`ffprobe.exe` 或 `avcodec-*.dll`、`avformat-*.dll`、`avutil-*.dll`、`avfilter-*.dll`、`swresample-*.dll`、`swscale-*.dll` 等 FFmpeg 动态库。
- 运行机需要把与原生 DLL **同 ABI** 的 FFmpeg 共享库放在单文件 EXE 所在目录，或加入系统 `PATH`；仅安装一个其他 major 版本的 `ffmpeg.exe` 不代表 ABI 兼容。
- 首次启动器在**单文件所在目录**安全创建 `player.ini` 并传递 `VSR_PLAYER_CONFIG`；已有 `player.ini` 时不覆盖；若该目录不可写则明确提示权限错误。
- 已验证：使用同目录 FFmpeg 库时启动器返回码 0；从独立位置仅通过 PATH 提供依赖时启动器也返回 0；配置文件能创建，并在下一次启动保留测试写入的 `[test] custom=42`。
- 启动器实际派发测量（并非 GUI 绘制或起播时间）：某次冷启动约 **428 ms**，暖启动约 **50 ms**；仅验证进程派发，不能据此宣称播放画面或 HDR 已正常输出。
- 单文件脚本以已编译的 Lite GUI 和现有 1.0.10 发行依赖打包，**新 API20 内核未放入交付版**。保留了原有 `dist/1.0.10-PlayerLite` 和 `dist/1.0.10-Full` 作为可回退基线。

## 5. 网页阶段的验收边界（历史）

已证明：上游 commit/PR 核对、GUI/Lite 编译、重复按钮删除、单文件构建、FFmpeg 排除、运行时缓存生成、INI 路径/重开保留、FFmpeg 所在目录与 PATH 两种启动器路径。

失败或未验收：在本机 AgentDock 非交互式会话下运行 `ctest -R ^lite-native-playback$`，测试 #22 超过 90 秒超时，**未通过**；这不是能用编译成功替代的回归验证。仍需在可交互桌面核对是否属于测试会话 GUI 阻塞还是产品问题，随后执行完整 Lite 回归、打开视频/音视频输出检查、真实启动到首帧计时、HDR/空间音频/RTX VSR 与 D3D11 实机性能对比；以及上游 API20 与本地 VS 定制模块的源码移植、编译与交付。不要将新上游 PR 已合并误记为本项目核心已升级。

追加：上游PR22已合并到固定内核并编译，按用户要求未运行测试。此前API20运行证据来自PR22合入前；最终验收边界见 [续修记录](update-20261016.md)。
