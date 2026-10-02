# 1.0.3 更新包兼容1.0.2修复

## 原因

ModelScope 的 `VS-GUI/1.0.3.7z` 大小168,399,042字节，SHA-256为 `d3f9239acd66be80fa834c87040eee0050cd9de2d55d1e34386711e237d8fc9e`，与原本地包一致。

使用发布提交61b13a6中的原始1.0.2更新器源码执行实际联网下载，先通过大小与SHA-256校验，再报“更新包路径不安全或格式无效”。它只允许带版本号的根目录 `VS-Renderer-GUI-[0-9.]+-windows-x64`，原1.0.3包的固定根目录不满足此规则。不是上传损坏或哈希算法错误。

## 修复

打包脚本根目录恢复为 `VS-Renderer-GUI-1.0.3-windows-x64`。安装器仍写入已有安装目标，不重命名目录；本地开发目录继续使用固定名称，新版更新器同时支持两种归档根目录。没有放宽路径/哈希验证，没有修改两程序或原生DLL。

修正版：`dist/1.0.3.7z`，大小168,379,908字节，标准LZMA2 9级压缩。

SHA-256：`57de5cc1885958b555eb4454438a4126bd115ab6aa19bd8e689473f1d2c65aa9`。

旧包保存在 `dist/1.0.3-before-update-fix.7z`，未自动上传或删除远端附件。维护者需替换ModelScope `VS-GUI/1.0.3.7z`，同步上传 `.sha256`，并替换GitHub同名附件；旧重命名副本 `VS-Renderer&Player 1.0.3.7z` 仍是错误根目录的旧包，不要用于这次替换。

## 验证

- 原始1.0.2代码：原远端包拒绝；修正版完成下载、SHA-256、路径校验、解压、组件/版本验证，发出prepared信号。仅准备更新，没有执行安装。
- 专项4/4（含初始化/清理）；不运行无关回归。新测试 `releasedPackageCompatibility` 通过 `VSR_TEST_UPDATE_ARCHIVE` 指向真实发布包。
- 7z完整性检查通过；内部312个资源哈希一致；两EXE与FFF.Native.dll与活跃目录完全相同。
- 日志：`build/legacy-update-remote-original.txt`、`build/legacy-update-compatible-prepare.txt`、`build/update-compatible-tests.txt`、`build/update-compatibility-1.0.3-audit.json`。
