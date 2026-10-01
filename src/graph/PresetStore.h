#pragma once
#include "graph/FilterGraph.h"
#include "graph/VpyScriptBuilder.h"
#include <QJsonObject>

namespace vsr {
class PresetStore final {
public:
    static QString directory();
    static QJsonObject metadata(const FilterGraph &graph, SourceFilter filter, const QString &source, const QString &note);
    static QString create(const FilterGraph &graph, SourceFilter filter, const QString &source, const QString &note);
    static QJsonObject metadata(const QString &script);
    static FilterGraph graph(const QJsonObject &metadata);
    static ScriptBuildResult load(const QString &path, const QString &source = {});
    static bool write(const QString &path, const QString &script, QString *error = nullptr);
};
}
