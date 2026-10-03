#include "backend/VapourSynthFrameServer.h"

#define VS_USE_LATEST_API
#define VSSCRIPT_USE_LATEST_API
#include <VSScript4.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QMetaObject>
#include <QRegularExpression>
#include <QTimer>

#include <cmath>
#include <limits>
#include <future>
#include <vector>
#include <map>
#include <chrono>

namespace vsr {
namespace {

QString findVSScript()
{
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString bundled = appDir.filePath(
        QStringLiteral("runtime/python/Lib/site-packages/vapoursynth/vsscript.dll"));
    if (QFileInfo::exists(bundled))
        return QFileInfo(bundled).absoluteFilePath();

    const QString configured = qEnvironmentVariable("VSR_VSSCRIPT_DLL");
    if (QFileInfo::exists(configured))
        return QFileInfo(configured).absoluteFilePath();

    const QString besideApp = appDir.filePath(QStringLiteral("vsscript.dll"));
    if (QFileInfo::exists(besideApp))
        return QFileInfo(besideApp).absoluteFilePath();

    const QString development = appDir.absoluteFilePath(
        QStringLiteral("../../.deps/vs-python/Lib/site-packages/vapoursynth/vsscript.dll"));
    if (QFileInfo::exists(development))
        return QFileInfo(development).absoluteFilePath();
    return {};
}

QString describeScriptError(const QString &detail)
{
    static const QRegularExpression missingNamespace(
        QStringLiteral("No attribute with the name ([A-Za-z0-9_]+) exists"),
        QRegularExpression::CaseInsensitiveOption);
    const auto match = missingNamespace.match(detail);
    if (match.hasMatch()) {
        return QStringLiteral("VapourSynth 处理链缺少或无法加载插件 namespace “%1”。内置运行时不完整，当前滤镜不能执行。")
            .arg(match.captured(1));
    }
    return QStringLiteral("VPY 执行失败：%1").arg(detail.trimmed());
}

ThreeFpExternalPixelFormat mapFormat(const VSVideoFormat &format, bool *ok)
{
    *ok = format.sampleType == stInteger;
    if (!*ok)
        return ThreeFpExternalPixelFormat::Yuv420P8;

    if (format.colorFamily == cfGray) {
        *ok = format.bitsPerSample == 8 || format.bitsPerSample == 16;
        return format.bitsPerSample == 8 ? ThreeFpExternalPixelFormat::Gray8
                                         : ThreeFpExternalPixelFormat::Gray16;
    }

    const auto byDepth = [&format](ThreeFpExternalPixelFormat p8, ThreeFpExternalPixelFormat p10,
                                   ThreeFpExternalPixelFormat p12, ThreeFpExternalPixelFormat p16,
                                   bool *depthOk) {
        switch (format.bitsPerSample) {
        case 8: return p8;
        case 10: return p10;
        case 12: return p12;
        case 16: return p16;
        default: *depthOk = false; return p8;
        }
    };

    if (format.colorFamily == cfRGB && format.subSamplingW == 0 && format.subSamplingH == 0)
        return byDepth(ThreeFpExternalPixelFormat::GbrP8, ThreeFpExternalPixelFormat::GbrP10,
                       ThreeFpExternalPixelFormat::GbrP12, ThreeFpExternalPixelFormat::GbrP16, ok);
    if (format.colorFamily != cfYUV) {
        *ok = false;
        return ThreeFpExternalPixelFormat::Yuv420P8;
    }
    if (format.subSamplingW == 1 && format.subSamplingH == 1)
        return byDepth(ThreeFpExternalPixelFormat::Yuv420P8, ThreeFpExternalPixelFormat::Yuv420P10,
                       ThreeFpExternalPixelFormat::Yuv420P12, ThreeFpExternalPixelFormat::Yuv420P16, ok);
    if (format.subSamplingW == 1 && format.subSamplingH == 0)
        return byDepth(ThreeFpExternalPixelFormat::Yuv422P8, ThreeFpExternalPixelFormat::Yuv422P10,
                       ThreeFpExternalPixelFormat::Yuv422P12, ThreeFpExternalPixelFormat::Yuv422P16, ok);
    if (format.subSamplingW == 0 && format.subSamplingH == 0)
        return byDepth(ThreeFpExternalPixelFormat::Yuv444P8, ThreeFpExternalPixelFormat::Yuv444P10,
                       ThreeFpExternalPixelFormat::Yuv444P12, ThreeFpExternalPixelFormat::Yuv444P16, ok);
    *ok = false;
    return ThreeFpExternalPixelFormat::Yuv420P8;
}

std::uint32_t frameProperty(const VSAPI *api, const VSMap *properties, const char *name,
                            std::uint32_t fallback = 0)
{
    int error = peSuccess;
    const auto value = api->mapGetInt(properties, name, 0, &error);
    return error == peSuccess && value >= 0 && value <= std::numeric_limits<std::uint32_t>::max()
        ? static_cast<std::uint32_t>(value) : fallback;
}

}

struct VapourSynthFrameServer::Impl {
    using GetVSScriptApi = const VSSCRIPTAPI *(*)(int);

