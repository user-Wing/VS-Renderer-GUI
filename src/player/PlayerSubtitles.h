#pragma once
#include <QObject>
#include <QThread>
#include <QImage>
#include <QVariantMap>
#include <atomic>
#include <memory>
namespace vsr {
class PlayerSubtitles final : public QObject {
    Q_OBJECT
public:
    PlayerSubtitles(); ~PlayerSubtitles() override;
    void load(int slot,const QString &path,int stream,const QString &codec,const QVariantMap &style);
    void render(qint64 time,const QSize &canvas,const QSize &video,bool visible);
signals:
    void imageReady(const QImage &image);
    void errorOccurred(const QString &error);
private:
    struct Impl; std::unique_ptr<Impl> impl_; QThread thread_; QObject *worker_;
    std::atomic_bool pending_{false};
};
}
