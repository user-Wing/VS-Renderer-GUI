#pragma once

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTextEdit>
#include <QToolButton>
#include <cmath>
#include <functional>
#include <utility>

namespace vsr {

struct ShortcutBinding { QString key, action; double value = 0; };
struct ShortcutAction { QString id, label; bool takesValue = false; };

inline bool isShortcutEditor(const QWidget *widget)
{
    for (auto *current = widget; current; current = current->parentWidget()) {
        const auto *line = qobject_cast<const QLineEdit *>(current);
        const auto *plain = qobject_cast<const QPlainTextEdit *>(current);
        const auto *rich = qobject_cast<const QTextEdit *>(current);
        if ((line && !line->isReadOnly()) || (plain && !plain->isReadOnly()) ||
            (rich && !rich->isReadOnly()) || qobject_cast<const QAbstractSpinBox *>(current) ||
            qobject_cast<const QComboBox *>(current) || qobject_cast<const QKeySequenceEdit *>(current))
            return true;
        if (current->isWindow()) break;
    }
    return false;
}

class ShortcutTable final : public QTableWidget {
    class Recorder final : public QWidget {
    public:
        explicit Recorder(QWidget *parent) : QWidget(parent)
        {
            auto *layout = new QHBoxLayout(this);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->setSpacing(2);
            key = new QKeySequenceEdit(this);
            key->setMaximumSequenceLength(1);
            key->setClearButtonEnabled(true);
            setFocusPolicy(Qt::StrongFocus);
            setFocusProxy(key);
            layout->addWidget(key, 1);
            confirm = new QToolButton(this);
            confirm->setObjectName("confirmShortcutKey");
            confirm->setText(QStringLiteral("✓"));
            confirm->setToolTip(QStringLiteral("确认快捷键（Enter）"));
            confirm->setAccessibleName(confirm->toolTip());
            layout->addWidget(confirm);
            key->installEventFilter(this);
            if (auto *line = key->findChild<QLineEdit *>()) line->installEventFilter(this);
        }

        QKeySequenceEdit *key;
        QToolButton *confirm;
        bool confirmed = false;

    protected:
        bool eventFilter(QObject *object, QEvent *event) override
        {
            if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress ||
                event->type() == QEvent::KeyRelease) {
                const auto keyEvent = static_cast<QKeyEvent *>(event);
                if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                    event->accept();
                    if (event->type() == QEvent::KeyPress && !keyEvent->isAutoRepeat()) confirm->click();
                    return true;
                }
            }
            return QWidget::eventFilter(object, event);
        }
    };

    class SearchableActions final : public QComboBox {
    public:
        explicit SearchableActions(QWidget *parent) : QComboBox(parent)
        {
            setEditable(true);
            setInsertPolicy(QComboBox::NoInsert);
            completer()->setCompletionMode(QCompleter::PopupCompletion);
            completer()->setCaseSensitivity(Qt::CaseInsensitive);
            completer()->setFilterMode(Qt::MatchContains);
        }

        void showPopup() override
        {
            lineEdit()->selectAll();
            completer()->setCompletionPrefix({});
            completer()->complete();
        }
    };

    class Delegate final : public QStyledItemDelegate {
    public:
        explicit Delegate(ShortcutTable *table) : QStyledItemDelegate(table), table_(table) {}

        QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override
        {
            if (index.column() == 0) {
                auto *recorder = new Recorder(parent);
                auto *delegate = const_cast<Delegate *>(this);
                connect(recorder->confirm, &QToolButton::clicked, delegate, [delegate, recorder] {
                    recorder->confirmed = true;
                    emit delegate->commitData(recorder);
                    emit delegate->closeEditor(recorder);
                });
                return recorder;
            }
            if (index.column() == 1) {
                QComboBox *combo;
                if (table_->actionProvider_) {
                    table_->actions_ = table_->actionProvider_();
                    combo = new SearchableActions(parent);
                } else {
                    combo = new QComboBox(parent);
                }
                for (const auto &action : table_->actions_) combo->addItem(action.label, action.id);
                auto *delegate = const_cast<Delegate *>(this);
                connect(combo, &QComboBox::activated, delegate, [delegate, combo] {
                    emit delegate->commitData(combo);
                    emit delegate->closeEditor(combo);
                });
                return combo;
            }
            auto *value = new QDoubleSpinBox(parent);
            value->setRange(-36000, 36000);
            value->setDecimals(2);
            return value;
        }

        void setEditorData(QWidget *editor, const QModelIndex &index) const override
        {
            if (auto *recorder = dynamic_cast<Recorder *>(editor)) {
                recorder->key->setKeySequence(QKeySequence::fromString(index.data(Qt::EditRole).toString(), QKeySequence::PortableText));
            } else if (auto *combo = qobject_cast<QComboBox *>(editor))
                combo->setCurrentIndex(combo->findData(index.data(Qt::UserRole)));
            else
                QStyledItemDelegate::setEditorData(editor, index);
        }

        void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
        {
            if (auto *recorder = dynamic_cast<Recorder *>(editor)) {
                if (!recorder->confirmed) return;
                const auto sequence = recorder->key->keySequence().toString(QKeySequence::PortableText);
                for (int row = 0; !sequence.isEmpty() && row < table_->rowCount(); ++row) {
                    if (row != index.row() && table_->item(row, 0)->text() == sequence) {
                        table_->item(row, 0)->setText({});
                        break;
                    }
                }
                model->setData(index, sequence);
            } else if (auto *combo = qobject_cast<QComboBox *>(editor)) {
                const int selected = combo->currentIndex();
                if (selected < 0 || combo->currentText() != combo->itemText(selected)) return;
                model->setItemData(index, {{Qt::UserRole, combo->currentData()}, {Qt::DisplayRole, combo->itemText(selected)}});
                table_->updateValue(index.row());
            } else {
                QStyledItemDelegate::setModelData(editor, model, index);
            }
        }

    private:
        ShortcutTable *table_;
    };

