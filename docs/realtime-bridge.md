# VapourSynth → 3FP 实时桥边界

## 已实现接口

上游 3FP 只接收本地路径，因此仓库补丁新增 API 14 的 `FFF3FP_SubmitExternalVideoFrame`。生成的 VPY 将处理后 clip 暴露为 output 0、未处理源 clip 暴露为 output 1。Qt 读取两者的总帧数与 FPS，把左路当前实际呈现时间分别换算为源绝对帧和处理后绝对帧，再对 output 0 调用 `getFrame`。这避免把 3FP 时间 seek 后的局部 PTS 索引误当成绝对帧号，也支持常见倍帧/抽帧链。plane 内存在提交返回前保持有效，3FP 在返回前完成上传/复制。

当前 CPU ABI 支持 Gray 8/16-bit，以及 YUV 4:2:0、4:2:2、4:4:4 和 planar RGB 的 8/10/12/16-bit 整数格式。VS RGB 的 R/G/B plane 在提交前重排为 FFmpeg GBR 顺序；`_ColorRange`、`_Primaries`、`_Transfer`、`_Matrix`、`_ChromaLocation` 和帧时长一并映射。

左路是唯一播放时钟和音频来源。右路不再打开同一个文件，也不建立第二个独立计时器；播放、seek 或逐帧改变左路帧号后，UI 只保留最新的右路请求，完成时还会再次核对左路当前帧，过期结果不会提交或更新帧号。时间轴拖动期间的 3FP seek 按 timeline generation 合并，避免高分辨率素材连续拖动时积压旧跳转。

不采用 Y4M named pipe 或完整临时编码：前者随机 seek 需要重启进程，后者会掩盖参数迭代延迟。当前有一次 VS→Qt CPU copy；未来只有在 4K/高帧率测量确认它成为瓶颈后，才扩展同设备 D3D11 texture ABI。

3FP 补丁位于 `patches/3fp-vsrenderer-extensions.patch`，构建入口为 `tools/build-3fp.ps1`。VapourSynth 固定源码的构建与 L-SMASH Works 安装入口为 `tools/build-vapoursynth.ps1`。

## 实验开关

- `VRR low-latency present` 对应已存在的 `FFF3FP_SetPresentConfig`。
- `VRR Pacing` 对应已存在的 `FFF3FP_SetPacingConfig`。
- 所给 3FP 头文件没有名为 `VBR low latency` 的播放 API。若此名称指可变码率音频缓冲，需给出目标分支/commit 后才能做真实开关；若实际指 VFR（可变帧率）低延迟 seek，应设计为帧桥请求策略，不能绑定到 Present。
