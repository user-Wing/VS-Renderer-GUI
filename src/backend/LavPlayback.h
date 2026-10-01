#pragma once
#include <QString>
#include <cstdint>
#include <memory>
#include <QImage>
namespace vsr {
class LavPlayback final {
public:
    LavPlayback();
    ~LavPlayback();
    bool open(const QString &path, void *videoWindow = nullptr, bool video = true, bool audio = true);
    bool play();
    bool pause();
    bool seek(std::int64_t position);
    std::int64_t position() const;
    std::int64_t duration() const;
    void volume(float volume, bool muted);
    QString error() const;
    void resizeVideo(int width, int height);
    void showVideoSettings(void *owner);
    void showAudioSettings(void *owner);
    bool setRate(double rate);
    QImage capture() const;
    bool madvrActive() const;
    void setSubtitle(const QImage &image);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
