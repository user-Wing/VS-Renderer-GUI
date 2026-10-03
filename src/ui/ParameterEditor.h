#pragma once

#include <QWidget>
#include <QSize>

class QGridLayout;
class QSpinBox;
class QCheckBox;

namespace vsr {

struct FilterDefinition;
struct FilterNode;

class ParameterEditor final : public QWidget {
    Q_OBJECT
public:
    explicit ParameterEditor(QWidget *parent = nullptr);
    void setNode(const FilterDefinition *definition, const FilterNode *node);
    void setInputSize(QSize size);

signals:
    void parameterChanged(const QString &parameterId, const QVariant &value);

private:
    void clear();
    QGridLayout *form_ = nullptr;
    QSize inputSize_;
    QSpinBox *shaderWidth_ = nullptr;
    QSpinBox *shaderHeight_ = nullptr;
    QCheckBox *keepAspect_ = nullptr;
    QString dimensionAxis_;
};

}
