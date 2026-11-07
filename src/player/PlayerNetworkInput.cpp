#include "player/PlayerNetworkInput.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QTimer>
#include <QRegularExpression>
#include <QHostInfo>
#include <QUuid>

namespace vsr {
bool PlayerNetworkInput::allowedTarget(const QUrl &target,const QUrl &original){
    if(!target.isValid() || (target.scheme()!="http" && target.scheme()!="https") || target.host().isEmpty())return false;
    if(!target.userInfo().isEmpty() && (target.host().compare(original.host(),Qt::CaseInsensitive)!=0 || target.userInfo()!=original.userInfo()))return false;
    if(original.scheme()=="https" && target.scheme()!="https")return false;
    // A LAN server entered explicitly remains supported, but cannot redirect to another private authority.
    if(target.host().compare(original.host(),Qt::CaseInsensitive)==0 && target.port(target.scheme()=="https"?443:80)==original.port(original.scheme()=="https"?443:80))return true;
    QList<QHostAddress> addresses;QHostAddress literal;
    if(literal.setAddress(target.host()))addresses.append(literal);else addresses=QHostInfo::fromName(target.host()).addresses();
    if(addresses.isEmpty())return false;
    for(const auto &address:addresses){bool ipv4=false;const auto v4=address.toIPv4Address(&ipv4);const auto canonical=ipv4?QHostAddress(v4):address;
        if(canonical.isNull() || canonical.isLoopback() || canonical.isLinkLocal() || canonical.isMulticast())return false;
        for(const auto &subnet:{"0.0.0.0/8","10.0.0.0/8","100.64.0.0/10","127.0.0.0/8","169.254.0.0/16","172.16.0.0/12","192.168.0.0/16","240.0.0.0/4","::/128","fc00::/7","fe80::/10"})if(canonical.isInSubnet(QHostAddress::parseSubnet(subnet)))return false;
    }
    return true;
}
namespace {
void checkRedirects(QNetworkReply *reply,const QUrl &original){
    QObject::connect(reply,&QNetworkReply::redirected,reply,[reply,original](const QUrl &url){if(PlayerNetworkInput::allowedTarget(reply->url().resolved(url),original))reply->redirectAllowed();else reply->abort();});
}
QNetworkRequest request(const QUrl &url) {
    QNetworkRequest result(url);
    result.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::UserVerifiedRedirectPolicy);
    result.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
    result.setTransferTimeout(15000);
    result.setRawHeader("User-Agent", "VS-Player/" VSR_VERSION);
    result.setRawHeader("Accept-Encoding", "identity");
    return result;
}
class RangeConnection final : public QObject {
public:
    RangeConnection(QTcpSocket *socket, const QUrl &url, const QUrl &original, const QByteArray &path, qint64 size, QObject *parent)
        : QObject(parent), socket_(socket), manager_(new QNetworkAccessManager(this)), url_(url), original_(original), path_(path), size_(size) {
        socket->setParent(this);
        connect(socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
        connect(socket, &QTcpSocket::readyRead, this, [this] { readRequest(); });
        connect(socket, &QTcpSocket::bytesWritten, this, [this] { pump(); });
        QTimer::singleShot(0, this, [this] { readRequest(); });
        QTimer::singleShot(15000, this, [this] { if (!reply_) socket_->disconnectFromHost(); });
    }
    ~RangeConnection() override { if (reply_) { reply_->disconnect(this); reply_->abort(); } }
private:
    void readRequest() {
        if (reply_) return;
        header_ += socket_->readAll();
        if (header_.size() > 16384) { socket_->disconnectFromHost(); return; }
        if (!header_.contains("\r\n\r\n")) return;
        const auto lines = header_.split('\n');
        const auto first = lines.first().trimmed().split(' ');
        if (first.size() != 3 || first[1] != path_ || (first[0] != "GET" && first[0] != "HEAD")) {
            socket_->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            socket_->disconnectFromHost(); return;
        }
        if(size_<=0 || !PlayerNetworkInput::allowedTarget(url_,original_)){socket_->disconnectFromHost();return;}
        auto upstream = request(url_);
        for (const auto &line : lines) if (line.toLower().startsWith("range:")) upstream.setRawHeader("Range", line.mid(6).trimmed());
        reply_ = first[0] == "HEAD" ? manager_->head(upstream) : manager_->get(upstream);
        checkRedirects(reply_,original_);
        reply_->setParent(this); reply_->setReadBufferSize(256 * 1024);
        connect(reply_, &QNetworkReply::metaDataChanged, this, [this] { sendHeader(); });
        connect(reply_, &QNetworkReply::readyRead, this, [this] { sendHeader(); pump(); });
        connect(reply_, &QNetworkReply::finished, this, [this] { sendHeader(); pump(); });
    }
    void sendHeader() {
        if (sent_ || !reply_ || socket_->state()!=QAbstractSocket::ConnectedState) return;
        const int status = reply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (status >= 300 && status < 400) return;
        if (!status && !reply_->isFinished()) return;
        QByteArray header = "HTTP/1.1 " + QByteArray::number(status ? status : 502) + " Response\r\nConnection: close\r\n";
        if(reply_->rawHeader("Content-Length").isEmpty()) {
            const auto range=QRegularExpression("bytes (\\d+)-(\\d+)/").match(QString::fromLatin1(reply_->rawHeader("Content-Range")));
            const qint64 length=range.hasMatch()?range.captured(2).toLongLong()-range.captured(1).toLongLong()+1:status==200?size_:0;
            if(length>0)header+="Content-Length: "+QByteArray::number(length)+"\r\n";
        }
        for (const auto &name : {"Content-Length", "Content-Range", "Content-Type", "Accept-Ranges"}) {
            const auto value = reply_->rawHeader(name); if(value.contains('\r') || value.contains('\n')){reply_->abort();socket_->disconnectFromHost();return;} if (!value.isEmpty()) header += QByteArray(name) + ": " + value + "\r\n";
        }
        socket_->write(header + "\r\n"); sent_ = true;
    }
    void pump() {
        if (!reply_ || !sent_ || socket_->state()!=QAbstractSocket::ConnectedState) return;
        while (reply_->bytesAvailable() && socket_->bytesToWrite() < 2 * 1024 * 1024)
            socket_->write(reply_->read(qMin<qint64>(64 * 1024, reply_->bytesAvailable())));
        if (reply_->isFinished() && !reply_->bytesAvailable()) socket_->disconnectFromHost();
    }
    QTcpSocket *socket_;
    QNetworkAccessManager *manager_;
    QNetworkReply *reply_ = nullptr;
    QUrl url_, original_;
    QByteArray path_, header_;
    bool sent_ = false;
    qint64 size_ = 0;
};
}
class NetworkWorker final : public QObject {
    Q_OBJECT
public:
    void cancel() {
        ++generation_; if (resolver_) { resolver_->abort(); resolver_->deleteLater(); resolver_ = nullptr; }
        if (server_) { server_->close(); delete server_; server_ = nullptr; }
    }
    void open(const QUrl &url, quint64 generation) {
        cancel(); generation_ = generation; original_ = url;
        if (!manager_) manager_ = new QNetworkAccessManager(this);
        probe(url, false);
    }
signals:
    void ready(quint64 generation, const QString &url);
    void errorOccurred(quint64 generation, const QString &message);
private:
    void probe(const QUrl &url, bool resolved) {
        const auto generation = generation_;
        if(!PlayerNetworkInput::allowedTarget(url,original_)){emit errorOccurred(generation,tr("拒绝访问非用户指定的本机或私有网络目标。"));return;}
        auto query = request(url); query.setRawHeader("Range", "bytes=0-0");
        auto *reply = manager_->get(query); checkRedirects(reply,original_); resolver_ = reply; reply->setReadBufferSize(4096);
        connect(reply, &QNetworkReply::metaDataChanged, this, [this, reply, generation, resolved] {
            if (generation != generation_ || resolver_ != reply) return;
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status >= 300 && status < 400) return;
            if (status < 200 || status >= 300) return;
            const auto type = reply->rawHeader("Content-Type").toLower();
            if (!resolved && type.contains("text/html")) {
                resolver_ = nullptr; reply->abort(); reply->deleteLater(); resolveAList(); return;
            }
            if (type.contains("text/html") || type.contains("application/json")) {
                resolver_ = nullptr; reply->abort(); reply->deleteLater();
                emit errorOccurred(generation, tr("链接返回网页或 API 数据，未找到视频直链。")); return;
            }
            const auto actual = reply->url();
            const auto range=reply->rawHeader("Content-Range");const qint64 size=range.contains('/')?range.mid(range.lastIndexOf('/')+1).toLongLong():reply->rawHeader("Content-Length").toLongLong();
            resolver_ = nullptr; reply->abort(); reply->deleteLater();
            startProxy(actual, generation, size);
        });
        connect(reply, &QNetworkReply::finished, this, [this, reply, generation] {
            if (generation == generation_ && resolver_ == reply) {
                resolver_ = nullptr;
                emit errorOccurred(generation, tr("读取网络视频失败：%1(HTTP %2)").arg(reply->errorString()).arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()));
                reply->deleteLater();
            }
        });
    }
    void resolveAList() {
        auto api = original_; api.setPath("/api/fs/get"); api.setQuery(QString()); api.setFragment(QString());
        QString path = original_.path(QUrl::FullyDecoded); if (path.startsWith("/d/")) path.remove(0, 2);
        auto query = request(api); query.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        auto *reply = manager_->post(query, QJsonDocument(QJsonObject{{"path", path}, {"password", ""}}).toJson(QJsonDocument::Compact));
        checkRedirects(reply,original_);resolver_ = reply; reply->setReadBufferSize(1024 * 1024); const auto generation = generation_;
        connect(reply, &QNetworkReply::finished, this, [this, reply, generation] {
            if (generation != generation_ || resolver_ != reply) return;
            resolver_ = nullptr; const auto json = QJsonDocument::fromJson(reply->readAll()).object(); reply->deleteLater();
            const QUrl raw(json.value("data").toObject().value("raw_url").toString());
            if (json.value("code").toInt() != 200 || (raw.scheme() != "http" && raw.scheme() != "https")) {
                emit errorOccurred(generation, tr("AList 无法提供视频直链：%1").arg(json.value("message").toString())); return;
            }
            probe(raw, true);
        });
    }
    void startProxy(const QUrl &url, quint64 generation, qint64 size) {
        if(size<=0){emit errorOccurred(generation,tr("网络视频未提供有效长度，无法创建 Range 代理。"));return;}
        server_ = new QTcpServer(this);
        if (!server_->listen(QHostAddress::LocalHost, 0)) { emit errorOccurred(generation, server_->errorString()); return; }
        const auto path = "/" + QUuid::createUuid().toByteArray(QUuid::WithoutBraces) + "/video.mkv";
        connect(server_, &QTcpServer::newConnection, server_, [this, url, path, size] {
            while (server_->hasPendingConnections()) new RangeConnection(server_->nextPendingConnection(), url, original_, path, size, server_);
        });
        emit ready(generation, QString("http://127.0.0.1:%1%2").arg(server_->serverPort()).arg(QString::fromLatin1(path)));
    }
    QNetworkAccessManager *manager_ = nullptr;
    QTcpServer *server_ = nullptr;
    QPointer<QNetworkReply> resolver_;
    QUrl original_;
    quint64 generation_ = 0;

};
PlayerNetworkInput::PlayerNetworkInput(QObject *parent) : QObject(parent), worker_(new NetworkWorker) {
    worker_->moveToThread(&thread_); connect(&thread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(worker_, &NetworkWorker::ready, this, [this](quint64 generation, const QString &url) { if (generation == generation_) emit ready(url); });
    connect(worker_, &NetworkWorker::errorOccurred, this, [this](quint64 generation, const QString &message) { if (generation == generation_) emit errorOccurred(message); });
    thread_.start();
}
PlayerNetworkInput::~PlayerNetworkInput() { cancel(); thread_.quit(); thread_.wait(); }
void PlayerNetworkInput::open(const QUrl &url) { const auto generation = ++generation_; QMetaObject::invokeMethod(worker_, [this, url, generation] { worker_->open(url, generation); }, Qt::QueuedConnection); }
void PlayerNetworkInput::cancel() { ++generation_; QMetaObject::invokeMethod(worker_, [this] { worker_->cancel(); }, Qt::BlockingQueuedConnection); }
}
#include "PlayerNetworkInput.moc"
