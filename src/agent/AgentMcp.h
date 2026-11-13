#pragma once

#include "agent/AgentNodeRuntime.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QPointer>
#include <QProcess>
#include <functional>

namespace vsr {
class AgentSession;

struct AgentMcpConfig {
    QString id, name, transport = QStringLiteral("stdio"), command, workingDirectory, url;
    QStringList arguments;
    QJsonObject environment, headers;
    bool enabled = false;
    bool validate(QString *error = nullptr) const;
    QJsonObject toJson(bool includeSecrets = false) const;
    static AgentMcpConfig fromJson(const QJsonObject &object, const QString &id = {});
};

class AgentMcpManager final : public QObject {
    Q_OBJECT
public:
    using Completion = std::function<void(QJsonObject)>;
    explicit AgentMcpManager(const QString &configPath, const QString &runtimeDirectory, QObject *parent = nullptr);
    ~AgentMcpManager() override;
    AgentNodeRuntime *runtime() { return &runtime_; }
    QList<AgentMcpConfig> servers() const { return configs_; }
    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr) const;
    bool setServers(const QList<AgentMcpConfig> &servers, QString *error = nullptr);
    bool importConfiguration(const QString &path, QString *error = nullptr);
    bool exportConfiguration(const QString &path, QString *error = nullptr) const;
    void startEnabled();
    void connectServer(const QString &id);
    void disconnectServer(const QString &id);
    void refreshTools(const QString &id);
    QString state(const QString &id) const;
    QJsonArray serverTools(const QString &id) const;
    void callTool(const QString &id, const QString &name, const QJsonObject &arguments, Completion completion, QObject *owner = nullptr);
    void registerTools(AgentSession *session);
signals:
    void serverChanged(const QString &id);
    void toolsChanged();
    void errorOccurred(const QString &message);
private:
    struct Pending { Completion callback; QPointer<QObject> owner; QObject *ownerIdentity = nullptr; bool owned = false; QString server; };
    void ensureBridge();
    quint64 request(const QJsonObject &body, Completion completion, QObject *owner = nullptr);
    void readBridge();
    void complete(quint64 id, const QJsonObject &result);
    void syncTools();
    void cancelOwner(QObject *owner);
    QString configPath_;
    AgentNodeRuntime runtime_;
    QProcess bridge_;
    QByteArray buffer_;
    QList<AgentMcpConfig> configs_;
    QMap<QString, QString> states_;
    QMap<QString, quint64> serverGenerations_;
    QMap<QString, QJsonArray> tools_;
    QMap<quint64, Pending> pending_;
    QList<QPointer<AgentSession>> sessions_;
    QMap<AgentSession *, QStringList> registered_;
    QMap<AgentSession *, QMap<QString,bool>> toolPreferences_;
    void *processJob_ = nullptr;
    quint64 nextId_ = 0;
};

}
