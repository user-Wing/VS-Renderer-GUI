# BD 识别、播放与一键 Remux

2026-10-05，本地 1.0.5 开发记录。适用于已解密 BDMV；不包含 AACS 解密、BD-J 菜单或在线发行。

## 操作入口

Renderer 左侧打开 **BD Remux**。选择光盘根目录、`BDMV` 或包含多卷光盘的上级目录；程序在后台查找 `PLAYLIST/*.mpls`、`STREAM/*.m2ts`、`CLIPINF/*.clpi`。选择“装载光盘镜像”时先调用 Windows `Mount-DiskImage`，再读取返回的盘符根目录。镜像保持装载，使用完可在资源管理器中弹出。

默认按 MPLS 中的播放顺序、PlayItem 起止时间、章节与片段匹配节目。同一条 MPLS 包含多个 15–45 分钟 PlayItem 时，提供各集候选和完整节目；重复播放列表去重，花絮及疑似循环菜单不默认勾选。集号按卷目录自然顺序推断，需核对后编辑名称，不把推断当作官方剧集标题。

勾选需要的节目，双击时长列或点击“播放所选节目”预览。指定输出目录，点击“一键 Remux 所选节目”。默认查找便携目录、PATH 及 `C:\PortableSoft\Mkvtoolnix\mkvmerge.exe`，也可以手动指定。队列使用 MKVToolNix 保留全部音视频、字幕与章节，不重新编码；输出先写 `.partial.mkv`，成功后改为 `.mkv`。工具警告会留在日志，失败或停止保留未完成文件，已有目标不会覆盖。

Player 右键 **打开 BD 文件夹…** 或 **打开 BD 光盘镜像…** 后，直接后台读取节目并加入右侧播放列表，可以双击切换；Player 不打开 Remux 窗口，Remux/模板选择位于 Renderer。BD 通过 3FP 直通播放；MPLS 当前不进入 VPY 补帧链。

## 结构与路径

`BlurayCatalog` 有界读取 MPLS0100/0200/0300，核对长度、引用、片段 ID 和时间范围。播放与导出共用一个节目描述。拆集时在用户本地缓存中写入仅包含所选 PlayItem 的小 MPLS，重映射章节引用；使用 Windows 目录 junction 指向原始 STREAM/CLIPINF，不复制大视频，也不修改光盘源。

通常每盘都有标准 BDMV 目录，发行差异主要体现为“一集一条播放列表”“整卷一条播放列表”“正片与花絮混排”，无需为每个作品硬编码。多角度或音视频副播放路径保留完整原始 MPLS，不自动拆分；交互菜单图形副路径在普通拆集时移除。没有 MPLS 时可手动添加 M2TS，但这种方式没有原始章节和裁切边界。

P: 网络挂载可以读取目录及小元数据，但当前 junction/底层读取不保证这种远程挂载能直接播放。若加载失败，选择“通过 ModelScope Manager 下载 BD”，指定 Manager 程序目录和本地下载目录。支持挂载的 `ARXChem/Animations-List` 路径映射，使用其下载引擎后台下载完整盘；日志为 `bd-download.log`，断点状态为 `.download-state`。完成后重新打开本地目录。脚本也可独立运行，关闭 GUI 不会停止已启动下载。

## 自定义模式与反馈

勾选、命名正确节目后，“保存所选模式”导出 JSON；下一次选择“自定义 JSON 模板”并加载。PlayItem 从 0 编号，`itemCount` 可指定连续多个片段，保留拼接顺序。示例：

```json
{
  "schemaVersion": 1,
  "entries": [
    {"discContains": "Vol1", "playlist": "00000.mpls", "firstItem": 2, "itemCount": 1, "label": "第 3 集"}
  ]
}
```

也可用 `minimumMinutes`、`maximumMinutes`、`firstEpisode`、`splitPlayItems` 调整自动模式。“导出结构反馈”保存节目清单、错误和 MPLS/CLPI 小元数据，最多约 8 MiB，不包含视频；可据反馈分析新的发行布局，再添加模板。不能从结构可靠推断的多角度、伪播放列表或 BD-J 动态顺序仍需人工确认。

## 已验证与边界

- MyGO 上卷：识别七集候选及完整节目；Bloom Into You Vol1：识别三集候选，28:55 特典未默认选择。不是完整 BD 类型覆盖。
- 另通过 Manager 下载 Bloom 全四卷 190 份小元数据，识别全部 13 集候选；这一参考目录只有元数据，不用于视频播放验收。
- MPLS 截断/坏偏移/路径穿越、章节切片、模板、菜单与副路径分支、真实目录和界面均有测试。
- Bloom 第三集已完成真实 MKVToolNix remux：7,541,879,081 字节，AVC 视频、两条日语 PCM 音轨、五个输出章节，时长约 23:42；输出后段通过 FFmpeg 解码检查。
- Player 使用该完整片段的 MPLS 直接播放与跳转验收通过。MPLS 原始章节末尾标记与 MKVToolNix 最终输出章节数可能不同。
- 后续 GUI 按钮驱动第一集实际输出 7,537,297,909 字节，三条轨道与章节保留，`.partial.mkv` 正常转为最终文件。BD 专项测试 9 项通过；第一/第三集播放、跳转、下一集，以及新增 Native 对既有音频时钟的回归通过。
- 本轮参考盘下载遇到 CDN 超时/断连；只有通过大小与 SHA 检查的文件按完整处理。不能把本地部分大文件当作已下载完整盘；缺失/空视频在列表中提示并不默认勾选。
- ISO 装载入口已实现，但未使用真实镜像验收；加密盘、3 分钟以下 MPLS（FFmpeg/libbluray 默认短节目筛选）、多角度选择、BD-J、伪播放列表混淆和远程直读未完成支持验证。短片可手动添加 M2TS 或先 remux。

源码入口：`src/bluray/`、`PlayerPlaylist.cpp`、`PlayerWindow.cpp`。原生 `bluray:` 输入补丁在 `patches/3fp-bluray-input.patch`，由 `tools/build-3fp.ps1` 应用；下载桥接在 `tools/download-bd.py`，没有修改 ModelScope Manager 仓库。

## 本地交付

已覆盖 `dist/VS-Renderer-GUI-windows-x64` 的 Renderer、Player、Native、QtConcurrent 和下载脚本，包含另一会话本轮交互/音频时钟源码；没有推送、打包或发布。19 个 INI/VPY/color.md 文件部署前后 SHA256 相同。旧程序与审计记录在 `build/bd-deploy-backup-20261005`；新 Native SHA256 为 `FF49CFC45B22F073E079E85878E0C19BF23AC2E02263B1FCE375FD2BFC610719`。已有缓存未执行清理。

格式/工具依据：[MKVToolNix 文档](https://mkvtoolnix.download/doc/mkvmerge.html)、[libbluray MPLS 解析](https://raw.githubusercontent.com/ShiftMediaProject/libbluray/master/src/libbluray/bdnav/mpls_parse.c)、[FFmpeg bluray 协议](https://raw.githubusercontent.com/FFmpeg/FFmpeg/master/libavformat/bluray.c)。
