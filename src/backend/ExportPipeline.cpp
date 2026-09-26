#include "backend/ExportPipeline.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QLibrary>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace vsr {
namespace {

QString resolveExecutable(const QString &requested, const QString &applicationDirectory,
                          const QString &bundledRelative, const QString &localFallback = {})
{
    const QFileInfo requestedInfo(requested);
    if (requestedInfo.isAbsolute() && requestedInfo.isFile())
        return requestedInfo.absoluteFilePath();
    const QString bundled = QDir(applicationDirectory).filePath(bundledRelative);
    if (QFileInfo::exists(bundled))
        return bundled;
    const QString found = QStandardPaths::findExecutable(requested);
    if (!found.isEmpty())
        return found;
    if (!localFallback.isEmpty() && QFileInfo::exists(localFallback))
        return localFallback;
    return {};
}

bool isVideoMap(const QString &value)
{
    return value == QStringLiteral("0:v") || value == QStringLiteral("0:v?") ||
        value.startsWith(QStringLiteral("0:v:"));
}

}

ExportPipeline::ExportPipeline(QObject *parent)
    : QObject(parent), ffmpeg_(new QProcess(this)), vspipe_(new QProcess(this))
{
    vspipe_->setStandardOutputProcess(ffmpeg_);
    connect(ffmpeg_, &QProcess::readyReadStandardError, this, [this] {
        emit logMessage(QString::fromLocal8Bit(ffmpeg_->readAllStandardError()).trimmed());
    });
    connect(vspipe_, &QProcess::readyReadStandardError, this, [this] {
        const auto data = vspipe_->readAllStandardError();
        emit logMessage(QString::fromLocal8Bit(data).trimmed());
        producerBuffer_ += data;
        int end;
        while ((end = producerBuffer_.indexOf('\n')) >= 0) {
            const auto line = producerBuffer_.left(end).trimmed();
            producerBuffer_.remove(0, end + 1);
            if (line.startsWith("__VSR_DURATION__=")) emit durationKnown(line.mid(17).toDouble());
        }
    });
    connect(ffmpeg_, &QProcess::readyReadStandardOutput, this, &ExportPipeline::readProgress);
    for (auto *process : {ffmpeg_, vspipe_}) {
        connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart)
                return;
            processError_ = process->errorString();
            if (vspipe_->state() != QProcess::NotRunning)
                vspipe_->kill();
            finishIfDone();
        });
        connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
                [this, process](int, QProcess::ExitStatus) {
            if (process == ffmpeg_ && vspipe_->state() != QProcess::NotRunning) {
                consumerFinishedEarly_ = ffmpeg_->exitStatus() == QProcess::NormalExit && ffmpeg_->exitCode() == 0;
                vspipe_->kill();
            }
            finishIfDone();
        });
    }
}

