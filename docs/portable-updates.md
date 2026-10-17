# 安装、更新、部署与打包

## 用户更新

Renderer 设置或 Player“设置 → 基本设置”中检查更新。完整更新读取 ModelScope `ARXChem/Software-List` 的 `VS-GUI` 目录，按数字比较版本；同版本/旧版本不自动安装。

下载使用随包 aria2，完成大小与 SHA-256 校验、解压和必需文件检查后才允许安装。关闭更新窗可取消准备；失败时保留原目录。安装器等待同目录程序退出，不强杀正在播放的其它窗口。

安装在同级准备新目录，将旧目录保留为 `.previous-标识` 备份；保留顶层 INI、自定义 VPY、项目和截图，内置 VPY 随新版更新，旧缓存不直接沿用。备份不自动删除；确认新版本正常后再自行处理。更新失败先检查 `update.log`。

## 包形式与 FFmpeg

Full、当前 PlayerLite 和早期 Full 删减 Lite 的范围见 [README](../README.md)。不要把 Full 更新包覆盖到 PlayerLite；独立单文件按其分发方式替换。

单文件 `VS-Player.exe` 首次运行展开到 `%LOCALAPPDATA%/VSPlayerLite/runtime/<payload-hash>/`，`player.ini` 留在启动器旁。它内置 Qt/Native 等播放器依赖，但 FFmpeg 从 EXE 同目录或 PATH 查找。需要：

- `ffmpeg.exe`、`ffprobe.exe`；
- 匹配的 shared x64 DLL：`avcodec-63`、`avformat-63`、`avutil-61`、`swresample-7`、`swscale-10`、`avfilter-12` 及其依赖。

只有静态版 `ffmpeg.exe` 不够。缺少时按启动提示补齐同一套组件，不混搭不同 major ABI。单文件是自动展开的分发形式，需可写的配置与缓存目录。

## 维护者部署

当前 Full 固定为 `dist/VS-GUI-Full`，目录式 PlayerLite 为 `dist/1.0.12-PlayerLite`。`tools/deploy-local-runtime.py` 面向已存在的这两套运行目录，更新产物、备份旧文件并核对 INI/VPY/ICC/LUT 哈希；执行前先读脚本里的源目录与目标。`tools/stage-portable.ps1` 是较早的环境组装入口，默认目录和本机路径也必须显式核对。

只同步当前 guides 和许可，不部署 `development/` 历史测试数据。旧运行目录可能已有历史报告，本轮脚本调整不会自动清理这些既存文件；清理须另行核对，不能删除配置或媒体。`aria2-next.exe` 是有效依赖，不能按 `-next` 后缀一并清除。

## 打包与交付

1. 按 [构建指南](dependencies.md) 完成相关构建/测试，核对最终产物哈希和配置保护。
2. Full 可用 `tools/package-portable.ps1 -ProgramDirectory 'dist/VS-GUI-Full' -Edition Full`。该脚本的 `-Edition Lite` 是历史删减 Full，仍要求 PhotoCraft，不能用来生成当前 PlayerLite。
3. 当前 PlayerLite 的单文件构建使用 `tools/package-playerlite-single.ps1`，参数以脚本头部为准；目录式 PlayerLite 从其专用目标部署。不同版本的专用脚本不要互换。
4. 发布包使用干净暂存，排除 INI、用户 VPY、缓存、截图、测试程序和日志，包含许可证及资源哈希。兼容旧更新器时保持单个带版本号顶层目录；新更新器可识别唯一完整目录，拒绝路径穿越、链接和多候选。
5. 检查压缩完整性、独立解压后的清单/版本/依赖、主程序启动与配置保留。源码推送、Release 草稿、附件上传和正式发布分别确认，不由版本号或构建成功推断。

本地开发记录、测试数据和 README 改动不得混进上游 PR；提交前逐项检查暂存区。运行包可携带用户指南，开发归档留在源码工作区。
