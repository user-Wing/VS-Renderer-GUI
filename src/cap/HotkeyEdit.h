#pragma once
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QMessageBox>
#include <QPushButton>

namespace cap {
inline bool allowedHotkey(const QKeySequence &sequence)
{
    if(sequence.isEmpty())return true;
    if(sequence.count()!=1 || sequence[0].keyboardModifiers()==Qt::NoModifier)return false;
    for(auto key:{QKeySequence::Copy,QKeySequence::Paste,QKeySequence::Cut,QKeySequence::Undo,
                  QKeySequence::Redo,QKeySequence::SelectAll,QKeySequence::Save,QKeySequence::Open,
                  QKeySequence::Find,QKeySequence::Print,QKeySequence::Close,QKeySequence::Quit})
        for(const auto &reserved:QKeySequence::keyBindings(key))if(sequence==reserved)return false;
    return true;
}
class KeyRecorder final : public QKeySequenceEdit {
protected:
    void keyPressEvent(QKeyEvent *event) override {
        const int key=event->key();
        if(key==Qt::Key_Control || key==Qt::Key_Shift || key==Qt::Key_Alt || key==Qt::Key_Meta){event->accept();return;}
        setKeySequence(QKeySequence(QKeyCombination(event->modifiers(),Qt::Key(key))));event->accept();
    }
};
class HotkeyEdit final : public QWidget {
    KeyRecorder *editor_;
    QKeySequence confirmed_;
public:
    explicit HotkeyEdit(const QKeySequence &initial):confirmed_(initial) {
        auto *row=new QHBoxLayout(this);row->setContentsMargins(0,0,0,0);
        editor_=new KeyRecorder;editor_->setKeySequence(initial);row->addWidget(editor_);
        auto *accept=new QPushButton(QStringLiteral("✓"),this);accept->setToolTip(QStringLiteral("确认快捷键"));row->addWidget(accept);
        auto *remove=new QPushButton(QStringLiteral("×"),this);remove->setToolTip(QStringLiteral("移除快捷键"));row->addWidget(remove);
        connect(accept,&QPushButton::clicked,this,[this]{
            if(!allowedHotkey(editor_->keySequence())){QMessageBox::warning(this,"VS Cap",QStringLiteral("禁止使用常用系统/编辑快捷键；请选择包含 Ctrl、Alt、Shift 或 Win 的其他组合。"));return;}
            confirmed_=editor_->keySequence();
        });
        connect(remove,&QPushButton::clicked,this,[this]{confirmed_={};editor_->clear();});
    }
    QKeySequence keySequence() const {return confirmed_;}
};
}
