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
    bool resetVideoOutput();
    bool openFile(const QString &path);
    bool play();
    bool pause();
    bool setClockOnly(bool enabled);
    void stop();
    bool seek(std::int64_t position100ns);
    bool seekFrame(std::int64_t frame);
    bool stepFrame(int direction);
    void setMuted(bool muted);
    void setVolume(float volume);
    bool stepKeyframe(int direction);
    bool setPlaybackRate(double rate);
    bool setDecodeMode(unsigned mode);
    void setAutomaticHdr(bool enabled);
    QString mediaInfo() const;
    bool selectAudio(int stream);
    bool loadExternalAudio(const QString &path);
    bool clearExternalAudio();
    bool setAudioEffects(bool enabled, const float *gains, float wave, qint64 delay);
    bool audioLevels(ThreeFpAudioLevels &levels, bool input) const;
    bool setVrrPresent(bool enabled);
    bool setVrrPacing(bool enabled);
    bool setScalingAlgorithms(ThreeFpScalingAlgorithm upscale, ThreeFpScalingAlgorithm downscale);
    bool setChromaAlgorithm(int algorithm);
    void setView(float zoom, float panX, float panY);
    void redraw();
    ThreeFpSnapshot snapshot() const;
    bool setColorSettings(const VsrColorSettings &settings);
    VsrColorStatus colorStatus() const;
    bool samplePixel(int x, int y, ThreeFpPixelProbe &sample) const;
    bool submitFrame(const VapourSynthFrame &frame);
    bool setOutputFormat(const QString &format);
    void setAntiRinging(bool enabled);
    bool setSubtitle(const QImage &image);
    QImage capture() const;
    QImage captureFloat() const;

signals:
    void errorOccurred(const QString &message);

private:
    bool createSession();
    bool check(ThreeFpResult result, const QString &operation);

    ThreeFpApi &api_;
    QWidget *surface_ = nullptr;
    void *handle_ = nullptr;
    QString lastError_;
    int chromaAlgorithm_ = 1;
    bool muted_ = false;
    bool clockOnly_ = false;
    float volume_ = 1.0f;
    unsigned decodeMode_ = 2;
    bool automaticHdr_ = false;
    VsrColorSettings colorSettings_ = VsrColorDefaultSettings();
    bool customColorSettings_ = false;
    bool antiRinging_ = false;
    QString outputFormat_;
    quint64 subtitleSequence_ = 0;
    bool vrrPresent_ = false;
    bool vrrPacing_ = false;
    float zoom_ = 1.0f;
    float panX_ = 0.0f;
    float panY_ = 0.0f;
    ThreeFpScalingAlgorithm upscale_ = ThreeFpScalingAlgorithm::Nearest;
    ThreeFpScalingAlgorithm downscale_ = ThreeFpScalingAlgorithm::Lanczos3;
};

}
