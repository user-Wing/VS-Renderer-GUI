#include "ui/ParameterEditor.h"

#include "graph/FilterCatalog.h"
#include "graph/FilterGraph.h"

#include <QCheckBox>
#include <algorithm>
#include <QComboBox>
#include <QCompleter>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QAbstractItemView>
#include <QScreen>

namespace vsr {

ParameterEditor::ParameterEditor(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("parameterEditor"));
    setStyleSheet(QStringLiteral(
        "#parameterEditor,#parameterEditor QWidget{background:#ffffff;}"
        "#parameterEditor QComboBox,#parameterEditor QSpinBox,#parameterEditor QDoubleSpinBox,#parameterEditor QLineEdit{background:#f3f3f3;}"
        "#parameterEditor QComboBox:hover{background:#e9e9e9;}"));
    form_ = new QGridLayout(this);
    form_->setContentsMargins(8, 8, 8, 8);
    form_->setHorizontalSpacing(8);
    form_->setVerticalSpacing(10);
    form_->setColumnStretch(0, 1);
    form_->setColumnStretch(1, 0);
    form_->setAlignment(Qt::AlignTop);
    clear();
    form_->addWidget(new QLabel(QStringLiteral("选择处理链中的节点以编辑参数。"), this), 0, 0, 1, 2);
}

void ParameterEditor::setNode(const FilterDefinition *definition, const FilterNode *node)
{
    clear();
    if (!definition || !node) {
        form_->addWidget(new QLabel(QStringLiteral("选择处理链中的节点以编辑参数。"), this), 0, 0, 1, 2);
        return;
    }

    auto *description = new QLabel(definition->description, this);
    description->setWordWrap(true);
    description->setTextFormat(Qt::PlainText);
    form_->addWidget(description, 0, 0, 1, 2);
    int row = 1;

    QComboBox *shaderMode = nullptr;
    QLineEdit *shaderPath = nullptr;
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
            if (definition->id == "anime4k" && id == "mode") shaderMode = combo;
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setMinimumContentsLength(8);
            combo->addItems(parameter.choices);
            if(combo==shaderMode){combo->setEditable(true);combo->setInsertPolicy(QComboBox::NoInsert);combo->completer()->setCompletionMode(QCompleter::PopupCompletion);combo->completer()->setFilterMode(Qt::MatchContains);combo->completer()->setCaseSensitivity(Qt::CaseInsensitive);}
            int popupWidth = 0;
            for (const auto &item : parameter.choices) popupWidth = std::max(popupWidth, combo->fontMetrics().horizontalAdvance(item) + 48);
            combo->view()->setMinimumWidth(combo==shaderMode && screen()?std::min(popupWidth,screen()->availableGeometry().width()-32):popupWidth);
            combo->setToolTip(value.toString());
            combo->setCurrentText(value.toString());
            const auto fit = [combo, shaderMode] { const int width=std::max(70, combo->fontMetrics().horizontalAdvance(combo->currentText()) + 40);combo->setFixedWidth(combo==shaderMode?std::min(300,width):width); };
            fit();
            connect(combo, &QComboBox::currentTextChanged, this,
                    [this, id, combo, fit, shaderMode](const QString &v) { fit(); combo->setToolTip(v); if(combo!=shaderMode || combo->findText(v)>=0)emit parameterChanged(id, v); });
            editor = combo;
            break;
        }
        case ParameterType::File: {
            auto *container = new QWidget(this);
            auto *layout = new QHBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(4);
            auto *path = new QLineEdit(value.toString(), container);
            if (definition->id == "anime4k" && id == "shader") shaderPath = path;
            auto *browse = new QPushButton(QStringLiteral("浏览…"), container);
            connect(path, &QLineEdit::editingFinished, this,
                    [this, id, path] { emit parameterChanged(id, path->text()); });
            connect(browse, &QPushButton::clicked, this, [this, id, path] {
                const QString selected = QFileDialog::getOpenFileName(
                    this, QStringLiteral("选择 libplacebo GLSL"), path->text(),
                    QStringLiteral("mpv shader (*.glsl *.hook);;所有文件 (*.*)"));
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
        auto *label = new QLabel(parameter.label, this);
        label->setObjectName(QStringLiteral("parameterLabel_") + id);
        label->setWordWrap(true);
        label->setMinimumWidth(0);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        editor->setObjectName(QStringLiteral("parameter_") + id);
        if (parameter.type == ParameterType::File) {
            form_->addWidget(label, row++, 0, 1, 2);
            editor->setMinimumWidth(0);
            editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            form_->addWidget(editor, row++, 0, 1, 2);
        } else {
            editor->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            form_->addWidget(label, row, 0, Qt::AlignVCenter);
            form_->addWidget(editor, row++, 1, Qt::AlignRight | Qt::AlignVCenter);
        }
    }
    if (shaderMode && shaderPath) {
        shaderPath->parentWidget()->setEnabled(shaderMode->currentText() == QStringLiteral("自定义 GLSL") || !shaderPath->text().isEmpty());
        connect(shaderMode, &QComboBox::currentTextChanged, shaderPath, [this, shaderPath, shaderMode](const QString &mode) {
            if(shaderMode->findText(mode)<0)return;
            const bool custom = mode == QStringLiteral("自定义 GLSL");
            shaderPath->parentWidget()->setEnabled(custom);
            if (!custom && !shaderPath->text().isEmpty()){shaderPath->clear();emit parameterChanged("shader",QString());}
        });
    }
    if (definition->parameters.isEmpty())
        form_->addWidget(new QLabel(QStringLiteral("该节点没有可调参数。"), this), row, 0, 1, 2);
}

void ParameterEditor::clear()
{
    while (auto *item = form_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

}
