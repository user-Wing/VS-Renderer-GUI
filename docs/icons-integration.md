# 图标接入 · 2026-10-02

用户选择：Renderer R3、Player P2、视频 V1、图片 I3。原始AI图片和候选保持不变，所选ICO复制至 `assets/icons/renderer.ico`、`player.ico`、`video.ico`、`image.ico`。

Windows资源：Renderer组101为R3；Player组101为P2、102为V1、103为I3。Qt通过内嵌qrc设置应用窗口图标。资源编译采用配置后的绝对ICO路径，MinGW图标RC不传无关C++头文件路径，解决空格目录的预处理问题；ICO设为资源对象依赖，后续替换会触发重编。

Player注册分别使用 VSPlayer.Video、VSPlayer.Image、VSPlayer.Audio；前两者DefaultIcon及TypeOverlay使用对应图标资源，音频使用应用图标。遵循[微软TypeOverlay机制](https://learn.microsoft.com/zh-cn/windows/win32/shell/thumbnail-providers)，由Windows叠加角标，不直接修改缩略图或替换已有处理程序。不安装新的图片/视频缩略图解码器，某格式能否显示缩略图仍取决于系统已有支持。

已有本程序注册格式在正式Player首次启动时更新图标和候选ProgID，兼容VSPlayer.Media打开命令，不改写UserChoice、不新增未注册的格式。Windows旧统一VSPlayer.Media默认关联可继续打开媒体，但图片专用角标需要在Player设置注册图片格式，并在Windows默认应用中重新选择对应格式的VS Player；新分类关联后视频/图片使用各自标记。

验证：Release两主程序及Player测试构建通过。`build/icons-tests.txt`三项相关专项含初始化/清理5/5、无跳过：Qt多尺寸图标读取、Windows组资源及LoadImage、分类关联/TypeOverlay、旧关联迁移与取消注册、其它应用项保留，以及原有图片/音频关联回归。只运行本轮相关测试。正式两EXE通过独立启动、读取WM_GETICON非空、仅向各自产生的窗口发送WM_CLOSE并退出0；未进行资源管理器缩略图视觉验收。

正式目录 `dist/VS-Renderer-GUI-windows-x64` 只替换两个EXE，旧EXE备份在 `build/icons-backup-20261002-203912`。构建与交付SHA256一致：

- VSRenderer.exe：4E052AB1D55547431F2C7C1B8FF62551C6A5FDA082F853D53F064D325E6E22F4
- vs-player.exe：E53B8CA17D5DF63B8B38237C26B331DC5075A1EEAAF284E0319786B7FC6435A0

原有INI、VPY及两份FFF.Native.dll共20项校验值保持；没有全量stage、修改LAV或发布压缩包，版本保持1.0.3。
