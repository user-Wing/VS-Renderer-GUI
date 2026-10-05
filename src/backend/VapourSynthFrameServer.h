#pragma once

#include "backend/ThreeFpApi.h"

#include <QByteArray>
#include <QObject>
#include <QThread>

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

namespace vsr {

struct VapourSynthClipInfo {
    int width = 0;
    int height = 0;
    int totalFrames = 0;
    std::int64_t fpsNumerator = 0;
    std::int64_t fpsDenominator = 0;
    QString formatName;
};

struct VapourSynthFrame {
    int width = 0;
    int height = 0;
    ThreeFpExternalPixelFormat format = ThreeFpExternalPixelFormat::Yuv420P8;
    std::array<QByteArray, 4> planes;
    std::array<std::int32_t, 4> strides{};
    std::uint32_t colorRange = 0;
    std::uint32_t colorPrimaries = 0;
    std::uint32_t colorTransfer = 0;
    std::uint32_t colorMatrix = 0;
    std::uint32_t chromaLocation = 0;
    std::int64_t frameIndex = 0;
    std::int64_t duration100ns = 0;
    std::int64_t totalFrames = 0;
};

class VapourSynthFrameServer final : public QObject {
    Q_OBJECT
public:
    explicit VapourSynthFrameServer(QObject *parent = nullptr);
    ~VapourSynthFrameServer() override;

    bool available() const;
    bool initializing() const;
    QString libraryPath() const;
    QString errorString() const;
    void loadScript(const QString &script, const QString &scriptPath);
    void unloadScript();
    void requestFrame(int frameIndex, int prefetchFrames = 0, bool warmStart = false);
    void setResourceLimits(int threads, int cacheMiB);

signals:
    void initialized(bool success);
    void scriptUnloaded();
    void scriptLoaded(const vsr::VapourSynthClipInfo &processed, const vsr::VapourSynthClipInfo &source);
    void frameReady(const vsr::VapourSynthFrame &frame);
    void errorOccurred(const QString &message);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    QThread workerThread_;
    QObject *worker_ = nullptr;
    QString libraryPath_;
    QString initError_;
    bool available_ = false;
    bool initializing_ = true;
    std::atomic<quint64> scriptGeneration_{0};
    std::atomic<int> desiredFrame_{-1};
    std::atomic_bool frameRequestScheduled_{false};
};

}

Q_DECLARE_METATYPE(vsr::VapourSynthFrame)
Q_DECLARE_METATYPE(vsr::VapourSynthClipInfo)
