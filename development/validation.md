# 版本测试台账

核对日期：2026-11-07。历史轮次整理自仓库已留存报告；本轮 1.1.0 单独列出。当前源码为 1.1.1（Release 草稿待发布）；旧版本、旧 DLL、不同设备、电源状态或不同输出尺寸的结果不自动继承到当前产物。

## 如何读结论

- **专项通过**：仅列出的用例/样本满足断言；性能目标另行判断。
- **未达标**：已测但没有满足目标，不因 QtTest PASS 改写为通过。
- **未测/跳过**：没有覆盖或环境不满足，不算通过；各轮计数不相加。
- CTest 的组数与 QtTest 的用例数分开记录；历史 QtTest 计数可能包含 init/cleanup。
- 同版本多轮续修按轮次记录，旧失败须有后续相同路径的验证才能关闭，失败证据仍保留。

原报告原样合并至 [history.md](history.md)，按原文件名索引，每份保留字节数和 SHA-256。原文中的“当前”、路径和发布状态属于记录当时。`build/` 不是持久证据库：报告引用该目录，不保证换机器后日志仍可读。1.0.12 精选 CSV/日志保存在 [evidence](evidence/player-kernel-1.0.12/summary.json)。

## 1.1.1 播放控制修复（2026-11-08，未单独发布）

复现两个独立问题：Full 关闭 `render/hlsl` 后固定内置 A 被误拒绝，刷新提前返回导致 `ready_` 为 false、播放按钮失效；原生增强切换的跳转可能已经恢复内核播放，但 GUI 只等待 Ready/Paused，形成内核 Playing、界面未播放且 autoPlay 挂起的状态。Lite 后一场景连续六次复现失败。固定内置 HLSL 档位现独立于自定义 VPY 开关；不兼容或模型准备失败时退回可控的 D3D11；HLSL 视频切换等待跳转完成并接纳内核 Playing 状态，极简音频模式保留原恢复方式。

用户提供的 720p/24fps/15 秒 MP4 覆盖 Full/Lite 各 12 个数据行（各 14 passed，含 init/cleanup）：现有配置、关闭 VPY HLSL、色彩不兼容、冷启动、既有着色器、右键切换及拖入视频自动播放，随后验证暂停、继续、跳转。Lite 原失败场景修复后重复六次全部通过；Full 暂停实片画面及生成 VPY HLSL/回退通过。极简位置保持、拖入自动播放和不兼容回退最终 Full/Lite 各 5 passed。首次扩大跳转等待范围影响极简模式，限定到 HLSL 视频后该项复测通过；未把中间失败删去。

封装后的独立单文件窗口实测及用户确认问题解决；用户确认后停止 computer-use。最终 Full 与单文件已同步到 `dist`，50 个 INI/VPY/主题/custom 文件哈希保持不变，日志与部署清单位于本机 `build/hlsl-freeze-1.1.1/`。GitHub 已发布 1.1.1 附件和本地 `dist/1.1.1.7z` 未更新，不能将旧压缩包当作本次修复产物；本轮不新增性能或全量回归结论。

## 1.1.1 菜单与单文件封装（2026-11-08）

MoeSR 分类移至 Anime4K 前，Full/Lite 共用菜单和内嵌模型资源，原悬停说明保持不变。Full 专项 4 passed，Lite 专项 6 passed，Shader 全组 21 passed，更新包目录专项 4 passed（均含 init/cleanup，无跳过）；Lite 实片覆盖 A、A+增强和 MoeSR，均有正常空间图像。此前 MoeSR 48fps 未达标结论不变，本轮未重跑性能测试或全量回归。

单文件实际启动完成解包、图片导入、退出清理；内嵌版本、Native/Player 哈希、三个模型悬停说明已核对。发行打包排除配置、自定义背景和 custom shader，50 个原运行目录保护文件哈希保持不变。Full 内层目录固定为 `VS-GUI-Full`；单文件仍需外置 FFmpeg shared。构建与专项日志保存在本机 `build/release-1.1.1/`，Release 保持草稿，附件由维护者上传。

## 1.1.0 发版前代码审查修复（2026-11-07）

两轮审查处置见 [code-review-1.1.0.md](code-review-1.1.0.md)。本轮完成Windows Release构建、固定Native源码重建、功能回归及新FFmpeg专项；没有发布Release。用户配置与背景留在原运行目录，发行包由独立暂存目录生成。

Qt按数据行去重后的最新结果：**372通过、21失败、88跳过**（不含init/cleanup）；另两项多数据行测试90秒超时，不能宣称全覆盖。色彩元数据、蓝光元数据以及669条中英文字符串/占位符检查通过。专门性能用例排除；首轮误纳入混合时钟诊断后已停止、不计验收。本轮不是全绿回归，也没有新的性能结论。

新增缺陷检查与实际编码已通过：主题/custom shader更新保留、单文件关联、sRGB图片导出、多路恢复同步、HLSL回退、AI输出/指纹、网络目标、Full VPY极简模式、暂停/播放位置保持、对比画布Lanczos 4导出。网络回归发现Native汇总补丁遗漏HTTP支持，新增仅供本机代理使用的补丁后，实际HTTP播放、seek、AList代理及极简模式复测6 passed。编码队列深浅配色和极简按钮底部间距通过实窗像素/几何检查。

FFmpeg更新为用户提供的07-full-shared（2026-11-07，8.0.git，AVX2，ABI主版本不变），来源及SHA256见 `tools/ffmpeg-source.json`。新FFmpeg下的Lite图片/播放专项11 passed；导出管道9 passed；shader18 passed；参数、帧桥接、更新器、图像编辑布局完成复测。单文件仍使用外置FFmpeg。

未关闭：原生BT.2020 CL；导航数量旧断言；浮层、音频Tab/EQ、逐帧编辑、延时环绕、字幕、Anime/补帧、LAV与部分设置/快捷键回归。部分已确认旧预期，部分仍未归因，不能统一标记为环境问题。两项超时为 `animeStageSelection`、`fixedAnimePresets`。本轮结果不能替代长时间播放、全部素材、HDR色准与性能验收。完整数据行结果和原始日志交付为 `1.1.0-regression-results.json` / `1.1.0-regression-evidence.zip`。

## 1.1.0 更新、启动、Lite配置与多纹理shader续修（2026-11-07）

导入器与原生执行器此前均限制每pass最多8张纹理，完整Mode A（49个pass，最大15个绑定）在进入编译前失败。新增16纹理GPU读回用例与完整Mode A编译用例均先复现失败，再统一使用D3D11的16采样器槽；缓存格式/旧语义保持兼容。Shader套件17 passed；16张输入的GPU结果包含第9～16张纹理，17张仍拒绝。原先因8纹理限制失败的36份库文件全部重新编译成功，其他不支持语义仍不宣称兼容。完整Mode A以原始GLSL对照1×/2×/3×/4× GPU输出，最大RGB误差0.00244140625、最大平均误差0.0000516589，均低于本次0.005/0.0005阈值；这不是整条播放色彩链或帧率验收。

更新器文件ID测试在旧实现失败，证明已准备payload被完整复制；改成同盘目录移动后通过，异盘仍复制。NoLaunch更新专项5 passed，包含配置保留和无效父进程拒绝；小夹具耗时906→606ms，不推算真实1.4GB安装的更新时长。空播放器离屏计时发现隐藏Tab仍触发448ms硬件查询；改为非详细采样后同阶段低于1ms。窗口构造前段设置深色palette与自动填充；Windows原生白屏和冷启动总时间留给用户实窗验收，不用离屏首绘54ms作为实窗结论。临时计时代码已移除。

导出窗口三个页签改为深色、紧凑内边距并与页框对齐；导出套件9 passed。Full菜单/教程专项5 passed；Lite配置播种、热更新、悬停专项5 passed（均含init/cleanup）。本轮按要求不操作用户桌面。Lite单文件内嵌当前Full的43个顶层配置项，仅中间第三行改为`Lite GLSL/HLSL`；首次启动写出JSON，已有外部文件保留，当前本地Lite外部JSON按用户要求同步。Full的27个保护文件前后哈希一致。Full播放器、Renderer、Native、更新脚本与单文件均覆盖原位置；版本1.1.0不变，Release不修改，Full的旧`1.1.0.7z`未重打。证据见[专项目录](evidence/vsp110-startup-shaders-20261107/summary.json)。

## 1.1.0 父分类悬停说明续修（2026-11-07）

用户实测上一轮仍需进入子菜单才能看见文字。此前专项只主动发送ToolTip事件，未覆盖真正悬停触发。新增`categoryHoverDescriptionSurvivesSubmenu`从分类动作的Hover进入，不发送ToolTip事件；原实现返回空提示而失败。修复后由`PlayerMenu`在分类悬停时主动刷新配置说明，并在子菜单弹出打断后恢复；进入子菜单、移到普通项目、关闭菜单或说明为空时停止。

offscreen下该交互专项3 passed，配置绑定专项3 passed（各含init/cleanup），覆盖提示恢复和进入子菜单后的取消；界面仍由用户手动验收，不将离屏结果称为Windows实窗通过。本轮只覆盖本地Full，JSON内容和版本保持不变，Lite配置封装继续留到下一轮。

Full已覆盖且与构建产物逐字节一致；26个保护文件（含滤镜说明JSON及INI/VPY/ICC/LUT）哈希不变，单文件EXE保持原SHA-256。新测试不发送ToolTip事件，保留旧实现提示为空的失败日志，避免沿用上一轮人工注入提示事件的通过结论。

## 1.1.0 滤镜菜单说明（2026-11-07）

原菜单的常规直通算法未绑定外部说明，补帧自动档复用了stage 0全宽高RIFE的说明。`sharedFilterMenuDescriptions`先以原实现运行，实际得到`Nearest`而非配置文本，专项失败；修复后offscreen专项和原有内置档位/外部说明检查共4 passed（包含init/cleanup），覆盖九种算法在上/第二/下三处共享说明、11个分类标题、自动补帧与手动RIFE说明分离，以及发送ToolTip事件后取得分类说明。该专项只证明显式ToolTip事件能取得说明，不构成实际悬停触发通过；续修见上一节。

配置仅补缺失键，保留用户已填写的Jinc/D3D11、RIFE及教程内容。按用户要求不进行桌面验收；Lite配置封装进入压缩包留待下一轮，不改版本、Release或远端安装。

本地Full已覆盖并与构建产物逐字节一致，现有JSON补入20个说明键，原23个顶层键（含用户文字和教程对象）值全部保留；25个INI/VPY/ICC/LUT保护文件哈希不变。单文件EXE的SHA-256仍为`8ea76e5b010c81869c3eb58ebbd33f0adc217d9b5418598707c058ee6cac8062`，本轮未打包或覆盖。

## 1.1.0 本地空置教程（2026-11-07）

在Full/Lite共用的空置覆盖层中绘制中间文字、按钮说明和虚线，控件ID、文字、线长及文字宽度来自滤镜说明JSON的`playerGuide`字段。Full复用`vpy/preset-descriptions.json`，Single使用启动器旁同名文件；文件监听支持原地保存与原子替换后刷新。已有说明键保留，单文件包不嵌入用户的外部配置。

Full的控件目标检查与打开图片后隐藏专项通过；Lite在offscreen平台通过控件横移后的像素定位、JSON线长修改及原子保存后的刷新检查（各3 passed，包含init/cleanup）。先前150%桌面环境的像素断言未通过；调整设备像素邻域采样后只复测了offscreen，未据此认定150%桌面通过。构建中发现并修正了Windows读句柄未关闭导致默认JSON无法替换的问题，以及隐藏刷新误放在播放列表停靠入口的问题。未将这些单项计为视觉验收。

按用户“我自己手动验收”停止桌面操作，不再启动或控制验收窗口。本轮仅覆盖本地`dist/VS-GUI-Full/vs-player.exe`与`dist/VS-Player.exe`，保留27个INI/VPY/ICC/LUT保护文件及已有滤镜说明；远端和Full压缩包保持前一阶段修复，不含本次教程。界面排版由用户手动验收，版本仍为1.1.0。

