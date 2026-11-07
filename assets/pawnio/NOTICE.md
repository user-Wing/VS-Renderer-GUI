# PawnIO 官方模块

本目录仅包含应用调用的模块；PawnIO 驱动和 PawnIOLib.dll 复用用户通过 [PawnIO 官网](https://pawnio.eu/) 安装的版本，不在此目录重新分发。

来源：[PawnIO.Modules 0.2.11](https://github.com/namazso/PawnIO.Modules/releases/tag/0.2.11) 的 [release_0_2_11.zip](https://github.com/namazso/PawnIO.Modules/releases/download/0.2.11/release_0_2_11.zip)。仓库说明该发行页提供官方签名构建。保留原始二进制，未重编译、修改或重签名；实际签名是否被驱动接受以 `pawnio_load` 结果为准。

| 文件 | 字节数 | SHA-256 |
| --- | ---: | --- |
| `AMDFamily17.bin` | 10652 | `DAE74615761B78BDF064DFB3E136252DDCC6FC727D88F14738D0E5800D427A91` |
| `IntelMSR.bin` | 5324 | `D6ED85D65AB17A22F813EF98207D6D537155EE2DED5976A21CB48413C9B92E5F` |

原始 ZIP 的 SHA-256：`43608CB89BC84247FEF1368A139013F7D043E17DB6D6C8DFC9B46BF0905A81F4`；与 GitHub Release API 的资产 digest 一致（2026-11-07 核对）。

模块采用 **LGPL-2.1-or-later**；原文见 [COPYING](COPYING)，版权及模块源文件见 [AMDFamily17.p](https://github.com/namazso/PawnIO.Modules/blob/0.2.11/AMDFamily17.p) 与 [IntelMSR.p](https://github.com/namazso/PawnIO.Modules/blob/0.2.11/IntelMSR.p)。对应完整源码可从 [0.2.11 源码归档](https://github.com/namazso/PawnIO.Modules/archive/refs/tags/0.2.11.zip) 获取。分发产品时保留本说明、许可证、原始模块及对应源码获取入口。

应用仅需调用 `ioctl_read_msr` 读取能量计数器；不要调用写入、超频或电源限制接口。接口和验证边界见 [研究记录](../../development/pawnio-power.md)。
