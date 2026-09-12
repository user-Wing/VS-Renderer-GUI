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
    : QObject(parent), api_(api)
{
    if (!api_.available()) {
        lastError_ = api_.errorString();
        return;
    }

    ThreeFpConfiguration configuration{};
    configuration.size = sizeof(configuration);
    configuration.version = api_.apiVersion();
    configuration.outputWindow = reinterpret_cast<void *>(surface->winId());
    configuration.decodeMode = 2;
    configuration.colorMode = 0;
    configuration.sdrPeakNits = 100.0f;
    configuration.hdrPeakNits = 0.0f;
    configuration.sdrPaperWhiteNits = 203.0f;
    configuration.videoScalingQuality = 1;

    check(api_.create(&configuration, &handle_), QStringLiteral("创建 3FP 会话"));
    if (handle_)
        check(api_.setScalingAlgorithms(handle_, ThreeFpScalingAlgorithm::Nearest,
                                        ThreeFpScalingAlgorithm::Lanczos3),
              QStringLiteral("设置默认缩放算法"));
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
void ThreeFpPlayer::setMuted(bool m) { if (handle_) check(api_.setVolume(handle_, 1.0f, m ? 1u : 0u), QStringLiteral("静音")); }

bool ThreeFpPlayer::setVrrPresent(bool enabled)
{
    return handle_ && check(api_.setPresentConfig(handle_, enabled), QStringLiteral("VRR low-latency present"));
}

bool ThreeFpPlayer::setVrrPacing(bool enabled)
{
    return handle_ && check(api_.setPacingConfig(handle_, enabled), QStringLiteral("VRR Pacing"));
}

bool ThreeFpPlayer::setScalingAlgorithms(ThreeFpScalingAlgorithm upscale,
                                         ThreeFpScalingAlgorithm downscale)
{
    return handle_ && check(api_.setScalingAlgorithms(handle_, upscale, downscale),
                            QStringLiteral("设置缩放算法"));
}

void ThreeFpPlayer::setView(float zoom, float panX, float panY)
{
    if (handle_)
        check(api_.setViewTransform(handle_, zoom, panX, panY), QStringLiteral("缩放"));
}

void ThreeFpPlayer::redraw() { if (handle_) api_.redraw(handle_); }

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
    if (!handle_)
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