## 1.1.0 NuxBox 全屏底边与 48F 专项（2026-11-07）

通过 AgentDock 在 NuxBox / Radeon 780M / 3840×2160 60Hz / 300% 缩放上操作独立测试实例。Qt 屏幕逻辑大小为1280×720，物理底边y=2159被转换成逻辑y=720，原矩形包含检查失败；原版Lite专项失败，修复后Full/Lite在底边左端、中点、右端均能显示控制栏。本机150%缩放专项也通过。修复只在全屏扩大一个逻辑像素的右/下边界，不改变窗口模式、版本或Native。

48F素材为远端Chou目录中的1080p HEVC版本，ffprobe确认7001/146≈47.9521fps，连续时间戳间隔20–21ms。Full使用原INI副本、4K全屏和Tab，D3D11/Jinc/透传HLSL六秒呈现均值分别47.959/47.927/47.950fps，测量窗口内丢弃/合并均0；片头D3D11为48.028fps。Lite原配置副本加Tab分别48.069/48.007/47.927fps。正式Full入口另用原配置副本切D3D11，稳定段每秒增加48帧，Tab约48.00–48.05fps；起播累计6帧丢弃单独保留。原配置CNN增强走VS路径时明显更慢，这不等同用户报告的固定24F。

用户确认24F指Tab实时帧率，随后复测已恢复；本轮未复现24F，不将其归因于未经验证的内核错误。测试入口`remote48FrameCadence`接受`VSR_NUX_SOURCE`、`VSR_NUX_SETTINGS`、`VSR_NUX_TAB`和`VSR_NUX_SEEK`，不修改实际用户配置。临时正式入口诊断代码已移除。原始日志位于本机`build/vsp110-nux-fixes-20261107/`及远端同名AgentDock工作目录；不构成全量媒体回归。

用户追加Full原装档位仍为GLSL的问题：发行VPY使用`core.placebo.Shader`，与当前模板的`getattr(core, 'vsrplacebo', core.placebo).Shader`仅插件名不同，但原精确匹配拒绝旧模板。`legacyBuiltinAnimeUsesNative`在修复前失败；修复后自动档及八档旧模板均通过，并逐一断言增加裁剪的修改脚本被拒绝。现有`fullNativeAnimeAndEditedPresetFallback`也通过，覆盖真实HLSL画面与修改后恢复VS。仅扩展完整已知模板匹配，不覆盖用户VPY或放宽任意Python检查。后一次本机底边复测中光标未停留在设定目标，日志物理目标与Qt光标不一致，保留失败记录，不视为通过；NuxBox的物理底边专项独立核对。

NuxBox最终复制实际发行VPY到独立测试目录，Full六个超分/1X档分别选中原生算法10/9/11/12/13/14，持续播放、3840×2160捕获和Tab的HLSL路径断言全部通过。首轮复用同一播放器重开文件的测试误用了上一次累计计数，修正为各档独立实例并等待Playing后通过，不将首轮失败隐藏。最终底边三点仍通过；Full D3D11/Jinc/透传HLSL六秒呈现均值47.910/47.840/47.871fps，测量窗口丢弃/合并均0，Tab约48–49fps。没有将短窗口累计增量等同严格稳定FPS基准。

本机`dist/VS-GUI-Full`与`dist/VS-Player.exe`、远端`C:/PortableSoft/VS-GUI-Full`与`D:/Downloads/VS-Player.exe`已同步同字节产物，版本仍1.1.0，Native未改。两端分别27/25个INI、VPY、ICC/LUT保护文件哈希相同。重新封装的单文件两端SHA-256一致，真实启动器图片导入专项退出0、输出91字节，并清理运行目录；该项只证明解包/启动/导入路径，不替代媒体播放检查。未上传Release附件或正式发布。

## 1.1.0 单文件启动与干净发行包（2026-11-07）

本轮不改播放器/Native算法，测量同一机器、同一共享FFmpeg与独立配置下从创建进程到可见窗口的时间；每次完整解包、关闭并清理运行目录，未清空Windows磁盘缓存，不称为冷盘启动。原单文件三次为2.483/3.038/2.399秒，中位2.483秒；直接运行播放器0.712–0.939秒。相同负载tar.xz解包1.139–1.174秒，gzip level 1为0.342–0.358秒，未压缩tar为0.184–0.215秒但约92MB。选择gzip，保留原退出清理和外置FFmpeg/相邻INI契约。

更换压缩后的真实单文件三次窗口时间1.688/1.542/1.680秒，中位1.680秒，比原中位减少约32%；播放器子进程出现时间0.468–0.582秒，退出码均0，没有遗留`.vs-player-run-*`目录。该结果是无媒体窗口专项，不等同打开大视频首帧时间或各机器冷启动保证。证据位于`build/vsp110-startup-release-20261107/`。

Full/Single均排除themes背景目录和主题INI，保留功能及现有用户主题。Full打包脚本补齐两份1X和四份超分补帧组合脚本，按固定PhotoCraft源码清单的80MB预算核对现有0.20.1组件，排除来自运行目录的旧资源清单，重新生成发行清单。

最终Single为40,768,764字节，95文件/94项清单、gzip负载及SHA-256校验通过；无背景、主题INI、用户player.ini或FFmpeg，保留原Pareto图及主题归档工具。最终产物再做三次无媒体启动：1.627/1.294/1.227秒，中位1.294秒，约比基线减少48%，退出均0。多轮温缓存结果存在波动，上述1.680秒与本轮1.294秒分别保留，不以最佳单次替代中位值。

Full为419,904,827字节，7-Zip完整性、独立解压后2209项资源清单、22份内置预设及主程序/Native/PhotoCraft原始字节匹配通过，无背景及用户配置。解压目录的Player/Renderer分别约0.783/1.623秒出现窗口，均正常退出；没有把这些空窗口启动检查当作全量媒体回归或VSCap交互验收。30个原保护文件哈希未变；dist/player.ini在独立测试结束后、打包期间另有新修改，已单独留存当时副本并保留新内容，不回滚配置。只准备1.1.0 Release草稿，附件由维护者手动上传，未正式发布。

## 1.1.0 手动多路预览与极简进度续修（2026-11-07）

本轮仅处理多路默认渲染、预览标题、播放器提示区高度及极简模式进度。复现日志、构建及专项结果位于`build/vsp110-route-minimal-20261107/`。改动前，加路后未点击预览即呈现1帧；停在5秒进入极简后位置回到开头，两个新增回归均失败。移除加路/换源/预设加载的隐式预览入口，换源清空旧路；极简重开使用待恢复时间戳并清除旧跳转状态，保留播放意图。

- Renderer专项6项通过（含init/cleanup）：两/三/四画面等待手动渲染、选中路单独启动、换源清理、减路后标题恢复、多路独立真实输出、HLSL预览及GLSL回退。补测还复现了首次初始化期间加路导致请求转到新选中路的问题；初始化完成后固定回到原请求的第一路执行，再恢复编辑选择，新增回归随后通过。
- Full专项4项通过、Lite专项4项通过（均含init/cleanup）：通过`.`往返切换，在关闭历史续播时保持约5秒位置，覆盖暂停与继续播放；Lite仍只解码音频且音频时钟前进。重开后Native可处于Ready静止状态，断言不自动播放而不强制枚举Paused。
- Full同时检查提示区恰为字体单行高度加4px、3秒后清空且高度稳定，以及EQ/音频下拉；双路与多路滑块专项4项通过（含init/cleanup）。构建Renderer、Full Player、Lite Player及相关测试通过。没有重跑尖峰或全量回归。

按原授权覆盖现有Full和单文件，不改版本或GitHub Release。31个保护文件、配置及主题/Pareto图全部保持原哈希。Full程序/Native匹配构建，2223项清单核对通过；Single为26,742,588字节、95文件/94项清单，代码/数据节、Native及Pareto图匹配，未内嵌FFmpeg或用户player.ini。没有追加单文件启动与退出清理验收。

## 1.1.0 图片导出、主题可见性与浮层续修（2026-11-07）

本轮不重测视频尖峰。构建Renderer、Full Player、Lite Player、VSCap及四个相关测试目标。专项日志和截图位于`build/vsp110-ui-images-20261107/`，不作为发行附件或全量回归通过依据。

- 主题：旧事件过滤器绘图被不透明页面覆盖，红/蓝背景像素断言先失败；改为独立底层QWidget后通过。参数套件10项通过（含init/cleanup），同时覆盖长文件名无省略换行。使用维护者指定的ModelScope-Manager“小圆/background.png”在Renderer主界面、比对、设置、BD Remux四页以及Player主界面、设置实窗截图，已检查主界面/BD/Player设置截图。页面即时刷新，PreviewPane子树不变透明；没有以编译成功代替图像验收。
- Full专项7项通过（含init/cleanup）：实际PNG8/16、JXL8/10/12/16输出后读回；比例解析与HLSL Jinc；旧Anime4K及选定库GLSL离线图片缩放（含Alpha/ICC）；图片对话框；状态3秒清空且高度稳定、EQ位于控制栏上方、音频下拉在反复布局时保持可见；指定Player主题。未逐项验收整个GLSL库。
- Lite专项4项通过（含init/cleanup）：FFmpeg传统缩放和HLSL缩放导出PNG8/16，实际JXL12读回；五种HLSL内核保持浮点2.5及Alpha0.5；界面无GLSL选择且质量搜索关闭。没有调用AWJ。JPEGli固定依赖只支持8位，未伪造10位JPEG选项。
- Renderer专项3项通过（含init/cleanup）：指定主题四页即时可见及预览隔离。Pareto旧设备绝对路径回退同名本地图并保存相对路径；单文件运行目录内的内嵌图可被发现。
- 真实JXL读回先触发AVX2崩溃，GDB定位到DequantBlock向量参数跨函数后的栈对齐；在固定libjxl版本的可复现CMake补丁中传递等值标量因子、函数内重建相同向量，并内联DequantLane。保留AVX2，以上8/10/12/16位样本随后全部通过；未扩称覆盖所有大图。

本轮覆盖现有Full及单文件，主题图复制到两份配置旁并使用相对路径；Pareto.avif保持原图且加入单文件负载。不改版本或GitHub Release。部署前按授权停止现有Full VSCap；28个保护文件中仅Full的player.ini/themes.ini及单文件的player.ini发生预期变化，原始背景图及Pareto图不变，单文件新增独立themes.ini。Full三个程序与Native逐字节匹配构建、2223项清单核对通过；Single为26,692,388字节，95文件/94项清单、负载校验及代码/数据节匹配通过，含原Pareto图及主题归档工具/许可，无FFmpeg或player.ini。专项之外没有补做单文件启动/退出清理、全量回归或长时间播放验收。

## 1.1.0 播放、界面与主题续修（2026-11-07）

本轮按维护者新授权，实际测试 GBC S01E02 2:15–2:35、K-ON S01E06 多轨字幕及 Yuru Camp Movie 音频；不继承上一轮“未测”为通过，也不等同全量回归。

- 字幕：原生独立读取器仍逐包经过无关音视频，K-ON 跳到60秒/300秒分别需要约197/210次调用、3.36/3.47秒。只读取所选字幕轨后，Qt路径首次渲染约60–124毫秒；三次跳转均首次调用显示当前字幕，加载状态立即提示。
- 参数面板：先前孤立 editor 测试未覆盖实际滚动容器，维护者仍观察到 MPV 说明文字略有遮挡。增加完整容器宽度变化回归，保留 height-for-width 并按实际宽度更新标签最小高度、补上下留白后，参数/主题套件8项通过（含init/cleanup）。
- 界面：2px底部留边、移除圆点、Tab文字无滚动且失焦保留、极简窗口与音量布局，专项5项通过（含字幕及init/cleanup）。双路/三路/四路复用 Compare 滑块及原双路原生裁切，4项通过（含init/cleanup）。独立处理路和时间组布局通过；较广的 `rendererLayoutAndParameterSizing` 仍在“逐帧控件水平居中”断言失败，不作为完整布局回归通过。
- Yuru：默认 FLAC 2.0，另有7.1；无额外响度增益元数据。原始音频在120秒与900秒各取30秒，双声道分别为−29.2/−35.6 LUFS、峰值−13.1/−18.9 dBFS；7.1为−27.7/−34.2 LUFS。保持100%=1.0，不给所有视频增加固定增益；没有修改素材或做主观听测验收。
- 功耗：已安装 PawnIO 2.0.0 服务运行，但普通权限 `pawnio_open` 返回0x80070005，未加载驱动或提权，CPU保留明确N/A。NVML只读实测RTX4070 Laptop GPU成功，直接编译产品`PowerSensors.h`和实际资源对象的QtCore探测也成功（GPU25.665W、CPU准确提示需管理员权限）；模块来源及权限见 [研究记录](pawnio-power.md)。

