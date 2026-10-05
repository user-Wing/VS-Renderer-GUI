#pragma once
#include <QWidget>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QUrl>
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;
class QLabel;
class QPushButton;
class QNetworkAccessManager;
namespace vsr {
struct ComponentRelease {QString id,version,path,sha256;QUrl url;qint64 size=0;};
class ComponentDownloads final : public QWidget {
    Q_OBJECT
public:
    explicit ComponentDownloads(QWidget *parent=nullptr);
    ~ComponentDownloads() override;
    static QList<ComponentRelease> releases(const QString &id,const QJsonArray &files);
    static bool newer(const QString &version,const QString &current);
    static QString destination(const QString &id);
    static QString payloadDirectory(const QString &id,const QString &root);
    void refresh();
signals:
    void componentPrepared(const QString &id,const QString &version,const QString &payload);
private:
    void fetch(const QString &id,int page);
    void finishRefresh();
    void downloadNext();
    void verify();
    void unpack();
    void extracted();
    void finishDownload();
    void fail(const QString &message);
    void installComponents();
    void installSoftware();
    QString localVersion(const QString &id) const;
    QString cacheFile(const ComponentRelease &release) const;
    QTreeWidget *table_;QCheckBox *automatic_;QLabel *status_;QPushButton *apply_,*softwareButton_;
    QNetworkAccessManager *network_;QProcess process_;
    QHash<QString,QList<ComponentRelease>> releases_;QHash<QString,QTreeWidgetItem *> groups_;
    QList<ComponentRelease> queue_;ComponentRelease selected_,software_;
    QJsonArray plan_;QString cache_,archive_,staging_;int pending_=0;bool downloading_=false;
};
}
