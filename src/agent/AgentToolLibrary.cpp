#include "agent/AgentToolLibrary.h"
#include "agent/AgentSession.h"

#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <cmath>

namespace vsr {
namespace {
bool fail(QString *error, const QString &message)
{
    if (error) *error = message;
    return false;
}

bool validSchema(const QJsonObject &schema, QString *error, int depth = 0)
{
    static const QSet<QString> supported{"$schema", "$id", "title", "description", "default", "examples", "type", "properties",
        "required", "additionalProperties", "enum", "items", "minimum", "maximum", "minItems", "maxItems", "minLength", "maxLength"};
    for (auto it = schema.begin(); it != schema.end(); ++it)
        if (!supported.contains(it.key())) return fail(error, "Unsupported parameter schema keyword: " + it.key());
    const auto type = schema.value("type").toString();
    const QStringList types{"object", "array", "string", "number", "integer", "boolean", "null"};
    if (depth > 12 || !types.contains(type)) return fail(error, "Invalid parameter schema type");
    if (schema.contains("enum") && (!schema.value("enum").isArray() || schema.value("enum").toArray().isEmpty()))
        return fail(error, "Parameter enum must be a non-empty array");
    for (const auto &bounds : {qMakePair(QString("minimum"), QString("maximum")),
                              qMakePair(QString("minItems"), QString("maxItems")),
                              qMakePair(QString("minLength"), QString("maxLength"))}) {
        for (const auto &name : {bounds.first, bounds.second}) {
            if (!schema.contains(name)) continue;
            const auto value = schema.value(name);
            if (!value.isDouble() || (name != "minimum" && name != "maximum" &&
                (value.toDouble() < 0 || std::floor(value.toDouble()) != value.toDouble())))
                return fail(error, "Invalid parameter schema bound: " + name);
        }
        if (schema.contains(bounds.first) && schema.contains(bounds.second) &&
            schema.value(bounds.first).toDouble() > schema.value(bounds.second).toDouble())
            return fail(error, "Parameter schema minimum exceeds maximum");
    }
    if (type == "object") {
        if ((schema.contains("properties") && !schema.value("properties").isObject()) ||
            (schema.contains("required") && !schema.value("required").isArray()) ||
            (schema.contains("additionalProperties") && !schema.value("additionalProperties").isBool()))
            return fail(error, "Invalid object parameter schema");
        const auto properties = schema.value("properties").toObject();
        for (auto it = properties.begin(); it != properties.end(); ++it)
            if (!it.value().isObject() || !validSchema(it.value().toObject(), error, depth + 1)) return false;
        QSet<QString> required;
        for (const auto &value : schema.value("required").toArray()) {
            const auto name = value.toString();
            if (!value.isString() || !properties.contains(name) || required.contains(name))
                return fail(error, "Required parameters must name distinct declared properties");
            required.insert(name);
        }
    } else if (type == "array") {
        if (!schema.value("items").isObject()) return fail(error, "Array parameters require an items schema");
        return validSchema(schema.value("items").toObject(), error, depth + 1);
    }
    return true;
}

bool validValue(const QJsonValue &value, const QJsonObject &schema, QString *error, const QString &path)
{
    const auto type = schema.value("type").toString();
    const bool matches = (type == "object" && value.isObject()) || (type == "array" && value.isArray()) ||
        (type == "string" && value.isString()) || (type == "number" && value.isDouble()) ||
        (type == "integer" && value.isDouble() && std::floor(value.toDouble()) == value.toDouble()) ||
        (type == "boolean" && value.isBool()) || (type == "null" && value.isNull());
    if (!matches) return fail(error, path + ": expected " + type);
    if (schema.contains("enum") && !schema.value("enum").toArray().contains(value))
        return fail(error, path + ": value is not in enum");
    if (value.isDouble() && ((schema.contains("minimum") && value.toDouble() < schema.value("minimum").toDouble()) ||
        (schema.contains("maximum") && value.toDouble() > schema.value("maximum").toDouble())))
        return fail(error, path + ": number is outside the allowed range");
    if (value.isString() || value.isArray()) {
        const auto count = value.isString() ? value.toString().toUcs4().size() : value.toArray().size();
        const auto minimum = value.isString() ? "minLength" : "minItems", maximum = value.isString() ? "maxLength" : "maxItems";
        if ((schema.contains(minimum) && count < schema.value(minimum).toDouble()) ||
            (schema.contains(maximum) && count > schema.value(maximum).toDouble()))
            return fail(error, path + ": length is outside the allowed range");
    }
    if (value.isObject()) {
        const auto object = value.toObject(), properties = schema.value("properties").toObject();
        for (const auto &required : schema.value("required").toArray())
            if (!object.contains(required.toString())) return fail(error, path + ": missing " + required.toString());
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (properties.contains(it.key())) {
                if (!validValue(it.value(), properties.value(it.key()).toObject(), error, path + "." + it.key())) return false;
            } else if (!schema.value("additionalProperties").toBool(true)) {
                return fail(error, path + ": unknown parameter " + it.key());
            }
        }
    } else if (value.isArray()) {
        int index = 0;
        for (const auto &item : value.toArray())
            if (!validValue(item, schema.value("items").toObject(), error, path + "[" + QString::number(index++) + "]")) return false;
    }
    return true;
}

QString placeholder(const QJsonValue &value)
{
    static const QRegularExpression expression("^\\$([A-Za-z_][A-Za-z0-9_-]*)$");
    return value.isString() ? expression.match(value.toString()).captured(1) : QString();
}

bool resolve(const QJsonValue &value, const QJsonObject &arguments, QJsonValue *result, QString *error, int depth = 0)
{
    if (depth > 32) return fail(error, "Tool argument nesting exceeds 32 levels");
    const auto name = placeholder(value);
    if (!name.isEmpty()) {
        if (!arguments.contains(name)) return fail(error, "Missing workflow parameter: " + name);
        *result = arguments.value(name);
    } else if (value.isObject()) {
        QJsonObject object;
        const auto source = value.toObject();
        for (auto it = source.begin(); it != source.end(); ++it) {
            QJsonValue child;
            if (!resolve(it.value(), arguments, &child, error, depth + 1)) return false;
            object.insert(it.key(), child);
        }
        *result = object;
    } else if (value.isArray()) {
        QJsonArray array;
        for (const auto &item : value.toArray()) {
            QJsonValue child;
            if (!resolve(item, arguments, &child, error, depth + 1)) return false;
            array.append(child);
        }
        *result = array;
    } else {
        *result = value;
    }
    return true;
}

bool failedResult(const QJsonObject &result)
{
    const auto error = result.value("error");
    return (!error.isUndefined() && !error.isNull() && error != QJsonValue("") && error != QJsonValue(false)) ||
        (result.contains("ok") && result.value("ok") == QJsonValue(false)) ||
        (result.contains("success") && result.value("success") == QJsonValue(false));
}
}

