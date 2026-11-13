#include "agent/AgentModelCapabilities.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

namespace vsr {
namespace {
QUrl apiUrl(const QString &baseUrl, const QString &resource)
{
    QUrl url(baseUrl.trimmed());
    if (!url.isValid() || url.host().isEmpty() || (url.scheme() != "http" && url.scheme() != "https") ||
        !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment()) return {};
    auto path = url.path();
    while (path.endsWith('/')) path.chop(1);
    if (path.endsWith("/chat/completions")) path.chop(QString("/chat/completions").size());
    else if (path.isEmpty()) path = "/v1";
    url.setPath(path + '/' + resource);
    return url;
}

QString key(const QString &baseUrl, const QString &model) { return apiUrl(baseUrl, "models").toString() + '\n' + model; }

void imageCapability(AgentModelCapability &result,const QJsonObject &metadata)
{
    bool declared=false,supported=false,denied=false;
    const auto flag=[&](const QJsonValue &value){if(!value.isBool())return;declared=true;if(value.toBool())supported=true;else denied=true;};
    const auto modalities=[&](const QJsonValue &value){if(!value.isArray())return;const auto values=value.toArray();for(const auto &item:values)if(!item.isString())return;declared=true;bool image=false;for(const auto &item:values)image|=item.toString().compare("image",Qt::CaseInsensitive)==0;if(image)supported=true;else denied=true;};
    for(const auto &scope:{metadata,metadata.value("capabilities").toObject(),metadata.value("architecture").toObject()}){
        modalities(scope.value("input_modalities"));modalities(scope.value("inputModalities"));modalities(scope.value("modalities").toObject().value("input"));
        for(const auto *name:{"vision","image_input","imageInput","supports_image_input","imageInputSupported"})flag(scope.value(name));
    }
    if(metadata.value("capabilities").isArray()){
        const auto values=metadata.value("capabilities").toArray();bool vision=false,valid=true;for(const auto &item:values){valid&=item.isString();vision|=item.toString()=="vision";}
        if(valid){declared=true;if(vision)supported=true;else denied=true;}
    }
    if(declared){result.imageInputSupported=supported && !denied;result.imageInputSource="metadata";}
}

bool levelName(const QString &name)
{
    static const QRegularExpression valid("^[a-z][a-z0-9_-]{0,31}$");
    return valid.match(name).hasMatch() && name != "auto" && name != "default";
}

QString normalizedLevel(const QString &level) { return level == "none" ? "off" : level; }

QString levelLabel(const QString &level)
{
    static const QMap<QString, QString> labels{{"auto", QStringLiteral("自动（端点默认）")}, {"off", QStringLiteral("关闭")},
        {"on", QStringLiteral("开启")}, {"minimal", QStringLiteral("最少")}, {"low", QStringLiteral("低")},
        {"medium", QStringLiteral("中")}, {"high", QStringLiteral("高")}, {"xhigh", QStringLiteral("极高")}, {"max", QStringLiteral("最高")}};
    return labels.value(level, level);
}

void addLevels(AgentModelCapability &result, const QStringList &levels, bool deepseek)
{
    result.levels.clear(); result.optionsByLevel = {{"auto", QJsonObject{}}};
    for (const auto &wire : levels) {
        if (!levelName(wire)) continue;
        const auto level = normalizedLevel(wire);
        if (level == "off") { result.supportsOff = true; continue; }
        if (result.levels.contains(level)) continue;
        result.levels.append(level);
        QJsonObject fields{{"reasoning_effort", wire}};
        if (deepseek) fields.insert("thinking", QJsonObject{{"type", "enabled"}});
        result.optionsByLevel.insert(level, fields);
    }
    if (result.supportsOff) result.optionsByLevel.insert("off", deepseek ? QJsonObject{{"thinking", QJsonObject{{"type", "disabled"}}}} :
        QJsonObject{{"reasoning_effort", levels.contains("off") && !levels.contains("none") ? "off" : "none"}});
}

QJsonObject safeFields(const QJsonObject &object)
{
    QJsonObject result;
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (it.key() == "reasoning_effort" && it.value().isString() && levelName(it.value().toString())) result.insert(it.key(), it.value());
        else if (it.key() == "thinking" && it.value().isObject()) {
            const auto thinking = it.value().toObject(); const auto type = thinking.value("type").toString();
            if (thinking.size() != 1 || (type != "enabled" && type != "disabled")) return {};
            result.insert(it.key(), thinking);
        } else return {};
    }
    if (result.value("thinking").toObject().value("type") == QJsonValue("disabled") && result.contains("reasoning_effort")) return {};
    if (result.value("thinking").toObject().value("type") == QJsonValue("enabled") &&
        (result.value("reasoning_effort") == QJsonValue("none") || result.value("reasoning_effort") == QJsonValue("off"))) return {};
    return result;
}

QJsonObject jsonReply(QNetworkReply *reply, bool *ok)
{
    const auto bytes = reply->readAll();
    QJsonParseError parse;
    const auto document = QJsonDocument::fromJson(bytes, &parse);
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    *ok = reply->error() == QNetworkReply::NoError && status >= 200 && status < 300 && bytes.size() <= 4 * 1024 * 1024 &&
        parse.error == QJsonParseError::NoError && document.isObject();
    return *ok ? document.object() : QJsonObject{};
}

void boundReply(QNetworkReply *reply)
{
    QObject::connect(reply, &QNetworkReply::readyRead, reply, [reply] { if (reply->bytesAvailable() > 4 * 1024 * 1024) reply->abort(); });
    QTimer::singleShot(20000, reply, [reply] { if (!reply->isFinished()) reply->abort(); });
}

QNetworkRequest metadataRequest(const QUrl &url, const QString &apiKey)
{
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    if (!apiKey.isEmpty()) request.setRawHeader("Authorization", "Bearer " + apiKey.toUtf8());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(15000);
    return request;
}
}

