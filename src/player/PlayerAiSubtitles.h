#pragma once
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QJsonObject>
class QSettings;
namespace vsr {
class ThreeFpPlayer;
class PlayerAiSubtitles final : public QObject {
    Q_OBJECT
public:
    explicit PlayerAiSubtitles(QObject *parent=nullptr);
    ~PlayerAiSubtitles() override;
    static QString validatePackage(const QString &path,QJsonObject *resolved=nullptr);
    QString start(ThreeFpPlayer *player,const QString &package);
    void stop();
    bool active() const {return timer_.isActive();}
signals:
    void caption(const QString &text);
    void status(const QString &message);
private:
    void pump();
    void receive();
    ThreeFpPlayer *player_=nullptr;
    QProcess worker_;QTimer timer_;QByteArray lines_;
    quint64 generation_=0;bool stamped_=false;
    qint64 captionUntil_=0;
};
}
