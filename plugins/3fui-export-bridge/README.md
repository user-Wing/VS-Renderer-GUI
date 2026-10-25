# 3FUI → Renderer 导出

本插件面向当前 3FUI v6 参数面板，使用宿主公开的预设与命令生成接口。只读取已经打开的参数面板，不创建宿主窗口，不启动编码。

1. 用 .NET 10 SDK 构建：`dotnet build VsrExportBridge.vbproj -c Release`。
2. 将输出的 `VsrExportBridge.3fui.dll` 放入 3FUI 的插件目录，然后由用户重启 3FUI 加载。不要复制项目依赖或测试文件。
3. 在 Renderer 打开导出窗口；在 3FUI 参数面板配置完成后，点击插件的发送按钮。
4. 在 Renderer 检查接收的 FFmpeg 命令、输入输出和历史记录，再手动开始导出。

本机通过插件编译及 Renderer 命名管道接收测试；未操作正在运行的 3FUI，真实宿主按钮流程仍需用户验收。多个 Renderer 实例同时运行时，接收者为首先占用命名管道的实例。
