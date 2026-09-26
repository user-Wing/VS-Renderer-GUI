#pragma once
#include <QObject>
#include "backend/VapourSynthFrameServer.h"
#include <optional>
#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QTimer>
#include <memory>

namespace vsr {
class ThreeFpPlayer;
class VapourSynthFrameServer;

class StartupWarmup final : public QObject {
    Q_OBJECT
public:
    StartupWarmup(ThreeFpPlayer *source, ThreeFpPlayer *processed,
                  VapourSynthFrameServer *server, QObject *parent = nullptr);
    bool isRunning() const;
    bool ownsScript() const;
    void releaseScript();
    void start();
signals:
    void progress(int percent, const QString &stage);
    void finished(bool success, const QString &message);
private:
    void poll();
    void complete(bool success, const QString &message);
    ThreeFpPlayer *source_;
    ThreeFpPlayer *processed_;
    VapourSynthFrameServer *server_;
    std::shared_ptr<QTemporaryDir> directory_ = std::make_shared<QTemporaryDir>();
    QTimer timer_;
    QElapsedTimer elapsed_;
    bool running_ = true;
    bool ownsScript_ = true;
    bool sourceStarted_ = false;
    bool sourceReady_ = false;
    bool processedReady_ = false;
    bool started_ = false;
    std::optional<VapourSynthFrame> pendingFrame_;
};
}
