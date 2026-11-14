# 两轮色彩审查修复与上游草稿（2026-11-14）

## 范围

依据 [实际复现审查](color-review-20261114.md) 和 [另一轮静态审查](review-3fp-color-20261114.md) 修复确认的问题。用户已明确选择：向 `Lake1059/FFF_Project` 提交上一轮可独立上游化的基础色彩修正和本轮修复，VSP 专属帧桥、脚本及构建修复留在本地。

保留本库所有无关工作区修改；上游草稿仅包含 Native 色彩源码与配套生成字节码，不含 README、本地测试数据、开发记录、Qt、VS接口、OpenNGU或音频扩展。

## 已修复

| 问题 | 改动及验证 |
| --- | --- |
| Gray8有限范围强制转Gray16误标全范围 | `PlayerSession.cpp` 按明确范围保留Gray输入/输出标签；有限/全范围×黑位/中间值/白位6组验证 |
| VPY future声明被预加载前缀破坏 | `VapourSynthFrameServer.cpp` 在同一VSScript上下文独立初始化插件，再按原文本执行用户脚本；真实VS加载回归 |
| 未知YUV矩阵绕过CPU路径检查 | `VideoRenderer.cpp` 将支持检查覆盖packed YUV；实际YUYV422＋chroma-derived-nc输入明确失败，不再静默显示 |
| 旧Native降级静默丢新metadata | `ThreeFpApi.cpp` 对非默认PTS、SAR分子/分母、隔行、HDR10+流标记返回NotSupported；旧DLL五组验证，普通v1帧仍通过 |
| Native外部帧ABI布局未锁定 | Native头文件锁定v1前缀112字节、v2结构240字节；完整MSVC重建验证 |
| 新默认源绑定本机历史SDK目录 | CMake新增`VSR_FFMPEG_ROOT`，源与测试共用；缺SDK配置明确失败、替代SDK路径配置及构建通过 |
| 上游化后HDR10+倒退/重复帧继承未来曲线 | `HdrProcessor`独立保存bitstream帧序，开流/Reset清空，倒退或重复清除旧动态曲线；本帧SEI和注入表仍按既有优先级解析 |

上一轮基础修正经隔离移植到上游 `db940218ab435f2edca247aa6daa0bac350ff387`：显式SDR TRC、原色、CL/YCgCo/FCC/240M、浮点上传和32F缩放纹理、无标签色度默认，以及有流级声明的HDR10+有界间歇保持。移植后独立重建，未复制VSP定制shader字节码。

## 第二份审查中没有照单修改的项目

- **CL系数结论为误报**：报告建议的1.4746/1.8814属于NCL列；CL必须使用按色差符号分段的系数并在线性域求G。依据 [ITU-R BT.2020-2表4](https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.2020-2-201510-I!!PDF-E.pdf)。当前CL路径及独立参考像素通过，不换成NCL公式，也不在求G之前任意裁剪R/B。
- **SMPTE240M公式结论为误报**：4.0线性段、0.0912阈值、1.1115常数及1/0.45逆幂，与 [FFmpeg `trc_smpte240M_inv`](https://github.com/FFmpeg/FFmpeg/blob/master/libavutil/csp.c) 一致。新增纯上游真实文件对照验证。
- **Gamma2.2/2.8** 按显式标签解码，不猜测标签实际代表709；独立文件测试覆盖。
- **未标记FP32不自动推断线性**：无标签不足以证明编码。保留明确的packed FP16/JXR历史推断，显式LINEAR的RGBH/RGBS/Gray浮点已在本地套件验证。
- **普通709直通与sRGB输出策略** 沿用已有约定，不把审查意见扩展成整个显示EOTF策略迁移。未改swapchain格式或显示端HDR元数据策略。
- **HDR10+剥离粘性** 未引入。来源/滤镜改变后无授权恢复旧曲线；本次保持只针对流级明确声明的HDR10+。仅逐帧发现而无流级声明的内容没有扩大为持有场景。
- 两种709/2020传递函数常数各有适用范围，不为统一数值而改动普通709兼容路径。GBR通道顺序保持经两轮核实的正确实现。

## 验证

新增本地定向组：修复前5通过/10失败，修复后15通过/0失败；含init/cleanup。旧DLLmetadata测试使用已保留的前轮DLL，未替换已安装版本。

HDR10+独立CPU探针直接编译生产`HdrProcessor.cpp`与`HdrMetadataParse.cpp`：100→101、100→100、100→10，修复前1通过/2失败，修复后3通过。帧序重置不改变注入查表优先级，提交前独立复审未发现新阻断项。

纯上游对照使用独立构建的原版与PR版DLL、同一组真实FFV1/Matroska源及YUYV422拒绝样本。先按较宽容差完成检查，再按实际10-bit SDR输出收紧到1.5个量化级：原版5通过/12失败，修复版17通过/0失败，最大通道误差0.00052414。最终结果见 `upstream-before-final.json`、`upstream-after-final.json`。不是拿VSP DLL替代上游DLL验收。

最终本地完整回归：色彩58项、帧桥11项、脚本21项全部通过，0失败、0跳过。

最终计数与产物哈希见 `development/evidence/color-review-fix-20261114/summary.json`。构建、测试、生成器及探针日志保存在同目录；完整合成媒体、旧DLL与临时工作文件仍在忽略目录 `build/color-review-fix-20261114/`。

## 构建与交付边界

- 本地 Native 与上游草稿 Native 均使用完整MSVC/IAMF构建；Qt Renderer/Player/Lite与定向测试重新链接。
- 更新本地汇总补丁，验证对固定Native基线干净应用及当前工作树反向检查；配套头文件仍由标准构建脚本复制。
- 开发构建目录使用新Native；没有覆盖用户安装目录、Full发行目录或单文件发行包。
- 未做全量播放器、真实HDR显示器校准、复杂VFR或长视频压力验收。SDR GPU读回与API契约测试不能替代这些验收。
- 未强制格式转换的Gray8既存范围问题不是本次新增转换回归；本轮只修报告已定位的强制Gray格式转换路径，不宣称所有灰阶路径均已完成色准验收。
- 上游草稿的测试结果写入PR页面，不把本地记录、夹具或README修改带进提交。

上游草稿：[Lake1059/FFF_Project#60](https://github.com/Lake1059/FFF_Project/pull/60)，提交 `f39b81dc740f0631f2b1b3d6dad9813b116755ac`，分支 `user-Wing:codex/color-cicp-review-20261114` → `main`。创建后查询确认 `OPEN`、`isDraft=true`，文件列表恰为上述5个Native文件；未合并。GitHub连接器创建时返回403，改用已登录GitHub CLI成功创建。