尖峰性能的初始测量新增53丢帧，其中152–156秒集中48帧，与维护者观察一致：2GiB总预算扣除32帧解码流水线后实际只容纳约10帧，且前瞻只覆盖首个高码率秒。修正为常规8帧、尖峰最多48帧/4GiB总预算，合并相邻尖峰并在时间线变化后重探测。修正后的第一遍在完整指定区间保持0丢帧，156秒后恢复8帧目标；该遍在163秒后停止并报渲染错误，保留失败日志，不宣称整个长测通过。随后增加窗口提前关闭识别及原生错误日志，再做完整55秒复测：129.265–184.266秒、约47.9446fps，0新增丢帧、0音频欠载，最大音画时间差0.022665秒；135.279–155.271秒专项区间为47.9716fps、0丢帧。156秒后目标恢复8帧。RSS峰值7696.33MiB、末尾7674.63MiB，私有内存峰值8189.58MiB；分配器保留内存，未回到约6GiB的起始基线，但没有再常驻12GB以上。实测为4K48输入、约2561×1601原生显示输出，D3D11档位、软解及字幕开启；这一遍未再出现上次后段停止的错误，不据此声称所有长期播放场景通过。

本地日志与CSV：`build/vsp110-followup-20261107/`；这些是专项证据，不是发行包附件。Full覆盖现有目录，Single覆盖既有`dist/VS-Player.exe`，不更改版本或GitHub Release。静态包核对：Single含94文件、93项清单，无FFmpeg及player.ini；Native逐字节匹配构建产物，播放器代码/数据节匹配已构建Lite，内含主题7z工具及许可。Full2214项清单和三个程序/DLL核对通过；25个保护文件仅两份player.ini新增空paretoImage键，其余内容未变。独立Native帧桥/滑块实际输出专项3项通过（含init/cleanup）。单文件本轮没有追加桌面启动与退出清理验收，已停止桌面控制；完整回归和长期播放仍未验收。

## 1.1.0 基础修正续轮（2026-11-06）

按维护者要求，这轮不启动播放器/GUI/媒体测试、内存压力测试或回归套件，GBC第二集2:15–2:35留待之后验收。仅做源码核对、构建、离线shader编译、部署哈希与Git核对；不把本轮界面、响度或内存修正写成实机通过。

常规缓冲改为8帧预算，尖峰临近时最多48帧且2GiB；Tab/状态/最简/设置/音量控件与多路同步已修源码。100%主观响度差异没有听测确认，原生1.0增益与旧代码一致；修复EQ的100%上限对200%主音量的钳制，不人为添加未经测量的固定增益。

输出只覆盖既有`dist/VS-GUI-Full`，保存配置、VPY及色彩文件；发布状态不变。本轮Qt Renderer/Full Player/Lite Player/VSCap构建通过；915份文件离线逐个处理，266份可打包（231份复用已验证字节码，35份新增编译），649份执行语义或编译不兼容，0份超时。只确认编译，不确认新增shader画面等价。详细逐文件结果留在`build/vsp110-basicfix-20261106/shader-batch-report.json`。以下数字均属于此前轮次，不能继承为当前产物验证。

## 1.1.0：本地开发与全量回归尝试（2026-11-06）

**当前未达到发布验收。** 全部25组CTest已执行，完成轮次为20组通过/5组失败，包含两组异常退出；续修专项与最终状态见[逐项交付清单](evidence/vsp-1.1.0/README.md)及[机器摘要](evidence/vsp-1.1.0/summary.json)。不将修复后的局部通过折算为25组全量通过。

| 验证 | 结果与限制 |
| --- | --- |
| 构建/上游重放 | Qt Full/Lite/Renderer/Cap与Native通过；干净上游checkout应用1.1.0汇总补丁并重建通过 |
| Shader | 15通过/0失败；231/915份预编译可用，684份未支持；HLSL亮度R16改动48组实际像素对照通过 |
| Lite | 最后整套37通过/0失败/5跳过；实际PCM100%→200%电平由0.0707068升至0.141421；最简不解码视频、真实HDR切换恢复通过 |
| PhotoCraft | 最后5通过/0失败/1跳过；保留嵌入颜色配置后8/16/32位宽图精度通过；1.536Gpixel样本未提供 |
| Renderer处理路 | 聚焦4通过/0失败，含独立输出、隐藏后重新验证不重启输出、启动预算；完整套件另有下载/居中失败并超时 |
| Full原生启动/定位/8K硬解 | 聚焦9通过/0失败，含单CPU软解定位；8K48硬解约10秒485/486帧、零掉帧，显示区域1182×665；D3D11名义档实际原生shader路径 |
| 8K48软件解码 | 3840×2160显示区域测量用例执行通过，实际逐帧实时未达标，有掉帧/音频欠载；以summary.json和CSV实际数据为准 |
| GLSL性能 | A/B/B/A无稳定收益，异步上传实验已撤回；不能声称本轮GLSL性能改善 |
| AI/NCNN/新增滤镜 | 真实日语ASR与原生音频tap通过，RTF约0.14；21个NCNN模型真实子进程及VS单帧/实时跳过通过；QTGMC/SMDegrain/FFT3DFilter/BM3D均实际取帧 |
| 图片/截图/主题 | AWJ RAW缩放、格式转换及HDR截图专项通过；GIF/TIFF动画通过；主题实际绘制测试通过。完整套件中的图片布局失败另有7通过/0失败/1跳过专项重测 |
| 色彩/完整交互 | 原生BT.2020 CL仍9通过/1失败；Full浮层额外redraw、音频Tab面板布局及Renderer完整回归未关闭 |

码率探测在播放器析构后的回调曾导致异常退出，已改为释放播放对象前停止探测并断开回调；保留修复前日志，最终完整播放器复测以机器摘要为准。不能从单项成功推出连续整套生命周期通过。

AI组件ZIP已单独准备，上传`ARXChem/Software-List`的`AI-Subtitles/1.1.0/AI-Subtitles-1.1.0.zip`，大小/哈希见[上传收据](evidence/vsp-1.1.0/ai-upload.json)，本轮未上传。Full和单文件位于独立1.1.0路径；最终资源、生命周期与原有46项配置/预设/色彩哈希核对见机器摘要。

明确延期：FFmpeg重编译/MoeSR/教程与Pareto/NuxBox。仍未完成：PawnIO真实功耗及下载入口、VideoEnhancer式HDR参数、指定GBC第二集尖峰验收、GLSL性能目标及780M48fps目标；LAV超过100%的真实增强未实现/未验收。

## 1.0.17：Full 与自定义 HLSL 本地更新（2026-11-01）

本轮将 Full 的 Anime 八档播放路径接入原生执行器：六个固定增强档使用 HLSL，Jinc/D3D11 两档保持原生直通；兼容的自定义 GLSL/HLSL 可导入、转换并缓存。Full 的自定义 VPY 与 Renderer 预览只提取可验证生成图的末尾 shader，前序 VS 处理继续执行，编码导出保留完整 VPY。最终证据、产物哈希和失败历史见 [本轮摘要](evidence/release-1.0.17/summary.json)。

**覆盖边界：本轮并未把全部内置 GLSL 都迁移。** 四个 `Super-*` 超分补帧组合仍执行原 VPY/GLSL，不能把 RIFE 前的 shader 移到链末；三个独立 `sharpen_edges`、`crispen_edges`、`enhance_detail` 节点的任意参数和原 Y 域 LUMA/色度语义仍保留原实现。固定 Anime 增强档内的专用 LUMA 实现不代表通用导入已支持 LUMA。多个 shader 串联仅提取兼容尾节点；仅存在于 shader 归档中、没有磁盘文件的库条目会明确回退。

| 专项 | 最终结果 | 覆盖与边界 |
| --- | --- | --- |
| 原生与 Qt 构建 | 通过 | MSVC Native、Full/Lite Player、Renderer、Cap；原 128 个 Native 导出全部保留，仅新增 `VSR3FP_SetCustomShader`，总计 129 |
| Shader 编译与缓存 | 13 passed / 0 failed / 0 skipped | 原生 HLSL、实际随包 Anime shader、Unicode 路径、缓存复用/失效/截断修复、严格拒绝不支持语义 |
| 图与 VPY 提取 | 17 passed / 0 failed / 0 skipped | 末尾节点、处理顺序、前序尺寸、修改过的 Python、多时间组与输出尺寸检查 |
| 参数编辑器 | 5 passed / 0 failed / 0 skipped | 高级参数 HLSL 开关及保存恢复 |
| Lite 播放专项 | 23 passed / 0 failed / 0 skipped | 导入菜单、原生/多 pass/PREKERNEL/COMPONENTS 实际像素、失败保留旧 shader、源副本与重启缓存、真实 PQ→SDR 恢复、真实动画八档及缩小清屏 |
| Full 播放专项 | 16 passed / 0 failed / 0 skipped | 内置原生路径、CRLF 正文匹配、修改正文保留 Python、自定义 VPY 的 HLSL 实际画面、不支持 hook 的 GLSL 回退及档位选择 |
| Renderer 专项 | 5 passed / 0 failed / 0 skipped | HLSL 预览、前序 VS resize 与节点 OUTPUT 尺寸、LUMA 回退、保留原导出、编辑正文保护及 128/1024/8192 倍实际像素间距 |
| 语言资源 | 613 条通过 | 中英文键覆盖和参数占位符匹配 |
| 产物与原有文件 | 13/13 项检查通过 | Full 632 项清单；Single 85 项内嵌资源、CRC/SHA、暂存逐项一致；转换工具/许可齐全；25 个原有 INI/VPY/色彩及相关文件哈希不变；无新增发布压缩包 |
| 最终程序启动 | 通过 | Full Renderer 空白主窗口响应并正常退出；最终单文件副本首次冷解包、新 GLSL 缓存、实际 HLSL 播放时间前进、加载本次 Native、正常退出及用户配置保持 |

QtTest 计数包含 init/cleanup；上述为不同专项，不相加宣称全量回归。Full 空白主窗口启动不等于媒体播放验证，实际 Native/VS 路径由对应 QtTest 像素与播放用例覆盖。

### 本轮失败与修复的对应证据

- Full 首轮因内置期望正文未同步规范化 CRLF，正确预设未进入原生链；两侧正文统一换行后通过，修改正文的反例仍保留 VPY。旧档位测试把其它菜单组的选中项算入，改为检查实际八档 QActionGroup 的排他性与唯一选择。
- 首个 FFV1 测试素材没有保留 PQ transfer/BT.2020 primaries，不能用它要求 HDR 标志。最终 fixture 采用 HEVC 编码后显式颜色参数 remux，并在播放前由 ffprobe 断言流包含 PQ/BT.2020；Native 也实际报告 HDR10。
- 真正的 HDR→SDR 连续换源曾在返回 SDR 时未恢复 shader：`SetCustomShader` 在新源首帧前错误读取上一 HDR 的颜色缓存。仅将该预检限制为有效缓存画面，实际逐帧颜色保护未放宽。原 RGB 恢复断言最终通过；无音轨重开也不再调用无效的 `selectAudio(-1)` 或 Failed 状态的 Stop。
- 初次测试缺少媒体/Qt 运行依赖导致启动失败；补全对应测试目录依赖后重新验证，最终产物另做独立启动。失败日志仍保留，不把这些轮次当作通过。