QJsonObject AgentModelCapability::toJson() const
{
    QJsonArray choices{QJsonObject{{"id", "auto"}, {"label", levelLabel("auto")}}};
    if (supportsOff) choices.append(QJsonObject{{"id", "off"}, {"label", levelLabel("off")}});
    for (const auto &level : levels) choices.append(QJsonObject{{"id", level}, {"label", levelLabel(level)}});
    QJsonObject result{{"endpoint", endpoint}, {"model", model}, {"models", QJsonArray::fromStringList(models)},
        {"levels", QJsonArray::fromStringList(levels)}, {"reasoningLevels", choices}, {"optionsByLevel", optionsByLevel},
        {"defaultLevel", defaultLevel}, {"source", source}, {"status", status}, {"label", label}, {"supportsOff", supportsOff},
        {"chatToolsSupported", chatToolsSupported}, {"chatToolsRequiresOff", chatToolsRequiresOff},
        {"imageInputSupported", imageInputSupported}, {"imageInputSource", imageInputSource}};
    if (contextWindow > 0) result.insert("contextWindow", contextWindow);
    if (maxOutputTokens > 0) result.insert("maxOutputTokens", maxOutputTokens);
    return result;
}

AgentModelCapability AgentModelCapability::fromJson(const QJsonObject &object)
{
    AgentModelCapability result;
    result.endpoint = object.value("endpoint").toString(); result.model = object.value("model").toString();
    result.defaultLevel = object.value("defaultLevel").toString(); result.source = object.value("source").toString("unknown");
    result.status = object.value("status").toString("unknown"); result.label = object.value("label").toString();
    result.chatToolsSupported = object.value("chatToolsSupported").toBool(true); result.chatToolsRequiresOff = object.value("chatToolsRequiresOff").toBool();
    result.imageInputSupported = object.value("imageInputSupported").toBool(); result.imageInputSource = object.value("imageInputSource").toString("unknown");
    result.contextWindow = object.value("contextWindow").toInteger(); result.maxOutputTokens = object.value("maxOutputTokens").toInteger();
    for (const auto &value : object.value("models").toArray()) if (value.isString()) result.models.append(value.toString());
    const auto options = object.value("optionsByLevel").toObject();
    result.optionsByLevel.insert("auto", QJsonObject{});
    for (const auto &value : object.value("levels").toArray()) {
        const auto level = value.toString(); const auto fields = safeFields(options.value(level).toObject());
        const auto effort = fields.value("reasoning_effort").toString();
        if (levelName(level) && level != "off" && !fields.isEmpty() && (effort == level || (level == "on" && effort == "high")) && !result.levels.contains(level)) { result.levels.append(level); result.optionsByLevel.insert(level, fields); }
    }
    const auto off = safeFields(options.value("off").toObject());
    result.supportsOff = object.value("supportsOff").toBool() && (off.value("thinking").toObject().value("type") == QJsonValue("disabled") ||
        off.value("reasoning_effort") == QJsonValue("none") || off.value("reasoning_effort") == QJsonValue("off"));
    if (result.supportsOff) result.optionsByLevel.insert("off", off);
    return result;
}

