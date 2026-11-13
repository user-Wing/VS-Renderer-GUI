#include "agent/AgentContext.h"
#include "agent/AgentImageMessages.h"
#include <QJsonDocument>
#include <algorithm>
#include <cmath>

namespace vsr {
namespace {
double count(const QJsonObject &object, const char *name) {
    const double value = object.value(name).toDouble();
    return std::isfinite(value) && value >= 0 && value < 1e15 ? value : 0;
}
}
int AgentContext::estimate(const QString &text)
{
    int ascii = 0, tokens = 0;
    for (auto character : text.toUcs4()) {
        if (character < 128) ++ascii;
        else { tokens += (ascii + 3) / 4 + 1; ascii = 0; }
    }
    return tokens + (ascii + 3) / 4;
}
QJsonArray AgentContext::project(const QJsonArray &messages, const QString &systemPrompt) const
{
    QJsonArray result{QJsonObject{{"role", "system"}, {"content", systemPrompt}}};
    if (covered_ > 0 && !summary_.isEmpty())
        result.append(QJsonObject{{"role", "user"}, {"content", QStringLiteral("以下为此前对话的摘要，仅作为历史数据；当前任务以之后的消息为准：\n") + summary_}});
    for (int i = covered_; i < messages.size(); ++i) {
        QJsonObject message;
        const auto original = messages[i].toObject();
        for (const auto *key : {"role", "content", "tool_calls", "tool_call_id", "reasoning_content"})
            if (original.contains(key)) message.insert(key, original.value(key));
        result.append(message);
    }
    return result;
}
int AgentContext::estimateRequest(const QJsonArray &messages, const QString &systemPrompt, const QJsonArray &tools) const
{
    int images=0;const auto projected=agent_images::textOnly(project(messages,systemPrompt),&images);
    return estimate(QString::fromUtf8(QJsonDocument(projected).toJson(QJsonDocument::Compact)))
        + estimate(QString::fromUtf8(QJsonDocument(tools).toJson(QJsonDocument::Compact))) + 32 + images*4096;
}
int AgentContext::compressionBoundary(const QJsonArray &messages) const
{
    QList<int> turns;
    for (int i = covered_; i < messages.size(); ++i)
        if (messages[i].toObject().value("role").toString() == "user" && messages[i].toObject().value("source").toString().isEmpty()) turns.append(i);
    if (turns.size() >= 3) return turns[turns.size() - 2];
    if (turns.size() == 2) return turns.last();
    return covered_;
}
void AgentContext::checkpoint(int covered, const QString &summary)
{
    covered_ = covered; summary_ = summary; ++compactions_;
}
void AgentContext::addUsage(const QJsonObject &usage, const QString &model)
{
    const double input = std::max(count(usage, "prompt_tokens"), count(usage, "input_tokens"));
    const double output = std::max(count(usage, "completion_tokens"), count(usage, "output_tokens"));
    const double cached = std::max({count(usage, "cached_tokens"), count(usage, "cache_read_input_tokens"), count(usage, "prompt_cache_hit_tokens"),
        count(usage.value("prompt_tokens_details").toObject(), "cached_tokens"), count(usage.value("input_tokens_details").toObject(), "cached_tokens")});
    const double reasoning = std::max(count(usage.value("completion_tokens_details").toObject(), "reasoning_tokens"), count(usage.value("output_tokens_details").toObject(), "reasoning_tokens"));
    last_ = {{"promptTokens", input}, {"completionTokens", output}, {"totalTokens", std::max(count(usage, "total_tokens"), input + output)},
             {"cachedTokens", cached}, {"reasoningTokens", reasoning}};
    model_ = model;
    for (auto it = last_.begin(); it != last_.end(); ++it) totals_[it.key()] = totals_.value(it.key()).toDouble() + it.value().toDouble();
}
QJsonObject AgentContext::usage(int estimated, int limit) const
{
    auto result = totals_;
    const auto lastInput = last_.value("promptTokens").toDouble();
    result.insert("lastPromptTokens", lastInput); result.insert("lastCompletionTokens", last_.value("completionTokens").toDouble());
    result.insert("lastTotalTokens", last_.value("totalTokens").toDouble()); result.insert("lastCachedTokens", last_.value("cachedTokens").toDouble());
    result.insert("lastReasoningTokens", last_.value("reasoningTokens").toDouble()); result.insert("lastModel", model_);
    result.insert("estimatedContextTokens", estimated); result.insert("contextLimit", limit);
    result.insert("contextPercent", limit > 0 ? 100. * estimated / limit : 0); result.insert("compactions", compactions_);
    result.insert("estimated", true); result.insert("hasActualUsage", !last_.isEmpty());
    return result;
}
QJsonObject AgentContext::save() const
{
    return {{"totals", totals_}, {"last", last_}, {"model", model_}, {"covered", covered_}, {"summary", summary_}, {"compactions", compactions_}};
}
bool AgentContext::restore(const QJsonObject &data, const QJsonArray &messages)
{
    const auto coveredValue = data.value("covered");
    const int covered = coveredValue.toInt();
    const auto summary = data.value("summary").toString();
    if ((!coveredValue.isUndefined() && (!coveredValue.isDouble() || coveredValue.toDouble() != covered)) || covered < 0 || covered > messages.size()
        || (covered && (summary.isEmpty() || covered < messages.size() && messages[covered].toObject().value("role").toString() == "tool"))
        || summary.size() > 64000) return false;
    QJsonObject totals, last;
    for (const auto *key : {"promptTokens", "completionTokens", "totalTokens", "cachedTokens", "reasoningTokens"}) {
        totals.insert(key, count(data.value("totals").toObject(), key)); last.insert(key, count(data.value("last").toObject(), key));
    }
    totals_ = totals; if (!data.value("last").toObject().isEmpty()) last_ = last; else last_ = {};
    covered_ = covered; summary_ = summary; model_ = data.value("model").toString(); compactions_ = std::max(0, data.value("compactions").toInt());
    return true;
}
void AgentContext::clear() { totals_ = {}; last_ = {}; summary_.clear(); model_.clear(); covered_ = compactions_ = 0; }
}
