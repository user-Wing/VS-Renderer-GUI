#pragma once
#include <QObject>
#include <QImage>
#include <QThread>
#include <atomic>

namespace vsr {
class PlayerImage final : public QObject {
    Q_OBJECT
public:
    explicit PlayerImage(QObject *parent = nullptr);
    ~PlayerImage() override;
    static bool supports(const QString &path);
    void open(const QString &path);
    void cancel();
signals:
    void loaded(const QImage &image);
    void failed(const QString &error);
private:
    QThread thread_;
    QObject *worker_;
    std::atomic<quint64> generation_{0};
};
}
