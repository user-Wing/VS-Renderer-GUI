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
#include <QFileInfo>
#include <QDir>
#include <QSignalBlocker>
#include <cmath>

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
    QComboBox *shaderCategory = nullptr;
    QComboBox *outputMode = nullptr;
    QComboBox *outputScale = nullptr;
    QLineEdit *shaderPath = nullptr;
    dimensionAxis_=node->parameters.value("dimension_axis","width").toString();
    for (const auto &parameter : definition->parameters) {
        QWidget *editor = nullptr;
        const QVariant value = node->parameters.value(parameter.id, parameter.defaultValue);
        const QString id = parameter.id;
        if(definition->id=="anime4k" && id=="dimension_axis")continue;
        switch (parameter.type) {
        case ParameterType::Integer: {
            auto *spin = new QSpinBox(this);
            spin->setRange(static_cast<int>(parameter.minimum), static_cast<int>(parameter.maximum));
            spin->setSingleStep(static_cast<int>(parameter.step));
            spin->setValue(value.toInt());
            if(definition->id=="anime4k" && id=="width")shaderWidth_=spin;
            if(definition->id=="anime4k" && id=="height")shaderHeight_=spin;
            connect(spin, &QSpinBox::valueChanged, this, [this, id, spin](int v) {
                if(spin==shaderWidth_ || spin==shaderHeight_){dimensionAxis_=id;emit parameterChanged("dimension_axis",id);setInputSize(inputSize_);}
                emit parameterChanged(id, v);
            });
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
            if(definition->id=="anime4k" && id=="keep_aspect")keepAspect_=check;
            connect(check, &QCheckBox::toggled, this, [this, id](bool v) { emit parameterChanged(id, v); });
            editor = check;
            break;
        }
        case ParameterType::Choice: {
            auto *combo = new QComboBox(this);
            if (definition->id == "anime4k" && id == "mode") shaderMode = combo;
            if(definition->id=="anime4k" && id=="category")shaderCategory=combo;
            if(definition->id=="anime4k" && id=="output_mode")outputMode=combo;
            if(definition->id=="anime4k" && id=="scale")outputScale=combo;
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setMinimumContentsLength(8);
            if(combo!=shaderMode)combo->addItems(parameter.choices);
            if(combo==shaderMode){combo->setEditable(true);combo->setInsertPolicy(QComboBox::NoInsert);combo->completer()->setCompletionMode(QCompleter::PopupCompletion);combo->completer()->setFilterMode(Qt::MatchContains);combo->completer()->setCaseSensitivity(Qt::CaseInsensitive);}
            int popupWidth = 0;
            for (const auto &item : parameter.choices) popupWidth = std::max(popupWidth, combo->fontMetrics().horizontalAdvance(item) + 48);
            combo->view()->setMinimumWidth(combo==shaderMode && screen()?std::min(popupWidth,screen()->availableGeometry().width()-32):popupWidth);
            if(combo==shaderMode){combo->view()->setMinimumWidth(420);combo->view()->setMaximumWidth(520);combo->view()->setTextElideMode(Qt::ElideMiddle);combo->view()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);}
            combo->setToolTip(value.toString());
            if(combo!=shaderMode)combo->setCurrentText(value.toString());
            const auto fit = [combo, shaderMode, shaderCategory] { const int width=std::max(70, combo->fontMetrics().horizontalAdvance(combo->currentText()) + 40);combo->setFixedWidth(combo==shaderMode || combo==shaderCategory?140:width); };
            fit();
            if(combo!=shaderMode)connect(combo, &QComboBox::currentTextChanged, this,
                    [this, id, combo, fit](const QString &v) { fit(); combo->setToolTip(v); emit parameterChanged(id, v); });
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
        QStringList choices;for(const auto &p:definition->parameters)if(p.id=="mode")choices=p.choices;
        const auto populate=[shaderMode,shaderCategory,choices](const QString &selected) {
            const QSignalBlocker blocker(shaderMode);shaderMode->clear();
            for(const auto &path:choices){
                if(shaderCategory->currentText()!="全部" && FilterCatalog::shaderCategory(path)!=shaderCategory->currentText())continue;
                QString label=QFileInfo(path).fileName();const auto hardware=FilterCatalog::shaderHardwareLabel(path);
                if(!hardware.isEmpty())label+=" · "+hardware;
                if(path.contains('/'))label+=" · "+path.section('/',1,1)+" / "+QFileInfo(path).dir().dirName();
                shaderMode->addItem(label,path);shaderMode->setItemData(shaderMode->count()-1,path+"\n"+FilterCatalog::shaderCategory(path)+"\n"+hardware,Qt::ToolTipRole);
            }
            const int index=shaderMode->findData(selected);shaderMode->setCurrentIndex(index>=0?index:0);
            shaderMode->setToolTip(shaderMode->currentData(Qt::ToolTipRole).toString());shaderMode->lineEdit()->setCursorPosition(0);
        };
        const auto selected=node->parameters.value("shader").toString().isEmpty()?node->parameters.value("mode").toString():QString("自定义 GLSL");
        if(shaderCategory->currentText()!="全部" && shaderCategory->currentText()!=FilterCatalog::shaderCategory(selected)){const QSignalBlocker blocker(shaderCategory);shaderCategory->setCurrentText(FilterCatalog::shaderCategory(selected));}
        populate(selected);
        shaderPath->parentWidget()->setEnabled(selected=="自定义 GLSL");
        const auto select=[this,shaderPath,shaderMode] {
            const auto mode=shaderMode->currentData().toString();if(mode.isEmpty())return;
            const bool custom = mode == QStringLiteral("自定义 GLSL");
            shaderPath->parentWidget()->setEnabled(custom);
            if (!custom && !shaderPath->text().isEmpty()){shaderPath->clear();emit parameterChanged("shader",QString());}
            shaderMode->setToolTip(shaderMode->currentData(Qt::ToolTipRole).toString());shaderMode->lineEdit()->setCursorPosition(0);emit parameterChanged("mode",mode);
        };
        connect(shaderMode,qOverload<int>(&QComboBox::activated),this,[select](int){select();});
        connect(shaderCategory,&QComboBox::currentTextChanged,this,[populate,select,shaderMode](const QString &){const auto selected=shaderMode->currentData().toString();populate(selected);select();});
        connect(shaderPath,&QLineEdit::textChanged,this,[shaderCategory](const QString &path){if(!path.isEmpty())shaderCategory->setCurrentText("自定义 GLSL");});
        const auto updateOutput=[this,outputMode,outputScale]{const bool resolution=outputMode->currentText()=="指定分辨率";outputScale->setEnabled(!resolution);shaderWidth_->setEnabled(resolution);shaderHeight_->setEnabled(resolution);keepAspect_->setEnabled(resolution);};
        updateOutput();connect(outputMode,&QComboBox::currentTextChanged,this,[updateOutput](const QString &){updateOutput();});
        connect(keepAspect_,&QCheckBox::toggled,this,[this](bool enabled){if(enabled)setInputSize(inputSize_);});
    }
    if (definition->parameters.isEmpty())
        form_->addWidget(new QLabel(QStringLiteral("该节点没有可调参数。"), this), row, 0, 1, 2);
}

void ParameterEditor::clear()
{
    shaderWidth_=shaderHeight_=nullptr;keepAspect_=nullptr;
    while (auto *item = form_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

void ParameterEditor::setInputSize(QSize size)
{
    inputSize_=size;
    if(size.isEmpty() || !shaderWidth_ || !shaderHeight_ || !keepAspect_->isChecked())return;
    const bool width=dimensionAxis_!="height";auto *other=width?shaderHeight_:shaderWidth_;
    const int computed=std::clamp(int(std::lround(width?double(shaderWidth_->value())*size.height()/size.width():double(shaderHeight_->value())*size.width()/size.height())),other->minimum(),other->maximum());
    if(other->value()==computed)return;
    const QSignalBlocker blocker(other);other->setValue(computed);emit parameterChanged(width?"height":"width",computed);
}

}
