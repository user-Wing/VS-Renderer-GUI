Imports System.IO
Imports System.IO.Pipes
Imports System.Linq
Imports System.Reflection
Imports System.Text
Imports System.Text.Json
Imports System.Threading
Imports System.Threading.Tasks
Imports System.Windows.Forms

Namespace VsrExportBridge
    Public Class Entry
        Private Shared addPanel As Action(Of String, Control)
        Public Shared Sub SetHost_AddCustomWinformPanel(callback As Action(Of String, Control))
            addPanel = callback
        End Sub
        Public Shared Sub Entry()
            If addPanel Is Nothing Then Throw New InvalidOperationException("宿主未注入面板回调")
            Dim panel As New ExportBridgePanel()
            Dim send = panel.SendButton
            Dim status = panel.StatusLabel
            AddHandler send.Click, Async Sub(sender, args)
                send.Enabled = False
                Try
                    ' Use the existing panel and the host public preset builder.
                    Dim parameterPanel = Application.OpenForms.Cast(Of Form)().SelectMany(Function(form) Descendants(form)).FirstOrDefault(Function(control) control.GetType().Name = "Form_v6_参数面板")
                    If parameterPanel Is Nothing Then Throw New InvalidOperationException("请先在 3FUI 打开并配置参数面板。")
                    Dim manager = parameterPanel.GetType().Assembly.GetTypes().First(Function(type) type.Name = "预设管理_v6")
                    Dim preset = manager.GetMethod("从面板创建预设", BindingFlags.Public Or BindingFlags.Static).Invoke(Nothing, {parameterPanel})
                    Dim command = CStr(manager.GetMethod("生成命令行展示文本", BindingFlags.Public Or BindingFlags.Static).Invoke(Nothing, {preset, "<输入文件>", "<输出文件>"}))
                    Await SendCommand(command)
                    status.Text = "已发送并保存到命令历史，请在 Renderer 检查后导出。"
                Catch ex As OperationCanceledException
                    status.Text = "VS Renderer 未响应。请确认当前版本已启动。"
                Catch ex As Exception
                    status.Text = If(ex.InnerException, ex).Message
                Finally
                    send.Enabled = True
                End Try
            End Sub
            addPanel("VS Renderer 导出", panel)
        End Sub
        Private Shared Iterator Function Descendants(root As Control) As IEnumerable(Of Control)
            Yield root
            For Each child As Control In root.Controls
                For Each item In Descendants(child)
                    Yield item
                Next
            Next
        End Function
        Public Shared Async Function SendCommand(command As String) As Task
            Using timeout As New CancellationTokenSource(TimeSpan.FromSeconds(3)), pipe As New NamedPipeClientStream(".", "VSRenderer.ExportCommand", PipeDirection.InOut, PipeOptions.Asynchronous)
                Await pipe.ConnectAsync(timeout.Token)
                Dim packet = Encoding.UTF8.GetBytes(JsonSerializer.Serialize(New With {.command = command}) & vbLf)
                If packet.Length > 65536 Then Throw New InvalidOperationException("命令超过 64 KiB，无法发送。")
                Await pipe.WriteAsync(packet, timeout.Token)
                Await pipe.FlushAsync(timeout.Token)
                Using reader As New StreamReader(pipe, Encoding.UTF8)
                    Dim response = Await reader.ReadLineAsync(timeout.Token)
                    If response = "ERROR: open the export window first" Then Throw New InvalidOperationException("当前 VS Renderer 版本需要先打开导出窗口。")
                    If response <> "OK" Then Throw New InvalidOperationException("VS Renderer 没有接受这条导出命令。")
                End Using
            End Using
        End Function
    End Class
End Namespace
