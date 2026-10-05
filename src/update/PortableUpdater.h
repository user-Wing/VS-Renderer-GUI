#pragma once
#include <QDialog>
#include <QJsonArray>
#include <QUrl>
#include <memory>
class QLabel;
class QPushButton;
class QProgressBar;
class QNetworkAccessManager;
class QNetworkReply;
class QProcess;
class QTemporaryDir;
class QFile;
class QCryptographicHash;
namespace vsr {
struct PortableRelease {
    QString version, sha256;
    QUrl url;
    qint64 size = 0;
};
class PortableUpdater final : public QDialog {
    Q_OBJECT
public:
    explicit PortableUpdater(QWidget *parent = nullptr);
    ~PortableUpdater() override;
    static QList<PortableRelease> releases(const QJsonArray &files);
    static bool newer(const QString &version, const QString &current);
    static bool safeArchiveListing(const QString &listing);
    static QString payloadDirectory(const QString &directory);
    void check();
    void prepare(const PortableRelease &release);
    void prepareLocal(const PortableRelease &release,const QString &archive);
signals:
    void prepared(const QString &payload);
    void failed(const QString &message);
protected:
    void reject() override;
private:
    void fetchPage(int page);
    void finishCheck();
    void fail(const QString &message);
    void verifyChunk();
    void listArchive();
    void extractArchive();
    void verifyPayload();
    void install();
    QLabel *status_;
    QPushButton *action_;
    QProgressBar *progress_;
    QNetworkAccessManager *network_;
    QNetworkReply *reply_ = nullptr;
    QProcess *process_;
    std::unique_ptr<QTemporaryDir> work_;
    std::unique_ptr<QFile> archive_;
    std::unique_ptr<QCryptographicHash> hash_;
    QList<PortableRelease> candidates_;
    PortableRelease selected_;
    QString payload_;
    bool busy_ = false, cancelled_ = false;
};
}
