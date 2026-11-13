#pragma once

#include <QJsonObject>
#include <QMap>
#include <QPointer>

namespace vsr {

class AgentSession;

class AgentToolLibrary final {
public:
    explicit AgentToolLibrary(AgentSession *session);
    ~AgentToolLibrary();
    AgentToolLibrary(const AgentToolLibrary &) = delete;
    AgentToolLibrary &operator=(const AgentToolLibrary &) = delete;

    bool importTools(const QJsonObject &document, QString *error = nullptr);
    QJsonObject exportTools() const;

private:
    QJsonObject execute(const QJsonObject &definition, const QJsonObject &arguments);
    QPointer<AgentSession> session_;
    QMap<QString, QJsonObject> definitions_;
};

}
