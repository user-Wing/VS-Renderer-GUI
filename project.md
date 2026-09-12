# VS Renderer 项目地图

## 1. 发心

面向了解视频处理概念、但不愿反复编写脚本且需要快速 A/B 验证参数的用户。GUI 负责组织模块、顺序与参数，VapourSynth 负责处理，3FP 负责双路呈现和帧级检查。成功标准是用户能从“打开源”走到“生成并验证脚本、同步比较结果”，并能明确知道缺少哪个插件。非目标：首阶段不实现任意 Python IDE、矩阵表达式可视化编辑器、编码/封装工作站或 3 路以上对比。

## 2. 快速开始

- 环境：Windows 10 22H2+、Qt 6.8+、CMake 3.25+、C++20；运行时需 API 14 补丁版 3FP 与项目固定的 VapourSynth。
- 配置：设置 `QT_ROOT` 后运行 `cmake --preset windows-mingw-release`。
- 构建：`cmake --build --preset windows-mingw-release`。
- 测试：`ctest --preset windows-mingw-release`。
- 已知坑：3FP 必须连同匹配的 FFmpeg DLL；MinGW 通过动态 C ABI 使用 MSVC 构建的 `FFF.Native.dll`；L-SMASH 首次打开媒体会建立 `.lwi` 索引，长视频可能等待数秒。

## 3. 架构决策

| 决策 | 理由 | Anti-choice | 重估条件 |
| --- | --- | --- | --- |
| Qt Widgets + C++20 | 原生 HWND、紧凑桌面布局、3FP C ABI 接入直接 | 不采用 QML：首阶段不需要动画/触控场景图 | UI 需要高度动画化或跨端 |
| 3FP 运行时动态加载 | 不绑定 MSVC/MinGW import library，缺失可诊断 | 不静态链接 fork：构建和许可分发成本过高 | 内核提供稳定 CMake 包 |
| 滤镜定义在 C++ catalog，图仅存 ID+参数 | MVP 类型安全且容易测试 | 不先造通用 DSL/插件系统 | 外部作者需要无编译扩展 catalog |
| 生成纯 VPY，由专用线程动态调用 VSScript | 脚本透明；随机帧请求无需重启管道；GUI 不阻塞在 VS 计算 | 不用 vspipe/Y4M 作为实时桥：seek 和积压控制较差 | VS 提供稳定的跨进程随机帧协议 |
| 左路 3FP 为唯一时钟，按源/输出 clip 时间轴换算绝对帧 | 3FP 时间 seek 后的 `frameIndex` 是局部 PTS 索引，不能代表源绝对帧；按呈现时间和两端 FPS 映射也兼容倍帧/抽帧 | 不让两个播放器各自计时，不直接用 3FP 局部帧号驱动 VS | 需要完整 VFR timecode 映射或独立处理后音频预听 |

## 4. 目录地图

```text
src/app/       主窗口与应用样式
src/backend/   3FP 动态 C ABI、双路播放协调、运行时探测
src/graph/     滤镜 catalog、图状态、VPY 生成
src/ui/        预览表面、可折叠侧栏与参数编辑器
tests/         无 UI 的图与脚本生成测试
docs/          UI 规格、滤镜覆盖和后端桥设计
tools/         依赖/构建诊断脚本
patches/       可应用到 FFF Project 的 3FP C ABI 扩展
third_party/   官方 VapourSynth Git 工作树；其他二进制不入库
```

## 5. UI 地图

当前只有主工作台：顶部命令栏 → 左侧三段可折叠 dashboard → 中央源/处理后双路视图 → 底部共享时间轴与播放控制。入口为 `src/app/MainWindow.cpp`，尺寸和遮挡约束见 `docs/ui-layout-spec.md`。

## 6. 外部依赖

