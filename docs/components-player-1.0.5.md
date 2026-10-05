# Player 播放入口与 Full 组件下载

2026-10-05，1.0.5 Full 本地交付及发行草稿记录。

## BD 与播放器界面

Player 右键“打开 BD 文件夹”直接后台读取 MPLS，选择完整可用的剧集候选，加入右侧播放列表并按自动播放设置打开首集，不显示 Remux 页面。光盘镜像入口先装载再读取根目录；轨道和字幕仍由右键菜单切换。Renderer 的 BD Remux、模板和结构反馈功能保留。

未打开文件时，底部显示可点击的 `3FP-HW` / `3FP-SW`；打开文件后显示实际播放路径。Tab 面板用统一透明背景绘制，左侧“透明度”、右侧百分比，默认 50%；边框与右下角斜线调整手柄保持可见。设置窗口激活时信息面板隐藏，返回播放器恢复，不遮挡设置。

## 组件下载

Player 设置左侧新增“组件下载”，原基本设置的更新按钮迁移到该页面。树形分类展示软件本体、FFmpeg、MKVToolNix、MadVR、LAV-Filters、aria2-next；展开后只展示该组件目录内的 Windows 二进制程序包（7z/zip/exe），排除源码、网页、说明、签名和安装器。列表显示远端版本、本地版本与大小，保留旧版本。

来源为 `ARXChem/Software-List` 的对应目录，API 按目录递归分页读取。aria2-next 可从版本子目录识别版本；普通包按名称版本排序，FFmpeg 当前按包名构建日期，`madVR09217` 识别为 `0.92.17`。开启“自动下载最新版本”会刷新目录，只下载比本地更高的版本；本地组件不存在时仍可下载。读取实际 EXE/AX 版本，FFmpeg 日期版本由 `components.json` 安装记录维护。

当前仓库实测有 FFmpeg、MKVToolNix、`MadVR` 和 `LAV-Filters` 程序包。aria2-next 当前公开子目录只有源码包；页面不会拿源码包替代 EXE。本 Full 包已包含 aria2-next 2.8.3。

| 组件 | 固定安装位置 | 验证入口 |
| --- | --- | --- |
| 软件本体 | 完整便携目录 | release.json、Renderer/Player 与核心依赖 |
| FFmpeg | 程序根目录，CLI 与原生 DLL 共享 | ffmpeg.exe |
| MKVToolNix | runtime/mkvtoolnix | mkvmerge.exe |
| MadVR | madVR09217 | madVR64.ax、madHcCtrl.exe |
| LAV-Filters | LAVFilters64 | LAVVideo.ax、LAVAudio.ax、LAVSplitter.ax |
| aria2-next | runtime/tools | aria2-next.exe |

下载缓存位于用户 CacheLocation 下的 `components`。下载到 `.partial`，按源目录的大小与 SHA-256 校验后转为最终缓存；解压前拒绝路径穿越、链接及不安全名称，解压后递归定位唯一完整的 x64 主程序目录。包装目录名称或层数改变不会改变安装位置；定位不唯一时停止安装。

点击“应用组件并重启”，退出后由 `apply-components.ps1` 按固定目录替换对应文件。要求另一个 Renderer/Player 窗口已关闭；保留 INI、VPY、madVR settings.bin 和更新工具，其余旧文件备份到组件缓存，出错回滚已修改文件。组件安装版本写入 `components.json`。清理缓存只删除下载包及断点文件，保留已安装组件、解压暂存和备份。

旧版本本体可以下载，完成后再次提示“不会降级更新，包已经下载到 …”。“安装已下载的新版本本体”仅接受高于当前版本的完整包，复用既有便携更新器；其它组件允许用户手动下载并安装旧版。新软件本体更新和组件替换均需退出后应用。

## 验收

- Player：BD 直接加入播放列表并出帧、章节、空窗口解码切换、Tab 背景 alpha/底部一致性/百分比/调整大小、设置窗口优先级，以及既有音频时钟聚焦测试通过。
- 更新：数字/日期版本排序、源码过滤、MadVR 压缩版本号、x64 嵌套与重复 EXE 目录识别、固定安装目录、配置保留、禁止组件接口安装软件本体、原完整更新下载/哈希/取消/目录替换回归通过。
- 实际 ModelScope MKVToolNix 102.0 GUI 下载与 SHA/解压通过，暂存后的 mkvmerge --version 返回 102.0；没有将测试包安装覆盖用户其它软件目录。
- Full 包使用验证过的 MKVToolNix 102.0 源包，去除其中 INI 与缓存。现有性能核心保持 `094C19E0658235DAFA0C5B5D46E59C211C9390632C71794AAD45E4DCFE95DE50`；8K48 CPU 回退仍未达到实时目标。

核心入口为 `src/update/ComponentDownloads.*`、`tools/apply-components.ps1`、`tools/stage-mkvtoolnix.ps1`。发布归档排除个人数据、缓存、下载包、日志、测试程序及 AWJ 更新状态；发行附件由维护者上传。

## Full 归档验收

2026-10-05 最终聚焦 Player 回归 5 passed / 0 failed，更新回归 11 passed / 0 failed / 2 个可选实包用例 skipped；发行包格式专项 3 passed / 0 failed。此前真实 MKVToolNix 下载用例已单独通过。

部署前后 27 个配置、VPY 与色彩文件哈希一致。`dist/1.0.5.7z` 为 259894812 字节，7z 完整性、独立解压、682 个资源清单条目逐文件哈希核对通过；包内共 683 文件，保留 MKVToolNix 资源和许可，排除个人与测试状态。清空开发 Qt 插件路径后，解压目录的 Renderer 与 Player 均创建真实 Qt 主窗口并正常关闭，包内 mkvmerge 返回 102.0。

SHA-256：`f2c80dd377e667b5cc64d3a1801219d0d15ec5a5fbbef4bd4f4a93b74a7e7c9d`。旧归档与旧暂存目录已改名保留。归档后的交付记录保存在源码文档中，不修改已审计的发行包。
