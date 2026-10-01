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
    if (!api_.available()) {
        lastError_ = api_.errorString();
        return;
    }

    createSession();
}

bool ThreeFpPlayer::createSession()
{
    if (!api_.available() || !surface_)
        return false;

    ThreeFpConfiguration configuration{};
    configuration.size = sizeof(configuration);
    configuration.version = api_.apiVersion();
    configuration.outputWindow = reinterpret_cast<void *>(surface_->winId());
    configuration.decodeMode = decodeMode_;
    configuration.colorMode = automaticHdr_ ? 2 : 0;
    configuration.sdrPeakNits = 100.0f;
    configuration.hdrPeakNits = 0.0f;
    configuration.sdrPaperWhiteNits = 203.0f;
    configuration.videoScalingQuality = 1;

    if (!check(api_.create(&configuration, &handle_), QStringLiteral("创建 3FP 会话")))
        return false;
    if (!check(api_.setScalingAlgorithms(handle_, static_cast<ThreeFpScalingAlgorithm>(static_cast<unsigned>(upscale_) | ((chromaAlgorithm_ + 1) << 8) | (antiRinging_ ? 65536u : 0u)), downscale_), QStringLiteral("设置缩放算法")))
        return false;
    if (muted_)
        check(api_.setVolume(handle_, 1.0f, 1u), QStringLiteral("静音"));
    if (!outputFormat_.isEmpty()) api_.setExternalOutputFormat(handle_, outputFormat_.toUtf8().constData());
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

bool ThreeFpPlayer::ready() const { return handle_ != nullptr; }
QString ThreeFpPlayer::lastError() const { return lastError_; }

bool ThreeFpPlayer::resetVideoOutput()
{
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
    return handle_ && check(api_.open(handle_, utf8.constData()), QStringLiteral("打开媒体"));
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
bool ThreeFpPlayer::setDecodeMode(unsigned mode) { decodeMode_ = mode; return resetVideoOutput(); }
void ThreeFpPlayer::setAutomaticHdr(bool enabled) { automaticHdr_ = enabled; resetVideoOutput(); }
QString ThreeFpPlayer::mediaInfo() const { return handle_ ? api_.mediaInfo(handle_) : QString(); }

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
    return handle_ && check(api_.setScalingAlgorithms(handle_, static_cast<ThreeFpScalingAlgorithm>(static_cast<unsigned>(upscale) | ((chromaAlgorithm_ + 1) << 8) | (antiRinging_ ? 65536u : 0u)), downscale),
                            QStringLiteral("设置缩放算法"));
}

bool ThreeFpPlayer::setChromaAlgorithm(int algorithm)
{
    if (algorithm < 0 || algorithm > 10) return false;
    chromaAlgorithm_ = algorithm;
    return setScalingAlgorithms(upscale_, downscale_);
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
bool ThreeFpPlayer::setOutputFormat(const QString &format) { outputFormat_=format; return handle_ && check(api_.setExternalOutputFormat(handle_,format.toUtf8().constData()),QStringLiteral("输出像素格式")); }
void ThreeFpPlayer::setAntiRinging(bool enabled) { antiRinging_=enabled; setScalingAlgorithms(upscale_,downscale_); }
QImage ThreeFpPlayer::capture() const { const auto size=surface_->size()*surface_->devicePixelRatioF(); return handle_ ? api_.capture(handle_,size.width(),size.height()) : QImage(); }
bool ThreeFpPlayer::setSubtitle(const QImage &image) {
    TimedTextCommand command; command.width=image.width(); command.height=image.height(); command.bitmap=image.constBits(); command.bitmapWidth=image.width(); command.bitmapHeight=image.height(); command.bitmapStride=image.bytesPerLine(); command.bitmapBytes=static_cast<uint32_t>(image.sizeInBytes()); command.contentId=++subtitleSequence_;
    TimedTextLayer layer; layer.width=image.width(); layer.height=image.height(); layer.count=image.isNull()?0:1; layer.sequence=subtitleSequence_; layer.commands=&command;
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

bool ThreeFpPlayer::submitFrame(const VapourSynthFrame &frame)
{
    if (!handle_)
        return false;

    ThreeFpExternalVideoFrame native{};
    native.size = sizeof(native);
    native.version = 1;
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

    const bool rgb = frame.format == ThreeFpExternalPixelFormat::GbrP8 ||
        frame.format == ThreeFpExternalPixelFormat::GbrP10 ||
        frame.format == ThreeFpExternalPixelFormat::GbrP12 ||
        frame.format == ThreeFpExternalPixelFormat::GbrP16;
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

}
