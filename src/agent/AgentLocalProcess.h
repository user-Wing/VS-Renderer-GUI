#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <functional>
#include <memory>

namespace vsr {
class AgentSession;

// One instance belongs to one session. Process handles and handlers must not be
// delegated: child sessions register their own instance and permission boundary.
class AgentLocalProcess final : public QObject {
    Q_OBJECT
public:
    explicit AgentLocalProcess(AgentSession *session);
    ~AgentLocalProcess() override;
    void registerTools();
    QJsonArray processes() const;
    void shutdown();
signals:
    void processesChanged();
private:
    using Completion = std::function<void(QJsonObject)>;
    struct Process;
    void execute(const QJsonObject &arguments, Completion completion);
    void write(const QJsonObject &arguments, Completion completion);
    void terminate(const QJsonObject &arguments, Completion completion);
    void wait(const std::shared_ptr<Process> &process, int milliseconds, int outputLimit, Completion completion);
    void settle(const std::shared_ptr<Process> &process);
    void stop(const std::shared_ptr<Process> &process, const QString &reason);
    void read(const std::shared_ptr<Process> &process);
    QJsonObject result(const std::shared_ptr<Process> &process, int outputLimit);
    void cancelForeground();
    void prune();
    QPointer<AgentSession> session_;
    QMap<qint64, std::shared_ptr<Process>> processes_;
    bool destroying_ = false;
};
}
