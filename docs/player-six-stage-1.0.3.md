# Anime4K 六档降载与格式转换

日期：2026-10-02；本地版本：1.0.3。

## 六档与选择入口

设置 → 渲染设置 → Anime 起始档位；写入 `player.ini` 的 `player/animeStage`，支持完整 INI 保存 / 加载。只作用于内置 Anime；自定义 VPY 不继承此参数。Tab 显示当前档位。

| 数值 | 档位 | 实际路径 |
| --- | --- | --- |
| 0 | Anime4K CNN + 额外增强 | 细节增强 shader 与 A/Fast 拼接成一次 placebo.Shader |
| 1 | Anime4K CNN | A/Fast；含 Restore CNN，放大阶段保留上游倍率条件 |
| 2 | Anime4K no CNN + 额外增强 | 细节增强与非 CNN 线条处理合并为一次 Shader |
| 3 | Anime4K no CNN | Clamp_Highlights + Thin_HQ + Darken_HQ，无神经网络 |
| 4 | Jinc 直通 | 不加载 VS，3FPlayer 原生播放；默认显示缩放为 Jinc，保留手动选择缩放算法 |
| 5 | D3D11 原生直通 | 不加载 VS，使用原生 VideoProcessor；不支持的输入按既有机制轻量 Bilinear 回退 |

保持正常播放稳定观察超过 5% 丢帧才逐级降低。暂停、seek、缩放、全屏和变速之后仍先恢复两秒，再观察五秒；未改动这些门槛。Realistic / 4K 源 / 网络源沿用原生直通策略，略过不适用的前四档。

Renderer 的 Anime4K 模式下拉增加 `anime4k-no-cnn.glsl`；CNN 版本继续使用 `anime4k-a-fast.glsl` 等已有模式。两份文件随程序携带，不需要下载。

## 非 CNN 的含义和来源

当前 A/Fast 的网络不能通过去掉卷积而保持相同画质。非 CNN 版本使用同一固定上游版本的传统线条算法组合，画质属于另一种处理方式，不宣称与 A/Fast 等价。

`assets/anime4k-no-cnn.glsl`：保留原 MIT 许可，组合以下原文件，不改系数：

1. `glsl/Restore/Anime4K_Clamp_Highlights.glsl`
2. [Thin_HQ：Sobel / 高斯 / 线条变形](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/glsl/Experimental-Effects/Anime4K_Thin_HQ.glsl)
3. [Darken_HQ：DoG 线条加深](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/glsl/Experimental-Effects/Anime4K_Darken_HQ.glsl)

此文件没有 CNN 卷积、网络权重或 2× CNN 中间图；输出缩放由 libplacebo 执行。

## 减少链路转换

原先内置增强路径：16 位转换 → 增强 Shader → CPU 帧 → ShufflePlanes → 转成 YUV420P16 → Anime4K Shader → CPU 输出。

现在：必要时只升位深、保留源 420 / 422 / 444 色度采样 → 将增强 LUMA hooks 与 Anime4K hooks 拼接 → 一次 Shader → CPU 输出。减少两次 GPU 处理之间的读回与重新上传，取消中间的色度平面重组及强制 420 转换。Renderer 的普通 Anime4K 节点也改为保留原色度采样，只做插件所需的位深转换。

融合后的增强直接在 Anime4K 前的同一 GPU 管线内执行；由于不再在两个 Shader 之间进行 RGB/YUV 整数往返与色度重采样，不承诺与旧路径逐位相同。仍保留原有增强强度 0.3 和局部范围约束。

## Y410 / Y210 与 16 位的限制

