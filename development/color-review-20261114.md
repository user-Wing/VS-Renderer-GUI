# 上一轮 3FP 色彩更新审查（2026-11-14）

后续已按用户授权修复并准备上游草稿，见 [修复记录](color-review-fix-20261114.md)。本报告保留审查时点的原始结论及失败证据。

## 结论与范围

确认 **3 项 P2 问题**：有限范围 Gray 强制格式转换发生回归、合法用户 VPY 被预加载前缀破坏、未知矩阵拒绝检查未覆盖 CPU 转换路径。原有回归第二轮全过，不能据此认定上述边界已覆盖。

本轮仅审查、隔离复现和记录；没有修复生产代码、替换 DLL、部署、提交、推送或创建 PR。保留所有原有未提交改动。README 与 CHANGELOG 没有新增产品行为可记录，因此不新增“已修复”条目。

审查以本库 HEAD `bcab02fd2dbf95910c25986fff183ca17e242b0e` 到当前工作区的色彩相关差异为范围（`git diff HEAD -- <相关文件>`，另纳入两个未跟踪的新源文件）。这批更新尚未形成提交，不能用空的 `HEAD...HEAD` 代替工作区审查。需求依据为 [上一轮实施记录](color-fix-20261114.md)，并遵守其中的 DV 回退、显示端策略与已知限制。

## 上游、源码与产物

