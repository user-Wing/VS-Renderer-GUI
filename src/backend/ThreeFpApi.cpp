#include "backend/ThreeFpApi.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <vector>
#include <algorithm>

namespace vsr {

ThreeFpApi::ThreeFpApi()
{
    QString path = qEnvironmentVariable("VSR_3FP_DLL");
    if (path.isEmpty())
        path = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("FFF.Native.dll"));

    library_.setFileName(path);
    library_.setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::PreventUnloadHint);
    if (!library_.load()) {
        error_ = QStringLiteral("无法加载 3FP：%1").arg(library_.errorString());
        return;
    }

    bool ok = true;
    ok &= resolve(getApiVersion_, "FFF3FP_GetApiVersion");
    ok &= resolve(create_, "FFF3FP_Create");
    ok &= resolve(open_, "FFF3FP_Open");
    ok &= resolve(play_, "FFF3FP_Play");
    ok &= resolve(pause_, "FFF3FP_Pause");
    ok &= resolve(stop_, "FFF3FP_Stop");
    ok &= resolve(seek_, "FFF3FP_Seek");
    ok &= resolve(seekFrame_, "FFF3FP_SeekFrame");
    ok &= resolve(stepFrame_, "FFF3FP_StepFrame");
    ok &= resolve(setVolume_, "FFF3FP_SetVolume");
    ok &= resolve(setPresentConfig_, "FFF3FP_SetPresentConfig");
    ok &= resolve(setPacingConfig_, "FFF3FP_SetPacingConfig");
    ok &= resolve(setViewTransform_, "FFF3FP_SetViewTransform");
    ok &= resolve(setScalingAlgorithms_, "FFF3FP_SetScalingAlgorithms");
    ok &= resolve(snapshot_, "FFF3FP_GetSnapshot");
    ok &= resolve(readPixel_, "FFF3FP_ReadVideoPixel");
    ok &= resolve(submitExternalVideoFrame_, "FFF3FP_SubmitExternalVideoFrame");
    ok &= resolve(redraw_, "FFF3FP_Redraw");
    ok &= resolve(destroy_, "FFF3FP_Destroy");
    selectAudio_ = reinterpret_cast<StepFn>(library_.resolve("FFF3FP_SelectAudioStream"));
    setClockOnly_ = reinterpret_cast<StepFn>(library_.resolve("FFF3FP_SetClockOnly"));
    loadExternalAudio_ = reinterpret_cast<ExternalAudioFn>(library_.resolve("FFF3FP_LoadExternalAudio"));
    clearExternalAudio_ = reinterpret_cast<HandleFn>(library_.resolve("FFF3FP_ClearExternalAudio"));
    setAudioEffects_ = reinterpret_cast<EffectsFn>(library_.resolve("FFF3FP_SetAudioEffects"));
    stepKeyframe_ = reinterpret_cast<StepFn>(library_.resolve("FFF3FP_StepKeyframe"));
    setPlaybackRate_ = reinterpret_cast<RateFn>(library_.resolve("FFF3FP_SetPlaybackRate"));
    mediaInfo_ = reinterpret_cast<InfoFn>(library_.resolve("FFF3FP_GetMediaInfo"));
    setExternalOutputFormat_ = reinterpret_cast<OpenFn>(library_.resolve("FFF3FP_SetExternalOutputFormat"));
    setSubtitleLayer_ = reinterpret_cast<LayerFn>(library_.resolve("FFF3FP_SetTimedTextLayer"));
    readRegion_ = reinterpret_cast<RegionFn>(library_.resolve("FFF3FP_ReadVideoPixelRegion"));
    setColorSettings_ = reinterpret_cast<ColorSetFn>(library_.resolve("FFF3FP_SetColorSettings"));
    colorStatus_ = reinterpret_cast<ColorStatusFn>(library_.resolve("FFF3FP_GetColorStatus"));
    if (!ok)
        library_.unload();
}

template<typename T>
bool ThreeFpApi::resolve(T &target, const char *name)
{
    target = reinterpret_cast<T>(library_.resolve(name));
    if (target)
        return true;
    error_ = QStringLiteral("3FP 缺少 ABI：%1").arg(QString::fromLatin1(name));
    return false;
}

bool ThreeFpApi::available() const { return library_.isLoaded() && error_.isEmpty(); }
QString ThreeFpApi::libraryPath() const { return QFileInfo(library_.fileName()).absoluteFilePath(); }
QString ThreeFpApi::errorString() const { return error_; }
std::uint32_t ThreeFpApi::apiVersion() const { return available() ? getApiVersion_() : 0; }

