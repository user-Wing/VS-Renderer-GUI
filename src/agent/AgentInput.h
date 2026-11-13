#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <functional>

namespace vsr {
struct AgentInputResult {
    QString displayText, modelText, command, arguments, error;
    QJsonArray references;
};
class AgentInput final {
public:
    using Resolver = std::function<QJsonObject(const QString &, const QString &)>;
    static QJsonArray commands();
    static AgentInputResult parse(const QString &text);
    static AgentInputResult expand(const QString &text, const Resolver &resolver);
    static QString mention(const QString &kind, const QString &value = {});
    static QJsonObject readFile(const QString &path);
};
}
