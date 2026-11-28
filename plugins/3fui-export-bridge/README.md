# 3FUI → Renderer 导出

本插件使用 3FUI v6 的参数面板命令生成接口。界面通过宿主的 LakeUI 5.110 控件绘制。

1. 用 .NET 10 SDK 构建：`dotnet build VsrExportBridge.vbproj -c Release`。
2. 将输出的 `VsrExportBridge.3fui.dll` 放入 3FUI 程序旁的 `plugin` 目录，然后重启 3FUI。
3. 启动 Renderer。在 3FUI 参数面板配置完成后，从「VS Renderer 导出」页点击「发送到 VS Renderer」，导出窗口会自动打开。
4. 在 Renderer 检查接收的 FFmpeg 命令、输入输出和历史记录，再手动开始导出。

同时打开多个 Renderer 时，由先取得接收权的实例接收。该实例退出后，另一实例会接管。
