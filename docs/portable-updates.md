# 安装、更新、部署与打包

## 用户更新

Renderer 设置或 Player“设置 → 基本设置”中检查更新。完整更新读取 ModelScope `ARXChem/Software-List` 的 `VS-GUI` 目录，按数字比较版本；同版本/旧版本不自动安装。

下载使用随包 aria2，完成大小与 SHA-256 校验、解压和必需文件检查后才允许安装。关闭更新窗可取消准备；失败时保留原目录。安装器先关闭并终止本安装目录内的程序（含VSCap、转换和解码子进程），不会按进程名结束其它安装目录的实例。

安装在同级准备新目录；交换成功前保留旧目录用于失败回退。保留INI、自定义VPY、项目、截图和ICC/LUT，内置VPY随新版更新，旧缓存不直接沿用。成功后显示并重启原程序，删除更新器生成的旧安装备份、已验证下载包和更新暂存目录。更新失败先检查 `update.log`。

## 包形式与 FFmpeg

从1.0.13起只提供Full目录与单文件播放器，范围见 [README](../README.md)。不要把Full更新包覆盖到独立播放器；单文件按其分发方式替换。

1.1.0 单文件每次运行展开到启动器旁的 `.vs-player-run-<payload-hash>-<PID>/`，正常退出后删除；异常退出留下的自有目录在下次启动时清理，仍被运行中子进程持锁的目录保留。`player.ini` 留在启动器旁。它内置 Qt/Native 等播放器依赖，但 FFmpeg 从 EXE 同目录或 PATH 查找。需要：

- `ffmpeg.exe`、`ffprobe.exe`；
- 匹配的 shared x64 DLL：`avcodec-63`、`avformat-63`、`avutil-61`、`swresample-7`、`swscale-10`、`avfilter-12` 及其依赖。

只有静态版 `ffmpeg.exe` 不够。缺少时按启动提示补齐同一套组件，不混搭不同 major ABI。启动器所在目录必须可写，并需要支持 gzip 的 Windows `tar.exe`。当前单文件使用低压缩级别tar.gz，减少每次启动的解包耗时，体积会比此前tar.xz稍大；不保留常驻运行缓存。

## 维护者部署

从 1.0.14 的单文件代码起，启动后检查 GitHub 上游版本；发现可验证的新单文件附件时提示下载，核对大小与 SHA-256 后等待播放器退出，只替换启动器原 EXE 并重启。失败保留旧文件或备份；不使用 Full 更新包、不更新外置 FFmpeg。只有由单文件启动器传入原始路径的进程才启用此检查，普通 Lite 缓存 EXE 不执行替换。

单文件构建工具按CMake版本生成FileVersion/ProductVersion资源。打包后分别核对tar.gz/清单、启动器与子进程、本地配置保留；本地构建和真实网络更新安装的验证结果分别记入测试台账。

本轮继续使用现有`dist/VS-GUI-Full`和`dist/VS-Player.exe`；Full交付压缩包按当前CMake版本命名（本轮`dist/1.1.3.7z`）。不在分发包中预装页面背景或用户themes.ini，主题导入/导出功能保留；已有安装旁的自定义主题不会被删除。部署脚本会修改目标目录，执行前先读源目录与目标；`tools/stage-portable.ps1`是较早的环境组装入口，默认目录和本机路径也必须显式核对。

只同步当前 guides 和许可，不部署 `development/` 历史测试数据。旧运行目录可能已有历史报告，本轮脚本调整不会自动清理这些既存文件；清理须另行核对，不能删除配置或媒体。`aria2-next.exe` 是有效依赖，不能按 `-next` 后缀一并清除。

## Full按需工具

Full的 `runtime/awj.7z` 保存AWJ 1.3.1独立CLI，`runtime/mkvtoolnix.7z` 保存四个MKV命令行工具及许可/英文文档，兼容旧包的同名ZIP。首次实际转换或Remux时，通过随包7-Zip展开到用户缓存的内容哈希目录；后续复用，不在普通播放时展开，也不要求安装目录可写。更新组件后新的归档哈希使用独立缓存，已有的显式外部工具路径仍可使用。AWJ对应源码及许可证在 `runtime/awj-source`，只保留当前二进制对应的1.3.1源码；VLC仅保留BD菜单所需插件，未压缩其运行中加载链。

Full打包时调用 `tools/trim-full-runtime.py`：压缩上述工具、移除未进入超分目录的场景检测模型和Agent类型声明/源码映射，并剥离无签名原生模块的非运行符号。剥离前后校验代码段、导入DLL与导出项；保留签名、托管程序、VS指令集变体和组件许可。详细节省清单写在发行暂存旁的 `runtime-trim.json`，不进入交付包。

## 打包与交付

1. 按 [构建指南](dependencies.md) 完成相关构建/测试，核对最终产物哈希和配置保护。
2. Full 可用 `tools/package-portable.ps1 -ProgramDirectory 'dist/VS-GUI-Full' -Edition Full`。该脚本的 `-Edition Lite` 是历史删减 Full，仍要求 PhotoCraft，不能用来生成当前 PlayerLite。
3. 单文件构建使用`tools/package-playerlite-single.ps1`，默认读取当前版本暂存（本轮`build/playerlite-runtime-1.1.3`），并拒绝版本不匹配的旧暂存；使用Python 3进行gzip level 1压缩，不依赖dist下的PlayerLite目录。首次组装暂存可用`stage-editions-1.0.9.py --edition Lite`（其余依赖参数按实际路径设置）。不同版本的专用脚本不要互换。
4. 发布包使用干净暂存，排除INI、主题背景目录、用户VPY、缓存、截图、测试程序和日志，包含许可证及重新生成的资源哈希。Full包含八档缩放和四项超分补帧组合的内置脚本。兼容旧更新器时保持单个带版本号顶层目录；新更新器可识别唯一完整目录，拒绝路径穿越、链接和多候选。
5. 检查压缩完整性、独立解压后的清单/版本/依赖、主程序启动与配置保留。源码推送、Release 草稿、附件上传和正式发布分别确认，不由版本号或构建成功推断。

本地开发记录、测试数据和 README 改动不得混进上游 PR；提交前逐项检查暂存区。运行包可携带用户指南，开发归档留在源码工作区。

本轮1.1.0续修：Full更新在同盘直接将已解压payload重命名到`.update-…`，避免再次复制完整运行时；异盘保留复制路径，配置保留与回滚顺序不变。单文件包现在包含`preset-descriptions.json`默认配置，首次启动写到`player.ini`旁；已存在文件优先，更新不覆盖其手工修改，主题和INI仍不嵌入。
