# 1.0.12：BD 拖动定位与菜单入口

## 实现

BD 拖动进度条期间仅更新滑块位置；松开后提交一次最终定位，避免精确 seek 与中间帧渲染积压。适用于 BD 播放列表、光盘列表中的片段及活动菜单后端；普通视频保留拖动预览。播放 / 暂停状态沿用拖动开始前状态。

打开 BD 选择光盘菜单模式时，使用已有直接 Top Menu 缓存入口，跳过版权 FirstPlay。菜单缓存引用原盘素材，不修改原始 index.bdmv 或影片。低层原始 FirstPlay 接口仍可单独测试。

## 验证与交付

本轮 Release 构建完成。MyGO 下卷实盘 BD 拖动测试覆盖暂停与播放两种状态：拖动期间时间线代次不变、没有挂起 / 排队的定位，松开后仅增加一次代次，约 110～112 ms 到达 426.426 秒并恢复原状态。普通视频鼠标点击与拖动的软件解码两种播放状态通过，合计 6 passed（含 init/cleanup）。

菜单实盘 4 passed（含 init/cleanup）：首次打开后 1.5 秒截取的原生菜单图像已显示 PLAY ALL、第 8～13 集和字幕选项，可直接点击第 9 集并交回 3FP；返回菜单、再次选集及暂停后定位通过。截图为 build/bd-1.0.12-initial-menu.png。libVLC 通过替换 FirstPlay 的缓存进入主菜单时，标题编号仍可保持 FirstPlay 编号，不能把 title==0 当作唯一选集页判断；本轮以实际菜单图像和首次直接选集验证。

Blu-ray 解析 / 缓存测试 8 passed、3 skipped（对应环境变量未设）。Lite 测试 7 passed、1 skipped（实盘原始 FirstPlay 测试未设环境变量）。日志在 build/bd-1.0.12-*.txt。未运行全量播放器套件。

旧长光盘综合用例的单次 6426 秒定位在修复前也未能于 3 秒内完成；本轮拖动提交修复不等于底层全部长光盘随机定位性能已通过。

版本更新为 1.0.12；Full 固定部署在 dist/VS-GUI-Full，目录式 Lite 为 dist/1.0.12-PlayerLite，单文件为 dist/VS-Player.exe。保留 INI、VPY 和色彩配置；单文件沿用 FFmpeg 外置与同目录 player.ini，包含外层 Player 图标。源码上传 GitHub，发布压缩包由维护者自行制作。


部署审计 build/deployment-20261017-164604/deployment.json 确认 Full / Lite 的 INI、VPY、ICC/ICM、LUT 哈希保持一致。单文件通过 ZIP CRC、payload 哈希、1.0.12 清单、无 FFmpeg / 用户 INI 内嵌、图标资源组 101 及 Native 同源核对；播放器二进制除 strip 生成的 PE 时间戳和校验和外与目录式 Lite 一致。复制启动器到隔离目录，使用外部 FFmpeg 路径执行真实解包及子进程图像导入成功，已有 INI 哨兵内容不变；原盘及 BACKUP 的 index.bdmv 哈希不变。单文件审计为 build/audit-bd-1.0.12-single/result.json。未制作 Full / Lite 发布压缩包，未上传二进制附件。
