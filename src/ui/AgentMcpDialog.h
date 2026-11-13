#pragma once

#include <QDialog>

class QListWidget;
class QLabel;
class QPushButton;

namespace vsr {
class AgentMcpManager;
class AgentMcpDialog final : public QDialog {
    Q_OBJECT
public:
    explicit AgentMcpDialog(AgentMcpManager *manager, QWidget *parent = nullptr);
private:
    void refresh();
    void edit(bool create);
    QString selectedId() const;
    AgentMcpManager *manager_;
    QListWidget *servers_, *tools_;
    QLabel *runtimeStatus_, *serverStatus_;
    QPushButton *install_;
};
}