AgentToolLibrary::AgentToolLibrary(AgentSession *session) : session_(session) {}

AgentToolLibrary::~AgentToolLibrary()
{
    if (session_) for (auto it = definitions_.cbegin(); it != definitions_.cend(); ++it) session_->unregisterTool(it.key());
}

bool AgentToolLibrary::importTools(const QJsonObject &document, QString *error)
{
    if (error) error->clear();
    if (!session_) return fail(error, "Agent session is unavailable");
    if (document.value("schemaVersion") != QJsonValue(1) || !document.value("tools").isArray())
        return fail(error, "Expected workflow schemaVersion 1 and a tools array");
    const auto tools = document.value("tools").toArray();
    if (tools.size() + definitions_.size() > 64) return fail(error, "At most 64 workflow tools are allowed");
    QSet<QString> registered, incoming;
    for (const auto &tool : session_->allTools()) registered.insert(tool.toObject().value("function").toObject().value("name").toString());
    static const QRegularExpression validName("^[A-Za-z_][A-Za-z0-9_-]{0,63}$");
    for (const auto &value : tools) {
        const auto definition = value.toObject();
        const auto name = definition.value("name").toString();
        if (!value.isObject() || !validName.match(name).hasMatch() || registered.contains(name) || incoming.contains(name))
            return fail(error, "Invalid or already registered workflow name: " + name);
        incoming.insert(name);
    }
    for (const auto &value : tools) {
        const auto definition = value.toObject(), schema = definition.value("parameters").toObject();
        if (!definition.value("description").isString() || definition.value("description").toString().trimmed().isEmpty() ||
            schema.value("type") != QJsonValue("object") || !validSchema(schema, error))
            return fail(error, error && !error->isEmpty() ? *error : "Workflow requires a description and object parameter schema");
        const auto steps = definition.value("steps").toArray();
        if (!definition.value("steps").isArray() || steps.isEmpty() || steps.size() > 32)
            return fail(error, "Workflow requires between 1 and 32 steps");
        QJsonObject declared;
        const auto properties = schema.value("properties").toObject();
        for (auto it = properties.begin(); it != properties.end(); ++it) declared.insert(it.key(), QJsonValue());
        for (const auto &stepValue : steps) {
            const auto step = stepValue.toObject();
            const auto target = step.value("tool").toString();
            if (!stepValue.isObject() || !registered.contains(target) || definitions_.contains(target) || incoming.contains(target) ||
                !step.value("arguments").isObject()) return fail(error, "Each step must call an existing non-workflow tool with object arguments");
            QJsonValue checked;
            if (!resolve(step.value("arguments"), declared, &checked, error)) return false;
        }
    }
    for (const auto &value : tools) {
        const auto definition = value.toObject();
        const auto name = definition.value("name").toString();
        definitions_.insert(name, definition);
        session_->registerTool(name, definition.value("description").toString(), definition.value("parameters").toObject(),
            [this, definition](const QJsonObject &arguments) { return execute(definition, arguments); });
        session_->setToolDelegationAllowed(name, false);
    }
    return true;
}

QJsonObject AgentToolLibrary::exportTools() const
{
    QJsonArray tools;
    for (const auto &definition : definitions_) tools.append(definition);
    return {{"schemaVersion", 1}, {"tools", tools}};
}

QJsonObject AgentToolLibrary::execute(const QJsonObject &definition, const QJsonObject &arguments)
{
    QString error;
    if (!session_) return {{"error", "Agent session is unavailable"}};
    if (!validValue(arguments, definition.value("parameters").toObject(), &error, "arguments")) return {{"error", error}};
    QJsonArray prepared, results;
    for (const auto &value : definition.value("steps").toArray()) {
        auto step = value.toObject();
        QJsonValue resolved;
        if (!resolve(step.value("arguments"), arguments, &resolved, &error)) return {{"error", error}};
        step.insert("arguments", resolved);
        prepared.append(step);
    }
    for (const auto &value : prepared) {
        const auto step = value.toObject();
        const auto target = step.value("tool").toString();
        const auto result = session_->executeTool(target, step.value("arguments").toObject());
        results.append(QJsonObject{{"tool", target}, {"result", result}});
        if (failedResult(result)) return {{"error", "Workflow stopped after failed tool: " + target}, {"failedStep", results.size()}, {"steps", results}};
    }
    return {{"ok", true}, {"steps", results}};
}

}
