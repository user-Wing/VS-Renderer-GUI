#include "ui/ParameterEditor.h"

#include "graph/FilterCatalog.h"
#include "graph/FilterGraph.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

namespace vsr {

ParameterEditor::ParameterEditor(QWidget *parent)
    : QWidget(parent)
{
    form_ = new QFormLayout(this);
    form_->setContentsMargins(8, 8, 8, 8);
    form_->setHorizontalSpacing(8);
    form_->setVerticalSpacing(4);
    form_->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form_->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    clear();
    form_->addRow(new QLabel(QStringLiteral("选择处理链中的节点以编辑参数。"), this));
}

void ParameterEditor::setNode(const FilterDefinition *definition, const FilterNode *node)
{
    clear();
    if (!definition || !node) {
        form_->addRow(new QLabel(QStringLiteral("选择处理链中的节点以编辑参数。"), this));
        return;
    }

    auto *description = new QLabel(definition->description, this);
    description->setWordWrap(true);
    description->setTextFormat(Qt::PlainText);
    form_->addRow(description);

    for (const auto &parameter : definition->parameters) {
        QWidget *editor = nullptr;
        const QVariant value = node->parameters.value(parameter.id, parameter.defaultValue);
        const QString id = parameter.id;
        switch (parameter.type) {
        case ParameterType::Integer: {
            auto *spin = new QSpinBox(this);
            spin->setRange(static_cast<int>(parameter.minimum), static_cast<int>(parameter.maximum));
            spin->setSingleStep(static_cast<int>(parameter.step));
            spin->setValue(value.toInt());
            connect(spin, &QSpinBox::valueChanged, this, [this, id](int v) { emit parameterChanged(id, v); });
            editor = spin;
            break;
        }
        case ParameterType::Real: {
            auto *spin = new QDoubleSpinBox(this);
            spin->setRange(parameter.minimum, parameter.maximum);
            spin->setSingleStep(parameter.step);
            spin->setDecimals(parameter.step < 0.01 ? 3 : 2);
            spin->setValue(value.toDouble());
            connect(spin, &QDoubleSpinBox::valueChanged, this, [this, id](double v) { emit parameterChanged(id, v); });
            editor = spin;
            break;
        }
        case ParameterType::Boolean: {
            auto *check = new QCheckBox(this);
            check->setChecked(value.toBool());
            connect(check, &QCheckBox::toggled, this, [this, id](bool v) { emit parameterChanged(id, v); });
            editor = check;
            break;
        }
        case ParameterType::Choice: {
            auto *combo = new QComboBox(this);
            combo->addItems(parameter.choices);
            combo->setCurrentText(value.toString());
            connect(combo, &QComboBox::currentTextChanged, this,
                    [this, id](const QString &v) { emit parameterChanged(id, v); });
            editor = combo;
            break;
        }
        case ParameterType::File: {
            auto *container = new QWidget(this);
            auto *layout = new QHBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(4);
            auto *path = new QLineEdit(value.toString(), container);
            auto *browse = new QPushButton(QStringLiteral("浏览…"), container);
            connect(path, &QLineEdit::editingFinished, this,
                    [this, id, path] { emit parameterChanged(id, path->text()); });
            connect(browse, &QPushButton::clicked, this, [this, id, path] {
                const QString selected = QFileDialog::getOpenFileName(
                    this, QStringLiteral("选择 libplacebo GLSL"), path->text(),
                    QStringLiteral("GLSL shader (*.glsl);;所有文件 (*.*)"));
                if (!selected.isEmpty()) {
                    path->setText(selected);
                    emit parameterChanged(id, selected);
                }
            });
            layout->addWidget(path, 1);
            layout->addWidget(browse);
            editor = container;
            break;
        }
        }
        form_->addRow(parameter.label, editor);
    }
    if (definition->parameters.isEmpty())
        form_->addRow(new QLabel(QStringLiteral("该节点没有可调参数。"), this));
}

void ParameterEditor::clear()
{
    while (form_->rowCount() > 0)
        form_->removeRow(0);
}

}
