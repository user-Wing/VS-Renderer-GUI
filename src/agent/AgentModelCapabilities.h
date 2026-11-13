#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QStringList>

namespace vsr {

struct AgentModelCapability {
    QStringList levels, models;
    QString endpoint, model, defaultLevel, source = QStringLiteral("unknown"), status = QStringLiteral("unknown"), label;
    QJsonObject optionsByLevel;
    bool supportsOff = false, chatToolsSupported = true, chatToolsRequiresOff = false, imageInputSupported = false;
    QString imageInputSource = QStringLiteral("unknown");
    qint64 contextWindow = 0, maxOutputTokens = 0;
    QJsonObject toJson() const;
    static AgentModelCapability fromJson(const QJsonObject &object);
    QJsonObject requestOptions(const QString &selection, QString *error = nullptr) const;
};

class AgentModelCapabilities final : public QObject {
    Q_OBJECT
public:
    explicit AgentModelCapabilities(QObject *parent = nullptr);
    ~AgentModelCapabilities() override;
    void resolve(const QString &baseUrl, const QString &apiKey, const QString &model, bool refresh = false);
    QJsonObject cached(const QString &baseUrl, const QString &model) const;
    void clearCache();
    static AgentModelCapability conservative(const QString &baseUrl, const QString &model);
    static AgentModelCapability fromMetadata(const QString &baseUrl, const QString &model, const QJsonObject &metadata);
    static QJsonObject requestOptions(const QString &baseUrl, const QString &model, const QString &selection,
                                      const QJsonObject &capability, QString *error = nullptr);
signals:
    void resolved(const QString &baseUrl, const QString &model, const QJsonObject &capability);
private:
    struct Cached { QJsonObject capability; QDateTime updated; };
    void publish(const QString &baseUrl, const QString &model, AgentModelCapability capability);
    QNetworkAccessManager network_;
    QMap<QString, Cached> cache_;
    QMap<QString, quint64> generations_;
};

}
