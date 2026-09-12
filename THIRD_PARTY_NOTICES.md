# 第三方组件声明

本项目源码采用 MIT License。正式 Windows 包还包含以下独立第三方组件；它们分别遵循各自许可证，本项目许可证不会取代其条款。

| 组件 | 用途 | 来源 / 许可证 |
| --- | --- | --- |
| Qt 6.10.2 | GUI 与 Win32 平台插件 | [Qt](https://www.qt.io/)；LGPL-3.0/GPL-3.0/商业多重许可 |
| VapourSynth R80 | VPY 执行与逐帧处理 | [VapourSynth](https://github.com/vapoursynth/vapoursynth)；LGPL-2.1 |
| Python Embedded | VapourSynth Python 环境 | [Python](https://www.python.org/)；PSF License |
| FFF Project / 3FP | 左右视频呈现 | [FFF Project](https://github.com/Lake1059/FFF_Project)；MIT |
| FFmpeg shared libraries | 3FP 解码依赖 | [FFmpeg](https://ffmpeg.org/)；以发布包所含构建的配置为准，通常为 LGPL/GPL |
| L-SMASH Works、FFMS2 | VapourSynth 源滤镜 | 通过 VSRepo 获取；许可证及源码链接见各 VSRepo 包元数据 |
| fmtconv、RemoveGrain、AddGrain、VSZip、nlm-ispc、CAS、Zsmooth、Deblock、ZNEDI3、EEDI3、SangNom、Bwdif、VIVTC | VapourSynth 处理插件 | 通过 PyPI/VSRepo 获取；许可证及源码链接见各项目包元数据 |

发布包内同时保留 Qt、Python、VapourSynth 等组件随附的许可证文件。重新分发前请核对你所替换的 FFmpeg 和 VapourSynth 插件构建配置。