ExportPlan ExportPipeline::buildPlan(const QString &command, const QString &sourcePath,
                                     const QString &outputPath, const QString &scriptPath,
                                     const QString &applicationDirectory)
{
    ExportPlan plan;
    QString normalized = command.trimmed();
    normalized.replace(QStringLiteral("\\_"), QStringLiteral("_"));
    QStringList tokens = QProcess::splitCommand(normalized);
    if (tokens.size() < 2) {
        plan.error = QStringLiteral("请输入完整 FFmpeg 命令。");
        return plan;
    }
    if (tokens.contains(QStringLiteral("-filter_complex")) || tokens.contains(QStringLiteral("-lavfi"))) {
        plan.error = QStringLiteral("3FUI 参数兼容模式暂不接受 -filter_complex/-lavfi；请使用 -filter:v 和 -filter:a。");
        return plan;
    }

    const QString requestedFfmpeg = tokens.takeFirst();
    plan.ffmpegProgram = resolveExecutable(
        requestedFfmpeg, applicationDirectory, QStringLiteral("runtime/ffmpeg/ffmpeg.exe"),
        QStringLiteral("C:\\PortableSoft\\FFmpegFreeUI ReadyToRun x64\\ffmpeg.exe"));
    plan.vspipeProgram = resolveExecutable(
        QStringLiteral("vspipe.exe"), applicationDirectory,
        QStringLiteral("runtime/python/Lib/site-packages/vapoursynth/vspipe.exe"));
    if (plan.ffmpegProgram.isEmpty()) {
        plan.error = QStringLiteral("找不到 ffmpeg.exe；请把它放在程序目录、PATH，或使用绝对路径开头的命令。");
        return plan;
    }
    if (plan.vspipeProgram.isEmpty()) {
        plan.error = QStringLiteral("便携 VapourSynth 运行时缺少 vspipe.exe。");
        return plan;
    }
    if (!QFileInfo::exists(sourcePath) || outputPath.trimmed().isEmpty() || scriptPath.trimmed().isEmpty()) {
        plan.error = QStringLiteral("源文件、输出路径或 VPY 脚本无效。");
        return plan;
    }

    if (QFileInfo(outputPath).suffix().compare(QStringLiteral("mkv"), Qt::CaseInsensitive) != 0) {
        plan.error = QStringLiteral("导出仅支持 MKV，请使用 .mkv 扩展名。");
        return plan;
    }
    if (QFileInfo(outputPath).absoluteFilePath().compare(QFileInfo(sourcePath).absoluteFilePath(), Qt::CaseInsensitive) == 0) {
        plan.error = QStringLiteral("输出文件不能覆盖源视频。");
        return plan;
    }

    int sourceIndex = -1;
    int outputIndex = -1;
    bool videoMapped = false;
    for (int i = 0; i < tokens.size(); ++i) {
        if (tokens[i] == QStringLiteral("<输入文件>")) {
            sourceIndex = i;
            tokens[i] = QDir::toNativeSeparators(sourcePath);
        } else if (tokens[i] == QStringLiteral("<输出文件>")) {
            outputIndex = i;
            tokens[i] = QDir::toNativeSeparators(outputPath);
        }
        if (tokens[i] == QStringLiteral("-map") && i + 1 < tokens.size() && isVideoMap(tokens[i + 1])) {
            tokens[i + 1].replace(0, 1, QLatin1Char('1'));
            videoMapped = true;
        }
        if ((tokens[i] == QStringLiteral("-filter:v") || tokens[i] == QStringLiteral("-filter:v:0") ||
             tokens[i] == QStringLiteral("-vf")) && i + 1 < tokens.size() &&
            tokens[i + 1].contains(QStringLiteral("scale_cuda")) &&
            !tokens[i + 1].contains(QStringLiteral("hwupload_cuda"))) {
            tokens[i + 1].prepend(QStringLiteral("hwupload_cuda,"));
        }
    }
    if (sourceIndex < 1 || tokens[sourceIndex - 1] != QStringLiteral("-i")) {
        plan.error = QStringLiteral("命令必须包含 -i <输入文件>。");
        return plan;
    }
    if (outputIndex < 0) {
        plan.error = QStringLiteral("命令必须包含 <输出文件>。");
        return plan;
    }

    tokens.insert(sourceIndex + 1, QStringLiteral("-f"));
    tokens.insert(sourceIndex + 2, QStringLiteral("yuv4mpegpipe"));
    tokens.insert(sourceIndex + 3, QStringLiteral("-i"));
    tokens.insert(sourceIndex + 4, QStringLiteral("-"));
    outputIndex += 4;
    if (!videoMapped) {
        tokens.insert(outputIndex, QStringLiteral("-map"));
        tokens.insert(outputIndex + 1, QStringLiteral("1:v:0?"));
    }

    tokens.prepend(QStringLiteral("-nostdin"));
    tokens.prepend(QStringLiteral("pipe:1"));
    tokens.prepend(QStringLiteral("-progress"));
    // End other streams when the VS video pipe reaches EOF, including stop.
    const int destination = tokens.indexOf(QDir::toNativeSeparators(outputPath));
    tokens.insert(destination, QStringLiteral("-shortest"));
    tokens.insert(destination + 1, QStringLiteral("-f"));
    tokens.insert(destination + 2, QStringLiteral("matroska"));
    plan.ffmpegArguments = tokens;
    plan.vspipeArguments = {
        QStringLiteral("--requests"), QString::number(qMax(4, QThread::idealThreadCount())),
        QStringLiteral("--container"), QStringLiteral("y4m"),
        QDir::toNativeSeparators(scriptPath), QStringLiteral("-")};
    return plan;
}

