# PhotoCraft 原生图像编辑器接入（1.0.8，本地）

日期：2026-10-07。上游为 [storytold/photocraft](https://github.com/storytold/photocraft)，固定提交 `7eb3b2e072aa3110da2a329bb49109744292a87b`，版本 0.2.0；源码已拉到 `.deps/photocraft`。本轮沿用 1.0.8，源码更新 GitHub 并提供 Full/Lite 压缩包，正式发布由维护者完成。

## 接入与使用

播放器图片工具栏的首项“PhotoCraft 图像编辑器”进入上游原生 Rust/egui 工作区，保留其完整菜单、工具组、参数栏、文档标签、图层、文字、滤镜、历史和保存流程。旧 `ImageEditorWindow/Canvas/Tools` 不再链接进生产播放器；旧源码仅保留历史与独立核心回归。

`PhotoCraftEditor` 启动独立 GUI 进程，通过当前用户专用的 Windows 命名管道交接绝对文件路径；跨文件夹、盘符及中文路径无需复制原文件到统一工作目录。上游原有 TCP 自动化权限规则不变，普通播放器启动不启用 TCP 控制服务。重复打开已加载或正在加载的同一路径时切换到对应标签。

编辑进程通过 `QProcess::startDetached` 独立运行。关闭 VSP 会关闭管道，但不会关闭编辑器或丢弃未保存的工作；播放器视频、音频、渲染链均不被编辑器调用。

“文件 → 另存为”会把当前文档关联到新路径，之后重新打开原路径属于另一个文档。请选择新文件名以保留原图。PhotoCraft 自身使用原子保存；具体格式与保存警告沿用上游。

VSP 图片查看栏保留旋转、镜像、回收站、桌面背景和高精度导出/格式转换，原裁剪、尺寸编辑对话框由 PhotoCraft 接管。编辑器的“自动”界面语言跟随 VSP 基本设置；PhotoCraft 内部显式选择的语言仍由其自身偏好管理。

## 高精度数据

- 支持的普通图片、PSD/PSB 和 RAW 直接交给 PhotoCraft 读取，保持原始格式、位深与图层。
- 普通图片入口等待查看器解码完成，以识别 HDR 浮点附件；PSD/RAW 不依赖查看器代理，查看器的临时旋转不会使其图层或相机数据扁平化。
- VSP 已旋转/镜像的视图、AVIF/JXL 等原生编辑器暂不支持的编码，以及带浮点附件的 PNG，通过无损高精度副本传入；不是屏幕截图或 8 位显示代理。
- 整数视图使用原有 RGBA16 PNG 写出器，自适应滤波与 libdeflate 最高无损压缩；浮点视图使用已有 `ImageHdr::exportFloatTiff`，保留 float32、ICC、负值、大于 1 的数值及非预乘透明度。
- 交换副本位于 VSP 用户缓存下的 `photocraft-imports`，文件名含 `VSP-view-copy`，需要在 PhotoCraft 中另存为；缓存和编辑器用户状态不属于发行包。
- VSP 原有源/实画面高精度截图、HDR 浮点附件和 SDR 显示回退保留。数值往返通过不代表所有显示器、HDR 显示路径或全部上游滤镜的色准已经验收。
- 超大图使用原有 VSP 解码器及 32 MiB 分块 RGBA8/16/float32 交换，支持超过 32768 像素的宽度；真实 48000×32000 PNG 完整尺寸导入、局部像素比较、裁剪和撤销通过。超预算画布主动使用 CPU 预览，100% 缩放只绘制原始像素可见区域，避免整张 GPU 纹理分配失败。
- 大图文档仍占用对应像素内存，分块交换不等于磁盘交换；当前交换入口上限为 16 GiB 像素数据。

## 构建、许可与体积

可复现入口为 `tools/build-photocraft.ps1`，固定信息在 `tools/photocraft-source.json`，改动在 `patches/photocraft-vsp-integration.patch`。脚本检查源码提交和补丁，不重置已有不同提交或冲突修改。

```powershell
./tools/build-photocraft.ps1
cmake -S . -B build/mingw-release
cmake --build build/mingw-release --target VSRenderer VSPlayer
```

构建需要 Rust MSVC 工具链（上游最低 1.95）和 Visual Studio C++ Build Tools；便携版用户无需安装 Rust。运行时仅带 GUI、所需 x64 MSVC CRT 及许可文件，不带 CLI、Cargo、编译缓存或可选 craft-fonts 字体库。中文字体使用系统字体。`vsp-release` 继承上游体积优化配置，像素处理包保留原有高优化等级，原生平台仍使用正常的 panic unwinding。

上游软件采用 MIT OR Apache-2.0，本接入选择 MIT；保留两份上游许可、NOTICE、ATTRIBUTION，以及字体、图标、词典的独立许可。PhotoCraft 应用图标自身为 MIT OR Apache-2.0；`docs/brand` 的非开源 ArtCraft 商标图形不进入运行时。MSVC CRT 是独立的 Microsoft 再分发组件，见 `RUNTIME-NOTICE.txt`。

使用更严格的十进制 **45,000,000 字节、解压后净增量** 作为预算。原生组件（GUI、CRT、许可和固定信息）合计 **40,337,255 字节**，旧 Qt 编辑器移除后，最终本地便携安装净增量约 **39.69 MB**。最终安装净增量、哈希和保护文件校验见本地 `build/photocraft-1.0.8/size-audit.json` 与 `deployment.json`。打包脚本对组件执行 45 MB 检查，Full/Lite 都携带编辑器，排除 `PhotoCraftData`。

## 验收

`TestPhotoCraft` 使用真实原生编辑进程和 VSP 图片工具栏入口，覆盖中文路径、加载中及已加载文档复用、多文档切换、RGBA16 PNG/ICC 保存、旋转后的 float32 TIFF 交换、画笔、文字、PSD 保存与重新打开、裁剪及撤销。另一个播放器测试进程退出后，编辑器仍能响应操作，验证独立生命周期。Rust 的管道路径解析及 OS 文档事件回归通过，共享 UI 的 wasm32 编译检查及工作区格式检查通过。

已检查原生 GUI 截图：菜单、左侧工具、文档标签、右侧属性/图层及简体中文显示正常。测试浮点像素为 `[-0.25, 12.5, 0.50001, 0.4]`；32 位 TIFF 往返保留 ICC，逐通道浮点误差为零，负值、高光及透明度均未截断。

24 MP（6000×4000、RGBA16）合成图片原生打开实测约 **0.807–1.24 秒**，双三次缩至 3000×2000 约 **129–339 ms**。这是本机合成样本与当前负载的结果，不能外推为所有大 PSD、RAW 或复杂滤镜的性能。

本地证据目录为 `build/photocraft-1.0.8`，包含原生 UI、16/32 位往返文件、浮点检查、Qt/Rust 回归和构建日志。旧本地 PSD 核心测试从仓库根目录运行；CTest 默认目录会让两个既有 `build/...` 证据复制断言失败。大型个人 PSD 样本回归需要 `VSR_PSD_SAMPLE`，本轮未提供该样本。

PhotoCraft 仍属早期项目，完整接入其 UI 与功能不等于承诺 Photoshop 全部兼容。未修改上游滤镜算法、RAW 解码或 PSD 引擎；已有高级格式兼容边界继续适用。
