#include "player/PlayerWindow.h"
#include "player/PlayerInfoPanel.h"
#include "player/PlayerDiscMenu.h"
#include "backend/ThreeFpPlayer.h"
#include "ui/PreviewPane.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDateTime>
#include <cmath>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QThread>
#include <QColorSpace>
#include <windows.h>
#include <psapi.h>
#include <dxgi.h>

namespace vsr {
void PlayerWindow::updateInfo() {
    if(discMenu_ && discMenu_->active()){
        const auto programme=discMenu_->programme();
        if(infoVisible_)static_cast<PlayerInfoPanel *>(info_)->showText(tr("BD：%1\n渲染器：libVLC · 光盘导航/菜单合成\n位置：%2 s · 时长：%3 s\n输入视频：%4 × %5 · %6 fps\n视口：%7 × %8\n当前音轨：%9 · 当前字幕：%10\n章节：%11\n模式：%12\n处理链：菜单交互；正片识别后恢复所选滤镜").arg(source_).arg(position()/10000000.,0,'f',2).arg(discMenu_->duration()/10000000.,0,'f',2).arg(programme.value("width").toInt()).arg(programme.value("height").toInt()).arg(programme.value("fps").toDouble(),0,'f',3).arg(pane_->surface()->width()).arg(pane_->surface()->height()).arg(programme.value("audio").toInt()).arg(programme.value("subtitle").toInt()).arg(programme.value("chapters").toArray().size()).arg(discMenuNavigation_?tr("交互菜单 (方向键/Enter)"):tr("短节目")));
        return;
    }
    if(imageMode_){
        if(!infoVisible_)return;
        usage_=resources_.sample();const auto image=pane_->image();const QFileInfo file(source_);
        const auto fitted=QSizeF(pane_->imageDisplaySize()).scaled(QSizeF(pane_->surface()->size()),Qt::KeepAspectRatio)*pane_->zoom();
        const auto color=image.colorSpace().isValid()?image.colorSpace().description():tr("未提供");
        QStringList lines;
        lines<<tr("文件名：%1").arg(file.fileName())<<tr("路径：%1").arg(file.absoluteFilePath())
             <<tr("文件大小：%1 MiB · 格式：%2").arg(file.size()/1048576.0,0,'f',2).arg(file.suffix().toUpper())
             <<tr("输入图片：%1 × %2 · %3 MP · 颜色空间：%4").arg(image.width()).arg(image.height()).arg(double(image.width())*image.height()/1e6,0,'f',2).arg(color)
             <<tr("源像素：%1 · 原始位深：%2 bit/channel").arg(image.text("sourcePixelFormat").isEmpty()?file.suffix().toUpper():image.text("sourcePixelFormat"),image.text("sourceBitDepth").isEmpty()?tr("未提供"):image.text("sourceBitDepth"))
             <<tr("解码器：%1 · 耗时：%2 ms").arg(image.text("decoder"),image.text("decodeMilliseconds"))
             <<tr("解码像素：%1 bit/pixel · Alpha：%2 · 像素内存：%3 MiB").arg(image.depth()).arg(image.hasAlphaChannel()?tr("是"):tr("否")).arg(image.sizeInBytes()/1048576.0,0,'f',1)
             <<tr("输出：%1 × %2 · 视口：%3 × %4 · 缩放：%5%").arg(fitted.width(),0,'f',0).arg(fitted.height(),0,'f',0).arg(pane_->surface()->width()).arg(pane_->surface()->height()).arg(pane_->zoom()*100,0,'f',1)
             <<tr("图片渲染器：Qt Raster · 可见区域裁切 · SDR")
             <<tr("VS 滤镜：未启用 · 预解码：未启用")
             <<tr("CPU：%1 · GPU：%2 · 内存：%3 MiB").arg(usage_.processCpu>=0?QString::number(usage_.processCpu,'f',1)+"%":tr("采样中"),usage_.gpu>=0?QString::number(usage_.gpu,'f',1)+"%":tr("未提供")).arg(usage_.memoryMiB);
        static_cast<PlayerInfoPanel *>(info_)->showText(lines.join('\n'));info_->raise();return;
    }
    usage_=resources_.sample();
    const auto refreshed = QJsonDocument::fromJson(clock_->mediaInfo().toUtf8()).object();
    if (!refreshed.isEmpty()) media_ = refreshed;
    applyBlurayMetadata();
    const auto selectedAudio=clock_->snapshot().selectedAudioStream;
    QJsonObject video, audio;
    for (const auto &entry : media_.value("streams").toArray()) {
        const auto stream = entry.toObject();
        if (stream.value("type").toString() == "video" && video.isEmpty()) video = stream;
        if (stream.value("type").toString() == "audio" && stream.value("index").toInt()==selectedAudio) audio = stream;
    }
    videoBadge_->setText(audioMode_?tr("音频"):video.value("codec").toString("—")); audioBadge_->setText(audio.value("codec").toString("—"));
    const auto source = clock_->snapshot(); const auto rendered = outputSnapshot();
    const auto decoder = source_.isEmpty() ? (settings_->value("decode/mode",2).toUInt()==2?QStringLiteral("3FP-HW"):QStringLiteral("3FP-SW")) : lavVideo_ ? QStringLiteral("LAV") : !direct_ ? QStringLiteral("VS / VPY") : source.decodeMode == 2 ? QStringLiteral("3FP-HW") : QStringLiteral("3FP-SW");
    decoderBadge_->setText(decoder); hdrBadge_->setText(source.isHdrSource ? "HDR" : "SDR");
    hdrBadge_->setToolTip(madvrMode()?tr("HDR / SDR 输出与色调映射由 madVR 控制"):source.isHdrSource ? (rendered.actualColorMode ? tr("HDR 输出") : tr("HDR 源，映射到 SDR 显示器")) : tr("SDR 源"));
    if (!infoVisible_) return;
    const auto count=rendered.presentedVideoFrames>=rendered.coalescedVideoFrames?rendered.presentedVideoFrames-rendered.coalescedVideoFrames:0;
    if(infoFpsTimer_.isValid() && infoFpsTimer_.elapsed()>=500){infoCurrentFps_=count>=infoLastFrames_?(count-infoLastFrames_)*1000./infoFpsTimer_.elapsed():0;infoLastFrames_=count;infoFpsTimer_.restart();}
    else if(!infoFpsTimer_.isValid()){infoFpsTimer_.start();infoLastFrames_=count;}
    QString gpu = tr("未知"); IDXGIFactory1 *factory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory1(IID_IDXGIFactory1, reinterpret_cast<void **>(&factory)))) {
        IDXGIAdapter1 *adapter = nullptr;
        if (SUCCEEDED(factory->EnumAdapters1(0, &adapter))) { DXGI_ADAPTER_DESC1 desc{}; adapter->GetDesc1(&desc); gpu = QString::fromWCharArray(desc.Description); adapter->Release(); }
        factory->Release();
    }
    const auto fps = clip_.fpsDenominator > 0 ? static_cast<double>(clip_.fpsNumerator) / clip_.fpsDenominator : 0;
    const auto at = position();
    const auto videoRate = source.videoBitRate ? source.videoBitRate : static_cast<quint64>(video.value("bitRate").toDouble());
    const auto audioRate = source.audioBitRate ? source.audioBitRate : static_cast<quint64>(audio.value("bitRate").toDouble());
    const auto formatRate = [](quint64 value) { return value ? QString::number(value / 1000) + " kb/s" : tr("未知"); };
    const auto formatTime = [](qint64 ticks) { const auto ms = qMax<qint64>(0,ticks / 10000); return QString("%1:%2:%3.%4").arg(ms/3600000,2,10,QChar('0')).arg(ms/60000%60,2,10,QChar('0')).arg(ms/1000%60,2,10,QChar('0')).arg(ms%1000,3,10,QChar('0')); };
    const double sourceFps = video.value("averageFrameRateDenominator").toInt() > 0 ? video.value("averageFrameRateNumerator").toDouble() / video.value("averageFrameRateDenominator").toInt() : fps;
    const double frameTime = clip_.fpsNumerator > 0 ? lastFrame_ * 10000000.0 * clip_.fpsDenominator / clip_.fpsNumerator : 0;
    const auto fitted = QSize(clip_.width, clip_.height).scaled(pane_->surface()->size()*pane_->surface()->devicePixelRatioF(), Qt::KeepAspectRatio) * pane_->zoom();
    const auto outputBits = audio.value("outputValidBitsPerSample").toInt();
    const bool lavAudio=lavAudio_;
    const auto audioOutput = lavAudio ? tr("由 LAV / DirectShow 协商") : QString("%1 · %2 Hz · %3 声道 · %4 bit")
        .arg(audio.value("outputFloat").toBool() ? "Float PCM" : "PCM").arg(audio.value("outputSampleRate").toInt()).arg(audio.value("outputChannels").toInt()).arg(outputBits);
    QStringList lines;
    lines << tr("文件名：%1").arg(QFileInfo(source_).fileName())
          << tr("当前时间：%1 · 时间轴：%2 / %3 (%4%) · #帧数：%5 / %6")
             .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), formatTime(at), formatTime(source.duration100ns))
             .arg(source.duration100ns > 0 ? at * 100.0 / source.duration100ns : 0, 0, 'f', 1).arg(lastFrame_).arg(clip_.totalFrames)
          << tr("CPU：%1 · GPU：%2 · 内存：%3 MiB").arg(usage_.processCpu>=0?QString::number(usage_.processCpu,'f',1)+"%":tr("采样中"),usage_.gpu>=0?QString::number(usage_.gpu,'f',1)+"%":tr("未提供")).arg(usage_.memoryMiB)
          << ""
          << tr("视频解码器：%1 · VS 源：%2").arg(decoder, direct_ ? tr("原生直通") : preset_.isEmpty() ? "FFMS2" : tr("VPY 定义"))
          << tr("输入：%1 · %2×%3 · %4-%5 · 帧率：%6 · 位率：%7")
             .arg(video.value("codec").toString()).arg(video.value("width").toInt()).arg(video.value("height").toInt())
             .arg(video.value("pixelFormat").toString(),video.value("profile").toString()).arg(sourceFps,0,'f',3).arg(formatRate(videoRate))
          << (madvrMode()?tr("输出：%1 · %2×%3 · 目标帧率：%4 · 显示格式由 madVR 协商%5%6"):tr("输出：%1 · %2×%3 · 目标帧率：%4 · 显示：%5 bit / %6"))
             .arg(madvrMode()?tr("LAV / madVR 协商"):direct_?video.value("pixelFormat").toString():settings_->value("decode/output").toString().isEmpty()?clip_.formatName:settings_->value("decode/output").toString()).arg(clip_.width).arg(clip_.height).arg(fps*speed_,0,'f',3).arg(madvrMode()?QString():QString::number(rendered.videoOutputBitDepth)).arg(madvrMode()?QString():rendered.actualColorMode ? "HDR" : "SDR")
          << (madvrMode()?tr("视频渲染器：madshi video renderer"):tr("视频渲染器：VS Real-Time Video Renderer"))
          << tr("  设备：%1").arg(gpu)
          << (madvrMode()?tr("  帧与丢帧统计：由 madVR 控制器提供"):tr("  已提交：%1 · 丢帧：%2(VS 跳过 %3 / 渲染丢弃 %4 / 合并 %5)")
              .arg(direct_?rendered.presentedVideoFrames:submittedFrames_).arg(skippedFrames_+rendered.droppedVideoFrames+rendered.coalescedVideoFrames).arg(skippedFrames_).arg(rendered.droppedVideoFrames).arg(rendered.coalescedVideoFrames))
          << ((madvrMode() || direct_)?tr("  VS 处理：未启用 · 原生直通"):tr("  VS 请求耗时：%1 ms · 同步偏移：%2 ms · 平均呈现等待：%3 ms · 预解码：%4 帧")
              .arg(frameMilliseconds_,0,'f',1).arg((frameTime-at)/10000,0,'f',1).arg(rendered.swapChainPresents?rendered.presentWait100ns/10000.0/rendered.swapChainPresents:0,0,'f',2).arg(prefetchCount()))
          << tr("视频帧大小：源 %5×%6 → VS 输出 %3×%4 → 当前呈现 %1×%2")
             .arg(fitted.width()).arg(fitted.height()).arg(clip_.width).arg(clip_.height).arg(video.value("width").toInt()).arg(video.value("height").toInt())
          << ""
          << tr("音频解码器：%1").arg(lavAudio ? "LAV Audio Decoder" : "3FP / FFmpeg")
          << tr("输入：%1 · %2 Hz · %3 声道 · %4 · %5 bit · %6")
             .arg(audio.value("codec").toString()).arg(audio.value("sampleRate").toInt()).arg(audio.value("channels").toInt())
             .arg(audio.value("sampleFormat").toString()).arg(audio.value("rawSampleBits").toInt()).arg(formatRate(audioRate))
          << tr("输出：%1").arg(audioOutput)
          << tr("渲染输入：%1").arg(audioOutput)
          << tr("音频渲染器：%1").arg(lavAudio ? "DirectShow Audio Renderer" : "Built-in WASAPI Audio Renderer")
          << (lavAudio ? tr("  缓冲时间：未提供 · 同步偏移：未提供") : tr("  缓冲时间：%1 ms · 同步偏移：%2 ms · 时间戳抖动帧：%3")
              .arg(source.bufferedAudio100ns/10000.0,0,'f',1).arg((source.audioPosition100ns-at)/10000.0,0,'f',1).arg(source.audioTimestampJitterFrames))
          << "" << tr("倍速：%1× · %2").arg(speed_,0,'f',2).arg(preset_.isEmpty() ? tr("原画") : QFileInfo(preset_).fileName());
    lines << tr("补帧：%1").arg(interpolationStage()>=0?interpolationNames().at(interpolationStage())+(interpolationAuto_?tr(" · 自动降档"):tr(" · 手动固定")):tr("未启用"));
    const QStringList algorithms{"Nearest Neighbour","Bilinear","Bicubic","Lanczos3","Jinc","Spline36","Super-XBR","D3D11","Lanczos4"};
    lines << tr("缩放滤镜：%1").arg(madvrMode()?QString("madVR"):interpolationStage()>=0?(interpolationRenderer()?tr("D3D11"):tr("Jinc 直通")):profile()=="Anime"||profile()=="Realistic"||preset_.isEmpty()?qualityNames().at(std::clamp(qualityStage_,0,5)):algorithms.value(settings_->value("render/upscale",4).toInt(),tr("自定义")));

    if(madvrMode())lines << tr("色彩引擎：madVR");
    else {
        const auto color=(direct_?clock_:output_)->colorStatus();
        lines << tr("色彩引擎：%1 · %2").arg(QString::fromUtf8(color.engine),color.activeEngine?"libplacebo D3D11":tr("3FP 原生"));
        if(color.activeEngine) {
            const auto tagged=[](unsigned value,const QStringList& names){return QString("%1 (%2)").arg(names.value(value,"Unknown")).arg(value);};
            lines << tr("色彩输入：Matrix %1 · Primaries %2 · Transfer %3 · Range %4 · Chroma %5 · %6 bit")
                .arg(tagged(color.sourceMatrix,{"RGB","BT.709","Unspecified","Reserved","FCC","BT.470BG","SMPTE170M","SMPTE240M","YCgCo","BT.2020 NCL","BT.2020 CL","SMPTE2085","Chroma NCL","Chroma CL","ICtCp"}))
                .arg(tagged(color.sourcePrimaries,{"Reserved","BT.709","Unspecified","Reserved","BT.470M","BT.470BG","SMPTE170M","SMPTE240M","Film","BT.2020","SMPTE428","P3-DCI","P3-D65"}))
                .arg(tagged(color.sourceTransfer,{"Reserved","BT.709","Unspecified","Reserved","Gamma2.2","Gamma2.8","SMPTE170M","SMPTE240M","Linear","Log","Log-sqrt","IEC61966-2-4","BT.1361","sRGB","BT.2020 10","BT.2020 12","PQ","SMPTE428","HLG"}))
                .arg(tagged(color.sourceRange,{"Unspecified","Limited","Full"})).arg(tagged(color.sourceChroma,{"Unspecified","Left","Center","Top-left","Top","Bottom-left","Bottom"})).arg(color.sourceBits);
            lines << tr("元数据来源：%1 · 显式字段掩码 %2(其余按兼容规则推断)")
                .arg(color.sourceKind==2?"VS props":"Decoded AVFrame").arg(color.explicitFields);
            lines << tr("色彩目标：%1 nit · 黑位 %2 nit · ICC %3 · LUT %4 · 提交 %5 ms")
                .arg(color.targetPeak,0,'f',0).arg(color.targetBlack,0,'f',4).arg(color.iccState).arg(color.lutActive).arg(color.renderSubmitMs,0,'f',2);
            lines << tr("HDR 静态信息：Mastering %1 / %2 nit · MaxCLL %3 · MaxFALL %4")
                .arg(color.masteringPeak,0,'f',1).arg(color.masteringBlack,0,'f',4).arg(color.maxCll,0,'f',1).arg(color.maxFall,0,'f',1);
            if(color.profile[0])lines << tr("显示 ICC：%1").arg(QString::fromUtf8(color.profile));
            lines << QString("HDR10+ %1 · Dolby Vision %2 / reshape %3 · Vivid %4").arg(color.hdr10plus).arg(color.doviDetected).arg(color.doviActive).arg(color.vividDetected);
            const QStringList tones{"Auto","Spline","ST2094-40","BT.2390","Clip","Linear"},gamuts{"Auto","Perceptual","Softclip","Relative","Clip"};
            lines << QString("Tone %1 · Gamut %2 · Dither %3 · Stream fields %4")
                .arg(tones.value(color.tone),gamuts.value(color.gamut)).arg(color.dither).arg(color.streamFields);
        }
        if(color.fallback[0]) lines << tr("色彩回退 / 限制：%1").arg(QString::fromUtf8(color.fallback));
    }
    for(auto &line:lines)if(line.contains(tr("已提交：")))line=tr("  当前帧率：%1 fps · ").arg(playing_?infoCurrentFps_:0,0,'f',2)+line.trimmed();
    if(audioMode_){
        int videoStart=-1,audioStart=-1,audioEnd=-1;
        for(int i=0;i<lines.size();++i){if(lines[i].startsWith(tr("视频解码器：")))videoStart=i;if(lines[i].startsWith(tr("音频解码器：")))audioStart=i;if(lines[i].startsWith(tr("倍速：")))audioEnd=i;}
        if(videoStart>=0 && audioStart>videoStart){auto audioLines=lines.mid(audioStart,audioEnd-audioStart+1);lines=lines.mid(0,videoStart);lines<<tr("视频：无动态视频 · 音频模式")<<(audioMetadata_.cover.isNull()?tr("封面图片：无"):tr("封面图片：%1 × %2 · Qt Raster · 静态图片").arg(audioMetadata_.cover.width()).arg(audioMetadata_.cover.height()))<<tr("VS / 补帧 / 视频硬解：未启用")<<"";lines<<audioLines;lines<<tr("歌词：%1 · %2 行").arg(audioMetadata_.lyricSource.isEmpty()?tr("未提供"):audioMetadata_.lyricSource).arg(audioMetadata_.lyrics.size());}
        lines[1]=tr("当前时间：%1 · 时间轴：%2 / %3").arg(QDateTime::currentDateTime().toString("HH:mm:ss"),formatTime(at),formatTime(source.duration100ns));
    }
    static_cast<PlayerInfoPanel *>(info_)->showText(lines.join('\n'));info_->raise();
}
}
