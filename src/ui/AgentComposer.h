#pragma once
#include <QJsonArray>
#include <QWidget>

class QHBoxLayout;
class QPlainTextEdit;
class QTextBrowser;
class QListWidget;
class QPushButton;

namespace vsr {
class AgentComposer final : public QWidget {
    Q_OBJECT
public:
    explicit AgentComposer(QWidget *parent = nullptr);
    QPlainTextEdit *editor() const { return editor_; }
    QHBoxLayout *toolbar() const { return toolbar_; }
    QHBoxLayout *modeBar() const { return modeBar_; }
    void setCompletionItems(const QJsonArray &items);
signals:
    void submitted();
    void completionsRequested(const QString &prefix);
    void mentionActionRequested(const QString &action);
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    void updateCompletion();
    void fillCompletion();
    void acceptCompletion();
    void updatePreview();
    QPlainTextEdit *editor_;
    QTextBrowser *preview_;
    QListWidget *popup_;
    QPushButton *previewButton_;
    QHBoxLayout *toolbar_;
    QHBoxLayout *modeBar_;
    QJsonArray completions_;
    QString token_;
    int tokenStart_ = 0;
    bool composing_ = false, committing_ = false;
};
}
