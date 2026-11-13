#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QUrl>

namespace vsr {

struct AgentProvider {
    QString id, name, baseUrl, apiKey, model;
    QString reasoningEffort;
    int contextLimit = 131072;
    int maxOutputTokens = 0;
    QJsonObject modelCapability;
    bool validate(QString *error = nullptr) const;
    QUrl completionUrl() const;
    QJsonObject reasoningParameters(QString *error = nullptr) const;
};

class AgentProviderStore final {
public:
    static QList<AgentProvider> load(const QString &path, QString *error = nullptr);
    static bool save(const QString &path, const QList<AgentProvider> &providers, QString *error = nullptr);
    static QList<AgentProvider> importFfui(const QString &path, QString *error = nullptr);
};

}
