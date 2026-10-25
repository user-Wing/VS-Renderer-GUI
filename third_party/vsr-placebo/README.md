# VS-GUI planar Shader

基于 [Lypheo/vs-placebo](https://github.com/Lypheo/vs-placebo) `06389c9a41abaf668ee2e469dd562231a8af297d`，仅保留 Shader 与所需初始化，许可 LGPL-2.1，见 COPYING。

与上游的差异：GPU 直接生成 YUV 平面；三个平面异步提交回读、统一等待，移除 RGB48 临时缓冲及 CPU 拆包。每个实例仍由 VS 的 `fmParallelRequests` 串行调用，其独立 Vulkan context 不共用全局互斥锁。新增 `subsampling=1` 输出 YUV420P16，默认维持 YUV444P16。GPU 初始化或处理失败返回明确滤镜错误。

模块命名空间为 `vsrplacebo`，不覆盖原有 placebo 插件。libplacebo 使用现有 color 目录的隔离依赖。构建条件及版本见 `tools/planar-shader.cmake`；上游完整插件仍负责 Resample、Deband 和 Tonemap。