ThreeFpResult ThreeFpApi::create(const ThreeFpConfiguration *c, void **h) const { return available() ? create_(c, h) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::open(void *h, const char *p) const { return available() ? open_(h, p) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::play(void *h) const { return available() ? play_(h) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::pause(void *h) const { return available() ? pause_(h) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setClockOnly(void *h,bool enabled) const { return setClockOnly_?setClockOnly_(h,enabled?1:0):ThreeFpResult::NotSupported; }
ThreeFpResult ThreeFpApi::stop(void *h) const { return available() ? stop_(h) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::seek(void *h, std::int64_t p) const { return available() ? seek_(h, p) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::seekFrame(void *h, std::int64_t f) const { return available() ? seekFrame_(h, f) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::stepFrame(void *h, std::int32_t d) const { return available() ? stepFrame_(h, d) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setVolume(void *h, float v, std::uint32_t m) const { return available() ? setVolume_(h, v, m) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setPresentConfig(void *h, bool e) const { return available() ? setPresentConfig_(h, e ? 1u : 0u) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setPacingConfig(void *h, bool e) const { return available() ? setPacingConfig_(h, e ? 1u : 0u) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setViewTransform(void *h, float z, float x, float y) const { return available() ? setViewTransform_(h, z, x, y) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setScalingAlgorithms(void *h, ThreeFpScalingAlgorithm u, ThreeFpScalingAlgorithm d) const { return available() ? setScalingAlgorithms_(h, u, d) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::snapshot(void *h, ThreeFpSnapshot *s) const { return available() ? snapshot_(h, s) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setColorSettings(void *h, const VsrColorSettings *s) const { return setColorSettings_ ? setColorSettings_(h,s) : ThreeFpResult::NotSupported; }
ThreeFpResult ThreeFpApi::colorStatus(void *h, VsrColorStatus *s) const { return colorStatus_ ? colorStatus_(h,s) : ThreeFpResult::NotSupported; }
ThreeFpResult ThreeFpApi::readPixel(void *h, ThreeFpPixelProbe *p) const { return available() ? readPixel_(h, p) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::submitExternalVideoFrame(void *h, const ThreeFpExternalVideoFrame *f) const { return available() ? submitExternalVideoFrame_(h, f) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::setExternalOutputFormat(void *h, const char *f) const { return setExternalOutputFormat_ ? setExternalOutputFormat_(h,f) : ThreeFpResult::NotSupported; }
ThreeFpResult ThreeFpApi::setSubtitleLayer(void *h, const TimedTextLayer *layer) const { return setSubtitleLayer_ ? setSubtitleLayer_(h,layer) : ThreeFpResult::NotSupported; }
QImage ThreeFpApi::capture(void *h, int width, int height) const {
    if (!readRegion_ || width <= 0 || height <= 0 || static_cast<qint64>(width)*height > 40000000) return {};
    std::vector<float> pixels(static_cast<size_t>(width)*height*4); uint32_t depth=0;
    if (readRegion_(h,0,0,width,height,pixels.data(),static_cast<uint32_t>(pixels.size()),&depth) != ThreeFpResult::Success) return {};
    QImage image(width,height,QImage::Format_RGB32);
    for (int y=0;y<height;++y) { auto *row=reinterpret_cast<QRgb *>(image.scanLine(y)); for(int x=0;x<width;++x) { const auto at=(static_cast<size_t>(y)*width+x)*4; row[x]=qRgb(qRound(std::clamp(pixels[at],0.0f,1.0f)*255),qRound(std::clamp(pixels[at+1],0.0f,1.0f)*255),qRound(std::clamp(pixels[at+2],0.0f,1.0f)*255)); } }
    return image;
}
ThreeFpResult ThreeFpApi::redraw(void *h) const { return available() ? redraw_(h) : ThreeFpResult::NativeFailure; }
void ThreeFpApi::destroy(void *h) const { if (available() && h) destroy_(h); }

ThreeFpResult ThreeFpApi::stepKeyframe(void *h, int d) const { return stepKeyframe_ ? stepKeyframe_(h, d) : ThreeFpResult::NotSupported; }
ThreeFpResult ThreeFpApi::setPlaybackRate(void *h, double rate) const { return setPlaybackRate_ ? setPlaybackRate_(h, rate) : ThreeFpResult::NotSupported; }
ThreeFpResult ThreeFpApi::selectAudio(void *h,int stream) const {return selectAudio_?selectAudio_(h,stream):ThreeFpResult::NotSupported;}
ThreeFpResult ThreeFpApi::loadExternalAudio(void *h,const char *path) const {return loadExternalAudio_?loadExternalAudio_(h,path,-1,0):ThreeFpResult::NotSupported;}
ThreeFpResult ThreeFpApi::clearExternalAudio(void *h) const {return clearExternalAudio_?clearExternalAudio_(h):ThreeFpResult::NotSupported;}
ThreeFpResult ThreeFpApi::setAudioEffects(void *h,bool enabled,const float *gains,float wave,qint64 delay) const {return setAudioEffects_?setAudioEffects_(h,enabled?1u:0u,gains,wave,delay):ThreeFpResult::NotSupported;}
QString ThreeFpApi::mediaInfo(void *h) const {
    if (!mediaInfo_ || !h) return {};
    std::uint32_t length = 0; mediaInfo_(h, nullptr, 0, &length);
    if (!length || length > 16 * 1024 * 1024) return {};
    QByteArray text(length, '\0');
    return mediaInfo_(h, text.data(), length, &length) == ThreeFpResult::Success ? QString::fromUtf8(text.constData()) : QString();
}

QString ThreeFpApi::resultText(ThreeFpResult result)
{
    switch (result) {
    case ThreeFpResult::Success: return QStringLiteral("Success");
    case ThreeFpResult::InvalidArgument: return QStringLiteral("InvalidArgument");
    case ThreeFpResult::InvalidState: return QStringLiteral("InvalidState");
    case ThreeFpResult::BufferTooSmall: return QStringLiteral("BufferTooSmall");
    case ThreeFpResult::NativeFailure: return QStringLiteral("NativeFailure");
    case ThreeFpResult::FfmpegFailure: return QStringLiteral("FfmpegFailure");
    case ThreeFpResult::DeviceFailure: return QStringLiteral("DeviceFailure");
    case ThreeFpResult::NotSupported: return QStringLiteral("NotSupported");
    }
    return QStringLiteral("Unknown");
}

}
