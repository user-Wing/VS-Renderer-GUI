#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "backend/ThreeFpPlayer.h"

#include "backend/VapourSynthFrameServer.h"

#include <QWidget>

namespace vsr {
namespace {

bool isTransportReady(ThreeFpState state)
{
    return state == ThreeFpState::Ready || state == ThreeFpState::Playing ||
        state == ThreeFpState::Paused || state == ThreeFpState::Ended;
}

}

ThreeFpPlayer::ThreeFpPlayer(ThreeFpApi &api, QWidget *surface, QObject *parent)
    : QObject(parent), api_(api), surface_(surface)
{
}

bool ThreeFpPlayer::createSession()
{
    if (handle_) return true;
    if (!api_.available() || !surface_) {
        lastError_=api_.errorString();emit errorOccurred(lastError_);return false;
    }

    ThreeFpConfiguration configuration{};
    configuration.size = sizeof(configuration);
    configuration.version = api_.apiVersion();
    configuration.outputWindow = reinterpret_cast<void *>(surface_->winId());
    const auto endpointUtf8=audioEndpoint_.toUtf8();
    configuration.audioEndpointIdUtf8=endpointUtf8.isEmpty()?nullptr:endpointUtf8.constData();
    configuration.decodeMode = decodeMode_;
    configuration.colorMode = automaticHdr_ ? 2 : 0;
    configuration.sdrPeakNits = 100.0f;
    configuration.hdrPeakNits = 0.0f;
    configuration.sdrPaperWhiteNits = 203.0f;
    configuration.videoScalingQuality = 1;
    configuration.adaptiveDownscaleBeforeUpload = decodeMode_==1 &&
        qEnvironmentVariable("VSR_ADAPTIVE_UPLOAD")=="1";
    configuration.adaptiveDecoderThreads = decodeMode_==1 &&
        qEnvironmentVariable("VSR_ADAPTIVE_THREADS")!="0";
    if(qEnvironmentVariableIsSet("VSR_3FP_ADAPTER"))configuration.preferredAdapterIndex=qEnvironmentVariableIntValue("VSR_3FP_ADAPTER");

    if (!check(api_.create(&configuration, &handle_), QStringLiteral("创建 3FP 会话")))
        return false;
    audioOutputPending_=true;
    if(clockOnly_)api_.setClockOnly(handle_,true);
    if(softwarePreScaleHeight_)check(api_.setSoftwarePreScale(handle_,softwarePreScaleHeight_),QStringLiteral("CPU pre-scale"));
    if (!check(api_.setScalingAlgorithms(handle_, static_cast<ThreeFpScalingAlgorithm>(static_cast<unsigned>(upscale_) | ((chromaAlgorithm_ + 1) << 8) | (unsigned(secondaryScaling_ + 1) << 24) | (antiRinging_ ? 65536u : 0u)), downscale_), QStringLiteral("设置缩放算法")))
        return false;
    check(api_.setVolume(handle_, volume_, muted_ ? 1u : 0u), QStringLiteral("音量"));
    if (!outputFormat_.isEmpty()) api_.setExternalOutputFormat(handle_, outputFormat_.toUtf8().constData());
    if (!upscaleShaderPath_.isEmpty() && !check(api_.setUpscaleShader(handle_,upscaleShaderPath_.toUtf8().constData()),QStringLiteral("OpenNGU"))) return false;
    if (!customShaderPath_.isEmpty() && !check(api_.setCustomShader(handle_, customShaderPath_.toUtf8().constData(), customShaderWidth_, customShaderHeight_), QStringLiteral("加载 HLSL 着色器")))
        return false;
    if (customColorSettings_) api_.setColorSettings(handle_, &colorSettings_);
    if(videoSuperResolution_)api_.setVideoEnhancement(handle_,false,true);
    if(videoAutoHdr_)api_.setVideoEnhancement(handle_,true,true);
    if (vrrPresent_)
        api_.setPresentConfig(handle_, true);
    if (vrrPacing_)
        api_.setPacingConfig(handle_, true);
    if (zoom_ != 1.0f || panX_ != 0.0f || panY_ != 0.0f)
        check(api_.setViewTransform(handle_, zoom_, panX_, panY_), QStringLiteral("缩放"));
    return true;
}

ThreeFpPlayer::~ThreeFpPlayer()
{
    if (handle_) {
        api_.stop(handle_);
        api_.destroy(handle_);
    }
}
bool ThreeFpPlayer::setAudioOutput(const QString &endpoint,bool exclusive) {
    if((!endpoint.isEmpty() || exclusive) && !api_.hasAudioOutputControls()) {
        lastError_=QStringLiteral("当前 3FP 内核不支持音频设备/独占接口，请更新内核。");emit errorOccurred(lastError_);return false;
    }
    if(audioEndpoint_==endpoint && audioExclusive_==exclusive)return true;
    if(handle_ && audioEndpoint_!=endpoint && !check(api_.setAudioEndpoint(handle_,endpoint.toUtf8().constData()),QStringLiteral("音频输出设备")))return false;
    audioEndpoint_=endpoint;audioExclusive_=exclusive;audioOutputPending_=true;
    // Exclusive mode is state-gated in the existing ABI. Apply after asynchronous
    // Open completes, immediately before Play, not while the session is Idle.
    return true;
}
bool ThreeFpPlayer::setClockOnly(bool enabled) { clockOnly_=enabled;return !handle_ || api_.setClockOnly(handle_,enabled)==ThreeFpResult::Success; }
bool ThreeFpPlayer::enableAudioTap(bool enabled){return handle_ && api_.enableAudioTap(handle_,enabled)==ThreeFpResult::Success;}
QByteArray ThreeFpPlayer::readAudioTap(AudioTapBlock &block){
    QByteArray pcm(256*1024,Qt::Uninitialized);
    if(!handle_)return {};
    auto result=api_.readAudioTap(handle_,pcm.data(),pcm.size(),&block);
    if(result==ThreeFpResult::BufferTooSmall && block.requiredBytes<=1024*1024){pcm.resize(block.requiredBytes);block=AudioTapBlock{};result=api_.readAudioTap(handle_,pcm.data(),pcm.size(),&block);}
    if(result!=ThreeFpResult::Success)return {};
    pcm.resize(block.bytesWritten);return pcm;
}
bool ThreeFpPlayer::setVideoEnhancement(bool hdr,bool enabled){
    (hdr?videoAutoHdr_:videoSuperResolution_)=enabled;
    return !handle_ || api_.setVideoEnhancement(handle_,hdr,enabled)==ThreeFpResult::Success;
}
VideoEnhancementStatus ThreeFpPlayer::videoEnhancementStatus(bool hdr) const{return handle_?api_.videoEnhancementStatus(handle_,hdr):VideoEnhancementStatus{};}
bool ThreeFpPlayer::setSoftwarePreScale(int height) { if(height!=0 && height!=720 && height!=1080 && height!=2160)return false;softwarePreScaleHeight_=height;return !handle_ || api_.setSoftwarePreScale(handle_,height)==ThreeFpResult::Success; }

bool ThreeFpPlayer::ready() const { return const_cast<ThreeFpPlayer *>(this)->createSession(); }
QString ThreeFpPlayer::lastError() const { const auto native=api_.sessionError(handle_);return native.isEmpty()?lastError_:native; }

bool ThreeFpPlayer::resetVideoOutput()
{
    if (!handle_) return true;
    if (handle_) {
        api_.stop(handle_);
        api_.destroy(handle_);
        handle_ = nullptr;
    }
    lastError_.clear();
    return createSession();
}

bool ThreeFpPlayer::openFile(const QString &path)
{
    const QByteArray utf8 = path.toUtf8();
    audioOutputPending_=true;
    return createSession() && check(api_.open(handle_, utf8.constData()), QStringLiteral("打开媒体"));
}

bool ThreeFpPlayer::play()
{
    if (!handle_)
        return false;
    const auto state = snapshot().state;
    if (state == ThreeFpState::Playing)
        return true;
    if (state != ThreeFpState::Ready && state != ThreeFpState::Paused && state != ThreeFpState::Ended)
        return false;
    if(audioOutputPending_ && api_.hasAudioOutputControls()) {
        if(!check(api_.setAudioExclusiveMode(handle_,audioExclusive_),QStringLiteral("音频独占模式")))return false;
        audioOutputPending_=false;
    }
    return check(api_.play(handle_), QStringLiteral("播放"));
}

bool ThreeFpPlayer::pause()
{
    if (!handle_)
        return false;
    if (snapshot().state != ThreeFpState::Playing)
        return true;
    const auto result = api_.pause(handle_);
    if (result == ThreeFpResult::InvalidState && snapshot().state != ThreeFpState::Playing)
        return true;
    return check(result, QStringLiteral("暂停"));
}

void ThreeFpPlayer::stop() { if (handle_) check(api_.stop(handle_), QStringLiteral("停止")); }
bool ThreeFpPlayer::seek(std::int64_t p)
{
    if (!handle_ || !isTransportReady(snapshot().state))
        return false;
    return check(api_.seek(handle_, p), QStringLiteral("跳转"));
}

bool ThreeFpPlayer::seekFrame(std::int64_t f)
{
    const auto state = snapshot();
    if (!handle_ || !isTransportReady(state.state) || state.selectedVideoStream < 0)
        return false;
    return check(api_.seekFrame(handle_, f), QStringLiteral("按帧跳转"));
}

bool ThreeFpPlayer::stepFrame(int d)
{
    const auto state = snapshot();
    if (!handle_ || !isTransportReady(state.state) || state.selectedVideoStream < 0)
        return false;
    return check(api_.stepFrame(handle_, d), QStringLiteral("逐帧"));
}
void ThreeFpPlayer::setMuted(bool m)
{
    muted_ = m;
    if (handle_)
        check(api_.setVolume(handle_, volume_, m ? 1u : 0u), QStringLiteral("静音"));
}

void ThreeFpPlayer::setVolume(float volume) { volume_ = volume; if (handle_) check(api_.setVolume(handle_, volume_, muted_ ? 1u : 0u), QStringLiteral("音量")); }
bool ThreeFpPlayer::stepKeyframe(int d) { return handle_ && check(api_.stepKeyframe(handle_, d), QStringLiteral("关键帧")); }
bool ThreeFpPlayer::setPlaybackRate(double rate) { return handle_ && check(api_.setPlaybackRate(handle_, rate), QStringLiteral("倍速")); }
bool ThreeFpPlayer::setDecodeMode(unsigned mode) { if(decodeMode_==mode)return true;decodeMode_ = mode; return resetVideoOutput(); }
void ThreeFpPlayer::setAutomaticHdr(bool enabled) { if(automaticHdr_==enabled)return;automaticHdr_ = enabled; resetVideoOutput(); }
bool ThreeFpPlayer::selectAudio(int stream) {return handle_ && check(api_.selectAudio(handle_,stream),QStringLiteral("Select audio track"));}
bool ThreeFpPlayer::loadExternalAudio(const QString &path) {return handle_ && check(api_.loadExternalAudio(handle_,path.toUtf8().constData()),QStringLiteral("Load external audio"));}
bool ThreeFpPlayer::clearExternalAudio() {return handle_ && check(api_.clearExternalAudio(handle_),QStringLiteral("Clear external audio"));}
bool ThreeFpPlayer::setAudioEffects(bool enabled,const float *gains,float wave,qint64 delay) {return handle_ && check(api_.setAudioEffects(handle_,enabled,gains,wave,delay),QStringLiteral("Audio effects"));}
QString ThreeFpPlayer::mediaInfo() const {
    if(!handle_)return {};
    auto data=QJsonDocument::fromJson(api_.mediaInfo(handle_).toUtf8()).object();
    auto chapters=data.value("chapters").toArray();
    for(int i=0;i<chapters.size();++i){auto chapter=chapters[i].toObject();
        if(!chapter.contains("start100ns") && chapter.contains("startTime100ns"))chapter.insert("start100ns",chapter.value("startTime100ns"));
        chapters[i]=chapter;
    }
    data.insert("chapters",chapters);
    return QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact));
}

