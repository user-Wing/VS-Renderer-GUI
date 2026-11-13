#pragma once
#include "agent/AgentProvider.h"
#include <QJsonArray>

namespace vsr::agent_images {
constexpr int maximumImages = 4;
constexpr int maximumEncodedBytes = 512 * 1024;
bool providerSupportsImages(const AgentProvider &provider);
bool validDataUrl(const QString &url, QString *mime = nullptr);
bool validContent(const QJsonArray &blocks);
bool containsImages(const QJsonArray &messages);
QString textContent(const QJsonValue &content, int *images = nullptr);
QJsonArray textOnly(const QJsonArray &messages, int *images = nullptr);
}
