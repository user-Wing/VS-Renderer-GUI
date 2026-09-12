#pragma once

#include <QList>
#include <QString>
#include <QVariantMap>

namespace vsr {

struct FilterNode {
    QString instanceId;
    QString definitionId;
    bool enabled = true;
    QVariantMap parameters;
};

class FilterGraph final {
public:
    const QList<FilterNode> &nodes() const;
    int add(const QString &definitionId);
    bool remove(int index);
    bool move(int from, int to);
    bool setEnabled(int index, bool enabled);
    bool setParameter(int index, const QString &parameterId, const QVariant &value);
    FilterNode *at(int index);
    const FilterNode *at(int index) const;

private:
    QList<FilterNode> nodes_;
};

}
