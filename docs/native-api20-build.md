# API 20 原生内核构建

当前 API20 基线已更新到上游 main 的 `922e4b70268fcbc83f0deb06d9e2c1b62e7a33b2`，包含 #24、#25、#27～#29、#31～#35。主要变化是自适应调度、直接上传、硬解试解码回退、HDR 流级判定和自适应音频缓冲，原理与适用边界见 [内核优化](performance-debugging.md#上游内核优化)。

在上述固定提交的独立 checkout 应用 `patches/3fp-api20-vsrenderer-1.0.10.patch`；文件名沿用历史，内容已完整重建，不能再用于旧 ce195eb 基线。不能把 API18 补丁直接应用到 API20，也不能直接使用未移植扩展的上游 DLL。构建脚本输出 `FFF.Native.dll` 到指定目录，现行本地部署从 `build/mingw-release/FFF.Native.dll` 取库。

保留本项目的外部 VS 帧输入、clock-only、输出格式、像素探测、缩放、音频 EQ/tempo、软件解码队列和可选 libplacebo 色彩桥。snapshot、字幕及音频电平结构均按 API20 版本号传递，构建时必须保持 MinGW/MSVC 的配置、snapshot、字幕命令及图层布局一致。上游20移除了旧私有 DV/RPU 测试扩展；本补丁沿用上游公开色彩和元数据路径，不恢复该私有接口。

配置仍为 112 字节，snapshot 增至 464 字节；Qt 镜像同步新增上传耗时、字节数、次数和路径，并固定关键偏移。Jinc 权重使用纹理槽 t8，避免与上游辅助视频层 t5～t7 重叠；修改内嵌 HLSL 后必须运行上游 `tools/generate_shader_bytecode.py` 及 `--sdr-specializations` 重新生成两个字节码头。

1.0.14 按新更新单将 AI 字幕 #36/#37 的 Native 能力并入上述固定 main 的补丁，来源为 #37 head `edf533155727f681eefa771082702bab275bca48`（含 #36）。构建仍从 main `922e4b7` 应用完整补丁，不直接混用功能分支 DLL。音频 tap 在 VSP EQ/atempo 前采集，避免重复采样；Qt 只将有界 PCM 交给外置 worker。模型和 llama-server 不下载、不打包，上游 VB 页面不迁入。音频块 144 字节、tap 配置 16 字节、RTX 状态 44 字节，需与 Native 保持 ABI 一致。

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
