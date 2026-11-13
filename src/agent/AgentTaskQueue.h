#pragma once
#include "agent/AgentProvider.h"
#include "agent/AgentSession.h"
#include <QJsonArray>
#include <QPointer>
#include <memory>

namespace vsr {
class AgentSession;
class AgentTaskQueue final : public QObject {
    Q_OBJECT
public:
    explicit AgentTaskQueue(AgentSession *parentSession, const QString &path, QObject *parent = nullptr);
    ~AgentTaskQueue() override;
    QString enqueue(const QString &name, const QString &prompt, const AgentProvider &provider, bool allowWrites = false, QString *error = nullptr);
    QString enqueue(const QString &name, const QString &prompt, bool allowWrites = false, QString *error = nullptr);
    bool sendMessage(const QString &id, const QString &message, QString *error = nullptr);
    bool resume(const QString &id, const AgentProvider &provider);
    void cancel(const QString &id);
    void cancelAll();
    void clearFinished();
    QJsonArray tasks() const;
    QJsonObject task(const QString &id) const;
    QString summary() const;
    bool load(QString *error = nullptr);
    bool importTasks(const QJsonObject &document, QString *error = nullptr);
    QJsonObject exportTasks() const;
    void registerTools();
signals:
    void changed();
    void sessionCreated(vsr::AgentSession *session, bool allowWrites);
    void taskMessage(const QString &id, const QJsonObject &message);
    void errorOccurred(const QString &error);
private:
    struct Task { QJsonObject data, conversation; QJsonArray inbox; QString streamText; AgentProvider provider; QPointer<AgentSession> session; AgentAccessMode accessMode = AgentAccessMode::SoftwareOnly; QString workingDirectory; };
    void schedule();
    void start(const std::shared_ptr<Task> &task);
    void save();
    bool deliver(const QString &id, const QString &message, const QString &source, QString *error = nullptr);
    void report(const std::shared_ptr<Task> &task);
    void registerCommunicationTools(AgentSession *session, const QString &sender);
    std::shared_ptr<Task> find(const QString &id) const;
    AgentSession *parentSession_;
    QString path_;
    QList<std::shared_ptr<Task>> tasks_;
    bool shuttingDown_ = false;
};
}
