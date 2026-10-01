# Anime4K A/Fast 来源

`assets/anime4k-a-fast.glsl` 及便携 `shaders/anime4k-a-fast.glsl` 按官方 Windows Low-end 的 Mode A (Fast) 顺序组合，未改动六段算法，保留各段原有版权与许可。

上游：[bloc97/Anime4K](https://github.com/bloc97/Anime4K)，commit `7684e9586f8dcc738af08a1cdceb024cc184f426`。配置：[GLSL_Windows_Low-end/mpv.conf](https://github.com/bloc97/Anime4K/blob/7684e9586f8dcc738af08a1cdceb024cc184f426/md/Template/GLSL_Windows_Low-end/mpv.conf)。

1. `glsl/Restore/Anime4K_Clamp_Highlights.glsl`
2. `glsl/Restore/Anime4K_Restore_CNN_M.glsl`
3. `glsl/Upscale/Anime4K_Upscale_CNN_x2_M.glsl`
4. `glsl/Upscale/Anime4K_AutoDownscalePre_x2.glsl`
5. `glsl/Upscale/Anime4K_AutoDownscalePre_x4.glsl`
6. `glsl/Upscale/Anime4K_Upscale_CNN_x2_S.glsl`

当前 A/M Fast 使用 M / S CNN，不能把用户图片中的 5 ms 自动归给这个网络：上游早期 v0.9 是非 CNN 的梯度方法，图的版本 / 测试方法没有给出。早期上游提供 Vega64 与 Vega8 数字，不能解读为任意核显的当前整套 VS / 解码 / 音频 / 呈现性能。[早期官方说明](https://github.com/bloc97/Anime4K/blob/v0.9/Preprint.md) vs-placebo 在 VS 中返回 CPU 可读取帧，GPU 上传与读回等成本仍存在。本机配对数据见 `player-performance-1.0.2.md`。
