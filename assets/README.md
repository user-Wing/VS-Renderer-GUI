# 启动预热短片

`startup-warmup.mkv` 为本项目生成的 128×72 黑场 H.264 视频与静音 AAC，不含外部素材，嵌入 Qt resource。用于启动时初始化解码、音频、VS 源滤镜与双路渲染。

重建命令：

```powershell
ffmpeg -hide_banner -loglevel error -y -f lavfi -i "color=c=black:s=128x72:r=24:d=0.25" -f lavfi -i "anullsrc=r=48000:cl=stereo" -shortest -c:v libx264 -preset ultrafast -pix_fmt yuv420p -c:a aac assets/startup-warmup.mkv
```
