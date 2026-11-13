> 2026-11-11已获授权并完成780M实测，见[实测结果](780m-hlsl-performance-20261111.md)。下面是11-10研究时点记录，原“未授权”不是当前限制。本次D3D12没有胜过D3D11的FP32融合优化，采用后者。

# D3D12 对本项目硬件处理 / HLSL 的价值（2026-11-10）

## 结论

D3D12有更强的调度/资源控制和Shader Model 6能力，但不是“换API画质自然更好”，也不保证现有多pass HLSL自然更快。本轮仅研究，**未改3FP上游、未实现D3D12后端、未跑性能测试**；NuxBox正在进行用户FFmpeg压制，不用被污染的FPS判断收益。

### 对画质

相同数学算法、采样、精度和输出策略，D3D11/D3D12都能提供高精度纹理和HLSL处理。画质取决于算法、数值域、精度、边界与色彩契约，不由API版本决定。SM6提供实现更复杂算法的工具，不自带更好的Anime4K/MoeSR或tone mapping。FP16算术优化必须做误差验证，不能因吞吐高而默认为等价。

### 对性能，可能有价值的部分

- **多pass CPU提交**：PSO把部分状态验证/构建前移，可减少驱动提交开销；命令列表与描述符能复用。若已是GPU带宽/算力瓶颈，收益可能很小。
- **SM6/DXC**：wave intrinsics可改写归约、数据交换等算法，减少特定场景的barrier/shared memory；必须按`CheckFeatureSupport`查询，不能固定wave宽度，不能把重新编译SM5当作自动获得wave优化。
- **compute重构**：卷积/邻域滤波可研究tile/shared memory及pass融合，降低中间纹理读写；D3D11本身也支持compute，这一收益不能全部记到D3D12头上。
- **资源和同步**：明确的copy/compute/direct队列、fence和barrier可组织跨帧流水，但当前帧的依赖pass仍须有序；异步compute不代表把依赖链同时执行。
- **显存管理**：瞬时纹理别名复用、上传ring buffer和生命周期可更精细，但管理错误会引入花屏、设备移除、挂起。

### 硬件视频处理

D3D12 Video提供decode/process队列与配置查询。是否支持codec/profile、分辨率、输入输出格式、缩放和色彩转换仍取决于GPU和驱动，必须查询具体组合。固定功能视频处理器不会因API升级就变成另一个更强的物理单元；不能推断其缩放质量或硬解覆盖一定优于D3D11。

### 对现有项目的迁移成本

现有Native主要使用D3D11纹理、decoder共享设备、动态上传、shader包和swap chain。增设D3D12需要考虑：
1. D3D11解码纹理与D3D12增强的资源共享和同步，避免经CPU读回；跨GPU更复杂。
2. 当前DXBC/SM5 shader包与DXIL/SM6缓存版本、资源绑定及反射契约。
3. 16采样器约束与纹理绑定是不同问题，D3D12描述符更灵活不等于没有硬件采样器限制。
4. 多pass资源barrier、FP16/FP32临时格式、resize/recreate/device-lost生命周期。
5. 字幕/叠加层与GUI/视频纹理合成。

不建议把D3D11On12包装当免费加速方案。微软文档明确其互操作便利性，并提示CPU/内存额外开销；它更适合渐进迁移和D2D叠加。

## 推荐后续路线（尚未授权实施/测试）

保留当前D3D11稳定路径。待用户允许、设备空闲时，可独立做小型D3D12 HLSL执行器，不直接替换3FP：
- 固定输入图与同一算法，先验证逐像素/线性域误差和边缘行为。
- 再分别测CPU提交、GPU执行、copy/barrier开销，不用总FPS掩盖瓶颈。
- 对照D3D11像素pass、D3D11 compute、D3D12 compute，区分API与算法重构收益。
- 对不支持SM6/wave的设备保留回退。

目前最可靠的判断是：**D3D12给进一步优化和更复杂HLSL实现提供空间，不给现有路径保证收益。**如果D3D11已经接近硬件带宽/算力上限，迁移API本身不会突破该上限。

## 已阅读的微软官方资料

访问日期2026-11-10；不是基准结果。
- [D3D11到D3D12重要变化](https://learn.microsoft.com/en-us/windows/win32/direct3d12/important-changes-from-directx-11-to-directx-12)：显式同步、内存、PSO、命令列表、描述符。
- [迁移指南](https://learn.microsoft.com/en-us/windows/win32/direct3d12/porting-from-direct3d-11-to-direct3d-12)：DXC/SM6查询、barrier、swap chain生命周期。
- [HLSL Shader Model 6.0](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/hlsl-shader-model-6-0-features-for-direct3d-12)：wave操作与硬件能力查询。
- [D3D12 Video](https://learn.microsoft.com/en-us/windows/win32/medfound/direct3d-12-video-overview)：解码与video process、格式能力及队列管理。
- [D3D11On12](https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-11-on-12)：互操作机制、限制与额外开销。