bool ThreeFpPlayer::setVrrPresent(bool enabled)
{
    vrrPresent_ = enabled;
    return handle_ && check(api_.setPresentConfig(handle_, enabled), QStringLiteral("VRR low-latency present"));
}

bool ThreeFpPlayer::setVrrPacing(bool enabled)
{
    vrrPacing_ = enabled;
    return handle_ && check(api_.setPacingConfig(handle_, enabled), QStringLiteral("VRR Pacing"));
}

bool ThreeFpPlayer::setScalingAlgorithms(ThreeFpScalingAlgorithm upscale,
                                         ThreeFpScalingAlgorithm downscale)
{
    upscale_ = upscale;
    downscale_ = downscale;
    return !handle_ || check(api_.setScalingAlgorithms(handle_, static_cast<ThreeFpScalingAlgorithm>(static_cast<unsigned>(upscale) | ((chromaAlgorithm_ + 1) << 8) | (unsigned(secondaryScaling_ + 1) << 24) | (antiRinging_ ? 65536u : 0u)), downscale),
                            QStringLiteral("设置缩放算法"));
}

bool ThreeFpPlayer::setChromaAlgorithm(int algorithm)
{
    if (algorithm < 0 || algorithm > 10) return false;
    chromaAlgorithm_ = algorithm;
    return setScalingAlgorithms(upscale_, downscale_);
}

