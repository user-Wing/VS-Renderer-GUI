# 1.0.3 着色器、内置补帧与更新目录识别

## 更新包布局

`PortableUpdater` 在安全检查及解压之后，以必要程序文件和 `release.json` 识别完整目录，接受解压顶层直接放程序文件，或下一层任意名称目录放程序文件。只接受唯一完整候选，不递归猜测更深目录；内部版本、下载大小、SHA-256 与危险路径检查仍然有效。打包脚本继续采用带版本号的单个目录，兼容已发布的 1.0.2 更新器。

## mpv GLSL 库

来源为维护者指定的 [hooke007 着色器说明](https://hooke007.github.io/unofficial/mpv_shaders.html) 与 [mpv User Shaders](https://github.com/mpv-player/mpv/wiki/User-Scripts#user-shaders)，同时从其直接关联的仓库、gist 与 GLSL Release 资产获取文件。`assets/mpv-shaders/manifest.json` 记录原始地址、修订、大小和 SHA-256，包含同内容文件的别名来源；各目录保留许可证和原文件版权头。

本轮加入 915 份内容不同的 GLSL / hook，单文件最大约 5 MiB，没有单个着色器超过维护者设定的 50 MB 门槛。文件按来源目录显示在 Renderer 的 **mpv GLSL 着色器** 节点；下拉框可输入名称搜索。原有 `anime4k` 节点 ID 与预设参数保留，原 Anime4K 预设继续可用。运行时从 `shaders` 递归识别 `.glsl` / `.hook`，自定义文件也可加载；新增 0.5× 输出供缩小着色器使用。

执行沿用 Vulkan / vs-placebo；[libplacebo 的 mpv hook 语法](https://libplacebo.org/custom-shaders/)覆盖片段、计算、纹理、缓冲区与参数等块。保留原始 `rgba16hf` / `rgba16f` 等纹理格式和对应上传数据，不能简单互换格式名：前者上传半精度数据，后者采用32-bit浮点上传后转换为半精度。lensfix-ewa 的旧 `tex1D` / `LUT_POS` 改用等价 GLSL `textureLod` 与 LUT 中心坐标公式，改动及原始哈希单独记录在 manifest。

根据 [vs-placebo 的 Shader 接口](https://github.com/sgt0/vs-placebo#shader)，输入采用 16-bit YUV，保留原色度采样，输出为 YUV444P16。HOOK / WHEN 会根据输入色彩平面、输出倍率和阶段决定是否执行；放大算法应选择对应倍率，缩小算法应选择 0.5×，锐化 / 调色通常选择 1×。库的存在与小尺寸取帧验证不代表任意着色器都能实时处理 4K，也不代表每个条件块在所有输入上都会触发。

## Player 内置补帧

右键 → **图像处理 → 补帧**，可选自动切换、四个固定档位或关闭。开发者超分预设菜单前两项为 Anime 自动切换和 Realistic，之后为六个手动 Anime 档位。已有用户 MVTools / RIFE VPY 保持独立。

| 顺序 | 自动补帧档位 | 参数 |
| --- | --- | --- |
| 1 | RIFE 4.26 · 4queue | 2×，GPU 工作线程 4，原始推理尺寸，场景检测 |
| 2 | RIFE 4.26 · 4queue · 半宽高 | 2×，GPU 工作线程 4，仅中间帧推理半宽半高 |
| 3 | MVTools 最高质量 | 当前高质量预设：block=8，pel=2，overlap=true，chroma=true，searchparam=2，blend=false |
| 4 | MVTools 低质量 | 默认配置：block=16，pel=1，overlap=false，chroma=false，searchparam=2，blend=false |
| 5 | 关闭 | 恢复 Anime 自动超分，从最高档开始 |

切换后稳定 2 秒，再观察 5 秒；样本至少 48 帧且丢帧超过 5% 时降一档。自动档加载或取帧失败同样尝试下一档；固定档不自动降载。新文件的自动补帧从 RIFE 原始尺寸开始。补帧输出保持原分辨率和 YUV444P16，显示缩放固定 Jinc；不叠加内置超分。关闭补帧恢复 Anime 自动超分，切换保留播放位置和暂停/播放状态。图片、网络直通、madVR 与纯音频不提供 VS 补帧。

## UI 字体

Player 的主题页“应用”同步更新主界面、设置窗口、菜单和播放列表字体，并保存到 `player.ini`。西文字体用于基础界面，中文字体显式设为 Han 回退字体，避免系统默认中文字体覆盖所选字体。已打开的设置窗口也立即应用；原有粗体状态保持。

## 验证范围

只验证本轮相关项目：真实下载/解压的两种更新目录布局、危险路径与错误布局拒绝，着色器脚本生成及实际 Vulkan 取帧，四个补帧档的 24→48 FPS / 保持尺寸 / 暂停切换、自动降档与失败降档，以及字体应用和重新打开时的实际字形选择。

维护者本地另有高占用 FFmpeg 作业；着色器编译耗时及超时不用于实时性能结论。详细取帧结果随验证记录保存；未开展无关全量回归。