    std::unique_ptr<QLibrary> library;
    const VSSCRIPTAPI *scriptApi = nullptr;
    const VSAPI *vsApi = nullptr;
    VSScript *script = nullptr;
    VSNode *node = nullptr;
    VSVideoInfo info{};
    QString error;
    int threadLimit = qMax(4, QThread::idealThreadCount());
    int cacheMiB = 1024;
    struct RequestedFrame { const VSFrame *frame = nullptr; QString error; };
    std::map<int, std::future<RequestedFrame>> ahead;

    void schedule(int index) {
        if (ahead.contains(index)) return;
        auto *promise = new std::promise<RequestedFrame>;
        ahead.emplace(index, promise->get_future());
        vsApi->getFrameAsync(index, node, [](void *opaque, const VSFrame *frame, int, VSNode *, const char *error) {
            std::unique_ptr<std::promise<RequestedFrame>> promise(static_cast<std::promise<RequestedFrame> *>(opaque));
            promise->set_value({frame, error ? QString::fromUtf8(error) : QString()});
        }, promise);
    }

    void trimAhead(int current) {
        for (auto it = ahead.begin(); it != ahead.end();) {
            if ((it->first < current || it->first > current + 16) && it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                const auto result = it->second.get(); if (result.frame) vsApi->freeFrame(result.frame);
                it = ahead.erase(it);
            } else ++it;
        }
    }

    bool initialize(const QString &path)
    {
        library = std::make_unique<QLibrary>(path);
        library->setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::PreventUnloadHint);
        if (!library->load()) {
            error = QStringLiteral("无法加载 VSScript：%1").arg(library->errorString());
            return false;
        }
        const auto getApi = reinterpret_cast<GetVSScriptApi>(library->resolve("getVSScriptAPI"));
        if (!getApi) {
            error = QStringLiteral("VSScript.dll 缺少 getVSScriptAPI。");
            return false;
        }
        scriptApi = getApi(VSSCRIPT_API_VERSION);
        if (!scriptApi) {
            error = QStringLiteral("VSScript API 4.4 不可用；请使用项目固定的 VapourSynth R80 构建。");
            return false;
        }
        vsApi = scriptApi->getVSAPI(VAPOURSYNTH_API_VERSION);
        if (!vsApi) {
            error = QStringLiteral("VapourSynth API 4.3 不可用。");
            return false;
        }

        VSScript *probe = scriptApi->createScript(nullptr);
        if (!probe) {
            error = QStringLiteral("内置 VapourSynth 无法创建 Python 脚本环境。");
            return false;
        }
        static constexpr char probeScript[] =
            "import vapoursynth as vs\n"
            "required = ('std', 'resize', 'lsmas', 'ffms2', 'fmtc', 'rgvs', 'grain', "
            "'vszip', 'nlm_ispc', 'cas', 'placebo')\n"
            "missing = [name for name in required if not hasattr(vs.core, name)]\n"
            "if missing: raise RuntimeError('missing bundled namespace: ' + ', '.join(missing))\n";
        if (scriptApi->evaluateBuffer(probe, probeScript, "runtime-check.vpy") != 0) {
            const char *detail = scriptApi->getError(probe);
            error = QStringLiteral("内置 VapourSynth 运行时自检失败：%1")
                .arg(QString::fromUtf8(detail ? detail : "unknown error").trimmed());
            scriptApi->freeScript(probe);
            return false;
        }
        scriptApi->freeScript(probe);
        return true;
    }

