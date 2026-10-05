#pragma once
#include "bluray/BlurayCatalog.h"
#include <QWidget>
#include <QProcess>
class QTreeWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
namespace vsr {
class BlurayWidget final : public QWidget {
    Q_OBJECT
public:
    explicit BlurayWidget(QWidget *parent = nullptr);
    void open(const QString &path);
signals:
    void playRequested(const QVector<vsr::BlurayTitle> &titles, int index);
private:
    void scan();
    void display();
    QVector<BlurayTitle> selected() const;
    void remux();
    void next();
    void mountImage(const QString &path = {});
    void download();
    QLineEdit *source_, *output_, *tool_;
    QComboBox *mode_;
    QTreeWidget *table_;
    QLabel *status_;
    QPlainTextEdit *log_;
    QPushButton *run_, *stop_;
    BlurayScan scan_;
    QJsonObject profile_;
    QProcess process_;
    QVector<BlurayTitle> queue_;
    QString activeOutput_, queueOutput_, queueTool_;
    int pending_ = 0, scanGeneration_ = 0;
    bool stopped_ = false;
};
}
