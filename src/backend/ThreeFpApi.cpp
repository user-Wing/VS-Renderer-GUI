#include "backend/ThreeFpApi.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

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
ThreeFpResult ThreeFpApi::readPixel(void *h, ThreeFpPixelProbe *p) const { return available() ? readPixel_(h, p) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::submitExternalVideoFrame(void *h, const ThreeFpExternalVideoFrame *f) const { return available() ? submitExternalVideoFrame_(h, f) : ThreeFpResult::NativeFailure; }
ThreeFpResult ThreeFpApi::redraw(void *h) const { return available() ? redraw_(h) : ThreeFpResult::NativeFailure; }
void ThreeFpApi::destroy(void *h) const { if (available() && h) destroy_(h); }

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
