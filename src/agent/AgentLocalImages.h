#pragma once
#include <QObject>
#include <QPointer>
#include <atomic>
#include <memory>

namespace vsr {
class AgentSession;
// One Session owns one instance; only metadata is returned through tool JSON.
class AgentLocalImages final : public QObject {
public:
    explicit AgentLocalImages(AgentSession *session);
    ~AgentLocalImages() override;
    void registerTools();
private:
    void cancelJobs();
    QPointer<AgentSession> session_;
    QList<std::weak_ptr<std::atomic_bool>> jobs_;
    bool registered_ = false;
};
}
