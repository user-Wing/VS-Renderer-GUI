#pragma once
#include <QJsonArray>
#include <QSet>
#include <QTextBrowser>

namespace vsr {
class AgentMarkdownView final : public QTextBrowser {
    Q_OBJECT
public:
    explicit AgentMarkdownView(QWidget *parent = nullptr);
    void showMessages(const QJsonArray &messages,const QString &streamText,const QJsonArray &pending = {},bool busy = false);
protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    void render(bool followBottom = true);
    QJsonArray messages_,pending_;
    QString streamText_;
    QSet<QString> expanded_;
    bool scheduled_ = false,busy_ = false,selecting_ = false,renderPending_ = false;
};
}
