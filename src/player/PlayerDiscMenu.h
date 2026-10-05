#pragma once
#include <QString>
#include <QVector>
#include <QPair>
#include <QJsonObject>
#include <memory>
class QWidget;
namespace vsr {
// LibVLC owns the HDMV navigation VM, interactive graphics and disc-selected tracks.
class PlayerDiscMenu final {
public:
    PlayerDiscMenu();
    ~PlayerDiscMenu();
    bool open(const QString &root, void *window);
    void close(QWidget *retiredSurface=nullptr);
    bool active() const;
    QString error() const;
    int state() const;
    void navigate(unsigned command);
    void topMenu();
    void pause(bool paused);
    void seek(qint64 ticks);
    void volume(int value, bool muted);
    void rate(float value);
    qint64 position() const;
    qint64 duration() const;
    QVector<QPair<int,QString>> tracks(bool audio) const;
    void selectTrack(bool audio,int id);
    quint64 presentedFrames() const;
    QJsonObject programme() const;
    bool snapshot(const QString &path) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
