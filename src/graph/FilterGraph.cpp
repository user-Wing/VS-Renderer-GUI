#include "graph/FilterGraph.h"

#include "graph/FilterCatalog.h"

#include <QUuid>
#include <cmath>

namespace vsr {

const QList<FilterNode> &FilterGraph::nodes() const
{
    return nodes_;
}

int FilterGraph::add(const QString &definitionId)
{
    const auto *definition = FilterCatalog::find(definitionId);
    if (!definition)
        return -1;

    FilterNode node;
    node.instanceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    node.definitionId = definitionId;
    for (const auto &parameter : definition->parameters)
        node.parameters.insert(parameter.id, parameter.defaultValue);
    nodes_.append(std::move(node));
    return nodes_.size() - 1;
}

bool FilterGraph::remove(int index)
{
    if (index < 0 || index >= nodes_.size())
        return false;
    nodes_.removeAt(index);
    return true;
}

bool FilterGraph::move(int from, int to)
{
    if (from < 0 || from >= nodes_.size() || to < 0 || to >= nodes_.size())
        return false;
    nodes_.move(from, to);
    return true;
}

bool FilterGraph::setEnabled(int index, bool enabled)
{
    auto *node = at(index);
    if (!node)
        return false;
    node->enabled = enabled;
    return true;
}

bool FilterGraph::setParameter(int index, const QString &parameterId, const QVariant &value)
{
    auto *node = at(index);
    const auto *definition = node ? FilterCatalog::find(node->definitionId) : nullptr;
    if (!node || !definition)
        return false;
    for (const auto &parameter : definition->parameters) {
        if (parameter.id == parameterId) {
            QVariant checked=value;
            if(parameter.type==ParameterType::Choice){checked=value.toString();if(node->definitionId=="anime4k" && parameterId=="category" && checked.toString()=="自定义 GLSL/HLSL")checked=QStringLiteral("自定义 GLSL");if(!parameter.choices.contains(checked.toString()))return false;}
            else if(parameter.type==ParameterType::Integer || parameter.type==ParameterType::Real){bool ok=false;const double number=value.toDouble(&ok);if(!ok || !std::isfinite(number) || number<parameter.minimum || number>parameter.maximum || (parameter.type==ParameterType::Integer && std::trunc(number)!=number))return false;checked=parameter.type==ParameterType::Integer?QVariant(int(number)):QVariant(number);}
            else if(parameter.type==ParameterType::Boolean){if(value.metaType().id()!=QMetaType::Bool)return false;}
            else if(parameter.type==ParameterType::File){if(value.metaType().id()!=QMetaType::QString || value.toString().contains(QChar(0)))return false;}
            node->parameters.insert(parameterId, checked);
            return true;
        }
    }
    return false;
}

FilterNode *FilterGraph::at(int index)
{
    return index >= 0 && index < nodes_.size() ? &nodes_[index] : nullptr;
}

const FilterNode *FilterGraph::at(int index) const
{
    return index >= 0 && index < nodes_.size() ? &nodes_[index] : nullptr;
}

}