Y410 是打包 4:4:4 10 位，Y210 是打包 4:2:2 10 位；VS 使用平面格式，不能把这些名称当作同一种内存布局。[Windows 格式定义](https://learn.microsoft.com/en-us/windows/win32/medfound/10-bit-and-16-bit-yuv-video-formats)。

Anime4K 数学本身没有要求源必须先变成 Y416。当前随包的 [vs-placebo Shader 接口](https://github.com/Lypheo/vs-placebo#shader)接受 YUVxxxP16，固定输出 YUV444P16，限制来自插件接口。已用随包 Python 检查签名，没有输出位深参数。因此这轮仍保留必要的 16 位平面输入，没有声称实现直接 Y410 / Y210 输入或 GPU Y410 输出。

平面 YUV444P10 与 YUV444P16 都以 16 位容器存每个分量，无 alpha 时都是约六字节 / 像素。仅在 Shader 之后降成平面 10 位不会减少该段搬运，反而增加一次转换。打包 Y410 为四字节 / 像素，确实有降低带宽的空间；但必须扩展插件输出及原生接收路径，不能只改 GLSL / VPY。Y210 与 Y216 都约四字节 / 像素，位深降低也不等于字节数减半。

现有设置仍可选择 10 位 3FP 输出格式；这是 Shader 后的格式转换，并非改变其内部输出格式。默认保留 VS 输出，避免为了显示一个 10 位名称增加 CPU 转换。

## 迁移与验证

识别已有内置 Anime 的旧默认处理片段时，只替换该片段和阶段上限；迁移前保存 `.before-six-stage`。其他用户 VPY、内置脚本的其他编辑和 Realistic 不整体重写。

回归新增六个起始档位，实际读取 FFV1 的 10 位 420 / 422 / 444 文件，验证首帧、帧 17 精确定位、自定义 VPY 不继承档位。慢滤镜测试覆盖 CNN+增强 → CNN → no CNN+增强 → no CNN → Jinc；直接选择 D3D11 单独验证。为容纳新增观察阶段，测试片从 24 秒延长为 60 秒，不放宽降载门槛。设置测试验证六项下拉及 INI 持久化。

诊断素材：`D:\Animation Enhance\GBC 108048\Girls.Band.Cry.02.AV1.FLAC.1080p48F.mkv`，1920×1080 / 47.95 fps，目标 2560×1440。独立 VS 短样本，每档 128 帧、16 线程、1 GiB 缓存、四路请求：

| 档位 | fps | ms / 帧 |
| --- | ---: | ---: |
| 旧增强 + CNN，首轮 | 30.36 | 32.94 |
| 融合增强 + CNN | 44.08 | 22.69 |
| CNN | 36.83 | 27.15 |
| no CNN + 增强 | 52.07 | 19.20 |
| no CNN | 51.89 | 19.27 |
| 旧增强 + CNN，复测 | 41.00 | 24.39 |

顺序样本明显有暖机 / 功耗波动，不能据此宣称固定提升比例，也不能把纯 CNN 那次低于增强 CNN 解读为增强能加速。此测试不含整条音频 / 播放时钟 / 原生呈现开销，52 fps 不能保证完整 48 fps 全屏不丢帧。

证据：`build/six-stage-performance.json`、`build/profile-six-stage.py`、`build/six-stage-targeted.txt`、`build/six-stage-fallback.txt`。构建使用当前共享工作树，同时保留另一窗口的图片 / 菜单更新；不把其功能算作本轮实现。

最终共享 Release 构建后的 Player 回归：36 项通过、0 失败、7 项按条件跳过，包含本轮六档、动态降载、INI 和原生缩放测试；证据 `build/six-stage-final-player.txt`。语言包校验通过，280 项字符串的中英占位符一致。整套 CTest 首轮为 7/8；未修改导出代码，导出明细复测 8/8，再次 CTest 导出单组也通过，八个组最终均完成验证。整套首轮与单组复验分别记录，不声称一次整套全过。证据 `build/six-stage-shared-ctest.txt`、`build/image-menu-export-retest.txt` 和 `build/image-menu-export-ctest.txt`。

交付目录：`dist/VS-Renderer-GUI-windows-x64`。两正式 EXE 和 FFF.Native.dll 与最终构建 SHA-256 一致，着色器 / 语言包与源码一致。更新前 INI 全部已有值保持，12 个其他预设未变；内置 Anime 原件备份哈希匹配，新片段使用六档与融合 Shader。资源清单核验 73 项。证据 `build/six-stage-delivery-audit.json`。

两正式程序独立启动并通过自身窗口的关闭消息正常退出，退出码均为 0；隐藏启动时 Process.CloseMainWindow 找不到主窗口，复验改为只定位本次创建 PID 的应用窗口发送 WM_CLOSE，未改正式程序。记录 `build/six-stage-smoke.json`。未将启动检查称为人工长时播放验证。便携目录不含测试 EXE / Qt6Test.dll，本轮未推送 Git、打包或发布。
