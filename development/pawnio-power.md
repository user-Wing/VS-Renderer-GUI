# PawnIO 与 CPU/GPU 功耗采集

核对日期：2026-11-07。本记录区分官方源码观察、集成建议与本机验证。只读探测脚本和结果保存在忽略的 `build/vsp110-followup-20261107/`，不构成产品已接入或全量测试通过。

## 结论与边界

- **官方观察**：PawnIO 是可运行受支持模块的内核驱动，用户态通过 PawnIOLib 装入模块、调用具名函数。官方模块发行页提供签名构建；应用无需自行写驱动或启用测试签名。来源：[官网](https://pawnio.eu/)、[官方模块说明](https://github.com/namazso/PawnIO.Modules)、[使用指南](https://github.com/namazso/PawnIO.Modules/wiki/Using-PawnIO-Modules)。
- **集成建议**：CPU 先实现 package energy 的时间差分；AMD 用 `AMDFamily17.bin`，Intel 用 `IntelMSR.bin`。GPU 优先用厂商驱动提供的 NVML/ADL。两者都是设备整体读数，不能标为播放器进程自身耗电；CPU package 与独立 GPU 也不是整机插座功耗。依据：下文各 API 的测量域。
- **本机观察**：Ryzen 9 7940HX、16 核/32 线程；`Win32_Processor.ProcessorId=178BFBFF00A60F12` 的 CPUID signature 可解出 family 19h / model 61h。AMD 产品页确认为 Dragon Range / Zen 4。CPU 实测在打开 PawnIO 时因权限不足停止；GPU NVML 成功返回读数。来源：[AMD 产品页](https://www.amd.com/en/products/processors/laptop/ryzen/7000-series/amd-ryzen-9-7940hx.html)、本节末探测结果。

## 复用安装与权限

官方指南要求读取 `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\PawnIO` 的 `InstallLocation`，后备为 `%ProgramFiles%\PawnIO`，从该绝对路径动态载入 `PawnIOLib.dll`。应用自身携带官方模块 blob 即可；不要假设驱动安装目录附带所有模块。来源：[官方集成指南](https://github.com/namazso/PawnIO.Modules/wiki/Using-PawnIO-Modules)。

官方 INF 设置设备 ACL 为 `D:P(A;;GA;;;SY)(A;;GA;;;BA)`，授权 SYSTEM 与管理员；**服务已启动不代表普通权限应用可读取**。当前普通权限进程加载 DLL 成功，`pawnio_version` 返回 2.0.0，但 `pawnio_open` 返回 `0x80070005`。本轮没有提权、安装/启动/停止/改写驱动、修改 ACL，也未成功执行模块加载或 MSR 访问。来源：[官方 INF](https://github.com/namazso/PawnIO/blob/master/PawnIO/PawnIO.inf.in)、`build/vsp110-followup-20261107/pawnio-power-probe.json`。

建议：未安装、缺 DLL/模块、权限不足、不支持或读失败时，CPU 显示 `N/A` 并保留具体原因；未安装时提供 [PawnIO 官网下载](https://pawnio.eu/) 入口，不自动安装或弹出提权。不要把 `N/A` 显示为 0 W。接口调用失败后关闭 executor，避免继续用旧能量基线产生假读数。

## C ABI 与只读调用

已安装头文件与当前官方头文件的以下同步 ABI 相同，适合 Qt/MinGW 通过 DLL C 接口使用。`PAWNIOAPI` 展开为 `extern "C" ... HRESULT STDAPICALLTYPE`；函数指针保留 Windows 调用约定。来源：[PawnIOLib.h](https://github.com/namazso/PawnIO/blob/master/PawnIOLib/include/PawnIOLib.h)。

```cpp
HRESULT STDAPICALLTYPE pawnio_version(PULONG version);
HRESULT STDAPICALLTYPE pawnio_open(PHANDLE handle);
HRESULT STDAPICALLTYPE pawnio_load(HANDLE handle, const UCHAR* blob, SIZE_T size);
HRESULT STDAPICALLTYPE pawnio_execute(
    HANDLE handle, PCSTR name, const ULONG64* in, SIZE_T in_size,
    PULONG64 out, SIZE_T out_size, PSIZE_T return_size);
HRESULT STDAPICALLTYPE pawnio_close(HANDLE handle);
```

`pawnio_load` 的 size 是 blob 字节数；`pawnio_execute` 的 in_size/out_size/return_size 是 **64 位元素个数**。读取时 name=`"ioctl_read_msr"`，in[0]=寄存器地址，in_size=out_size=1；验证 `SUCCEEDED(hr)` 和 `return_size==1` 后取 out[0]。`return_size` 传有效指针；实现会写入它。`pawnio_version` 返回的是库版本，不是已运行驱动版本。来源：[库实现](https://github.com/namazso/PawnIO/blob/master/PawnIOLib/PawnIOLib.cpp)。

## 官方模块与能量寄存器

已取得 [0.2.11 官方发行 ZIP](https://github.com/namazso/PawnIO.Modules/releases/tag/0.2.11)，在内存核对 ZIP SHA-256 与官方 API digest 一致，再将原始 `AMDFamily17.bin`、`IntelMSR.bin` 和许可证存入 `assets/pawnio/`；模块大小和 SHA-256 见 [来源说明](../assets/pawnio/NOTICE.md)。这是来源完整性核对，尚未验证本机驱动对 blob 的签名接受情况。

| CPU 路径 | 模块 | 单位 MSR | package energy MSR | 源码观察 |
| --- | --- | --- | --- | --- |
| AMD | `AMDFamily17.bin` | `0xC0010299` | `0xC001029B` | AMD、x64、family 17h～1Ah；允许读取这些地址 |
| Intel | `IntelMSR.bin` | `0x606` | `0x611` | Intel、x64；地址在读取白名单，但不证明每款 CPU 都实现它 |

以上均调用 `ioctl_read_msr`。AMD 另有 core energy `0xC001029A`；Intel 还允许 PP0/PP1/DRAM 等域，但最小版本无需扩展。不要把这些域相加当成 package，也不要把同一 package 从每个逻辑处理器重复相加。来源：[AMDFamily17.p](https://github.com/namazso/PawnIO.Modules/blob/0.2.11/AMDFamily17.p)、[IntelMSR.p](https://github.com/namazso/PawnIO.Modules/blob/0.2.11/IntelMSR.p)。

## 单位、回绕与 affinity

AMD 和常见 Intel package RAPL 的能量单位读取为 `ESU=(unit_msr>>8)&0x1f`，每 tick 的焦耳数为 `2^-ESU`。相邻采样的瓦数为 `delta_ticks * joules_per_tick / elapsed_seconds`；间隔使用单调时钟实际时长。不要用 PU 字段直接乘能量计数，也不要硬编码默认 15.3 μJ。来源：[AMD 官方 energy 驱动说明](https://github.com/amd/amd_energy/blob/master/README.md)、[AMD 驱动单位解析源码](https://github.com/amd/amd_energy/blob/master/amd_energy.c)、[Intel SDM Vol.3B §16.10](https://cdrdv2-public.intel.com/858456/253669-088-sdm-vol-3b.pdf)。

对已确认的 32 位累计计数器，推荐差分为 `uint32_t(current) - uint32_t(previous)` 的无符号模 2^32 运算。例：前值 `0xfffffff0`、后值 `0x10`，差为 32 tick；不要使用 `UINT_MAX - previous + current` 丢掉一个 tick。此公式只处理不足一个完整周期的间隔；首样本只建立基线，休眠/长间隔/失败后重置基线。1 秒采样是实施建议，需结合实际单位与最大合理功率保证未跨越多次回绕。来源：[AMD OSRR，印刷页138](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/56255_OSRR.pdf#page=139)、上述能量定义；差分和采样策略为独立数学推导。

**7940HX 尚未验证**：引用的 OSRR 是 family 17h；AMD 当前 energy 驱动覆盖若干 EPYC 型号，且明确存在新型号 64 位能量 MSR，不能以此宣称全部 AMD 或 7940HX 已证实为 32 位。本机 family/model 落在 PawnIO 模块接受范围，说明能尝试只读调用，不保证寄存器成功或传感器精度。若采用低 32 位兼容差分，应在产品说明与实测中保留这一边界；高 32 位原值也应留下以便核对。来源：[AMD energy 源码与支持表](https://github.com/amd/amd_energy)。

官方模块只执行当前 CPU 的 `msr_read`，没有传入 CPU index，也没有在 `ioctl_read_msr` 中绑定 CPU；内核实现使用 `__readmsr`，同步 dispatch 直接执行模块。建议采样线程在读取单位和 package energy 时固定到所选 package 的一个在线逻辑 CPU，并保存/恢复线程 affinity；对当前单 package 7940HX 只读一份 package，避免全线程求和。多 package 应枚举拓扑各取代表 CPU；不是只设置 ideal processor。来源：[模块源码](https://github.com/namazso/PawnIO.Modules/blob/0.2.11/AMDFamily17.p)、[内核 MSR 实现](https://github.com/namazso/PawnIO/blob/master/PawnIO/src/natives_impl_windows.cpp)、[dispatch](https://github.com/namazso/PawnIO/blob/master/PawnIO/src/driver.cpp)、[Microsoft SetThreadGroupAffinity](https://learn.microsoft.com/en-us/windows/win32/api/processtopologyapi/nf-processtopologyapi-setthreadgroupaffinity)。绑定策略为集成建议，本机读取尚未验证。

## GPU：NVML / ADL

NVIDIA 首选驱动已有 `nvml.dll`，通过 `nvmlInit_v2` → 枚举/匹配实际 GPU → `nvmlDeviceGetPowerUsage(nvmlDevice_t, unsigned int*)` → `nvmlShutdown`。API 返回 GPU 及相关电路（例如显存）功耗，单位 mW，除以 1000 得 W；Ampere（除 GA100）及更新架构的该接口返回约 1 秒平均功率。驱动不支持时按 `NVML_ERROR_NOT_SUPPORTED` 降级。来源：[NVML 官方手册，PowerUsage](https://docs.nvidia.com/deploy/pdf/NVML_API_Reference_Guide.pdf)。

本机普通权限实测：`C:\Windows\System32\nvml.dll`（1499368 字节）初始化、枚举和读取成功，`NVIDIA GeForce RTX 4070 Laptop GPU` 的 `nvmlDeviceGetPowerUsage` 返回状态 0、95120 mW，即 **95.12 W**。这是一次设备整体读数，不能用于宣称播放功耗改善。结果：`build/vsp110-followup-20261107/nvml-power-probe.json`；脚本：同目录 `probe-nvml-power.py`。产品多 GPU 情况还需要以 PCI/LUID 等身份匹配实际渲染适配器，不能默认 index 0 永远正确。

AMD Windows 可选 ADL：`ADL2_Overdrive6_CurrentPower_Get(context, adapterIndex, powerType, int*)`，powerType=0 表示 ASIC Total Power，返回值有 8 个小数位，瓦数为 `value/256.0`；先创建 ADL2 context，检查 adapter/能力与返回码，结束销毁 context。不保证每款 APU/iGPU 都提供该读数。来源：[AMD ADL 官方 API](https://gpuopen-librariesandsdks.github.io/adl/group__OVERDRIVE6API.html)。本机 ADL 未验证，不以 TDP 或 CPU package 代替 GPU 读数。

PawnIO 的 `Nvidia.p` 当前提供显卡温度/显存温度寄存器读取，不能据此声称提供通用 GPU 功耗 API；本轮不需要靠 PCI/MMIO 或 RyzenSMU 探测来替代已有厂商 API。来源：[官方 Nvidia 模块](https://github.com/namazso/PawnIO.Modules/blob/0.2.11/Nvidia.p)。这是范围选择；未断言 PawnIO 无法扩展 GPU 功耗模块。

## 最小落地与验收

1. 可选采集组件动态加载已安装 PawnIOLib，按 CPU vendor 选官方 blob，仅调用 `ioctl_read_msr`；不增加驱动安装器、服务控制或写接口。
2. 独立采样线程约每秒读取 package energy，保存实际时间、有效性和失败原因；UI 只消费瓦数/N/A。缺权限的当前环境应显示 N/A，GPU 仍可独立成功。
3. NVIDIA 动态读系统驱动 NVML，保留 mW 原值/设备身份并转换为 W；未支持设备降级 N/A。AMD ADL 留待有对应设备时验证，不凭空填值。
4. 验收分别记录：无驱动、无权限、模块加载失败、首次样本、正常差分、计数回绕、采样长中断、实际 CPU/GPU 设备及 UI 显示。实际 CPU 能量读取与官方签名加载接受仍为阻塞项，不能用本轮 NVML 成功替代。

以上为实施建议。此次只写研究记录、来源资产和独立探测脚本，没有编辑产品代码；未启动或安装任何驱动，未执行任何 MSR/SMU/电源限制写入。