| 依赖 | 版本/来源 | 调用协议 | 缺失时降级 |
| --- | --- | --- | --- |
| Qt | 6.8+；本机 6.10.2 | Widgets/Win32 HWND | 无法配置构建 |
| VapourSynth | 官方 Git `master`；当前 `5b2d556` / R80RC2 | `QLibrary` 加载 VSScript 4.4，VS API 4.3 按帧读取 | 仍可编辑/导出脚本 |
| 3FP / FFF.Native | FFF Project + `patches/3fp-vsrenderer-extensions.patch` | API 14 `FFF3FP_SubmitExternalVideoFrame` | 显示诊断占位，不崩溃 |
| FFmpeg | 与 3FP ABI 匹配的 shared build | 由 3FP 内部加载；桥接进程使用 CLI | 禁用播放/桥接 |
| VS 插件 | LSMASHSource、FFMS2、fmtc、RemoveGrain、AddGrain、VSZip、nlm-ispc、CAS、Zsmooth、Deblock、ZNEDI3、EEDI3、SangNom、Bwdif、VIVTC | 随便携运行时部署的 API 4 插件 | 启动自检失败并阻止预览 |

## 7. 核心数据

`FilterGraph` 保存有序 `FilterNode{id, enabled, parameters}`。参数值使用 Qt 基础类型，写入项目 JSON 时保留 catalog schema 版本。生成脚本不覆盖用户文件，预览脚本写入应用缓存目录；用户显式“导出 VPY”才写目标路径。迁移规则：未知节点原样保留但禁用，参数新增取 catalog 默认值，删除参数不输出。

## 8. 代码拆解

依赖方向固定为 `ui/app -> graph/backend`，`graph` 不依赖 UI，`backend` 不依赖 graph。单文件接近 500 行时检查职责；3FP ABI 声明虽可能偏长，但只承担协议镜像，不按行数硬拆。高风险区是实时帧桥、3FP ABI 版本变化和 seek 重建，不与 MainWindow 混写。

## 9. 更新与发布

使用 CMake Presets 构建，测试先行覆盖 VPY 转义、顺序与参数边界。结构、依赖 commit 或关键决策变更当天同步本文件。发布前运行 CTest、启动冒烟、`windeployqt`，再打包与 3FP 对应的 FFmpeg DLL；不得混用其他 major ABI。

## 10. 当前状态

- 版本：`1.0.0`。
- 已完成：项目基线、UI 规格、VapourSynth Git 源码固定、内置便携 Python/VS/API 4 插件、滤镜图/VPY 生成、教程向 7 个插件扩展、API 14 外部帧桥、左右同帧预览、源像素密度感知缩放、缩放拖拽与全局空格播放。
- 下一步：扩充 VCB 全量 catalog、插件管理与 NGU 等独立高级缩放后端。
- 限制：当前帧桥支持常用整数 planar 格式，不支持 VS float/GPU-resident frame；NGU 属于独立 GPU 推理实现，不能把名称映射到现有 3FP shader 后伪称支持；处理后音频未接入，左路始终是音频时钟。

## 11. 故障排查

| 症状 | 原因 / 处理 |
| --- | --- |
| CMake 找不到 Qt | 设置 `CMAKE_PREFIX_PATH` 或使用仓库 preset 中的本机 Qt 路径 |
| UI 启动但预览为诊断页 | 将补丁版 `FFF.Native.dll` 与匹配 FFmpeg DLL 放到 exe 旁，或设置 `VSR_3FP_DLL` |
| `Create` 返回失败 | 检查 3FP API 版本与 FFmpeg DLL major，查看状态栏的原生错误 |
| `VapourSynth 未加载` | 当前程序包不完整；确认 exe 旁存在完整 `runtime/python`，开发构建可重跑 `tools/build-vapoursynth.ps1` |
| VPY 执行缺插件 | 当前程序包不完整或混入了旧 API 3 插件；重新 staging 内置运行时，不要求终端用户手动安装 |
| 3FP 报缺少外部帧 ABI | 对 FFF Project 应用仓库 patch 并重建；程序要求 API 14 |
| 处理顺序不对 | 在“处理链”中拖动节点；脚本严格按列表自上而下生成 |
