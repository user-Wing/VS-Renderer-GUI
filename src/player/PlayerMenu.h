#pragma once
#include <QMenu>
#include <QElapsedTimer>
#include <QTimer>
#include <QPointer>

namespace vsr {
class PlayerMenu final : public QMenu {
public:
    explicit PlayerMenu(QWidget *parent);
    static QMenu *add(QMenu *parent, const QString &title);
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
private:
    void showDescription();
    void clearDescription();
    QTimer descriptionTimer_;
    QPointer<QAction> descriptionAction_;
    QTimer animation_;
    QElapsedTimer elapsed_;
    qreal reveal_ = 1;
};
}
