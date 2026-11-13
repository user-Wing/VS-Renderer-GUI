#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace vsr {
class AgentContext final {
public:
    static int estimate(const QString &text);
    QJsonArray project(const QJsonArray &messages, const QString &systemPrompt) const;
    int estimateRequest(const QJsonArray &messages, const QString &systemPrompt, const QJsonArray &tools) const;
    int compressionBoundary(const QJsonArray &messages) const;
    void checkpoint(int covered, const QString &summary);
    void addUsage(const QJsonObject &usage, const QString &model);
    QJsonObject usage(int estimated, int limit) const;
    QJsonObject save() const;
    bool restore(const QJsonObject &data, const QJsonArray &messages);
    void clear();
    int covered() const { return covered_; }
private:
    QJsonObject totals_, last_;
    QString summary_, model_;
    int covered_ = 0, compactions_ = 0;
};
}