public:
    explicit ShortcutTable(QList<ShortcutAction> actions, QWidget *parent = nullptr)
        : QTableWidget(parent), actions_(std::move(actions))
    {
        setColumnCount(3);
        setHorizontalHeaderLabels({QStringLiteral("输入快捷键"), QStringLiteral("功能"), QStringLiteral("值")});
        horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        setColumnWidth(0, 145);
        setItemDelegate(new Delegate(this));
    }

    void append(const ShortcutBinding &binding)
    {
        const int row = rowCount();
        insertRow(row);
        setItem(row, 0, new QTableWidgetItem(binding.key));
        const auto action = findAction(binding.action);
        auto *mode = new QTableWidgetItem(action.label);
        mode->setData(Qt::UserRole, action.id);
        setItem(row, 1, mode);
        auto *value = new QTableWidgetItem;
        const auto amount = std::isfinite(binding.value) ? qBound(-36000.0, binding.value, 36000.0) : 0.0;
        value->setData(Qt::UserRole, qRound64(amount * 100.0) / 100.0);
        setItem(row, 2, value);
        updateValue(row);
    }

    ShortcutBinding binding(int row) const
    {
        const auto *value = item(row, 2);
        const auto amount = value->data(Qt::EditRole);
        return {item(row, 0)->text(), item(row, 1)->data(Qt::UserRole).toString(),
                amount.isValid() ? amount.toDouble() : value->data(Qt::UserRole).toDouble()};
    }

    void setActionProvider(std::function<QList<ShortcutAction>()> provider)
    {
        actionProvider_ = std::move(provider);
    }

    void setActions(QList<ShortcutAction> actions)
    {
        actions_ = std::move(actions);
        for (int row = 0; row < rowCount(); ++row)
            item(row, 1)->setText(findAction(item(row, 1)->data(Qt::UserRole).toString()).label);
    }

private:
    ShortcutAction findAction(const QString &id) const
    {
        for (const auto &action : actions_)
            if (action.id == id) return action;
        return {id, QCoreApplication::translate("PlayerWindow", "当前不可用") + " / " + id};
    }

    void updateValue(int row)
    {
        auto *value = item(row, 2);
        const auto previous = value->data(Qt::EditRole);
        if (previous.isValid()) value->setData(Qt::UserRole, previous);
        const bool takesValue = findAction(item(row, 1)->data(Qt::UserRole).toString()).takesValue;
        value->setFlags(takesValue ? Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable : Qt::NoItemFlags);
        value->setData(Qt::EditRole, takesValue ? value->data(Qt::UserRole) : QVariant());
    }

    QList<ShortcutAction> actions_;
    std::function<QList<ShortcutAction>()> actionProvider_;
};

}