void ThreeFpPlayer::setSecondaryScaling(int algorithm) {
    secondaryScaling_=algorithm==15?15:std::clamp(algorithm,-1,8);
}
bool ThreeFpPlayer::setUpscaleShader(const QString &path) {
    if(path==upscaleShaderPath_)return true;
    if(!createSession() || !check(api_.setUpscaleShader(handle_,path.toUtf8().constData()),QStringLiteral("OpenNGU")))return false;
    upscaleShaderPath_=path;return true;
}
bool ThreeFpPlayer::setCustomShader(const QString &path,int width,int height)
{
    if(width<0 || height<0 || width>16384 || height>16384 || bool(width)!=bool(height))return false;
    if(path.isEmpty() && !handle_){customShaderPath_.clear();customShaderWidth_=customShaderHeight_=0;return true;}
    if(!createSession() || !check(api_.setCustomShader(handle_,path.toUtf8().constData(),width,height),QStringLiteral("加载 HLSL 着色器")))return false;
    customShaderPath_=path;customShaderWidth_=width;customShaderHeight_=height;
    return true;
}

void ThreeFpPlayer::setView(float zoom, float panX, float panY)
{
    zoom_ = zoom;
    panX_ = panX;
    panY_ = panY;
    if (handle_)
        check(api_.setViewTransform(handle_, zoom, panX, panY), QStringLiteral("缩放"));
}