    void clearScript()
    {
        for (auto &[index, future] : ahead) { const auto result = future.get(); if (result.frame) vsApi->freeFrame(result.frame); }
        ahead.clear();
        if (node && vsApi)
            vsApi->freeNode(node);
        node = nullptr;
        if (script && scriptApi)
            scriptApi->freeScript(script);
        script = nullptr;
        info = {};
    }

    ~Impl() { clearScript(); }
};

VapourSynthFrameServer::VapourSynthFrameServer(QObject *parent)
    : QObject(parent), impl_(std::make_unique<Impl>()), worker_(new QObject)
{
    qRegisterMetaType<VapourSynthFrame>();
    qRegisterMetaType<VapourSynthClipInfo>();
    libraryPath_ = findVSScript();
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    workerThread_.setObjectName(QStringLiteral("VapourSynthFrameServer"));
    workerThread_.start();

    if (libraryPath_.isEmpty()) {
        initError_ = QStringLiteral("内置 VapourSynth 运行时不完整：未发现 VSScript.dll。请重新构建或解压完整程序包。");
        initializing_ = false;
        QTimer::singleShot(0, this, [this] { emit initialized(false); });
        return;
    }
    QMetaObject::invokeMethod(worker_, [this] {
        const bool success = impl_->initialize(libraryPath_);
        const QString error = impl_->error;
        QMetaObject::invokeMethod(this, [this, success, error] {
            available_ = success;
            initError_ = error;
            initializing_ = false;
            emit initialized(success);
        });
    }, Qt::QueuedConnection);
}

VapourSynthFrameServer::~VapourSynthFrameServer()
{
    desiredFrame_.store(-1);
    if (workerThread_.isRunning()) {
        QMetaObject::invokeMethod(worker_, [this] { impl_.reset(); }, Qt::BlockingQueuedConnection);
        workerThread_.quit();
        workerThread_.wait();
    }
    worker_ = nullptr;
}

bool VapourSynthFrameServer::initializing() const { return initializing_; }

bool VapourSynthFrameServer::available() const { return available_; }
QString VapourSynthFrameServer::libraryPath() const { return libraryPath_; }
QString VapourSynthFrameServer::errorString() const { return initError_; }

void VapourSynthFrameServer::unloadScript()
{
    const auto scriptGeneration=++scriptGeneration_;
    desiredFrame_.store(-1);
    QMetaObject::invokeMethod(worker_, [this,scriptGeneration] { if(scriptGeneration==scriptGeneration_.load()){impl_->clearScript();QMetaObject::invokeMethod(this,[this,scriptGeneration]{if(scriptGeneration==scriptGeneration_.load())emit scriptUnloaded();});} }, Qt::QueuedConnection);
}

void VapourSynthFrameServer::loadScript(const QString &source, const QString &scriptPath)
{
    if (!available_) {
        emit errorOccurred(initError_);
        return;
    }
    const auto scriptGeneration=++scriptGeneration_;
    desiredFrame_.store(-1);
    const QByteArray scriptUtf8 = source.toUtf8();
    const QByteArray pathUtf8 = scriptPath.toUtf8();
    QMetaObject::invokeMethod(worker_, [this, scriptUtf8, pathUtf8,scriptGeneration] {
        if(scriptGeneration!=scriptGeneration_.load())return;
        impl_->clearScript();
        impl_->script = impl_->scriptApi->createScript(nullptr);
        if (!impl_->script) {
            QMetaObject::invokeMethod(this, [this,scriptGeneration] { if(scriptGeneration==scriptGeneration_.load())emit errorOccurred(QStringLiteral("无法创建 VapourSynth 脚本环境。")); });
            return;
        }
        if (impl_->scriptApi->evaluateBuffer(impl_->script, scriptUtf8.constData(), pathUtf8.constData()) != 0) {
            const char *detail = impl_->scriptApi->getError(impl_->script);
            const QString message = describeScriptError(
                QString::fromUtf8(detail ? detail : "unknown error"));
            QMetaObject::invokeMethod(this, [this, message,scriptGeneration] { if(scriptGeneration==scriptGeneration_.load())emit errorOccurred(message); });
            return;
        }
        impl_->node = impl_->scriptApi->getOutputNode(impl_->script, 0);
        if (!impl_->node || impl_->vsApi->getNodeType(impl_->node) != mtVideo) {
            QMetaObject::invokeMethod(this, [this,scriptGeneration] { if(scriptGeneration==scriptGeneration_.load())emit errorOccurred(QStringLiteral("VPY 的 output 0 不是视频节点。")); });
            return;
        }
        impl_->info = *impl_->vsApi->getVideoInfo(impl_->node);
        if (VSCore *core = impl_->scriptApi->getCore(impl_->script)) {
            impl_->vsApi->setThreadCount(impl_->threadLimit, core);
            impl_->vsApi->setMaxCacheSize(static_cast<int64_t>(impl_->cacheMiB) * 1048576, core);
        }
        const auto makeInfo = [this](const VSVideoInfo &info) {
            char formatName[32]{};
            VapourSynthClipInfo result;
            result.width = info.width;
            result.height = info.height;
            result.totalFrames = info.numFrames;
            result.fpsNumerator = info.fpsNum;
            result.fpsDenominator = info.fpsDen;
            result.formatName = impl_->vsApi->getVideoFormatName(&info.format, formatName)
                ? QString::fromLatin1(formatName) : QStringLiteral("Variable");
            return result;
        };
        const auto processedInfo = makeInfo(impl_->info);
        auto sourceInfo = processedInfo;
        if (VSNode *sourceNode = impl_->scriptApi->getOutputNode(impl_->script, 1)) {
            if (impl_->vsApi->getNodeType(sourceNode) == mtVideo)
                sourceInfo = makeInfo(*impl_->vsApi->getVideoInfo(sourceNode));
            impl_->vsApi->freeNode(sourceNode);
        }
        QMetaObject::invokeMethod(this, [this, processedInfo, sourceInfo,scriptGeneration] {
            if(scriptGeneration!=scriptGeneration_.load())return;
            emit scriptLoaded(processedInfo, sourceInfo);
        });
    }, Qt::QueuedConnection);
}