QJsonObject AgentModelCapability::requestOptions(const QString &selection, QString *error) const
{
    if (error) error->clear();
    const auto selected = selection.trimmed();
    if (selected.isEmpty() || selected == "auto" || selected == "default") return {};
    const auto level = normalizedLevel(selected);
    if ((level == "off" && !supportsOff) || (level != "off" && !levels.contains(level))) {
        if (error) *error = QStringLiteral("当前端点与模型未声明支持思考选项“%1”，请选择自动或已列出的级别。").arg(selected);
        return {};
    }
    const auto fields = safeFields(optionsByLevel.value(level).toObject());
    if (fields.isEmpty() && error) *error = QStringLiteral("该思考选项缺少有效的请求参数，请刷新模型能力。");
    return fields;
}

AgentModelCapabilities::AgentModelCapabilities(QObject *parent) : QObject(parent), network_(this) {}

AgentModelCapabilities::~AgentModelCapabilities()
{
    // Replies must stop while cache_ and generations_ still exist.
    for (auto *reply : network_.findChildren<QNetworkReply *>()) { reply->disconnect(this); reply->abort(); }
}

AgentModelCapability AgentModelCapabilities::conservative(const QString &baseUrl, const QString &model)
{
    AgentModelCapability result; result.endpoint = baseUrl; result.model = model;
    result.optionsByLevel = {{"auto", QJsonObject{}}}; result.label = QStringLiteral("未获得思考能力元数据，使用端点默认");
    if (apiUrl(baseUrl, "models").isEmpty()) return result;
    const auto host = QUrl(baseUrl).host().toLower();
    if (host == "api.deepseek.com" && (model == "deepseek-flash" || model == "deepseek-v4-pro")) {
        // https://api-docs.deepseek.com/guides/thinking_mode/ (2026-11-08)
        result.supportsOff = true; result.defaultLevel = "high"; addLevels(result, {"low", "high", "max"}, true);
    } else if (host == "api.openai.com") {
        // https://developers.openai.com/api/docs/guides/latest-model and model pages.
        const auto matches = [&model](const QString &pattern) { return QRegularExpression(pattern + "(?:-[0-9]{4}-[0-9]{2}-[0-9]{2})?$").match(model).hasMatch(); };
        QStringList levels;
        if (matches("^gpt-5(?:-mini|-nano)?")) { levels = {"minimal", "low", "medium", "high"}; result.defaultLevel = "medium"; }
        else if (matches("^gpt-5\\.1")) { levels = {"none", "low", "medium", "high"}; result.defaultLevel = "off"; }
        else if (matches("^gpt-5\\.(?:2|4)")) { levels = {"none", "low", "medium", "high", "xhigh"}; result.defaultLevel = "off"; }
        else if (matches("^gpt-5\\.5")) { levels = {"none", "low", "medium", "high", "xhigh"}; result.defaultLevel = "medium"; }
        else if (matches("^gpt-5\\.6(?:-sol|-terra|-luna)?")) { levels = {"none", "low", "medium", "high", "xhigh", "max"}; result.defaultLevel = "medium"; }
        else if (matches("^gpt-(?:6-astra|6\\.1-sol)")) { levels = {"low", "medium", "high", "xhigh", "max"}; result.defaultLevel = "medium"; result.chatToolsSupported = false; }
        else if (matches("^gpt-6\\.1-terra")) { levels = {"none", "low", "medium", "high", "xhigh", "max"}; result.defaultLevel = "medium"; result.chatToolsSupported = false; }
        else if (matches("^gpt-6-(?:sol|luna)")) { levels = {"none", "low", "medium", "high", "xhigh", "max"}; result.defaultLevel = "medium"; result.chatToolsRequiresOff = true; }
        if (!levels.isEmpty()) addLevels(result, levels, false);
    }
    if (!result.levels.isEmpty() || result.supportsOff) { result.source = "official-rule"; result.status = "ready"; result.label = QStringLiteral("按官方端点与模型规则匹配"); }
    if (!result.chatToolsSupported) result.label += QStringLiteral("；该模型工具调用需要 Responses API");
    else if (result.chatToolsRequiresOff) result.label += QStringLiteral("；Chat Completions 工具调用需要关闭思考");
    return result;
}

