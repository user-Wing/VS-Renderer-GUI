#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace vsr {

enum class ParameterType {
    Integer,
    Real,
    Boolean,
    Choice,
    File
};

struct ParameterDefinition {
    QString id;
    QString label;
    ParameterType type = ParameterType::Integer;
    QVariant defaultValue;
    double minimum = 0.0;
    double maximum = 0.0;
    double step = 1.0;
    QStringList choices;
};

struct FilterDefinition {
    QString id;
    QString name;
    QString category;
    QString pluginNamespace;
    QString description;
    QList<ParameterDefinition> parameters;
};

class FilterCatalog final {
public:
    static const QList<FilterDefinition> &all();
    static const FilterDefinition *find(const QString &id);
    static QStringList categories();
    static QStringList shaderCategories();
    static QString shaderCategory(const QString &path);
    static QString shaderHardwareLabel(const QString &path);
};

}
