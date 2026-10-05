# 1.0.6 双便携包验收

核对日期：2026-10-05。发布说明在 [release-1.0.6.md](release-1.0.6.md)。本记录在压缩包完成后生成，作为独立交付凭据，不写回已经验收的压缩包。

| 包 | 压缩字节数 | 解压文件数 | 清单条目数 | SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `1.0.6.7z` | 286420323 | 1063 | 1062 | `7bd0c9fca35ac4f14663af8ee30c1e9bb1701005daf058e982a51ad1abdf9894` |
| `1.0.6-Lite.7z` | 237834827 | 768 | 767 | `57a57cd6acdd9480208f87b1be6c785a93f28de94a6e8cd05ad7b77440cf1aa2` |

两包采用标准 7z / LZMA2，均附同名 `.sha256` 文件。根目录均为 `VS-Renderer-GUI-1.0.6-windows-x64`，兼容已有客户端的版本根目录识别；`release.json` 分别标记 Full / Lite。

完整包包含 LAV Filters 0.83.0、madVR 0.92.17、MKVToolNix 102.0。精简包的 `LAVFilters64`、`madVR09217`、`runtime/mkvtoolnix` 及对应 `components.json` 安装版本记录均不存在；其余核心运行时和 BD 菜单的 libVLC 保留。

## 核验

- 两个压缩包的 7z 完整性测试及独立解压通过。
- 对解压目录使用包含隐藏文件的全量枚举；逐文件 SHA-256 与内部清单一致，清单仅排除自身。
- 无个人 INI、用户 VPY、媒体索引、缓存、日志、调试符号、测试程序或 AWJ 更新状态。保留 MKVToolNix 自带 `data/portable-app` 标记及资源；它们属于组件运行文件。
- 两包各含 16 个由本轮 Player 在独立目录重新生成的内置 VPY。
- 两包的 Renderer、Player 均移除开发工具链 PATH 及 Qt 插件环境后启动，找到 Qt 主窗口并正常关闭，退出码均为 0。
- 各自便携 Python 的 VapourSynth / FFMS2 及 libVLC 动态加载通过；识别 Python 3.15.0rc1、VapourSynth R80、libVLC 3.0.23，并核对 BD 插件存在。
- 打包前后原安装目录 1181 个文件的哈希全部一致；启动验收临时抑制测试路径的文件关联重注册，并还原原图标值。
- 验收数据：`build/package-verification-1.0.6.json`；构建回归：`build/hw-queue-20261005/final-regression.txt`（30 项）及 `deployed-smoke.txt`（7 项）。

## 核心二进制

两包与已部署、已验收的本地 1.0.6 二进制一致：

| 文件 | SHA-256 |
| --- | --- |
| `vs-player.exe` | `f48eb6a815652dc5e26fa51d34cb48306f451f4ce117663c1c75dd09bc3b7fe8` |
| `VSRenderer.exe` | `cc034c8f4078f621d3f852a6591b976308cf2cf7dcd4069ddb4d3219af5807ff` |
| `FFF.Native.dll` | `f80fc559dafdf255bc3ef3129d699504f477cc44897ae1cad559f27c331e2730` |

RX 6600 同片实机、8K 软件解码和全部补帧模式的持续实时性能仍按专项记录保留未验收边界。此次线上操作范围为源码上传和无附件 Release Draft；附件上传、ModelScope 镜像及正式发布由维护者操作。