AgentModelCapability AgentModelCapabilities::fromMetadata(const QString &baseUrl, const QString &model, const QJsonObject &metadata)
{
    auto result = conservative(baseUrl, model);
    if (metadata.value("id").isString() && metadata.value("id").toString() != model) return result;
    imageCapability(result,metadata);
    const bool deepseek = QUrl(baseUrl).host().compare("api.deepseek.com", Qt::CaseInsensitive) == 0;
    QList<QJsonObject> scopes{metadata, metadata.value("capabilities").toObject()};
    for (const auto *container : {"effort", "reasoning", "reasoning_effort", "thinking"}) {
        const auto initial = scopes;
        for (const auto &scope : initial) if (scope.value(container).isObject()) scopes.append(scope.value(container).toObject());
    }
    QJsonArray found; bool declared = false, ollama = false; QJsonValue defaultValue;
    for (const auto &scope : scopes) {
        for (const auto *name : {"supported_levels", "supported_efforts", "reasoning_efforts", "supported_reasoning_efforts", "reasoning_effort", "efforts", "levels", "enum"}) {
            if (!scope.value(name).isArray()) continue;
            found = scope.value(name).toArray(); declared = true; break;
        }
        if (declared) {
            for (const auto *name : {"default_level", "default_effort", "default"}) if (!scope.value(name).isUndefined()) { defaultValue = scope.value(name); break; }
            break;
        }
    }
    const auto thinking = metadata.value("thinking").toObject();
    if (!declared && thinking.value("values").isArray()) { found = thinking.value("values").toArray(); defaultValue = thinking.value("default"); declared = ollama = true; result.supportsOff = false; }
    if (declared) {
        if (!deepseek) result.supportsOff = false;
        QStringList levels;
        for (const auto &value : found) {
            if (value.isString() && levelName(value.toString())) levels.append(value.toString());
            else if (ollama && value.isBool()) { if (!value.toBool()) result.supportsOff = true; else levels.append("on"); }
        }
        addLevels(result, levels, deepseek);
        if (ollama && result.levels.contains("on")) result.optionsByLevel.insert("on", QJsonObject{{"reasoning_effort", "high"}});
        if (defaultValue.isString()) result.defaultLevel = normalizedLevel(defaultValue.toString());
        else if (ollama && defaultValue.isBool()) result.defaultLevel = defaultValue.toBool() ? "on" : "off";
        else result.defaultLevel.clear();
        if (!result.levels.contains(result.defaultLevel) && !(result.defaultLevel == "off" && result.supportsOff)) result.defaultLevel.clear();
        result.source = "metadata"; result.status = "ready"; result.label = ollama ? QStringLiteral("Ollama /api/show 思考元数据") : QStringLiteral("API 模型思考元数据");
    } else if ((metadata.value("reasoning").isBool() && !metadata.value("reasoning").toBool()) ||
        (metadata.value("capabilities").toObject().value("reasoning").isBool() && !metadata.value("capabilities").toObject().value("reasoning").toBool())) {
        result.levels.clear(); result.supportsOff = false; result.optionsByLevel = {{"auto", QJsonObject{}}}; result.defaultLevel.clear();
        result.source = "metadata"; result.status = "ready"; result.label = QStringLiteral("API 声明该模型不提供思考控制");
    }
    result.contextWindow = metadata.value("context_window").toInteger(metadata.value("context_window_tokens").toInteger());
    result.maxOutputTokens = metadata.value("max_output_tokens").toInteger();
    const auto chat = metadata.value("api_capabilities").toObject().value("chat_completions").toObject();
    if (chat.value("tools").isBool()) { result.chatToolsSupported = chat.value("tools").toBool(); result.chatToolsRequiresOff = chat.value("tools_require_reasoning_off").toBool(); }
    return result;
}

QJsonObject AgentModelCapabilities::requestOptions(const QString &baseUrl, const QString &model, const QString &selection, const QJsonObject &capability, QString *error)
{
    const auto matches = capability.value("model").toString() == model && key(capability.value("endpoint").toString(), model) == key(baseUrl, model);
    return (matches ? AgentModelCapability::fromJson(capability) : conservative(baseUrl, model)).requestOptions(selection, error);
}

QJsonObject AgentModelCapabilities::cached(const QString &baseUrl, const QString &model) const
{
    const auto it = cache_.constFind(key(baseUrl, model));
    return it == cache_.constEnd() ? conservative(baseUrl, model).toJson() : it->capability;
}

void AgentModelCapabilities::clearCache() { cache_.clear(); for (auto it = generations_.begin(); it != generations_.end(); ++it) ++it.value(); }

