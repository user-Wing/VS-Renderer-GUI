#pragma once

#include <QObject>
#include <QStringList>

class QProcess;

namespace vsr {

struct ExportPlan {
    QString ffmpegProgram;
    QStringList ffmpegArguments;
    QString vspipeProgram;
    QStringList vspipeArguments;
    QString error;
};

class ExportPipeline final : public QObject {
    Q_OBJECT
public:
    explicit ExportPipeline(QObject *parent = nullptr);

    static ExportPlan buildPlan(const QString &command, const QString &sourcePath,
                                const QString &outputPath, const QString &scriptPath,
                                const QString &applicationDirectory);
    bool start(const ExportPlan &plan);
    bool isRunning() const;
    void cancel();
    bool setPaused(bool paused);
    bool isPaused() const;

signals:
    void durationKnown(double seconds);
    void progress(qint64 frames, qint64 outputMicroseconds);
    void logMessage(const QString &message);
    void finished(bool success, const QString &message);

private:
    void finishIfDone();
    void readProgress();
    QByteArray progressBuffer_;
    QByteArray producerBuffer_;
    qint64 frames_ = 0;
    qint64 outputMicroseconds_ = 0;
    bool stopping_ = false;
    bool paused_ = false;
    bool consumerFinishedEarly_ = false;
    QString processError_;
    QProcess *ffmpeg_ = nullptr;
    QProcess *vspipe_ = nullptr;
    bool finishing_ = false;
};

}
