# 开发工作区清理记录（2026-10-03）

## 手动清理入口

主目录 `Clean-Workspace.bat`已提供，双击执行，结束保留窗口；`Clean-Workspace.bat --preview`仅预览。BAT只调用tools/clean-workspace.ps1，由PowerShell核对边界/链接、保留INI/VPY、删除固定候选并记录结果，不跨shell拼接删除。Windows PowerShell语法与只读预览已验证，代理未执行删除模式。

最新复查原24个候选已不存在，不归因于代理删除。体积优化新生成的FFmpeg/Shader/裁剪备份和试验目录约2.09 GiB也加入候选。当前Release、便携输出、Git、源码及retained-user-files保留。实际手动结果记录于build/workspace-cleanup-result.json；下面是先前盘点的历史状态。

当前实际文件占用约11.81 GiB，其中build约8.22 GiB、.deps约1.90 GiB、dist约1.14 GiB。已识别24个历史构建、发布副本、解包验证副本、旧FFmpeg备份、重复依赖构建及生成测试素材，合计约7.79 GiB。

计划保留当前 `build/mingw-release`、`dist/VS-Renderer-GUI-windows-x64`、当前依赖源码/工具、Git与全部开发改动。历史副本里的41份INI/VPY已复制到 `build/retained-user-files`，逐文件哈希一致。当前源码、图标、着色器、文档、程序、配置和预设共1171份受保护文件检查无变化。

删除尚未执行：执行策略先后拒绝按清单变量批量删除和明确绝对路径删除，返回 `blocked by policy`，没有更具体原因；未改用其他接口绕过该限制。工作区仍约11.81 GiB，不能视为清理完成。

精确候选路径、清理前大小与受保护文件哈希见 `build/workspace-cleanup-plan.json`；当前状态见 `build/workspace-cleanup-audit.json`。候选中的旧FFmpeg备份尚在，没有失去回退文件；旧测试素材尚在。仅做盘点和小体积配置保留，没有推送或打包。