void ThreeFpPlayer::redraw() { if (handle_) api_.redraw(handle_); }
bool ThreeFpPlayer::setColorSettings(const VsrColorSettings &settings) {
    colorSettings_ = settings; customColorSettings_ = true;
    return !handle_ || api_.setColorSettings(handle_, &settings) == ThreeFpResult::Success;
}
VsrColorStatus ThreeFpPlayer::colorStatus() const {
    VsrColorStatus value{}; value.size = sizeof(value); value.version = 1;
    if (!handle_ || api_.colorStatus(handle_, &value) != ThreeFpResult::Success) {
        value.requestedEngine = colorSettings_.engine;
        qstrncpy(value.engine, "3FP native", sizeof(value.engine));
        if (colorSettings_.engine) qstrncpy(value.fallback, "This 3FP build has no advanced color API.", sizeof(value.fallback));
    }
    return value;
}
bool ThreeFpPlayer::setOutputFormat(const QString &format) { outputFormat_=format; return handle_ && check(api_.setExternalOutputFormat(handle_,format.toUtf8().constData()),QStringLiteral("输出像素格式")); }
void ThreeFpPlayer::setAntiRinging(bool enabled) { antiRinging_=enabled; setScalingAlgorithms(upscale_,downscale_); }
QImage ThreeFpPlayer::capture() const { const auto size=surface_->size()*surface_->devicePixelRatioF(); return handle_ ? api_.capture(handle_,size.width(),size.height()) : QImage(); }
QImage ThreeFpPlayer::captureFloat() const { const auto size=surface_->size()*surface_->devicePixelRatioF(); return handle_ ? api_.captureFloat(handle_,size.width(),size.height()) : QImage(); }
bool ThreeFpPlayer::setSubtitle(const QImage &image) {
    QRect area=image.rect();const auto region=image.text("vsrSubtitleRect").split(',');
    if(region.size()==4)area=QRect(region[0].toInt(),region[1].toInt(),region[2].toInt(),region[3].toInt()).intersected(image.rect());
    // The canvas defines placement, but transparent pixels need no CPU color
    // conversion or bitmap upload. Keep HDR/subtitle math on the visible bitmap.
    const auto bitmap=area.isEmpty()?QImage{}:area==image.rect()?image:image.copy(area);
    TimedTextCommand command; command.x=area.x();command.y=area.y();command.width=area.width();command.height=area.height();command.bitmap=bitmap.constBits();command.bitmapWidth=bitmap.width();command.bitmapHeight=bitmap.height();command.bitmapStride=bitmap.bytesPerLine();command.bitmapBytes=static_cast<uint32_t>(bitmap.sizeInBytes());command.contentId=++subtitleSequence_;
    TimedTextLayer layer; layer.width=qMax(1,image.width()); layer.height=qMax(1,image.height()); layer.count=bitmap.isNull()?0:1; layer.sequence=subtitleSequence_; layer.commands=&command;
    return handle_ && api_.setSubtitleLayer(handle_,&layer)==ThreeFpResult::Success;
}

