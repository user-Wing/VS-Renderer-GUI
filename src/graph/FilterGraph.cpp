#include "graph/FilterGraph.h"

#include "graph/FilterCatalog.h"

#include <QUuid>

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
            node->parameters.insert(parameterId, value);
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
