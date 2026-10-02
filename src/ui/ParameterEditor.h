#pragma once

#include <QWidget>

class QGridLayout;

namespace vsr {

struct FilterDefinition;
struct FilterNode;

class ParameterEditor final : public QWidget {
    Q_OBJECT
public:
    explicit ParameterEditor(QWidget *parent = nullptr);
    void setNode(const FilterDefinition *definition, const FilterNode *node);

signals:
    void parameterChanged(const QString &parameterId, const QVariant &value);

private:
    void clear();
    QGridLayout *form_ = nullptr;
};

}