void AgentModelCapabilities::publish(const QString &baseUrl, const QString &model, AgentModelCapability capability)
{
    const auto json = capability.toJson();
    if (cache_.size() >= 128) cache_.erase(cache_.begin());
    cache_.insert(key(baseUrl, model), {json, QDateTime::currentDateTimeUtc()});
    emit resolved(baseUrl, model, json);
}

void AgentModelCapabilities::resolve(const QString &baseUrl, const QString &apiKey, const QString &model, bool refresh)
{
    const auto identity = key(baseUrl, model); const auto generation = ++generations_[identity];
    const auto previous = cache_.value(identity);
    const int lifetime = previous.capability.value("status") == QJsonValue("error") ? 15000 : 300000;
    if (!refresh && previous.updated.isValid() && previous.updated.msecsTo(QDateTime::currentDateTimeUtc()) < lifetime) {
        QTimer::singleShot(0, this, [this, baseUrl, model, identity, generation, previous] { if (generations_.value(identity) == generation) emit resolved(baseUrl, model, previous.capability); }); return;
    }
    const auto url = apiUrl(baseUrl, "models");
    if (url.isEmpty() || model.isEmpty() || apiKey.contains('\r') || apiKey.contains('\n')) {
        auto result = conservative(baseUrl, model); result.status = "error"; result.label = QStringLiteral("端点或模型配置无效，未发送元数据请求");
        QTimer::singleShot(0, this, [this, baseUrl, model, identity, generation, result] {
            if (generations_.value(identity) == generation) publish(baseUrl, model, result);
        }); return;
    }
    auto *reply = network_.get(metadataRequest(url, apiKey)); boundReply(reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply, baseUrl, apiKey, model, identity, generation, previous, url] {
        const auto server = reply->rawHeader("Server").toLower();
        bool ok = false; const auto document = jsonReply(reply, &ok); reply->deleteLater();
        if (generations_.value(identity) != generation) return;
        auto result = conservative(baseUrl, model); QJsonObject metadata;
        const auto models = document.value("data").toArray();
        for (const auto &value : models) {
            const auto entry = value.toObject(); const auto id = entry.value("id").toString();
            if (!id.isEmpty() && !result.models.contains(id)) result.models.append(id);
            if (id == model) metadata = entry;
        }
        const auto ids = result.models;
        if (!metadata.isEmpty()) { result = fromMetadata(baseUrl, model, metadata); result.models = ids; }
        const auto host = url.host().toLower();
        const bool ollama = host == "ollama.com" || ((host == "localhost" || host == "127.0.0.1" || host == "::1") && url.port() == 11434) || server.contains("ollama");
        if (ollama && (result.source != "metadata" || result.imageInputSource == "unknown")) {
            auto showUrl = url; auto path = showUrl.path(); path.chop(QString("/models").size()); if (path.endsWith("/v1")) path.chop(3); showUrl.setPath(path + "/api/show");
            auto request = metadataRequest(showUrl, apiKey); request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
            auto *show = network_.post(request, QJsonDocument(QJsonObject{{"model", model}}).toJson(QJsonDocument::Compact)); boundReply(show);
            connect(show, &QNetworkReply::finished, this, [this, show, baseUrl, model, identity, generation, result] {
                bool valid = false; const auto metadata = jsonReply(show, &valid); show->deleteLater();
                if (generations_.value(identity) != generation) return;
                auto resolved = result;
                if(valid){
                    const auto discovered=fromMetadata(baseUrl,model,metadata);
                    if(discovered.source=="metadata")resolved=discovered;
                    if(discovered.imageInputSource!="unknown"){resolved.imageInputSupported=discovered.imageInputSupported;resolved.imageInputSource=discovered.imageInputSource;}
                    else {resolved.imageInputSupported=result.imageInputSupported;resolved.imageInputSource=result.imageInputSource;}
                    if(discovered.contextWindow>0)resolved.contextWindow=discovered.contextWindow;
                    if(discovered.maxOutputTokens>0)resolved.maxOutputTokens=discovered.maxOutputTokens;
                }
                resolved.models = result.models;
                if (!valid) { resolved.status = "error"; resolved.label = QStringLiteral("无法读取 Ollama 思考元数据，使用已知的保守选项"); }
                publish(baseUrl, model, resolved);
            }); return;
        }
        if (!ok || !document.value("data").isArray()) {
            if (!previous.capability.isEmpty()) result = AgentModelCapability::fromJson(previous.capability);
            result.status = previous.capability.isEmpty() ? "error" : "stale";
            result.label = QStringLiteral("模型元数据读取失败，保留已知选项；可刷新重试");
        }
        publish(baseUrl, model, result);
    });
}

}
