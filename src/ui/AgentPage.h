#pragma once

#include "agent/AgentProvider.h"
#include <QWidget>
#include <memory>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;
class QTimer;

namespace vsr {
class AgentSession;
class AgentToolLibrary;
class AgentComposer;
class AgentMarkdownView;

class AgentPage final : public QWidget {
    Q_OBJECT
public:
    explicit AgentPage(AgentSession *session, QWidget *parent = nullptr);
    ~AgentPage() override;
    const QList<AgentProvider> &providers() const { return providers_; }
    void setQueueSummary(const QString &summary);
    void setCompletionItems(const QJsonArray &items);
    void setModelCapabilities(const QJsonObject &capabilities);
    void insertInputText(const QString &text);
    void showNotice(const QString &text, bool error = false);
    void startNewConversation();
    void openUsage();
    void openTools();
    void openModelMenu();
    void selectModel(const QString &model);
    void selectReasoning(const QString &level);
signals:
    void skillsSettingsRequested();
    void mcpSettingsRequested();
    void queueRequested();
    void providerModelChanged(const vsr::AgentProvider &provider);
    void messageSubmitted(const QString &text);
    void commandSubmitted(const QString &text);
    void completionsRequested(const QString &prefix);
    void mentionActionRequested(const QString &action);
private:
    void refreshProviders(const QString &selected = {});
    void openProviderMenu();
    void selectProviderId(const QString &id);
    void showProviderSettings();
    void showFeatureSettings();
    void showUsage();
    void updateUsage();
    void updateModelButtons();
    void showReasoningMenu();
    void showAccessMenu();
    void chooseWorkingDirectory();
    void updateAccessControls();
    void showPendingMessages();
    void updatePendingMessages();
    bool saveProviders();
    void showTools();
    void refreshConversations();
    void newConversation();
    void saveConversation();
    void loadConversation(const QString &id);
    void importConversation();
    void exportConversation();
    void renderConversation();
    void updateBusy(bool busy);
    void send();
    void report(const QString &message, bool error = false);
    AgentSession *session_;
    std::unique_ptr<AgentToolLibrary> toolLibrary_;
    QList<AgentProvider> providers_;
    QString directory_, conversationId_, conversationTitle_, pendingText_;
    QPushButton *provider_ = nullptr;
    int providerIndex_ = -1;
    QPushButton *modelButton_ = nullptr, *reasoning_ = nullptr, *pending_ = nullptr;
    QListWidget *conversations_ = nullptr;
    AgentMarkdownView *transcript_ = nullptr;
    AgentComposer *composer_ = nullptr;
    QPlainTextEdit *input_ = nullptr;
    QLabel *status_ = nullptr;
    QLabel *chatTitle_ = nullptr;
    QPushButton *send_ = nullptr, *stop_ = nullptr, *settings_ = nullptr, *tools_ = nullptr;
    QPushButton *new_ = nullptr, *remove_ = nullptr, *import_ = nullptr, *export_ = nullptr;
    QPushButton *features_ = nullptr, *queue_ = nullptr, *usage_ = nullptr;
    QPushButton *access_ = nullptr, *workingDirectory_ = nullptr;
    QTimer *renderTimer_ = nullptr;
    bool submissionRejected_ = false;
};
}
