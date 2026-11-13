#pragma once

#include <QObject>
#include <QProcess>

namespace vsr {

class AgentNodeRuntime final : public QObject {
    Q_OBJECT
public:
    explicit AgentNodeRuntime(const QString &directory, QObject *parent = nullptr);
    QString directory() const { return directory_; }
    QString nodePath() const;
    QString bridgePath() const;
    bool ready() const;
    bool isInstalling() const { return install_.state() != QProcess::NotRunning; }
    void install(const QString &sourceDirectory = {});
signals:
    void statusChanged(const QString &message);
    void installFinished(bool ok, const QString &message);
private:
    QString directory_;
    QProcess install_;
};

}