- GitHub API 本轮确认 [官方 main](https://github.com/Lake1059/FFF_Project/commit/db940218ab435f2edca247aa6daa0bac350ff387) 为 `db940218ab435f2edca247aa6daa0bac350ff387`。
- 本地固定构建基线为 `248cb9bb0cef91922ab8f88ea7f6ef38ec0e311c`。两提交间 `git diff --stat 248cb9bb db940218 -- FFF.Native` 为空；上游其他 GUI、接口封装与文档差异存在，但没有漏合并这一区间的 Native 色彩修复。
- 已读上游副本 `.deps/fff-color-audit-upstream-20261114`，本轮实现位于 `.deps/fff-color-fix-20261114`；后者 HEAD 为固定基线，汇总补丁的 `git apply --reverse --check` 通过。
- 当前测试目录与 Full 的 `FFF.Native.dll` SHA256 均为 `04EC8B84BBEDA33240A2B8515ED373645DDDC43A2E5BE39990B7C8B6FB202216`。
- 两处 `vsr-source.dll` SHA256 均为 `92CC8443704B553590FE6E4642FF7D0DF1FFAB7C70636EEF57A5256F49AD6C25`。
- 前轮保留的旧集成 DLL 为 `build/color-fix-20261114/FFF.Native.baseline.dll`，SHA256 `E0EB4A76F604DA55F77BE0E196F863A4B5269FC54468748D15B9E252FED2B8C9`。它用于新旧差分，不是纯上游 DLL。

## 规范轴

没有确认需要单独阻断的规范硬违反；未将命名、重复或拆分启发当作硬性规则。旧 VPY 兼容性问题转入下面的行为证据。

另有 **构建可移植性风险，尚未执行干净机器构建**：`CMakeLists.txt:15–24` 只识别 `.deps/fff-player/third_party/ffmpeg`，缺失时静默不生成新默认源；`:394–396` 的色彩测试却无条件引用同一路径。Native 构建脚本允许独立 `DependenciesRoot`，该配置没有传给 Qt 的新目标。应统一 SDK 路径，并对缺少默认功能依赖给出明确诊断；不能把当前机器构建成功推广为干净环境可复现。

## 行为轴：确认问题

### P2：有限范围 Gray 强制转换时被标记为全范围

位置：`.deps/fff-color-fix-20261114/FFF.Native/3FP/Core/PlayerSession.cpp:10305–10308,10316`；持久补丁位置 `patches/3fp-vsp-upstream-20261113.patch:1751–1754`。

前轮承诺支持整数 Gray，并正确传递范围信息。`inputFull` 把所有 `sourceGray` 视为全范围，`outputFull` 对 `targetGray` 做同样处理，随后将输出标签改为 JPEG/full。有限范围 Gray8 转 `gray16le` 时，样本没有完成相应的黑白位展开，显示却按全范围解释。

用同一 Qt/Native API 探针，分别加载旧、新 DLL，输入 `Gray8 / colorRange=1 / transfer=1 / primaries=1`，读取真实 GPU 输出：

| 8-bit 源码值 | 旧 DLL 强制 gray16le | 新 DLL 强制 gray16le |
| --- | ---: | ---: |
| 16 | 0.00000000 | 0.06256109 |
| 128 | 0.50928640 | 0.50146627 |
| 235 | 0.99608994 | 0.92179865 |

结果是黑位抬高、白位压低。旧 DLL 白位也不是理想精确值，因此这里只将新增的范围误释判为回归，不声称旧路径完整色准正确。未强制格式转换的 Gray8 在新旧 DLL 中都把 16 显示为约 0.062745、235 显示为约 0.921569，属于另一个既存范围问题。

建议：按实际输入标签解释 Gray，明确输出范围，并验证转换后的样本与标签一致。不要只修改标签；swscale 的灰阶路径不能假定与 YUV 的范围处理完全相同。

### P2：原生源预加载破坏合法用户 VPY

位置：`src/backend/VapourSynthFrameServer.cpp:302–305`。

前轮承诺“现有用户VPY不重写”“显式L-SMASH/FFMS2仍按原脚本执行”。随包 `vsr-source.dll` 存在时，对所有脚本无条件 prepend 可执行语句。原来合法的首行 `from __future__ import annotations` 被挪到第4行，Python 拒绝编译，甚至尚未执行用户所选源。

最小输入：

```python
from __future__ import annotations
import vapoursynth as vs
vs.core.std.BlankClip(width=64, height=48, length=1).set_output()
```

已验证原脚本 Python 编译成功，且通过生产 `VapourSynthFrameServer` 对象实际执行失败：`SyntaxError: from __future__ imports must occur at the beginning of the file (future-review.vpy, line 4)`。

建议：在同一 VS 上下文的独立初始化步骤加载插件，避免改变用户脚本开头；仅对本项目生成的脚本注入也必须保持 Python 的 future 语法约束。

### P2：未知矩阵拒绝检查遗漏 CPU 转换路径

位置：`.deps/fff-color-fix-20261114/FFF.Native/3FP/Render/VideoRenderer.cpp:7975–7987,8052`；持久补丁位置 `patches/3fp-vsp-upstream-20261113.patch:99021`。

前轮承诺“未实现的显式矩阵/原色/TRC明确拒绝”。新矩阵白名单仅检查 `directYuv`。YUYV422 等进入 layout 0 的 CPU 转换路径只额外排除 CL/YCgCo，其他未知矩阵仍被 `ToSwsColorSpace()` 默认按 601/709 推断。

已生成短 rawvideo/Matroska 文件，先编码、再 copy-remux 写入标签；ffprobe 确认 `pix_fmt=yuyv422 / color_space=chroma-derived-nc / color_transfer=bt709 / color_primaries=smpte432`。当前 DLL 以软件解码打开并 seek 第0帧，探针得到 `opened=1 / state=Ready / sample=1 / presents=2 / error为空`，实际输出约 `(1, 0.666667, 0)`。这证明被承诺拒绝的矩阵仍被接受；不把这个输出值称为独立正确色彩参考。

该兜底来自既有转换路径，本轮修复只覆盖了部分入口，所以属于修复遗漏，而非声称本轮新引入该兜底。

建议：所有实际 YUV 输入共用矩阵支持检查，并覆盖 packed、planar、硬件转换路径；RGB/Gray 不应误套 YUV 检查。

## 验证与边界

| 检查 | 结果 |
| --- | --- |
| 色彩套件首轮 | 44通过、1失败、0跳过；`invalidProfileFallsBackAndRecovers` 即时呈现计数断言失败 |
| 上述 ICC 用例单独复跑 | 3通过、0失败（含init/cleanup） |
| 色彩套件第二轮 | 45通过、0失败、0跳过 |
| 原有帧桥套件 | 11通过、0失败、0跳过 |
| 图脚本套件 | 21通过、0失败、0跳过 |
| 生产对象独立探针 | 复现合法VPY失败、Gray新旧差分、未知矩阵被接受 |
| 补丁核验 | 当前 Native 源码工作树反向应用检查通过 |

所有 QtTest 计数含 init/cleanup。首轮 ICC 失败保留原始记录；未稳定复现，也没有证明其根因，不计为第四个产品缺陷。测试没有覆盖上述3项边界；第二轮全过不撤销独立探针结论。

没有全量播放器回归、纯上游二进制重建、实际 HDR 显示器测量或长视频/VFR验收。首开完整索引、罕见格式拒绝和 DV 底层回退仍按原记录边界处理，不扩大为本轮新缺陷。

持久日志、探针源码及构建脚本见 `development/evidence/color-review-20261114/`；可运行产物和合成媒体位于忽略目录 `build/color-review-20261114/`、`build/mingw-release/vsr_color_review_probe.exe`。构建脚本复用本机现有生产对象，不是独立 SDK 发行工具。证据仅供本地开发，未来上游 PR 不应混入这些文件或 README 修改。

规范轴：0项确认硬违反、1项待验证构建风险。行为轴：3项确认问题，均为P2。