void VapourSynthFrameServer::requestFrame(int frameIndex, int prefetchFrames)
{
    if (!available_ || frameIndex < 0)
        return;
    desiredFrame_.store(frameIndex);
    if (frameRequestScheduled_.exchange(true))
        return;

    const auto scriptGeneration=scriptGeneration_.load();
    QMetaObject::invokeMethod(worker_, [this, frameIndex, prefetchFrames,scriptGeneration] {
        const auto finish = [this, frameIndex] {
            frameRequestScheduled_.store(false);
            const int latest = desiredFrame_.load();
            if (latest >= 0 && latest != frameIndex)
                requestFrame(latest);
        };
        if (scriptGeneration!=scriptGeneration_.load() || !impl_->node || desiredFrame_.load() != frameIndex) {
            finish();
            return;
        }
        const int bounded = impl_->info.numFrames > 0 ? qMin(frameIndex, impl_->info.numFrames - 1) : frameIndex;
        impl_->trimAhead(bounded);
        const int availableAhead = impl_->info.numFrames > 0
            ? qMax(0, impl_->info.numFrames - bounded - 1) : 0;
        const int warmCount = qBound(0, prefetchFrames, qMin(16, availableAhead));
        impl_->schedule(bounded);
        for (int offset = 1; offset <= warmCount; ++offset) {
            const int warmFrame = bounded + offset;
            if (impl_->ahead.size() < 20) impl_->schedule(warmFrame);
        }
        const auto result = impl_->ahead.at(bounded).get();
        impl_->ahead.erase(bounded);
        const VSFrame *source = result.frame;
        if (!source) {
            const bool current = desiredFrame_.load() == frameIndex;
            const QString message = QStringLiteral("请求 VS 帧 %1 失败：%2")
                .arg(bounded).arg(result.error);
            finish();
            if (current)
                QMetaObject::invokeMethod(this, [this, message,scriptGeneration] { if(scriptGeneration==scriptGeneration_.load())emit errorOccurred(message); });
            return;
        }
        if (desiredFrame_.load() != frameIndex) {
            impl_->vsApi->freeFrame(source);
            finish();
            return;
        }

        VapourSynthFrame frame;
        const VSVideoFormat *format = impl_->vsApi->getVideoFrameFormat(source);
        bool supported = false;
        frame.format = mapFormat(*format, &supported);
        if (!supported) {
            char nameBuffer[32]{};
            impl_->vsApi->getVideoFormatName(format, nameBuffer);
            impl_->vsApi->freeFrame(source);
            const bool current = desiredFrame_.load() == frameIndex;
            const QString message = QStringLiteral("3FP 外部帧接口暂不支持 VS 格式 %1；请先转换为 8/10/12/16-bit Gray/YUV/RGB。")
                .arg(QString::fromLatin1(nameBuffer));
            finish();
            if (current)
                QMetaObject::invokeMethod(this, [this, message,scriptGeneration] { if(scriptGeneration==scriptGeneration_.load())emit errorOccurred(message); });
            return;
        }

        frame.width = impl_->vsApi->getFrameWidth(source, 0);
        frame.height = impl_->vsApi->getFrameHeight(source, 0);
        frame.frameIndex = bounded;
        frame.totalFrames = impl_->info.numFrames;
        frame.duration100ns = impl_->info.fpsNum > 0
            ? static_cast<std::int64_t>(std::llround(10000000.0 * impl_->info.fpsDen / impl_->info.fpsNum)) : 0;

        const VSMap *properties = impl_->vsApi->getFramePropertiesRO(source);
        int durationError = peSuccess;
        const auto durationNum = impl_->vsApi->mapGetInt(properties, "_DurationNum", 0, &durationError);
        int denominatorError = peSuccess;
        const auto durationDen = impl_->vsApi->mapGetInt(properties, "_DurationDen", 0, &denominatorError);
        if (durationError == peSuccess && denominatorError == peSuccess && durationNum > 0 && durationDen > 0)
            frame.duration100ns = static_cast<std::int64_t>(std::llround(10000000.0 * durationNum / durationDen));

        const auto range = frameProperty(impl_->vsApi, properties, "_ColorRange", 2);
        frame.colorRange = range == 0 ? 2u : (range == 1 ? 1u : 0u);
        frame.colorPrimaries = frameProperty(impl_->vsApi, properties, "_Primaries", 2);
        frame.colorTransfer = frameProperty(impl_->vsApi, properties, "_Transfer", 2);
        frame.colorMatrix = frameProperty(impl_->vsApi, properties, "_Matrix", 2);
        int chromaError = peSuccess;
        const auto chroma = impl_->vsApi->mapGetInt(properties, "_ChromaLocation", 0, &chromaError);
        frame.chromaLocation = chromaError == peSuccess && chroma >= 0 ? static_cast<std::uint32_t>(chroma + 1) : 0u;

        for (int plane = 0; plane < format->numPlanes; ++plane) {
            if (desiredFrame_.load() != frameIndex) {
                impl_->vsApi->freeFrame(source);
                finish();
                return;
            }
            const auto stride = impl_->vsApi->getStride(source, plane);
            const int height = impl_->vsApi->getFrameHeight(source, plane);
            const auto byteCount = stride * height;
            if (stride <= 0 || byteCount <= 0 || byteCount > std::numeric_limits<int>::max()) {
                impl_->vsApi->freeFrame(source);
                const bool current = desiredFrame_.load() == frameIndex;
                const QString message = QStringLiteral("VS 帧 %1 的 plane %2 stride 无效。").arg(bounded).arg(plane);
                finish();
                if (current)
                    QMetaObject::invokeMethod(this, [this, message,scriptGeneration] { if(scriptGeneration==scriptGeneration_.load())emit errorOccurred(message); });
                return;
            }
            frame.strides[plane] = static_cast<std::int32_t>(stride);
            frame.planes[plane] = QByteArray(reinterpret_cast<const char *>(impl_->vsApi->getReadPtr(source, plane)),
                                             static_cast<int>(byteCount));
        }
        impl_->vsApi->freeFrame(source);
        const bool current = desiredFrame_.load() == frameIndex;
        finish();
        if (current)
            QMetaObject::invokeMethod(this, [this, frame = std::move(frame),scriptGeneration] { if(scriptGeneration==scriptGeneration_.load())emit frameReady(frame); });
    }, Qt::QueuedConnection);
}

void VapourSynthFrameServer::setResourceLimits(int threads, int cacheMiB)
{
    QMetaObject::invokeMethod(worker_, [this, threads, cacheMiB] {
        impl_->threadLimit = qBound(1, threads, qMax(1, QThread::idealThreadCount()));
        impl_->cacheMiB = qBound(64, cacheMiB, 8192);
        if (impl_->script) if (auto *core = impl_->scriptApi->getCore(impl_->script)) {
            impl_->vsApi->setThreadCount(impl_->threadLimit, core);
            impl_->vsApi->setMaxCacheSize(static_cast<int64_t>(impl_->cacheMiB) * 1048576, core);
        }
    }, Qt::QueuedConnection);
}

}
