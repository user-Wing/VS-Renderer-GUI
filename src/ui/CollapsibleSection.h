#pragma once

#include <QWidget>

class QToolButton;

namespace vsr {

class CollapsibleSection final : public QWidget {
    Q_OBJECT
public:
    CollapsibleSection(const QString &title, QWidget *content, QWidget *parent = nullptr);
    bool isExpanded() const;
    void setExpanded(bool expanded);

signals:
    void expandedChanged(bool expanded);

private:
    QToolButton *header_ = nullptr;
    QWidget *content_ = nullptr;
};

}
