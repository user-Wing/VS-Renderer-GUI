#include "agent/AgentProvider.h"
#include "agent/AgentModelCapabilities.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QUuid>
#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace vsr {
namespace {
bool fail(QString *error, const QString &text)
{
    if (error) *error = text;
    return false;
}

QByteArray protect(const QByteArray &data, bool encrypt)
{
    if (data.isEmpty()) return {};
#ifdef Q_OS_WIN
    DATA_BLOB input{DWORD(data.size()), reinterpret_cast<BYTE *>(const_cast<char *>(data.constData()))}, output{};
    const bool ok = encrypt ? CryptProtectData(&input, L"VS Renderer Agent", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)
                            : CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output);
    if (!ok) return {};
    const QByteArray result(reinterpret_cast<char *>(output.pbData), output.cbData);
    LocalFree(output.pbData);
    return result;
#else
    Q_UNUSED(encrypt)
    return data;
#endif
}
}

bool AgentProvider::validate(QString *error) const
{
    if (contextLimit < 4096 || contextLimit > 2000000 || maxOutputTokens < 0 || maxOutputTokens > contextLimit)
        return fail(error, QStringLiteral("上下文限制须为4096–2000000，输出限制须介于0和上下文限制之间。"));
    const QUrl url(baseUrl.trimmed());
    if (!url.isValid() || url.host().isEmpty() || (url.scheme() != "https" && url.scheme() != "http")
        || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment())
        return fail(error, QStringLiteral("端点须为不含凭证、查询或片段的 HTTP/HTTPS 地址。"));
    if (model.trimmed().isEmpty()) return fail(error, QStringLiteral("请填写模型 ID。"));
    if (apiKey.contains('\r') || apiKey.contains('\n')) return fail(error, QStringLiteral("API Key 不能包含换行。"));
    if (!reasoningEffort.isEmpty() && !QRegularExpression("^[a-z][a-z0-9_-]{0,31}$").match(reasoningEffort).hasMatch())
        return fail(error, QStringLiteral("推理级别不受支持。"));
    return true;
}

QUrl AgentProvider::completionUrl() const
{
    QUrl url(baseUrl.trimmed());
    QString path = url.path();
    while (path.endsWith('/')) path.chop(1);
    if (path.isEmpty()) path = "/v1";
    if (!path.endsWith("/chat/completions")) path += "/chat/completions";
    url.setPath(path);
    return url;
}

QJsonObject AgentProvider::reasoningParameters(QString *error) const
{
    auto selection = reasoningEffort.isEmpty() ? QStringLiteral("auto") : reasoningEffort;
    auto capability = modelCapability;
    if (capability.isEmpty()) capability = AgentModelCapabilities::conservative(baseUrl, model).toJson();
    if (capability.value("chatToolsRequiresOff").toBool() && selection == "auto") selection = "off";
    return AgentModelCapabilities::requestOptions(baseUrl, model, selection, capability, error);
}

QList<AgentProvider> AgentProviderStore::load(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.exists()) return {};
    if (!file.open(QIODevice::ReadOnly)) { fail(error, QStringLiteral("无法读取 Agent 来源配置。")); return {}; }
    if (file.size() > 4 * 1024 * 1024) { fail(error, QStringLiteral("Agent 来源配置过大。")); return {}; }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject() || document.object().value("schemaVersion").toInt() != 1
        || !document.object().value("providers").isArray()) { fail(error, QStringLiteral("Agent 来源配置格式不正确。")); return {}; }
    QList<AgentProvider> result;
    for (const auto &value : document.object().value("providers").toArray()) {
        const auto item = value.toObject();
        AgentProvider provider{item.value("id").toString(), item.value("name").toString(), item.value("baseUrl").toString(), {}, item.value("model").toString(), item.value("reasoningEffort").toString()};
        provider.contextLimit = item.value("contextLimit").toInt(131072);
        provider.maxOutputTokens = item.value("maxOutputTokens").toInt(0);
        const auto encrypted = item.value("apiKeyProtected").toString().toLatin1();
        if (!encrypted.isEmpty()) {
            const auto plain = protect(QByteArray::fromBase64(encrypted), false);
            if (plain.isEmpty()) { fail(error, QStringLiteral("某个来源的密钥无法解密，请在当前 Windows 用户下重新填写。")); return {}; }
            provider.apiKey = QString::fromUtf8(plain);
        }
        if (!provider.validate(error)) return {};
        if (provider.id.isEmpty()) provider.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        result.append(provider);
    }
    return result;
}

bool AgentProviderStore::save(const QString &path, const QList<AgentProvider> &providers, QString *error)
{
    QJsonArray items;
    for (const auto &provider : providers) {
        if (!provider.validate(error)) return false;
        const auto encrypted = protect(provider.apiKey.toUtf8(), true);
        if (!provider.apiKey.isEmpty() && encrypted.isEmpty()) return fail(error, QStringLiteral("无法保护 Agent API Key，未保存配置。"));
        items.append(QJsonObject{{"id", provider.id}, {"name", provider.name}, {"baseUrl", provider.baseUrl}, {"model", provider.model},
                                 {"reasoningEffort", provider.reasoningEffort}, {"contextLimit", provider.contextLimit},
                                 {"maxOutputTokens", provider.maxOutputTokens}, {"apiKeyProtected", QString::fromLatin1(encrypted.toBase64())}});
    }
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) return fail(error, QStringLiteral("无法创建 Agent 配置目录。"));
    QSaveFile file(path);
    const auto bytes = QJsonDocument(QJsonObject{{"schemaVersion", 1}, {"providers", items}}).toJson();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) return fail(error, QStringLiteral("无法保存 Agent 来源配置。"));
    return true;
}

QList<AgentProvider> AgentProviderStore::importFfui(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 4 * 1024 * 1024) { fail(error, QStringLiteral("无法读取 3FUI 设置文件。")); return {}; }
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (parse.error != QJsonParseError::NoError || !document.isObject()) { fail(error, QStringLiteral("3FUI 设置必须是 JSON 对象。")); return {}; }
    const auto object = document.object();
    AgentProvider provider{QUuid::createUuid().toString(QUuid::WithoutBraces), QStringLiteral("3FUI 导入"), object.value("AgentEndPoint").toString(),
                           object.value("AgentApiKey").toString(), object.value("AgentModelId").toString(), object.value(QStringLiteral("Agent推理级别")).toString()};
    if (!provider.validate(error)) return {};
    provider.name += " · " + provider.model;
    return {provider};
}

}
