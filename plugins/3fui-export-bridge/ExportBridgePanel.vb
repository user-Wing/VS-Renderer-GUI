Imports System.Drawing
Imports System.Windows.Forms
Imports LakeUI

Namespace VsrExportBridge
    Friend NotInheritable Class ExportBridgePanel
        Inherits UserControl

        ' The host supplies the background source through this member.
        Private ReadOnly ModernPanel1 As New ModernPanel
        Friend ReadOnly SendButton As New ModernButton
        Friend ReadOnly StatusLabel As New HtmlColorLabel

        Public Sub New()
            SuspendLayout()
            AutoScaleMode = AutoScaleMode.None
            Dock = DockStyle.Fill
            BackColor = Color.FromArgb(24, 24, 24)
            Font = New Font("Microsoft YaHei UI", 10.0F)
            ForeColor = Color.FromArgb(232, 232, 232)

            ModernPanel1.Name = "ModernPanel1"
            ModernPanel1.Dock = DockStyle.Fill
            ModernPanel1.Margin = Padding.Empty
            ModernPanel1.Padding = New Padding(20, 16, 20, 12)
            ModernPanel1.BackColor = Color.Transparent
            ModernPanel1.BackColor1 = Color.Transparent
            ModernPanel1.BorderSize = 0
            ModernPanel1.BorderRadius = 0

            Dim instructions As New HtmlColorLabel With {
                .Dock = DockStyle.Fill, .Margin = Padding.Empty, .BackColor = Color.Transparent,
                .BackColor1 = Color.Transparent, .BorderSize = 0, .ForeColor = ForeColor, .Font = Font,
                .BackgroundSource = ModernPanel1,
                .Text = "在 3FUI 参数面板完成配置。发送后，在 VS Renderer 的导出设置中核对命令和输出路径。"
            }

            ' Keep the footer in the host's LakeUI render tree.
            Dim footer As New ModernPanel With {
                .Dock = DockStyle.Bottom, .Height = 44, .Padding = New Padding(0, 4, 0, 4),
                .Margin = Padding.Empty, .BackColor = Color.Transparent, .BackColor1 = Color.Transparent,
                .BorderSize = 0, .BorderRadius = 0, .BackgroundSource = ModernPanel1
            }
            StatusLabel.Dock = DockStyle.Fill
            StatusLabel.Margin = Padding.Empty
            StatusLabel.BackColor = Color.Transparent
            StatusLabel.BackColor1 = Color.Transparent
            StatusLabel.BorderSize = 0
            StatusLabel.ForeColor = ForeColor
            StatusLabel.Font = Font
            StatusLabel.BackgroundSource = ModernPanel1
            StatusLabel.TextAlign = HtmlColorLabel.TextAlignEnum.MiddleLeft
            StatusLabel.Text = "就绪"

            SendButton.Text = "发送到 VS Renderer"
            SendButton.Dock = DockStyle.Right
            SendButton.Width = 220
            SendButton.Margin = Padding.Empty
            SendButton.ForeColor = ForeColor
            SendButton.Font = Font
            SendButton.BackgroundSource = ModernPanel1
            SendButton.BorderSize = 0
            SendButton.BorderRadius = 6
            SendButton.BackColor1 = Color.FromArgb(52, 52, 52)
            SendButton.BackColor2 = SendButton.BackColor1
            SendButton.HoverBackColor1 = Color.FromArgb(68, 68, 68)
            SendButton.HoverBackColor2 = SendButton.HoverBackColor1
            SendButton.PressedBackColor1 = Color.FromArgb(44, 44, 44)
            SendButton.PressedBackColor2 = SendButton.PressedBackColor1
            SendButton.DisabledOverlayColor = Color.FromArgb(32, 0, 0, 0)
            footer.Controls.Add(StatusLabel)
            footer.Controls.Add(SendButton)
            ModernPanel1.Controls.Add(instructions)
            ModernPanel1.Controls.Add(footer)
            Controls.Add(ModernPanel1)
            AutoScaleMode = AutoScaleMode.Dpi
            AutoScaleDimensions = New SizeF(96, 96)
            ResumeLayout(True)
        End Sub
    End Class
End Namespace