bool ExportPipeline::start(const ExportPlan &plan)
{
    if (isRunning() || !plan.error.isEmpty())
        return false;
    finishing_ = false;
    stopping_ = false;
    paused_ = false;
    consumerFinishedEarly_ = false;
    processError_.clear();
    progressBuffer_.clear();
    producerBuffer_.clear();
    frames_ = outputMicroseconds_ = 0;
    ffmpeg_->setProgram(plan.ffmpegProgram);
    ffmpeg_->setArguments(plan.ffmpegArguments);
    vspipe_->setProgram(plan.vspipeProgram);
    vspipe_->setArguments(plan.vspipeArguments);
    vspipe_->start();
    if (!vspipe_->waitForStarted(3000))
        return false;
    ffmpeg_->start();
    if (!ffmpeg_->waitForStarted(3000)) {
        vspipe_->kill();
        return false;
    }
    return true;
}

bool ExportPipeline::isRunning() const
{
    return ffmpeg_->state() != QProcess::NotRunning || vspipe_->state() != QProcess::NotRunning;
}

void ExportPipeline::cancel()
{
    if (!isRunning() || stopping_)
        return;
    if (paused_ && !setPaused(false))
        return;
    stopping_ = true;
    // EOF lets FFmpeg drain packets and write the trailer; never kill the muxer.
    if (vspipe_->state() != QProcess::NotRunning)
        vspipe_->kill();
}

bool ExportPipeline::isPaused() const { return paused_; }

bool ExportPipeline::setPaused(bool paused)
{
    if (!isRunning() || stopping_) return false;
    if (paused == paused_) return true;
#ifdef Q_OS_WIN
    using ProcessControl = LONG (NTAPI *)(HANDLE);
    const auto suspend = reinterpret_cast<ProcessControl>(QLibrary::resolve(QStringLiteral("ntdll"), "NtSuspendProcess"));
    const auto resume = reinterpret_cast<ProcessControl>(QLibrary::resolve(QStringLiteral("ntdll"), "NtResumeProcess"));
    if (!suspend || !resume) return false;
    QList<HANDLE> handles;
    for (auto *process : {vspipe_, ffmpeg_}) {
        if (process->state() == QProcess::NotRunning) continue;
        HANDLE handle = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, static_cast<DWORD>(process->processId()));
        if (!handle || (paused ? suspend(handle) : resume(handle)) < 0) {
            if (handle) CloseHandle(handle);
            for (HANDLE changed : handles) {
                if (paused) resume(changed); else suspend(changed);
                CloseHandle(changed);
            }
            emit logMessage(QStringLiteral("无法更改编码进程的暂停状态。"));
            return false;
        }
        handles.append(handle);
    }
    for (HANDLE handle : handles) CloseHandle(handle);
    paused_ = paused;
    return true;
#else
    return false;
#endif
}

void ExportPipeline::readProgress()
{
    progressBuffer_ += ffmpeg_->readAllStandardOutput();
    int end = -1;
    while ((end = progressBuffer_.indexOf('\n')) >= 0) {
        const QByteArray line = progressBuffer_.left(end).trimmed();
        progressBuffer_.remove(0, end + 1);
        const int equals = line.indexOf('=');
        const auto key = line.left(equals);
        const auto value = line.mid(equals + 1);
        if (key == "frame") frames_ = value.toLongLong();
        if (key == "out_time_us") outputMicroseconds_ = value.toLongLong();
        if (key == "progress") emit progress(frames_, outputMicroseconds_);
    }
}

void ExportPipeline::finishIfDone()
{
    if (finishing_ || isRunning())
        return;
    finishing_ = true;
    paused_ = false;
    readProgress();
    const bool muxed = processError_.isEmpty() && ffmpeg_->exitStatus() == QProcess::NormalExit
        && ffmpeg_->exitCode() == 0;
    const bool success = muxed && !stopping_ && (vspipe_->exitCode() == 0 || consumerFinishedEarly_);
    emit finished(success, stopping_
        ? (muxed ? QStringLiteral("已停止，已输出的文件已保留。")
                 : QStringLiteral("已停止并保留输出文件；编码器异常退出，请检查文件完整性。"))
        : (success ? QStringLiteral("导出完成。")
                   : QStringLiteral("导出失败：FFmpeg %1，vspipe %2。%3")
                         .arg(ffmpeg_->exitCode()).arg(vspipe_->exitCode()).arg(processError_)));
}

}
