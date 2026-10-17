#pragma once
#include <QDialog>
#include <QJsonObject>
#include <QUrl>
#include <functional>
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;
class QNetworkAccessManager;
class QPushButton;
namespace vsr {
class PresetCommunityDialog final : public QDialog {
public:
    PresetCommunityDialog(std::function<void(const QString &,bool)> import,QWidget *parent=nullptr);
    static QString defaultSource();
private:
    void refresh();
    void scanDirectory(const QUrl &url,QTreeWidgetItem *parent=nullptr,int page=1);
    void enqueue(bool all);
    void downloadNext();
    void importSelected(bool currentGroup);
    QString directory() const;
    QString source_;
    QUrl listingUrl_;
    std::function<void(const QString &,bool)> import_;
    QNetworkAccessManager *network_;
    QTreeWidget *files_;
    QLabel *status_;
    QPushButton *refresh_;
    QList<QTreeWidgetItem*> queue_;
    int active_=0;
    int listing_=0;
};
}