### 本地产物与可行性结论

干净 Full 位于 `dist/VS-GUI-Full-1.0.17`；保留正在使用的旧 `dist/VS-GUI-Full`。单文件为 `dist/VS-Player.exe`，34,028,319 字节（约 32.45 MiB），没有嵌入用户 INI、Python/VapourSynth 或 FFmpeg 专用文件，继续使用外置 FFmpeg。原单文件备份在 `build/release-1.0.17/backup`。Full、Single 和本次重建目录中的 FFF.Native.dll 的 SHA-256 均为 `57ca00bae99c12be3131f3bc2085d496f9ed54f747557b63117aa708d7bb135e`。

通用导入当前支持 MAIN/PREKERNEL 像素 pass、纹理依赖与保存、条件和尺寸表达式、COMPONENTS；NATIVE 仅提供尺寸。HDR/广色域或额外颜色变换、LUMA 通用 hook、计算 shader、外部纹理等保留原链或明确拒绝。直接播放中的不兼容 shader 停用后继续视频；Full/Renderer 可以回到原 GLSL。

任意 VPY 是 Python 和插件执行图，并非单份 shader 语言。可继续扩展显式算子白名单及相应 GPU 实现，但不能仅靠 SPIRV-Cross 把任意 Python、逐帧回调、时域滤镜或 AI 插件自动转换成等价 HLSL。本轮没有重测“八倍”性能，不将原单文件的样本收益推广到所有滤镜；HDR 仅验证切换、回退与恢复，未做色准、长时间稳定性或核显/4K 实时性能验收。

本轮只保留本地源码与运行产物，没有 push、上传、创建 PR/Release 或生成独立 ZIP/7z 发布包。既有删除与实验文件保持原状态。

## 1.0.16：缩小套叠修复与统一内核（2026-11-01）

Anime4K 分支在公共清屏前返回，缩小后留存交换链旧帧；在该分支先清为黑色，再执行现有处理。测试直接读取交换链视频内/外像素，避免离屏截图自身清空背景而漏报。原版八档测试为 4 passed / 6 failed（含 init/cleanup），六个 Anime 档在 50% 时视频外仍有橙色旧帧；普通 Jinc/D3D11 通过。单项重复复现，原 Full 内核普通两档对照通过。仅补清屏后八档 10 passed / 0 failed。

Full/Lite 统一使用 VSP → 比对扩展 → Anime4K 的 API20 内核，1.0.16 最终构建验收见 [本轮证据](evidence/release-1.0.16/summary.json)。

| 专项 | 结果 | 覆盖与边界 |
| --- | --- | --- |
| Lite 回归 | 25 passed / 0 failed / 0 skipped | 含 init/cleanup；八档反复缩小至25%再放大、菜单/像素、设置迁移、暂停/定位/重开、无VS播放及既有设置/音量边界 |
| 真实动画八档 | 10 passed / 0 failed / 0 skipped | 含 init/cleanup；960×540 SDR六秒动画，实际软解 |
| GLSL/HLSL 对照 | 48/48通过 | 六变体×八比例，FP16中间纹理；仅shader及探针双线性最终适配，不等同完整Full/libplacebo链 |
| 预览回归 | 10 passed / 0 failed / 0 skipped | 含 init/cleanup；大倍率滚轮、裁剪、联动与滑块 |
| Renderer 比对 | 8 passed / 0 failed / 0 skipped | 含 init/cleanup；128/1024/8192倍实际GPU像素间距、10位YUV源码值、直通/多源/色度缩放及各滑块拖动 |
| 真实三路 AVIF | 3 passed / 0 failed / 0 skipped | 含 init/cleanup；三张Stray 10/12位AVIF各60次拖动呈现、原始YUV及超过128倍同步滚轮缩放 |
| 最终产物与配置 | 通过 | 单文件26.23 MiB，69项资源的CRC/完整SHA、构建来源、冷解包及960×540图片导入通过；实际单文件窗口显示A4KCNN+。Full/Lite资源清单逐项匹配，29项现有INI/VPY/色彩文件哈希不变 |

初次 Lite 回归与两组GPU测试并行时，旧极短warmup片的暂停断言遇到片尾状态，24 passed / 1 failed；相同断言独立运行，新旧内核均3 passed，最终完整Lite专项独立复跑25 passed，未修改或放宽断言。不同专项计数不相加为全量回归；未验收HDR色准、长时间稳定性、核显/4K实时性能。

保留正在使用的 `dist/VS-GUI-Full`，干净1.0.16 Full暂存为 `dist/VS-GUI-Full-1.0.16`；单文件替换 `dist/VS-Player.exe`，不嵌入用户INI或FFmpeg专用文件。代码提交及Release草稿与附件上传分开，附件由维护者上传。

## 1.0.15 本地 Full：多路 AVIF 交互与源 YUV（2026-11-01）

续修固定缩放上限：原版 `nativePreviewZoomContinuesBeyond1600Percent` 在 16 倍停止，新版预览 10 passed；原版内核请求 128 倍实际只有 32 倍，新版在 128/1024/8192 倍下源像素间距分别为 128/1024/8192 个屏幕像素，并通过红/绿相邻像素的实际 GPU 取样。视口裁剪与 UV 映射避免超过 D3D11 视口坐标范围；百分比改用浮点格式化，防止大倍率整数显示溢出。原始日志见 [续修证据](evidence/avif-zoom-20261101/summary.json)。比对专项 7 passed（含 init/cleanup），覆盖真实三路 AVIF 超过 128 倍的同步滚轮放大、旧拖动/原始 YUV、直通比对及色度缩放；未重新操作用户窗口。这里只解除固定倍率限制，仍保留坐标可表示范围的数值保护。

使用 Downloads 中三张 Stray 2560×1600 AVIF，分别为 `yuv444p12le`、`yuv444p10le`、`yuv420p10le`。独立 checkout 在固定 API20/VSP 补丁后构建比对扩展；未改版本、打包或发布。日志、码值与最终二进制哈希见 [专项摘要](evidence/avif-compare-20261101/summary.json)。

| 专项 | 结果 | 覆盖与边界 |
| --- | --- | --- |
| 原版复现 → 呈现调度修复 | 60 次拖动：原版第一路仅呈现 1 次；修复后三路各 60 次 | 独立 view generation 绕过静态媒体帧率限速，仍由交换链同步；新版 UI 事件处理 p95 4.10 ms，不等于端到端显示延迟 |
| 新取样与三路联动 | 4 passed / 0 failed / 0 skipped（含 init/cleanup） | 10 位 4:2:0 原码值、坐标往返、越界/留黑；真实三图加载、平移、并排/ABC 2+1 同源坐标及 RGB/YUV 切换 |
| 独立 FFmpeg 解码对照 | 三路全部匹配 | 源坐标 (1232,731) 的三组 YUV：1153/1832/2221、286/458/556、284/455/556；按各源位深与抽样读取，无 RGB 反算 |
| 既有比对与预览 | 直通比对、多源/移除/换源、色度/缩放三项通过；预览 9 passed（含 init/cleanup） | 滑块专项单独对照旧/新 DLL 均通过；首次与 FFmpeg 对照并行时四路拖动 1558 ms 超过 1500 ms，隔离测量后旧/新为 434/450 ms，阈值未改 |
| 部署与实际界面 | 本地 Full 已更新，用户已完成界面检查 | 仅替换 Renderer 与 Native；27 项 INI/VPY/ICC/LUT 哈希不变，旧 127 个导出均保留，仅新增源像素导出；当前窗口留给用户，不再自动操作 |

静态图片不再等待不存在的 seek 完成代次。RGB 显示渲染输出，YUV 显示转换前源码值；不支持或越界明确提示。此次是比对专项，未运行完整 Player 回归，未宣称物理 HDR 色准验收。原文件和已发布附件未修改，Lite Anime4K 目录独立保留。

## 1.0.15 本地单文件：八档原生 Anime4K（2026-11-01）

接续下方Fast实验，将Full的CNN＋额外增强、CNN、no CNN＋额外增强、no CNN、Jinc、D3D11、两档1X接入精简版。本地替换 `dist/VS-Player.exe`，保留版本1.0.15、旁边INI及Full运行产物。持久证据见 [专项摘要](evidence/anime4k-presets-20261101/summary.json)。

| 专项 | 结果 | 覆盖与边界 |
| --- | --- | --- |
| 六变体GLSL/HLSL对照 | 48/48通过 | 六种处理路径×0.75/1/1.25/1.5/2/3/4/5倍，最大RGB误差0.00244140625、最大平均误差0.00002684；探针统一双线性最终适配，仅对照shader处理，不等同整个Full/libplacebo链 |
| 八档与设置回归 | 17 passed / 0 failed / 0 skipped | 包含init/cleanup；菜单逐档实际输出、CNN/no CNN额外增强差异、1X·Jinc/D3D11输出差异、暂停/续播/定位、持久化、旧Fast迁移、默认档位设置及既有后端/音量边界 |
| 真实动画逐档 | 10 passed / 0 failed / 0 skipped | 包含init/cleanup；同一960×540 SDR短片全部八档，含重开/定位/截图；本样本软解，不算新的硬解能力验收 |
| 单文件包与实际运行 | 通过 | 约26.23 MiB；69项、原有文件/许可全部保留，ZIP CRC、资源哈希、实际冷解包及图片导入通过，GUI分别确认A4KCNN+与最终1X·Jinc；无用户INI、Python、VapourSynth或FFmpeg专用组件嵌入 |
| 语言资源 | 通过 | 591条播放器字符串，中英文键及占位符检查通过；新入口和设置文字已同步到单文件资源与外置语言包 |

额外增强使用原sharpen_edges/enhance_detail参数；1X将shader的OUTPUT固定源尺寸，再复用原生RGB缩放器。旧50 MiB暂存门槛已过时：原单文件解包60.89 MiB，新包约61.14 MiB，仅增加约0.25 MiB；按实际单文件压缩大小记录。打包改为读取指定暂存内EXE，避免取另一构建目录旧EXE。一次逐档测试被窗口布局晚到1像素变化干扰，稳定布局后原断言通过；未放宽像素或功能断言。GNU strip会更新时间戳/校验和，已在核对构建来源时仅归一化这两个PE字段，包内资源仍按完整SHA-256验证。

本轮没有验收HDR色准、长时间稳定性、核显或4K实时性能；Full现有比对扩展与远端发行附件未替换。不同轮次计数不相加为完整播放器回归。

## 1.0.15 本地 Lite：Anime4K Fast HLSL（2026-11-01）