ThreeFpSnapshot ThreeFpPlayer::snapshot() const
{
    ThreeFpSnapshot value{};
    value.size = sizeof(value);
    value.version = 8;
    if (handle_)
        api_.snapshot(handle_, &value);
    return value;
}

bool ThreeFpPlayer::samplePixel(int x, int y, ThreeFpPixelProbe &sample) const
{
    if (!handle_ || snapshot().presentedVideoFrames == 0)
        return false;
    sample = {};
    sample.size = sizeof(sample);
    sample.version = 1;
    sample.x = static_cast<std::uint32_t>(qMax(0, x));
    sample.y = static_cast<std::uint32_t>(qMax(0, y));
    return api_.readPixel(handle_, &sample) == ThreeFpResult::Success;
}

bool ThreeFpPlayer::sampleSourcePixel(int x, int y, bool surfaceCoordinates, VsrSourcePixelProbe &sample) const
{
    if(!handle_)return false;
    sample={};sample.x=x;sample.y=y;sample.coordinates=surfaceCoordinates?1:0;
    return api_.readSourcePixel(handle_,&sample)==ThreeFpResult::Success;
}

bool ThreeFpPlayer::submitFrame(const VapourSynthFrame &frame)
{
    if (!createSession())
        return false;

    ThreeFpExternalVideoFrame native{};
    native.size = sizeof(native);
    native.version = 2;
    native.width = static_cast<std::uint32_t>(frame.width);
    native.height = static_cast<std::uint32_t>(frame.height);
    native.format = frame.format;
    native.colorRange = frame.colorRange;
    native.colorPrimaries = frame.colorPrimaries;
    native.colorTransfer = frame.colorTransfer;
    native.colorMatrix = frame.colorMatrix;
    native.chromaLocation = frame.chromaLocation;
    native.frameIndex = frame.frameIndex;
    native.duration100ns = frame.duration100ns;
    native.totalFrames = frame.totalFrames;
    native.colorMetadata = frame.colorMetadata;
    native.colorMetadata.sideData = frame.colorSideData.constData();
    native.colorMetadata.sideDataBytes = static_cast<std::uint32_t>(frame.colorSideData.size());

    const bool rgb = frame.format == ThreeFpExternalPixelFormat::GbrP8 ||
        frame.format == ThreeFpExternalPixelFormat::GbrP10 ||
        frame.format == ThreeFpExternalPixelFormat::GbrP12 ||
        frame.format == ThreeFpExternalPixelFormat::GbrP16 ||
        frame.format == ThreeFpExternalPixelFormat::GbrPF16 ||
        frame.format == ThreeFpExternalPixelFormat::GbrPF32;
    const int order[4] = {rgb ? 1 : 0, rgb ? 2 : 1, rgb ? 0 : 2, 3};
    for (int plane = 0; plane < 4; ++plane) {
        const int source = order[plane];
        native.data[plane] = frame.planes[source].isEmpty() ? nullptr : frame.planes[source].constData();
        native.linesize[plane] = frame.strides[source];
    }
    return check(api_.submitExternalVideoFrame(handle_, &native), QStringLiteral("提交 VapourSynth 帧"));
}

bool ThreeFpPlayer::check(ThreeFpResult result, const QString &operation)
{
    if (result == ThreeFpResult::Success)
        return true;
    if (result == ThreeFpResult::NotSupported && operation.startsWith(QStringLiteral("VRR")))
        return false;
    lastError_ = QStringLiteral("%1失败：%2").arg(operation, ThreeFpApi::resultText(result));
    emit errorOccurred(lastError_);
    return false;
}

bool ThreeFpPlayer::audioLevels(ThreeFpAudioLevels &levels,bool input) const {return handle_ && api_.audioLevels(handle_,&levels,input)==ThreeFpResult::Success;}

}

namespace vsr {bool ThreeFpPlayer::setDecodeBuffer(quint64 bytes,unsigned frames){return handle_ && api_.setDecodeBuffer(handle_,bytes,frames)==ThreeFpResult::Success;}}

namespace vsr {QSize ThreeFpPlayer::processingSize() const {return handle_?api_.processingSize(handle_):QSize();}}

namespace vsr {void ThreeFpPlayer::setLoudnessNormalization(bool enabled){if(handle_)api_.setLoudnessNormalization(handle_,enabled);}}
