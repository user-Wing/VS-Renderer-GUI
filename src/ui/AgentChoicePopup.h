#pragma once
#include <QList>
#include <QPushButton>
#include <QWidget>

class QListWidget;
class QKeyEvent;

namespace vsr {
struct AgentChoice {
    QString id, label, description;
};

class AgentChoiceButton final : public QPushButton {
public:
    explicit AgentChoiceButton(QWidget *parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void paintEvent(QPaintEvent *) override;
};

class AgentChoicePopup final : public QWidget {
    Q_OBJECT
public:
    AgentChoicePopup(const QString &title, const QString &description,
                     const QList<AgentChoice> &choices, const QString &selected, QWidget *parent = nullptr);
    void openFor(QWidget *anchor);
signals:
    void chosen(const QString &id);
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *) override;
private:
    void chooseCurrent();
    QListWidget *list_;
    bool chosen_ = false;
};
}
