#pragma once

#include "agent/AgentProvider.h"
#include "agent/AgentContext.h"
#include <QJsonArray>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <functional>

class QNetworkReply;

namespace vsr {

enum class AgentAccessMode { SoftwareOnly, FullEnvironment };

class AgentSession final : public QObject {
    Q_OBJECT
public:
    using Handler = std::function<QJsonObject(const QJsonObject &)>;
    using Completion = std::function<void(QJsonObject)>;
    using AsyncHandler = std::function<void(const QJsonObject &, Completion)>;
    explicit AgentSession(QObject *parent = nullptr);
    ~AgentSession() override;
    void setProvider(const AgentProvider &provider);
    const AgentProvider &provider() const { return provider_; }
    void registerTool(const QString &name, const QString &description, const QJsonObject &parameters, Handler handler);
    void registerAsyncTool(const QString &name, const QString &description, const QJsonObject &parameters, AsyncHandler handler);
    void executeToolAsync(const QString &name, const QJsonObject &arguments, Completion completion);
    void copyToolsTo(AgentSession *target, const QStringList &excluded = {}) const;
    void setToolDelegationAllowed(const QString &name, bool allowed);
    void setSystemPrompt(const QString &prompt);
    QString systemPrompt() const { return systemPrompt_; }
    QJsonObject usage() const;
    void compactContext();
    QJsonArray tools() const;
    QJsonArray allTools() const;
    QJsonObject executeTool(const QString &name, const QJsonObject &arguments);
    void unregisterTool(const QString &name);
    void setToolEnabled(const QString &name, bool enabled);
    bool toolEnabled(const QString &name) const;
    void setToolEnvironmentAccess(const QString &name, bool required = true, bool writes = false);
    void setToolImageInputRequired(const QString &name, bool required = true);
    bool queueToolImageAttachment(const QJsonObject &image, bool softwareFrame = false);
    bool toolRequiresEnvironment(const QString &name) const;
    bool toolWritesEnvironment(const QString &name) const;
    bool toolAvailable(const QString &name) const;
    void setToolPermissionParent(AgentSession *parent);
    void setAccessMode(AgentAccessMode mode);
    AgentAccessMode accessMode() const { return accessMode_; }
    bool setWorkingDirectory(const QString &path, bool fromTool = false);
    QString workingDirectory() const { return workingDirectory_; }
    bool isBusy() const { return busy_; }
    void send(const QString &text);
    void sendWithContext(const QString &displayText, const QString &modelText);
    QString postMessage(const QString &text, const QString &source = {});
    QJsonArray pendingMessages() const { return inbox_; }
    bool pendingPaused() const { return inboxPaused_; }
    void resumePending();
    bool dropPendingMessage(const QString &id);
    QJsonObject takePendingMessage(const QString &id);
    void cancel();
    void clear();
    QJsonArray messages() const { return messages_; }
    bool importConversation(const QJsonObject &document, QString *error = nullptr);
    QJsonObject exportConversation() const;
signals:
    void textDelta(const QString &text);
    void messageAdded(const QJsonObject &message);
    void toolExecuted(const QString &name, const QJsonObject &arguments, const QJsonObject &result);
    void busyChanged(bool busy);
    void errorOccurred(const QString &error);
    void finished();
    void usageChanged(const QJsonObject &usage);
    void toolExecutionCancelled();
    void pendingMessagesChanged(const QJsonArray &messages);
    void accessModeChanged(vsr::AgentAccessMode mode);
    void workingDirectoryChanged(const QString &path);
    void conversationReset();
private:
    struct Tool { QJsonObject definition; Handler handler; AsyncHandler asyncHandler; bool enabled = true; bool delegationAllowed = true; bool requiresEnvironment = false; bool writesEnvironment = false; bool requiresImageInput = false; };
    QString toolAccessError(const QString &name) const;
    void request();
    void consume(bool final = false);
    void parseEvent(const QByteArray &event);
    void completeReply(QNetworkReply *reply);
    void addMessage(const QJsonObject &message);
    void finish(const QString &error = {});
    void processToolCalls(quint64 generation);
    bool beginCompaction(bool resume);
    QString effectivePrompt() const;
    void publishUsage();
    QString queueMessage(const QString &content, const QString &display, const QString &source);
    void injectPending();
    QNetworkAccessManager network_;
    QPointer<QNetworkReply> reply_;
    AgentProvider provider_, turnProvider_;
    QMap<QString, Tool> tools_;
    QJsonArray messages_;
    QJsonArray inbox_;
    bool inboxPaused_ = false;
    AgentContext context_;
    QString systemPrompt_;
    QJsonObject replyUsage_;
    QJsonArray pendingCalls_;
    QJsonArray pendingImages_;
    int nextCall_ = 0, compressionBoundary_ = 0;
    bool compacting_ = false, resumeAfterCompaction_ = false, compressedThisTurn_ = false;
    QByteArray buffer_, response_;
    QString content_, reasoning_, finishReason_, streamError_;
    QMap<int, QJsonObject> calls_;
    bool busy_ = false, sawEvent_ = false, executingTools_ = false, cancelRequested_ = false;
    quint64 generation_ = 0;
    int rounds_ = 0;
    AgentAccessMode accessMode_ = AgentAccessMode::SoftwareOnly;
    QString workingDirectory_;
    QPointer<AgentSession> permissionParent_;
};

}
Q_DECLARE_METATYPE(vsr::AgentAccessMode)
