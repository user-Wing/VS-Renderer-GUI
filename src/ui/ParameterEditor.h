#pragma once

#include <QWidget>

class QFormLayout;

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
    QFormLayout *form_ = nullptr;
};

}
