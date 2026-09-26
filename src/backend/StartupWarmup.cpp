#include "backend/StartupWarmup.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace vsr {

StartupWarmup::StartupWarmup(ThreeFpPlayer *source, ThreeFpPlayer *processed,
                             VapourSynthFrameServer *server, QObject *parent)
    : QObject(parent), source_(source), processed_(processed), server_(server)
{
    connect(server_, &VapourSynthFrameServer::initialized, this, [this] { if (running_) start(); });
    // The demuxer and VS source nodes can keep the warmup media open until teardown.
    connect(source_, &QObject::destroyed, [keep = directory_] {});
    connect(server_, &QObject::destroyed, [keep = directory_] {});
    timer_.setInterval(25);
    connect(&timer_, &QTimer::timeout, this, &StartupWarmup::poll);
    connect(server_, &VapourSynthFrameServer::scriptLoaded, this, [this] {
        if (running_) { emit progress(55, QStringLiteral("VS 源滤镜就绪，正在取首帧")); server_->requestFrame(0); }
    });
    connect(server_, &VapourSynthFrameServer::frameReady, this, [this](const VapourSynthFrame &frame) {
        if (!running_) return;
        pendingFrame_ = frame;
        emit progress(70, QStringLiteral("VS 首帧就绪，等待原生着色器"));
    });
    connect(server_, &VapourSynthFrameServer::errorOccurred, this, [this](const QString &message) {
        if (running_) complete(false, message);
    });
}

bool StartupWarmup::isRunning() const { return running_; }
bool StartupWarmup::ownsScript() const { return ownsScript_; }
void StartupWarmup::releaseScript() { ownsScript_ = false; }

void StartupWarmup::start()
{
    if (started_) return;
    if (server_->initializing()) { emit progress(5, QStringLiteral("后台初始化 VS / Python")); return; }
    started_ = true;
    emit progress(20, QStringLiteral("后台预热解码、源滤镜与着色器"));
    if (!source_->ready() || !processed_->ready() || !server_->available()) {
        complete(false, QStringLiteral("运行时不可用，预热未完成。"));
        return;
    }
    const QString path = directory_->filePath(QStringLiteral("warmup.mkv"));
    if (!directory_->isValid() || !QFile::copy(QStringLiteral(":/startup/warmup.mkv"), path)) {
        complete(false, QStringLiteral("无法读取内置预热视频。"));
        return;
    }
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    // JSON string escaping is also valid Python string escaping for this path.
    const QByteArray encoded = QJsonDocument(QJsonArray{path}).toJson(QJsonDocument::Compact);
    const QString literal = QString::fromUtf8(encoded.mid(1, encoded.size() - 2));
    const QString script = QStringLiteral(
        "import vapoursynth as vs\n"
        "core = vs.core\n"
        "a = core.lsmas.LWLibavSource(source=%1)\n"
        "b = core.ffms2.Source(source=%1)\n"
        "clip = core.std.StackHorizontal([a, b])\n"
        "clip = core.resize.Bicubic(clip, width=128, height=72, format=vs.YUV420P8)\n"
        "clip.set_output()\n").arg(literal);
    source_->setMuted(true);
    if (!source_->openFile(path)) { complete(false, source_->lastError()); return; }
    elapsed_.start();
    timer_.start();
    server_->loadScript(script, directory_->filePath(QStringLiteral("warmup.vpy")));
}

void StartupWarmup::poll()
{
    const auto source = source_->snapshot();
    if (source.state == ThreeFpState::Failed) { complete(false, QStringLiteral("源解码预热失败。")); return; }
    if (!sourceStarted_ && (source.state == ThreeFpState::Ready || source.state == ThreeFpState::Paused))
        sourceStarted_ = source_->play();
    if (!sourceReady_ && source.presentedVideoFrames > 0) {
        source_->pause();
        sourceReady_ = true;
    }
    if (sourceReady_ && pendingFrame_) {
        // Source decoding compiles shared shader bytecode on the native worker first.
        if (!processed_->submitFrame(*pendingFrame_)) { complete(false, processed_->lastError()); return; }
        pendingFrame_.reset();
        processedReady_ = true;
        emit progress(90, QStringLiteral("正在完成双路首帧呈现"));
    }
    if (sourceReady_ && processedReady_ && processed_->snapshot().swapChainPresents > 0) {
        complete(true, QStringLiteral("解码、VS 源滤镜与双路渲染已预热，可以导入视频。"));
    } else if (elapsed_.elapsed() > 30000) {
        complete(false, QStringLiteral("启动预热超时，仍可导入视频；详情请检查运行时。"));
    }
}

void StartupWarmup::complete(bool success, const QString &message)
{
    if (!running_) return;
    running_ = false;
    timer_.stop();
    source_->pause();
    source_->setMuted(false);
    emit progress(100, message);
    emit finished(success, message);
}

}
