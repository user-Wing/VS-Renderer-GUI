#pragma once

#include "backend/ThreeFpApi.h"

#include <QObject>

class QWidget;

namespace vsr {

struct VapourSynthFrame;

class ThreeFpPlayer final : public QObject {
    Q_OBJECT
public:
    ThreeFpPlayer(ThreeFpApi &api, QWidget *surface, QObject *parent = nullptr);
    ~ThreeFpPlayer() override;

    bool ready() const;
    QString lastError() const;
    bool openFile(const QString &path);
    bool play();
    bool pause();
    void stop();
    bool seek(std::int64_t position100ns);
    bool seekFrame(std::int64_t frame);
    bool stepFrame(int direction);
    void setMuted(bool muted);
    bool setVrrPresent(bool enabled);
    bool setVrrPacing(bool enabled);
    bool setScalingAlgorithms(ThreeFpScalingAlgorithm upscale, ThreeFpScalingAlgorithm downscale);
    void setView(float zoom, float panX, float panY);
    void redraw();
    ThreeFpSnapshot snapshot() const;
    bool samplePixel(int x, int y, ThreeFpPixelProbe &sample) const;
    bool submitFrame(const VapourSynthFrame &frame);

signals:
    void errorOccurred(const QString &message);

private:
    bool check(ThreeFpResult result, const QString &operation);

    ThreeFpApi &api_;
    void *handle_ = nullptr;
    QString lastError_;
};

}
