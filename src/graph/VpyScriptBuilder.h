#pragma once

#include <QString>
#include <QStringList>

namespace vsr {

class FilterGraph;

enum class SourceFilter {
    Lsmas,
    Ffms2
};

struct ScriptBuildResult {
    QString script;
    QStringList requiredNamespaces;
    QStringList errors;
};

class VpyScriptBuilder final {
public:
    static ScriptBuildResult build(const QString &sourcePath, SourceFilter sourceFilter,
                                   const FilterGraph &graph);
    static QString pythonString(const QString &value);
    static QString upgradeSharpen(const QString &script, const FilterGraph &graph);
    static QString networkSource(const QString &script, const QString &source);
};

}
