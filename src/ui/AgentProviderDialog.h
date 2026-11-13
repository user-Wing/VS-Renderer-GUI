#pragma once

#include "agent/AgentProvider.h"
#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;

namespace vsr {
class AgentProviderDialog final : public QDialog {
    Q_OBJECT
public:
    AgentProviderDialog(QList<AgentProvider> providers, QString configPath,
                        const QString &selectedId = {}, QWidget *parent = nullptr);
signals:
    void applied(const QList<AgentProvider> &providers, const QString &selectedId);
private:
    void storeForm();
    void selectProvider(int row);
    void refreshList(const QString &selectedId);
    void importProviders(bool ffui);
    void exportProviders();
    bool apply();
    QList<AgentProvider> providers_;
    QString path_;
    int active_ = -1;
    QListWidget *list_;
    QWidget *form_;
    QLineEdit *name_, *endpoint_, *key_, *model_;
    QComboBox *reasoning_;
    QSpinBox *context_, *output_;
    QLabel *status_;
};
}
