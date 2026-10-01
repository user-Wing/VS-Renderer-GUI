#pragma once
#include <QObject>
#include <QThread>
#include <QUrl>

namespace vsr {
class NetworkWorker;
class PlayerNetworkInput final : public QObject {
    Q_OBJECT
public:
    explicit PlayerNetworkInput(QObject *parent = nullptr);
    ~PlayerNetworkInput() override;
    void open(const QUrl &url);
    void cancel();
signals:
    void ready(const QString &localUrl);
    void errorOccurred(const QString &message);
private:
    QThread thread_;
    NetworkWorker *worker_;
    quint64 generation_ = 0;
};
}