仅更新 `build/lite-anime4k-test/lite`，版本不变，Full/已发布单文件未替换。转换源为现有 A Fast 六段组合，D3D11 调度27个 pass，无 VS 播放依赖；构建步骤见 [API20 指南](../docs/native-api20-build.md#lite-anime4k-fast)。证据和最终哈希见 [专项摘要](evidence/anime4k-fast-20261101/summary.json)。

| 专项 | 结果 | 覆盖与边界 |
| --- | --- | --- |
| GLSL/HLSL GPU 像素对照 | 8/8 个比例通过 | 原 GLSL 独立 OpenGL 编译，对照实际 D3D11 回读；0.75/1/1.25/1.5/2/3/4/5倍，RGB最大误差0.001953125，最大平均误差约0.000027；相同FP16中间纹理与双线性最终适配，不代表完整libplacebo色彩/缩放链一致 |
| Lite 菜单与输出专项 | 8 passed / 0 failed / 0 skipped | 包含init/cleanup；菜单触发、渲染像素变化、底栏/Tab、模式切换与持久化、播放/定位、无VS播放和既有设置/音量 |
| 真实动画片段 | 4 passed / 0 failed / 0 skipped | 包含init/cleanup；从现有动画生成960×540 SDR六秒片段，软件/硬解优先设置，暂停、恢复、定位及输出截图；该High 10 H.264样本实际软解，不算硬解画质验收 |
| 部署与直接启动 | 通过 | EXE与新Native已放回指定Lite；补齐共享媒体依赖并同步配套libwinpthread，直接启动实际窗口可见；原player.ini哈希未变，shader许可随目录保留 |

旧Native运行菜单专项在设置算法9处返回 `InvalidArgument`，新Native同一入口通过；此前全黑warmup不适合验证输出差异，最终改用有细节的测试图。独立GPU对照查出并修正转换PS与VS的输入寄存器布局。仅为SDR短片专项，不宣称HDR色准、核显/4K实时性能、长时间稳定性或全量播放器回归通过。

## 1.0.15：近期内核与启动PR48同步（2026-10-31）

固定上游 main `146b5e0` + 启动PR48 `cc545d2`，保留VS帧桥、色彩、相对逐帧、AI音频tap和多轨硬解修复。19文件Native补丁干净应用及反向检查通过，原104项导出无移除，新增性能配置导出；构建仍要求IAMF。Qt迁入低优先级库文件预读，未照搬.NET图标、LakeUI设备预热和全进程HIGH。原始日志、逐次采样与产物哈希见[本轮证据](evidence/release-1.0.15/kernel-startup-20261031/summary.json)。

| 专项 | 结果 | 覆盖与边界 |
| --- | --- | --- |
| 无窗转换及硬解优先 | 8 passed / 0 failed / 0 skipped | 含audio tap；旧Native无窗转换断言确实失败，新库转换计数为0。真实8K AV1、HEVC444、多轨HEVC实际硬解；不支持的AV1 444p12实际软解，均可暂停/定位/续播 |
| Full播放与色彩专项 | 17 passed / 0 failed / 0 skipped | 软件HEVC/AV1生命周期、VFR、指定GBC长片逐帧、HDR软件/硬件×Jinc/D3D11、VS色彩链；非物理HDR显示器色准验收 |
| RTX请求与真实BD | 4 passed / 0 failed / 0 skipped | 请求/状态/持续呈现与截图、BD章节/定位/切换；RTX效果未激活，不算增强画质或性能通过 |
| 单文件内核边界 | 6 passed / 0 failed / 0 skipped | 实际媒体逐帧、允许模式、拒绝Full后端、无VS软件播放 |
| VS帧桥 | 10 passed / 0 failed / 0 skipped | 实际VS帧到3FP呈现 |
| 最终产物 | 清单、CRC、图标及版本资源通过 | Full清单624项、单文件66项；最终单文件26.15MiB，实际展开/图片导入/播放/退出通过，无测试Player残留 |

7940HX / RTX4070 Laptop本机首个实际绿色画面计时：Full预读关/开中位963.9/962.6ms；单文件缓存复用关/开1037.7/1018.2ms，首次展开中位1516.6ms。两类各8组交替配对，差异不足以证明稳定提速；不能套用上游645.6ms。未清操作系统文件缓存。首轮Full误用单文件启动器的基线已排除；采样包后续仅刷新project.md，最终包另做启动/正常退出复验。

Full输出 `dist/VS-GUI-Full`，单文件输出 `dist/VS-Player.exe`，版本1.0.15；没有dist/PlayerLite目录。配置/VPY/ICC/LUT与预设说明哈希不变，运行中的VSCap保留原文件。本轮为专项回归，未重新验收RTX实际增强、8K软件吞吐、既有BT.2020 CL失败或全量Full套件。发行压缩包与附件由维护者制作上传，本轮不修改远端Release/tag/附件。

## 1.0.15：多轨视频硬解误判（2026-10-31）

指定 K-ON! The Movie 文件的前 32 个封装包全部属于其他轨道。旧版以封装包总数限制试解，未送入视频包就判失败；修复后按视频包计数，另设总读取上限，未取得证据时保留硬解并交给播放时回退。旧 Native 请求硬解实际为软解；新 Native 同一文件试解通过、实际硬解并呈现 32 帧。另用 FFmpeg D3D11VA/CUDA 各解出前 120 帧。未完成整片播放验证。

单文件压缩率对照使用上一版完全相同的 63.8 MB 解包内容：原 ZIP 35.84 MB，Python DEFLATE level 9 为 27.32 MB；1.0.15 实际单文件 26.96 MiB（上一版 34.26 MiB，减小约 21%），ZIP CRC 与 66 项清单哈希通过，隔离缓存首次启动和图片导入通过。完整目录由现有 Full 目录复制生成，包含原有配置和预设；运行中的原 VSCap 未替换。3FP 的 EXE 是 .NET 单文件格式，其外部依赖与 VSP 的 Qt 运行时不同，体积不能直接等同功能范围。

## 各版本总览

| 版本 / 基线 | 留存测试范围与结果 | 未通过、未测与解释 | 原始依据 |
| --- | --- | --- | --- |
| 1.0.14 / API20 main 922e4b7 + AI Native 补丁 | 帧桥10、导出9、播放器新项5、Renderer设置3、Cap新项4及扩展专项8通过；4070指定链路与NuxBox对照均有实际数据 | 非全量回归；色彩9通过1既有失败；Lite7通过5跳过；AI模型/真实3FUI/UAC/目标Intel与1650s未验收；仅部分组合达2160p48 | [本轮证据](evidence/release-1.0.14/README.md) |
| 1.0.13续修 / API20 main 922e4b7 | 上游非AI内核、464字节快照、干净checkout构建；VS帧桥10、色彩专项8、播放生命周期9、增强模式长片逐帧6、Full BD4、Lite5 passed | 非全量回归；8K软件约33.87fps且有欠载，色彩套件两项旧首帧断言失败；AI仅单独拉取 | [本轮证据](evidence/native-upstream-20261024/summary.json) |
| 1.0.13 / API20 PR23 | 更新器进程/清理/可见重启、压缩工具、实盘BD与AVIF专项，详见本轮记录 | Full一条旧控件身份断言在原库同样失败；未做全量回归或重跑8K性能 | [本轮证据](evidence/release-1.0.13/summary.json) |
| 1.0.12 / API20 PR23 | BD整卷定位/按段索引/菜单续播；Jinc/无音轨VS；Anime预取；软件内核/像素/生命周期聚焦回归，详见下表 | 未运行全量Full播放器套件；8K48软件仍非实时；P/E调度等未测 | [BD](history.md#record-docs-update-1-0-12-md)、[内核](history.md#record-docs-player-kernel-performance-1-0-12-md) |
| 1.0.11 / API20，后续固定PR23 | 菜单/章节、时间组/社区、R81插件、JXL/白位、单文件FFmpeg外置和图标；VSCap工作流16 passed，窗口/全屏/底栏5 passed | 8K软件约31.03fps（4K输出）；桌面合成有跳过；HDR热键端到端、UIAccess/多DPI人工验收未完 | [更新](history.md#record-docs-update-1-0-11-md)、[JXL](history.md#record-docs-jpeg-xl-1-0-11-md)、[HDR](history.md#record-docs-vscap-hdr-1-0-11-md) |
| 1.0.10 / API18；10-16同版迁移API20 | 色彩4 passed；VSCap原生5、工作流先7后扩至14；Lite5；HEVC/AV1合成生命周期4；文字/选区/贴图分轮通过 | 8K AV1 AAC+ASS约33.16fps、61次欠载；迁移PR22当轮按要求未跑其测试；长盘跨段拖动当时失败，至1.0.12才有覆盖性验证 | [验收](history.md#record-docs-validation-1-0-10-md)、[交互](history.md#record-docs-vscap-interaction-1-0-10-md)、[迁移](history.md#record-docs-update-20261016-md) |
| 1.0.9 / API18，本地开发 | Full发行目录13 passed；Lite5；后续HDR黑屏修复原生截图5、工作流6；指定文件暂停/seek/续播/EOF/重开通过 | 8K48软件约22–24fps；D盘4K444p12开ASS最终目录约24.80fps；硬解稳定段通过但启动有丢帧；严格50MB/总包100MB目标未达；未运行全套 | [验收](history.md#record-docs-validation-1-0-9-20261010-md)、[续修](history.md#record-docs-fix-av1-vscap-1-0-9-20261010-md) |
| 1.0.8 | PhotoCraft真实GUI/16-bit PNG/float TIFF/ICC；48000×32000大图；780M双路4K48约47.9fps、10秒零丢帧；BD片段逐帧哈希及无音轨直通时钟专项 | 双路短窗口不能外推长片；CPU预缩放性能未验收；HDR/编辑器复杂对象另有限制 | [性能/大图](history.md#record-docs-performance-large-images-1-0-8-md)、[编辑器](history.md#record-docs-photocraft-editor-1-0-8-md)、[时钟](history.md#record-docs-player-startup-clock-1-0-8-md) |
| 1.0.7 | HEVC/AV1软件解码线程/队列、生命周期、字幕匹配；PNG高精度截图、透明底栏/音频信息及Full/Lite包专项 | 未记录一次统一全量回归；高码率尖峰/实时性能不能由功能通过推断；当时PNG附件规则与现行VSCap不同 | [软解/字幕](history.md#record-docs-software-decode-subtitles-1-0-7-md)、[截图](history.md#record-docs-player-high-precision-screenshots-1-0-7-md)、[包](history.md#record-docs-release-1-0-7-md) |
| 1.0.6 | 鼠标/原盘六集/滤镜字幕30项；菜单语言等另轮20项；本机8K硬解三组60秒约47.95fps、零新增丢帧；包完整性/哈希/启动 | 各项不是同一次总回归；RX6600原设备待复测；HEVC800Mbps峰值问题有复现，实验参数未部署 | [定位](history.md#record-docs-player-bd-seek-acceptance-1-0-6-md)、[硬解](history.md#record-docs-hardware-8k-queue-1-0-6-md)、[尖峰](history.md#record-docs-hevc444-peak-diagnosis-1-0-6-md) |
| 1.0.5 | 真实BD Remux/播放/seek、节目元数据、组件安装和更新回归；包完整性/解压哈希/主窗口启动 | 780M 8K48软件约18–19fps；完整RIFE/MVTools未全达实时；镜像/复杂盘型/部分HDR观感未测 | [发行记录](history.md#record-docs-release-1-0-5-md)、[软解](history.md#record-docs-av1-software-decode-1-0-5-md) |
| 1.0.4 | 色彩8/8、配置/语言/状态6/6、VS桥10/10、便携色彩smoke4/4；图片/导出分轮专项 | 不是一次全量回归；旧Qt编辑功能不能当PhotoCraft能力；完整PSD/DNG/AI等未实现 | [发行记录](history.md#record-docs-release-1-0-4-md)、[旧编辑](history.md#record-docs-image-editor-workflows-1-0-4-md) |
| 1.0.3 | 图片/音频/设置/Anime、Renderer布局/导出、图标、更新兼容和包校验专项 | 发行轮未跑无关全量；更早轮次曾8个CTest最终通过，但滑块性能断言整套超时后单测通过，不可写成末轮整套全绿 | [发行记录](history.md#record-docs-release-1-0-3-md)、[更早轮次](history.md#record-docs-history-development-records-through-2026-10-03-md) |
| 1.0.2 | Release构建；CTest8/8；便携Player27/27，更新7/7；移除测试依赖后两正式程序启动/退出 | 历史不同轮次另有6/6、7/7等，不能相加；HDR面板、多DPI、长片未全面人工验收 | [发行记录](history.md#record-docs-release-1-0-2-md) |
| 1.0.1 | Release构建、CTest五组；真实原生解码/像素、VS桥及导出 | Qt布局截图不等于完整人工画面验收；九路4K/VFR长片/多DPI未穷举 | [发行记录](history.md#record-docs-release-1-0-1-draft-md) |
| 1.0.0 | 留存基础Renderer功能记录 | 没有独立、可对应本版的完整测试结论，标记未确认，不倒填后版结果 | [早期原文](history.md#record-docs-history-development-records-through-2026-10-03-md) |

## 1.0.14：音量、静音与均衡器浮层续修（2026-10-25）

版本保持 1.0.14。Full/Lite 增加音量百分比、带斜线/不带斜线的话筒图标，EQ 按钮位于静音正下方；均衡器改为与倍速互斥的 Popup，展开时保持底栏可见。载入配置先保存音量/静音值，避免音量变化信号覆盖静音配置。

| 专项 | 本轮结果 | 覆盖与边界 |
| --- | --- | --- |
| Full 音频与界面 | 7 passed / 0 failed / 0 skipped | 含 init/cleanup；百分比、静音恢复/图标真实像素、1000/1280 宽度、EQ 位置/边界/重复点击与外部点击收起、与倍速互斥、实际音频增益/WAV、轨道/字幕/音频拖放、透明度和窗口底栏 |
| 设置应用回归 | 4 passed / 0 failed / 0 skipped | 含 init/cleanup；中英文与配置保存、应用后保持播放/暂停位置 |
| Lite 界面与边界 | 5 passed / 0 failed / 0 skipped | 含 init/cleanup；同一音量/静音/EQ 操作，原生模式和禁用 Full 后端 |
| 实际均衡器增益 | 峰值 0.0265137 → 0.0528351 → 0.0264384 | 1 kHz +6 dB 约翻倍，WAV 0.5 后约减半；不等于空间音频或听感验收 |
| 语言资源 | 590 条通过 | 中英文键与占位符一致 |
| 旧原生音频延迟探针 | 保留失败 | 先修正测试绕过 API20 适配、沿用旧峰值布局的问题；实际增益/WAV 断言通过后，延迟补静音计数 `>=4000` 仍失败。测试分出 gain-wave 与 delay-surround 两项，未删除延迟检查；不将后者算作通过 |

Native DLL SHA-256 与前轮记录相同，仍为 `db8cec2b4f4082c4bdf8337e7404a68c58ecac84fa9037c2c83896dcf1b01ac6`，本轮未修改 Native。原始日志、实际 UI 抓图和发行草稿正文在本地 `build/audio-ui-1.0.14`；未运行全量 Full 播放器套件。

部署 Full/Lite Player 后重新封装单文件 Player；配置、VPY、ICC/LUT 和 Full 的 `preset-descriptions.json` 保持原哈希。封装检查版本资源、ZIP CRC、内嵌清单及隔离首次启动；结果与原 EXE 备份留在同一轮目录。源码推送并准备 v1.0.14 Release 草稿，发行压缩包及附件由维护者手动制作、上传。

## 1.0.14：Player 设置与缩放显示续修（2026-10-25）

本轮保持版本 1.0.14，更新 Player/Renderer 的设置布局、实际缩放状态与主题；源码同步到本项目仓库，不制作压缩包、不更新远端 Release 附件。上轮不上传代码的边界属于当轮记录。

| 专项 | 本轮结果 | 覆盖与边界 |
| --- | --- | --- |
| Qt Release 构建 | Full / Lite / Renderer / Cap 及对应专项测试目标成功 | About 共用头与语言资源均纳入构建；未重封装单文件 EXE |
| Player 设置与缩放 | 10 passed / 0 failed / 0 skipped | 含 init/cleanup；字幕页与 SRT 样式入口、AI 迁移、关于作者链接、八档全列表、双排独立补帧档位、Lanczos 顺序与 D3D11 限制、切 Jinc 后徽标/Tab、跳过未勾选档位、实际 2160p FFV1 原生回退；设置应用保持播放/暂停状态 |
| Renderer 深色主题 | 4 passed / 0 failed / 0 skipped | 含 init/cleanup；参数编辑器深/浅色像素与文字颜色、既有设置页，实际抓图检查可读性 |
| 参数编辑器 | 4 passed / 0 failed / 0 skipped | 含 init/cleanup；Shader 分类、路径及宽高联动 |
| 语言资源 | 589 条通过 | 中英文键与占位符一致 |

已部署本地 Full 的 Player/Renderer 及 Lite Player，更新语言和操作文档；配置、VPY、ICC/LUT 的前后哈希一致。备份与逐文件哈希在 `build/ui-deployment-20261025-084535/deployment.json`。

后续按用户要求重新封装单文件 Player 1.0.14：版本资源、内嵌 ZIP CRC、66 项清单哈希、Lite/Native 与当前构建匹配均通过；隔离缓存首次启动成功并由子进程完成图片导入，既有 INI 与 Full 滤镜说明文件保持原哈希。单文件仍使用 Lite 原生播放内核、FFmpeg shared 外置；此冒烟不等于重新跑过播放矩阵。原包备份与封装审计位于 `build/single-update-1.0.14`。

本轮原始日志和截图保留在本地 `build/ui-fixes-1.0.14-*` 与 `build/player-*-1.0.14.png`、`build/mingw-release/renderer-parameters-*.png`。以上为专项验证，未运行全量 Full 播放器回归；不重新声称 AI 推理、目标 Intel/1650 Super 或上一轮性能矩阵已验收。

## 1.0.14：本地功能更新与设备性能检查

更新清单逐项状态、精确链路和能力边界见 [本轮记录](evidence/release-1.0.14/README.md)，NuxBox 原始数值见 [JSON](evidence/release-1.0.14/nuxbox.json)。

- 本机4070增强模式，用户指定1080p23.976样本：整帧540p RIFE后走Jinc/D3D11，3840×2160物理输出面30秒约47.933/47.967fps，跳帧/Native丢弃/合并均0；末端A4K仅22.161fps。保留1080原帧的半尺寸推理→A4K约42.700fps，未达标。
- 780M真实VS no CNN+额外增强：1080输出40.826→59.561fps、2160输出17.446→38.509fps；不含Native上传/呈现。
- 7840HS dav1d同编译器CLI三轮配对约+4.2%，1087帧像素MD5相同，仍低于48fps；独立FFmpeg已编译但端到端无稳定收益，未替换原软件。
- 色彩初次采样测试增加等待后，RGB/ICC测试通过；原生BT.2020 CL仍失败，在部署前旧Native相同位置复现。保留失败，不将其写成新修复。
- Native补丁可应用到固定main的干净索引，19个文件仅在FFF.Native；没有把Qt页面、模型或测试文件混入Native补丁。Shader生成头保留上游尾随空格，普通源码单独检查。
- 本次只做本地构建与目录部署，配置/VPY/ICC/LUT哈希部署及冒烟后核对一致；不封装压缩包、不上传代码、不更新外部Release。旧单文件EXE未重封装。

## 1.0.13续修：上游非AI内核同步（2026-10-24）

固定 `Lake1059/FFF_Project:main` 的 `922e4b70268fcbc83f0deb06d9e2c1b62e7a33b2`；完整[PR清单](evidence/native-upstream-20261024/pr-inventory.json)与[测试/采样汇总](evidence/native-upstream-20261024/summary.json)独立保存。API仍为20，配置112字节，snapshot由408扩到464字节并同步Qt镜像。保留VS外部帧、BD/concat、相对逐帧、clock-only、音频效果与可选色彩桥；Jinc权重从t5移到t8，避免与上游辅助视频层冲突，两个着色器字节码头已重编。

AI字幕#36、#37仅拉取在`.deps/fff-ai-subtitles`，停在#37的`edf5331`；不参与构建或分发。已实际检查DLL存在VS输入、clock-only、音频效果、流级side-data、RTX和外部声床渲染接口，缺少`FFF3FP_EnableAudioTap`/`FFF3FP_ReadAudioTap`。上游VB设置页、弹幕泵、外部授权模块没有搬入Qt。

| 专项 | 实际结果 | 覆盖与限制 |
| --- | --- | --- |
| Native与宿主构建 | Release x64 Native及Qt Full/Lite/Renderer成功 | 在另一个干净checkout用现行脚本重新应用完整补丁并构建；两个构建路径的DLL不要求字节相同，实际测试与部署的哈希见summary/artifacts |
| 上传与色彩聚焦 | 8 passed / 0 failed / 0 skipped | 新增上传次数/精确字节/重绘不重复上传/回读像素断言；原DLL在快照408≠464处失败，证明测试能识别旧库。默认色彩、手动色彩桥、硬解、VS输出通过 |
| 旧色彩完整套件 | 保留两项基线失败 | `constantLuminanceAndHdr`、`customIccLutAndRgbOrder`的首帧回读在原DLL同样失败（3 passed / 2 failed）；没有改动这些无关断言。初轮新增快照version错误已修复，初轮另一即时Present断言在专项复跑通过；不宣称全色彩套件全绿 |
| 播放生命周期/时钟 | 9 passed / 0 failed / 0 skipped | HEVC/AV1软件暂停、seek、EOF、关闭重开；VFR相对逐帧；三种直通启动/定位/恢复时钟及音频信息。小帧线程断言改为1～4起步，10-bit输出允许按显示条件选择10/16-bit |
| VS帧桥全套 | 10 passed / 0 failed / 0 skipped | 真实VS脚本、两路渲染、原生表面替换、请求合并、缩放/布局/全屏与像素回读 |
| 真实长片逐帧，增强模式开启后 | 6 passed / 0 failed / 0 skipped | GBC第1集4K48 444p12，SW/HW优先×Jinc/D3D11四组，约1/10/20分钟定位、进退帧、关键帧跳转及恢复；本机AV1 444p12硬件优先最终回退CPU，不能当成该格式硬解通过 |
| 增强模式开启前逐帧 | 2 passed / 4 failed；原DLL选取同位置2 passed / 1 failed | 关键帧后退一帧5秒断言超时，旧库同样复现；用户随后确认此前未开增强模式，开启后重新运行四组全部通过。两种电源状态不合并计数或比较性能 |
| Full BD拖动 | 4 passed / 0 failed / 0 skipped | MyGO下卷，播放与暂停状态均只在松开时提交一次定位，约220～235ms完成 |
| Lite原生路径 | 5 passed / 0 failed / 0 skipped | 同盘原生菜单控件、长卷前后跳转位置保持、无VS软件播放 |

增强模式由用户确认开启；采样时没有同时运行编译或其他播放测试。原生探针每臂先预热5秒，之后约15秒稳定窗口，输出交换链实际验证为3840×2160，D3D11缩放；单轮短窗口不构成严格A/B收益或长片稳定性结论。音频启用、静音播放，未加载VSP宿主字幕。

| 素材/模式 | 新画面呈现fps | 窗口新增丢帧/欠载 | 上传与同步 |
| --- | --- | --- | --- |
| OP 4K48 444p12 / CPU | 47.93 | 0 / 0 | 动态纹理，49,766,400字节/次；CPU复制3.63ms/次、约13.71GB/s；末端画面落后约17.5ms |
| 辉夜姬8K48 420p10 / CPU | 33.87 | 250 / 3 | 动态纹理，99,532,800字节/次；CPU复制8.27ms/次、约12.04GB/s；采样中最大画面落后约1.21秒，末端追到约30.7ms；**仍未达到逐帧实时** |
| 同一8K48 / GPU | 47.94 | 0 / 0 | 实际decodeMode=2、uploadPath=7；GPU本地复制，末端画面落后约7.3ms。快照复制计时为CPU提交耗时，不能据此计算真实GPU带宽 |

开启增强前的两份CSV仅保留诊断：早期探针误用render-target version1，目标尺寸字段为0；已改为version20且要求调用成功后重新测量，旧CSV不用于输出尺寸已验证的性能横比。所有QtTest计数含init/cleanup，不能相加视为一次全量回归。新七格式半平面、RTX增强、外部声床模块、原生AVIF、Intel P/E域和空间音频听感未做完整实机矩阵。

已部署本地`dist/VS-GUI-Full`的Renderer/Player/Native，并同步`build/playerlite-runtime`、重新生成`dist/VS-Player.exe`（约34.21MiB）。三处Native SHA-256均为`490d4cd7f7e0cbc95d2ad93b573497557aeeb3b692d91ccf73963a2192c87bbb`；单文件内嵌ZIP CRC、payload哈希和DLL内容一致，未嵌入player.ini或AI worker。Full/Lite配置、VPY、ICC/LUT前后哈希一致；VSCap运行时遇占用后改为只部署本轮文件，未替换其程序。备份在`build/pr-integration-20261024/deployment`，旧单文件另存于同轮`baseline`；[产物清单](evidence/native-upstream-20261024/artifacts.json)记录对应关系。这轮没有发布新版本或上传附件。

## 1.0.13：更新器、单文件BD与运行时精简

2026-10-18实际执行；构建为Qt 6.10.2 / MinGW Release，Native保持前轮DLL `0c5be410c1496ce6217346ba53d1546b6db2fbc1cb5f9129e36723b3daaa4580`，没有修改解码/渲染内核。1.0.12的8K/Y412性能数字属于前轮，不视为1.0.13重新测量；仍不保证8K48逐帧实时。日志留本机 `build/release-1.0.13/`，精选结果在 [summary.json](evidence/release-1.0.13/summary.json)。

| 专项 | 实际结果 | 范围与限制 |
| --- | --- | --- |
| 更新器 | 14 passed / 0 failed / 2 skipped | 隔离安装目录内关闭模拟VSCap、保留另一安装的进程；成功后备份/已验证下载包/临时目录清理、可见重启、ICC/配置保留；压缩MKV更新仍压缩；本地HTTP下载/验证/取消通过。跳过真实ModelScope在线下载及既有发布包审计 |
| 单文件后端 | 独立长卷前后跳转3 passed；之后两轮串行各9 passed / 0 failed / 0 skipped，退出码0 | MyGO下卷真实菜单/长卷、暂停恢复、硬件直通、进度目标保持。首轮串行日志未写完且未留退出码，不能计为通过；单次退出原因未定位，保留日志 |
| 单文件菜单续修 | 组合11 passed / 0 failed；终版两轮窗口级各4 passed / 0 failed，均退出码0 | 实盘菜单时间轴禁用/恢复、列表双击与右键返回、去除铺屏节目子菜单、每轮12次同盘返回、长卷列表播放后重新打开菜单、切回普通文件。候选构建曾捕获一次0xC0000005，调试器下未复现，未断言其精确崩溃点；终版移除原生关闭时的嵌套事件循环，并复用同盘信息，避免重复拆建。最终验证覆盖所述工作流，不代表穷举所有盘型 |
| Full精简VLC与图片 | 15 passed / 1 failed / 0 skipped | 六集菜单选择、返回主菜单和SDR/HDR PNG/AVIF/JXL通过。失败为旧 `bdMenuFromEmptyWindow` 要求重建surface的断言；使用未精简原VLC插件同样复现（2 passed / 1 failed），与当前复用surface的实现不符；未修改该无关测试，也不报告全套通过 |
| Full菜单续修回归 | 10 passed / 0 failed / 0 skipped，退出码0 | 精简VLC下返回菜单、六集选集、选集后交接3FP；不是上面含图片套件的重跑 |
| VSCap AWJ | 3 passed / 0 failed / 0 skipped | 真实HDR AVIF输出；Full截图另覆盖10/12-bit及CICP；不是全显示器色准验收 |
| MKV压缩CLI | 四个工具均能输出102.0版本，小样本Remux与识别成功 | 保留AVC/AAC轨道类型；未重跑长片/全盘Remux，不能外推所有媒体 |

以上QtTest计数含init/cleanup，独立运行计数不相加。AWJ 1.2.1二进制和对应源码按官方SHA-256固定，只分发 `AWJ.com` CLI、许可/NOTICE与对应源码；不分发AWJ GUI。VLC只按BD职责保留插件，不能当作完整VLC播放器使用。MKV保留四个CLI、许可证和英文文档，不分发GUI及其辅助程序；归档首次使用会在用户缓存占用展开空间，后续复用。组件更新保留压缩形式；旧受管工具路径迁移到当前缓存，外部用户指定路径保留。

版本、最终Full/单文件哈希、配置保护和体积见同目录 [artifacts.json](evidence/release-1.0.13/artifacts.json)。从本版起dist不再生成PlayerLite目录；其依赖暂存于build，只交付Full目录及 `VS-Player.exe`。本轮不制作Full压缩包，不上传Release附件。

### VSCap颜色窗口续修

2026-10-18：贴图“拖动”模式点击“颜色”后无法交互。原颜色窗口以QGraphicsProxyWidget中的画布为父级，拖动视图接管鼠标事件；改为归属真实工具栏窗口的独立Qt颜色对话框。

- 旧实现的窗口归属回归为3 passed / 2 failed：贴图拖动、画笔两行失败，普通截图行通过；为检查旧对话框结构，基线强制使用Qt对话框。
- 修复后按默认对话框环境运行三种场景，专项5 passed / 0 failed；验证真实窗口获得焦点、确认/取消、恢复拖动，以及导出线条仍保持确认的蓝色，取消选绿不改变颜色。
- 整套VSCap工作流20 passed / 0 failed / 0 skipped，退出码0；含初始化/清理，不与专项相加。期间有系统剪贴板占用告警，相关断言最终通过；不是多屏/混合DPI的人工验收。
- 证据见 [vscap-color](evidence/release-1.0.13/vscap-color/summary.json)。只部署Full的VSCap和对应指南，保留配置、Player和单文件播放器；新Full哈希以本次 [artifacts](evidence/release-1.0.13/vscap-color/artifacts.json) 为准，上方首轮artifacts保留历史值。

核对时1.0.13已正式发布且维护者已上传附件；本次不修改Release、tag或附件，不制作压缩包。源码/本地Full续修不代表既有1.0.13下载包已包含修复。

### HDR软解与默认色彩续修

2026-10-18：指定《花の塔》MP4为3840×2160 HEVC Main10、BT.2020/HLG、Dolby Vision Profile 8.4。旧默认/自动libplacebo路径下，软解Jinc和D3D11实际读回图像均只有两种绿色像素，红蓝通道为0；硬解有图像，但旧默认仍请求引擎2。原生3FP能正常输出，保留素材有意过曝的观感，本次没有修改Native DLL、HDR映射或亮度系数。

- 默认改为3FP（0），旧自动档（2）启动迁回0；移除自动高级引擎选项，Full只保留手动libplacebo（1）对照。显式手动档与ICC/LUT参数保留。
- 指定HLG实片、配置迁移和VS链不重载专项：旧版4 passed / 6 failed；修复后15 passed / 0 failed / 0 skipped，退出码0。失败旧例包括4条实际绿屏和2条旧默认引擎断言；不是硬解画面失败。
- `HDRTEST/out`实际19个AV1 10-bit文件，每个软解/硬解×Jinc/D3D11共76次呈现；38组同算法软硬解对照，在64×36平滑缩略图上的最大平均RGB绝对差为0.00019744（约0.0197%），最大单通道8-bit差2。包含9个明确PQ/BT.2020标记样本；其余10个缺传递函数等标记（含两个命名为SDR的对照），只能确认按当前标记的输出一致，不能追认为HDR色准通过。
- 目录回归与已有SDR/PQ/HLG中性灰、Jinc对照共22 passed / 0 failed / 0 skipped，退出码0；灰阶测试中最大算法通道差0.000977516。单文件后端基础回归5 passed / 0 failed，退出码0；各组计数含初始化/清理，不相加成全套回归。
- 本次实片呈现的实际输出模式为SDR（HDR→SDR），不代表物理HDR显示器或色度计校准验收。截图只留本机build，不上传用户视频或画面；19个目录样本的原文件哈希核对不变。

证据见 [hdr-native](evidence/release-1.0.13/hdr-native/summary.json)，部署与单文件信息见 [artifacts](evidence/release-1.0.13/hdr-native/artifacts.json)。版本保持1.0.13，更新本地Full和单文件；已发布tag、Release及附件不修改，Full压缩包仍由维护者制作。

### 10月18日：长视频相对逐帧与单文件打开

指定《Girls Band Cry》2160p48 AV1/Opus MKV，实际帧率7001/146、时间基1/1000。在60秒定位并按+1秒后，旧按钮调用绝对帧号定位，会从片头重建索引；HW-D3D11旧例2 passed / 1 failed，单次进帧超过5秒。改走相对PTS后又发现关键帧跳转留下的稀疏缓存不能直接视为相邻帧，已改为解码当前PTS附近的GOP确认严格前后帧，并处理片头继续退帧的无操作边界。单文件相对预设路径还暴露首次打开先用clock-only再切直通丢失会话的问题，现打开时固定直通；没有新增帧池或修改解码/色彩参数。

- Full四条实片路径（软/硬解×Jinc/D3D11）在1、10、20分钟位置，双向逐帧、关键帧后退帧、方向键、播放中逐帧暂停、片头边界和续播通过；GPU截图缩略内容确实变化，每次媒体时间移动20或21毫秒。连同25/50fps可变帧率相邻帧测试，7 passed / 0 failed / 0 skipped。60次按钮样本为50–1480ms；仍需解码局部GOP，不保证瞬时逐帧。
- 最终DLL的Full聚焦回归7 passed / 1 failed / 0 skipped，退出码1：指定实片HW-D3D11三位置、可变帧率、VS输出帧号进退、HEVC/AV1生命周期通过；失败为既有 `frameEditTakesPriorityAndPrefetchKeepsGpuBusy` 程序化移焦断言。用HEAD的 `PlayerWindow.cpp`、`PlayerControls.cpp` 与旧生产DLL重链同一测试，独立对照仍2 passed / 1 failed，同位置失败；未修改该用例或相关焦点逻辑，不报告全量回归通过。
- 最终单文件后端实片软/硬解×Jinc/D3D11、相对预设打开、逐帧后续播及既有模式/软解基础回归6 passed / 0 failed / 0 skipped，退出码0；确认未初始化VS。
- 全矩阵初测Native为 `1fc2385a4ed4ea2bf54d24fa12518f4106b46194646490e8cda5cde0328d8706`；随后只更新三行注释重建，最终Native为 `3fd4737c9e455fa2ba62d31579f80098df42a80546d8a85d2cffc6e81f724c24`，完成上述最终Full/Lite回归。完整补丁对固定上游PR23可应用，生产源码逆向检查通过，API布局不变。

证据见 [frame-stepping](evidence/release-1.0.13/frame-stepping/summary.json)，本地Full与单文件已更新；[产物校验](evidence/release-1.0.13/frame-stepping/artifacts.json)涵盖相同Native、ZIP CRC、图标、实际启动器子进程、外置FFmpeg、24份Full/1份内部单文件配置与预设等保护文件哈希保持不变，VSCap二进制保持不变。单文件35,809,965字节；版本仍1.0.13，已发布tag、Release及附件保持不变，Full压缩包由维护者制作。绝对帧号输入仍可能因未建立索引而需要较长等待，本轮修复的是相对逐帧。

## 1.0.12：按续修轮次区分

| 轮次 / 场景 | 已测与结论 | 保留的限制 / 证据 |
| --- | --- | --- |
| BD拖动与直接主菜单 | 普通视频/分集拖动6 passed；实盘菜单4 passed | 最初没有覆盖整卷失败，旧“慢例”已纠正；[原记录](history.md#record-docs-update-1-0-12-md) |
| 整卷点击/拖动续修 | MyGO下卷约8538.53秒，普通/BD、软/硬解、播放/暂停、跨段点击/反复拖动、双向seek与菜单；14 passed / 0 failed / 0 skipped | Lite该轮7 passed / 1 skipped；报告引用`build/bd-full-seek-after.txt`，原失败`build/bd-full-mouse-before.txt` |
| Jinc色彩与无音轨VS | 指定部署Full DLL，灰阶/8K SDR、四档Anime、时钟、VS属性、桌面白位、HDR FP16、BD；14 passed / 0 failed / 0 skipped | 不是全套，GPU像素不等于显示器绝对色准；[原记录](history.md#record-docs-player-color-vs-1-0-12-md) |
| Anime预取/大文件/菜单 | 专项13 passed / 0 failed；交互另3 passed；Lite7 passed / 1 skipped | 4K24→4K、Y41248→1440p指定20/30秒窗口通过；Y412→4K48仍非通过项；[原记录](history.md#record-docs-player-anime-performance-1-0-12-md) |
| BD VS按段索引与续播 | 四种菜单路径、全新索引、远距离定位、自然跨界，6 passed / 0 failed；冷首帧5.54～5.68秒；该轮Lite8 passed | 解析/缓存8 passed / 3 skipped（缺该套件实盘变量）；未跑全量Full；报告引用`build/bd-lazy-boundary-final.txt` |
| 软件内核最终回归 | 生命周期4 passed；聚焦9 passed / 3 skipped；默认设置6 passed；Lite6 passed；BD目录8 passed / 3 skipped | 聚焦跳过：两项缺独立素材、一项前台占用；语言脚本仍报9个已复现基线缺失；[保存的日志](evidence/player-kernel-1.0.12/final-regressions.txt) |

## 1.0.12：2026-10-18 软件同步与内存续修

目标：8K48允许省略画面，优先接近实际经过时间；4K48 Y412保留既有吞吐，减少缓冲和内存。以下为本轮真实运行，不能视作全量回归。原始日志在本机 `build/perf-sync-20261018/`，精选数据见 [summary.json](evidence/player-sync-20261018/summary.json)。

环境：7940HX / RTX4070 Laptop，2560×1600 /165Hz；完整Qt播放器全屏surface 2561×1601，CPU解码、D3D11，保留音频/字幕。用户确认115W增强模式、CPU可稳定95–100W；本轮没有独立采集功耗或重新查询Windows HDR开关，素材均为SDR。每轮定位10秒、预热5秒后采样；默认MMCSS/DXGI等待对象均关闭，无线程环境覆盖。

| 素材 / DLL | 实际采样窗口 | 媒体进度 / 画面PTS进度 | 呈现fps | 新增原生丢弃 / 合并 / 音频欠载 | 末尾私有内存 / 本地显存 |
| --- | --- | --- | --- | --- | --- |
| ChouKaguya432048-Part，旧DLL | 30.001秒 | 26.146 / 25.442秒 | 40.532 | 4 / 0 / 130 | 4669 / 377 MiB |
| 同一8K，最终DLL | 60.011秒 | 59.965 / 60.331秒 | 33.877 | 860 / 0 / 12 | 3835 / 277 MiB |
| OP-yuv444p12-SWTest，旧DLL | 60.009秒 | 60.008 / 60.018秒 | 47.959 | 0 / 0 / 0 | 6441 / 232 MiB |
| 同一Y412，最终DLL | 60.006秒 | 60.005 / 59.997秒 | 47.945 | 0 / 0 / 0 | 5153 / 185 MiB |
| Rebellion HEVC444p10 700Mbps，最终DLL | 30.006秒 | 30.005 / 29.988秒 | 23.962 | 0 / 0 / 0 | 2897 / 161 MiB |

本轮8K媒体进度/墙钟约99.92%，末尾画面落后媒体时钟0.482秒，每秒采样最大1.429秒；关键包之间仍有落后与追赶波动。60秒呈现2033帧、省略860帧，另插入67680个输出音频sample frames；**仍非48fps逐帧实时，也不是无缺口音频**。丢弃计数与CFR源PTS对应帧数核对误差小于4帧，暂停时位置冻结、定位第72帧并截图、恢复播放、跳至片尾均通过。定位后的恢复预填充约1.65秒，之后单独核对稳态进度。8K旧/新窗口长度不同，不作严格吞吐提升百分比。

Y412最终使用24工作线程/24帧上下文，对照旧32/32，60秒窗口私有内存下降19.99%、进程本地显存下降20.58%，零新增丢帧/合并/欠载。CPU进程时间/墙钟为8.101→8.105，基本持平，不能把线程数减少解释成计算成本同比下降。内存数字为采样末尾的进程私有提交与按adapter合计的DXGI本地用量，不是全机内存、整个显卡总占用或分配器硬上限；早期用QSignalSpy保留每次字幕QImage产生的13–16GiB结果无效，已修正测试采样并重新测基线。

实现范围：按帧字节约束dav1d深度、在途帧和显示队列；默认单交接槽、关闭额外双DEFAULT/双STAGING上传环，保留实验开关；8K AV1恒帧率持续落后时只在关键包切换完整/非参考帧过滤，定位恢复完整解码；欠载静音提交推进音频时钟并去掉晚到音频重叠。压缩视频包上限仅8K CPU AV1由16升至32MiB、仍限64包，避免长GOP堵住后续音频。未改变素材分辨率、位深、色度、色彩shader或FFmpeg版本。

构建与专项：完整Native补丁对固定上游 `git apply --check` 成功，并从独立checkout重建成功。最终生产Native SHA-256：`0c5be410c1496ce6217346ba53d1546b6db2fbc1cb5f9129e36723b3daaa4580`；旧DLL `fc61e2f52e6d418c02ea311cb1059649f18a541b8be4af62375e9af0b18fd39a`；avcodec保持 `07298f3f3bb71ba121e536f2495402ce43dddbb0a3b9d40178a20e19437c4c79`。最终8K同步/导航、Y412吞吐、HEVC吞吐各3 passed / 0 failed / 0 skipped（含init/cleanup）；[Full聚焦回归](evidence/player-sync-20261018/final-regressions.txt)12 passed、[Lite](evidence/player-sync-20261018/final-lite.txt)6 passed，均0 failed / 0 skipped（含init/cleanup）；覆盖HEVC/AV1生命周期、实际Y412多方向定位与恢复、无音轨时钟、音频轨道与欠载统计、呈现计数。3840×2160合成8/10/16-bit SDR/PQ上传与旧DLL对照最大RGB误差0，Jinc色彩专项通过；这不是显示器色准验收。语言完整性脚本仍有两包各9个已复现基线缺失，本轮新增0，详见[languages.json](evidence/player-sync-20261018/languages.json)。

本地 `dist/VS-GUI-Full`、`dist/1.0.12-PlayerLite` 已部署；初次因VSCap占用中断，用户退出后完成原目录更新。INI/VPY/ICC/LUT哈希保持不变，旧包残留的开发记录转存到本机部署备份，运行目录仅保留现行指南。[单文件与产物校验](evidence/player-sync-20261018/artifacts.json)涵盖manifest/Native一致、ZIP CRC、图标、解包子进程导入、外置FFmpeg、配置和原盘索引保持不变。`dist/VS-Player.exe`为34.15MiB，仍需外部FFmpeg组件。版本保持1.0.12，不修改已正式发布的GitHub Release附件。

## 1.0.12：10月17日性能与最终 DLL 对应

环境：7940HX / RTX4070 Laptop，Windows HDR关闭，显示器2560×1600 /165Hz；完整Qt播放器全屏surface 2561×1601、16:9内容约2560×1440，CPU解码、D3D11，保留音频/字幕。定位10秒、预热5秒后采样。

| 样本 / 轮次 | 实际窗口 | 呈现 fps | 新增丢帧 / 合并 / 音频欠载 | 结论 |
| --- | --- | --- | --- | --- |
| 4K AV1 YUV444P12 OP | 30.000秒，单轮 | 47.967 | 0 / 0 / 0 | 采样窗口通过；起采前已有2个启动期丢帧，不是全程零丢帧 |
| HEVC444p10 700Mbps | 60.004秒，单轮 | 23.982 | 0 / 0 / 0 | 指定窗口通过；初始16线程，启动曾升24，不能写为始终固定16 |
| 8K AV1 420p10，16/16 | 20.000秒 | 34.00 | 212 / 0 / 19 | 未达8K48实时 |
| 同一8K，8/16 | 20.011秒 | 40.578 | 1 / 0 / 89 | 仍非实时 |
| 同一8K，8/8+双槽+软提示+上传组合 | 20.010秒 | 40.53 | 1 / 0 / 87 | 仍非实时；该轮按用户要求停止追逐45fps |

前两项采样启用了默认关闭的MMCSS/DXGI等待对象，不能表述为默认配置成绩。与1.0.11的4K输出31.03fps条件不同，不计算严格A/B提升百分比。14700K P/E分支未在该机实测，强制绑核等负收益实验未默认启用。

采样后只追加交接槽容量互斥保护，重跑生命周期4 passed，没有重跑两项性能采样。采样Native SHA-256：`991069f9b84aa8bd568c12d031c02738ae3c940bfa7956f940df9f149a7a5fa4`；最终Native：`fc61e2f52e6d418c02ea311cb1059649f18a541b8be4af62375e9af0b18fd39a`；avcodec：`07298f3f3bb71ba121e536f2495402ce43dddbb0a3b9d40178a20e19437c4c79`。依据见 [summary.json](evidence/player-kernel-1.0.12/summary.json) 和同目录CSV/日志。

## 跨版本结论如何更新

| 历史问题 | 当前可确认的变化 | 仍不能确认的内容 |
| --- | --- | --- |
| 1.0.10/11长盘跨段点击/拖动失败 | 1.0.12同实盘整卷鼠标路径已修复并验证 | 其它盘型、菜单后端全部路径 |
| 1.0.9 4K444p12完整GUI不实时 | 1.0.12指定OP软件直通窗口47.967fps；Anime48输出1440p另有通过窗口 | 不同机器/输出4K增强/所有高压字幕片段全面实时 |
| 1.0.10/11 8K48软件不实时 | 1.0.12指定条件约40.53fps，但仍欠载 | 实时目标仍未通过，不能关闭该项 |
| 1.0.9黑屏、1.0.10文字/选区、1.0.11白位/JXL | 各轮有对应修复与专项依据 | HDR端到端耗时、多屏DPI、UIAccess及全部面板观感 |
| 1.0.10合入PR22当轮未跑测试 | 后续1.0.11/12在对应后续基线运行聚焦测试 | 不能追认当轮已测，也不等于PR23全量回归完成 |

## 维护与10月17日文档校验

后续在本文件增加版本行与必要轮次，每项记录构建/素材/环境、实际范围、通过/失败/跳过、目标是否满足和证据位置。日志留build，必要数据集中到evidence，不再新增零散测试说明。

10月17日该轮仅整理文档和文档复制路径；未构建、未启动播放器、未重新测性能、未更新dist或Release，不能把历史数字当作该轮执行结果。

2026-10-17 文档校验通过：87份归档逐份核对字节数/SHA-256，并与Git HEAD原文比对（仅容许Git工作区换行转换）；14份证据、3份第三方文件内容保留；103处本地链接/锚点有效；11节项目骨架、13个倒序版本完整；2份Python语法、5份PowerShell语法和`git diff --check`通过。隔离目录复制10份指南、4份许可/清单成功，指南哈希与源文件一致。检查摘要在本机`build/doc-reorganization-check.json`；没有执行完整部署或打包脚本。

## 1.0.17局部续修：单文件多pass HLSL（2026-11-06）

仅为本机MoeSR 52-pass模型增加原生HLSL的HOOK分段读取，普通单pass导入保持兼容。ShaderPackage专项QtTest为14通过、0失败；加入多pass用例后先观察到旧路径资源重定义失败，再修复为逐段编译。测试日志、真实52-pass文件导入及三种D3D11数值用例保存在本机`EnhanceModel-Test-New/GLSL-Student/Production-3d-20261101/acceptance-20261106`，入口为该目录的ImportProbe.cpp和HlslProbe.cpp。

真实1920×1080→3840×2160输出相对同权重PyTorch AMP的最大RGB绝对误差0.00146484、平均0.00013364；GPU执行计时约47.29ms，排除解码、显示和回读，不能当作播放器实测帧率。VSPlayerLite构建成功，另存`dist/VS-Player-MoeSR.exe`；原`dist/VS-Player.exe`SHA256保持`4609364236946011ce13d3857784ad222580f6ee73774534c4c7fc471f263015`。未执行播放器菜单/视频播放验收或整套回归，未改版本、提交、发布。
