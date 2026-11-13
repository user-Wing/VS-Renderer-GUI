#pragma once
#include <QObject>
#include <QPointer>
#include <atomic>
#include <memory>

namespace vsr {
class AgentSession;
// One instance per Session; filesystem work runs outside the GUI thread.
class AgentLocalFiles final : public QObject {
public:
    explicit AgentLocalFiles(const QString &workingDirectory = {},QObject *parent = nullptr);
    ~AgentLocalFiles() override;
    bool setWorkingDirectory(const QString &path,QString *error = nullptr);
    QString workingDirectory() const;
    void registerTools(AgentSession *session);
private:
    void cancelJobs();
    QString directory_;
    QPointer<AgentSession> session_;
    QList<std::weak_ptr<std::atomic_bool>> jobs_;
};
}
