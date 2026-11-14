# 色彩问题更新复核（2026-11-14）

## 结论

**部分解决，尚未全部解决。** 上游修正了HLG逐通道OOTF，CPU参考实现与GPU着色器改为共同亮度因子，通用shader字节码也有更新。线性整数RGB、P3→SDR、PQ显式709原色、罕见矩阵兜底及BT.2020 CL仍未关闭。

沿用 [11月7日讨论修正](color-audit-followup-20261107.md) 的边界：不将显示端元数据策略直接判为bug；不把合成PQ+709输入推广为普通HDR10普遍失准；DV主线强制回退是用户要求，不扩展为DV/FEL补全任务。

## 基线与测试对象

- 官方main经fetch及再次ls-remote核实为 `db940218ab435f2edca247aa6daa0bac350ff387`，提交时间2026-11-14 05:18:31 +08:00。
- 上次上游基线为 `72af915`；主要色彩变化来自11月13日的 `248cb9bb0cef91922ab8f88ea7f6ef38ec0e311c`（PR #57）。之后到db94021，`FFF.Native/3FP/Render`与`FFF.Native/3FP/Hdr`没有新增差异。
- 新上游独立检出于 `.deps/fff-color-audit-upstream-20261114`；旧审计副本保留，现有集成工作树未覆盖。
- 当前项目文档已固定248cb9b，并记录11月13日完成Native构建及Full部署。
- 本轮实际读取当前 `dist/VS-GUI-Full/FFF.Native.dll`，SHA256为 `E0EB4A76F604DA55F77BE0E196F863A4B5269FC54468748D15B9E252FED2B8C9`，与 `build/mingw-release/FFF.Native.dll` 相同。
- 未构建纯上游db94021 DLL；源码比较与本地集成DLL实测分别陈述。

## 已有修正

### HLG OOTF

最新 `VideoRenderer.cpp:525`（CPU）和`:700`（HLSL）均先还原场景线性RGB，再按2020亮度权重得到共同亮度因子：

```text
Y = 0.2627 R + 0.6780 G + 0.0593 B
scale = 1000 × max(Y, 0)^0.2
输出 = scale × RGB
```

这修正了上次指出的“分别对三个通道做1.2次幂”问题。仍采用名义1000 nit、gamma=1.2；不能因此宣称任意显示条件下的完整HLG色准已验收。

旧探针输入(180,100,60)/255，当前DLL的SDR输出由旧记录 `(1.00000000,0.32941177,0.14509805)` 变为 `(1.00000000,0.41960788,0.23137257)`。本轮两次运行结果一致。该变化证明本地HLG输出已变化，结合源码支持已接入修正；它不是HDR实屏测量，也不是对整条tone mapping的独立色准证明。

### 其他色彩相关变化

- `HdrProcessor::ProcessFrame`在没有动态元数据接管且类别可重分类时，允许显式HLG/PQ帧标记纠正先前的普通静态分类，不再仅在原类别为SDR时升级。此处没有新增所有SDR TRC到显示光的完整分派，也不能将注释中的“双向”理解为所有HDR→SDR重分类均已实现。
- `ReadSdrWhiteLevelDisplayConfig`在 `QDC_DATABASE_CURRENT` 查询失败后尝试 `QDC_ALL_PATHS`，过滤非活动路径；单目标拒绝查询时继续检查其他目标，改善Windows SDR白位读取的兼容性。
- `BuildDxgiHdr10Metadata`在源最小亮度缺失或非正时使用0.0001 nit下限。这里只记录实现变化，不将元数据字段调整等同物理显示色准通过。

## 仍未关闭的项目

| 项目 | 本轮证据 | 状态 |
| --- | --- | --- |
| 线性整数RGB | 当前DLL中Transfer=8的128/255灰阶仍输出0.50196081，sRGB编码参考约0.73665 | 合成输入仍复现 |
| Display P3→SDR | 709与P3标记的(180,100,60)输入仍输出相同RGB | 合成输入仍复现 |
| PQ显式709原色 | PQ+709与PQ+2020仍同输出(0.51372552,0.20392159,0.06666667)；ResolveSourceGamut仍先识别P3，再将HDR默认归入2020 | 合成输入仍复现，真实来源范围未扩展 |
| 罕见矩阵 | YuvCoefficients仍仅区分2020、709，其余按601系数处理 | 源码未修正，未新增真实码流测试 |
| BT.2020 CL | 色彩套件在TestColor.cpp:109同一差异断言失败 | 当前DLL实测未解决 |
| SDR传递函数归并 | ResolveDecodeTransfer及主映射分支与旧版相同，ToLinear709仍使用旧公式 | 原覆盖缺口仍在 |
| 无标签色度位置 | D3D11默认LEFT，软件路径仍使用原策略 | 代码差异仍在，未做软硬解同码流相位验收 |
| 显示端HDR元数据策略 | SetHdrMetadata与主映射分支仍和旧版相同 | 保留策略边界，不记为新失败 |
| VS桥与ICC缓存 | VS仍读基础CICP；可选ColorBridge仍按monitor/HDR缓存ICC | 未见针对旧问题的补全，实际VS映射损失仍待闭环 |

逐段比较旧/新上游，六组文本保持一致：ResolveSourceGamut、主色彩映射分支、chroma/matrix、SetHdrMetadata、ResolveDecodeTransfer、默认chroma。注意HLG被调函数已经变化，因此“主映射分支文本相同”不代表HLG运行结果不变。

## 本轮验证结果

- 当前Full DLL与测试DLL哈希一致。
- 独立探针连续两轮各20次读回完成；SDR/HDR请求实际都输出 `actual_mode=0 / bits=8 / scaling=0`。未获得HDR输出，不能声称HDR实屏验收。
- 当前色彩Qt套件：**10 passed / 1 failed / 0 skipped**，包含init/cleanup；失败为已知CL。比旧记录增加一项接口/snapshot检查，不表示旧色彩问题减少。
- 可选色彩桥元数据探针通过：203 nit PQ→scRGB为2.5352；HDR10+曲线、合成DV测试及抖动检查通过。这是可选桥能力检查，不是DV主线策略批准或实际内容验收。
- 失败CL所在函数后续PQ/HLG检查仍提前退出，不能拿该套件证明原生HLG已完整回归。
- 未运行全量播放器回归、纯上游二进制测试、物理HDR显示、真实PQ+709素材或mpv端到端对照。

证据位于 `build/color-audit-20261114/`：`pixels.csv`、`color-tests.txt`、`metadata-tests.txt`、`source-comparison.json`。复用探针源位于旧审计目录 `build/color-audit-20261107/probe.cpp`，未改变其输入条件。

本轮只拉取独立上游副本、检查源码、运行隔离测试及记录结论；未修复、部署、提交或发布。

源码参考：[上游db94021](https://github.com/Lake1059/FFF_Project/commit/db940218ab435f2edca247aa6daa0bac350ff387)、[VideoRenderer.cpp](https://github.com/Lake1059/FFF_Project/blob/db940218ab435f2edca247aa6daa0bac350ff387/FFF.Native/3FP/Render/VideoRenderer.cpp)、[HdrProcessor.cpp](https://github.com/Lake1059/FFF_Project/blob/db940218ab435f2edca247aa6daa0bac350ff387/FFF.Native/3FP/Hdr/HdrProcessor.cpp)。
