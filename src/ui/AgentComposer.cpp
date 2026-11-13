#include "ui/AgentComposer.h"
#include <QAbstractTextDocumentLayout>
#include <QHBoxLayout>
#include <QInputMethodEvent>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>

namespace vsr {
AgentComposer::AgentComposer(QWidget *parent) : QWidget(parent)
{
    setObjectName("agentComposer");setAttribute(Qt::WA_StyledBackground,true);
    auto *layout=new QVBoxLayout(this);layout->setContentsMargins(14,10,12,12);layout->setSpacing(9);
    auto *mode=modeBar_=new QHBoxLayout;auto *hint=new QLabel(QStringLiteral("Markdown"),this);hint->setObjectName("agentMarkdownHint");mode->addWidget(hint);mode->addStretch();
    previewButton_=new QPushButton(QStringLiteral("预览"),this);previewButton_->setObjectName("agentPreviewToggle");previewButton_->setCheckable(true);previewButton_->setProperty("agentControl",true);mode->addWidget(previewButton_);layout->addLayout(mode);
    auto *split=new QWidget(this);split->setObjectName("agentComposerBody");auto *body=new QHBoxLayout(split);body->setContentsMargins(0,0,0,0);body->setSpacing(10);layout->addWidget(split);
    editor_=new QPlainTextEdit(split);editor_->setObjectName("agentInput");editor_->setPlaceholderText(QStringLiteral("描述任务；/ 打开命令，@ 引用内容。Enter 发送，Ctrl+Enter 换行。"));editor_->setFixedHeight(84);editor_->installEventFilter(this);
    preview_=new QTextBrowser(split);preview_->setObjectName("agentInputPreview");preview_->setOpenLinks(false);preview_->setOpenExternalLinks(false);preview_->setMinimumWidth(160);preview_->hide();
    body->addWidget(editor_,1);body->addWidget(preview_,1);
    toolbar_=new QHBoxLayout;toolbar_->setSpacing(5);layout->addLayout(toolbar_);
    popup_=new QListWidget(this);popup_->setObjectName("agentCompletionPopup");popup_->setWindowFlags(Qt::ToolTip|Qt::FramelessWindowHint|Qt::WindowDoesNotAcceptFocus);popup_->setFocusPolicy(Qt::NoFocus);popup_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);popup_->setTextElideMode(Qt::ElideRight);popup_->setMinimumWidth(360);popup_->setMaximumWidth(620);
    connect(popup_,&QListWidget::itemClicked,this,[this]{acceptCompletion();});
    connect(previewButton_,&QPushButton::toggled,this,[this](bool visible){preview_->setVisible(visible);updatePreview();editor_->setFocus();});
    connect(editor_,&QPlainTextEdit::textChanged,this,[this]{editor_->setFixedHeight(std::clamp(24+editor_->document()->blockCount()*editor_->fontMetrics().lineSpacing(),84,190));updatePreview();updateCompletion();});
    connect(editor_,&QPlainTextEdit::cursorPositionChanged,this,&AgentComposer::updateCompletion);
}

void AgentComposer::setCompletionItems(const QJsonArray &items) { completions_=items;fillCompletion(); }

void AgentComposer::updatePreview()
{
    if(!preview_->isVisible())return;
    preview_->document()->setMarkdown(editor_->toPlainText(),QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectGitHub)|QTextDocument::MarkdownNoHTML);
    for(auto block=preview_->document()->begin();block.isValid();block=block.next()){QTextCursor cursor(block);auto format=block.blockFormat();format.setLineHeight(145,QTextBlockFormat::ProportionalHeight);format.setBottomMargin(9);format.setNonBreakableLines(false);cursor.setBlockFormat(format);}
    preview_->setFixedHeight(editor_->height());
}

void AgentComposer::updateCompletion()
{
    if(composing_)return;
    const auto cursor=editor_->textCursor();const auto before=cursor.block().text().left(cursor.positionInBlock());
    static const QRegularExpression pattern(QStringLiteral("(?:^|\\s)([/@][^\\s]*)$"));const auto match=pattern.match(before);
    if(!match.hasMatch()){token_.clear();popup_->hide();return;}
    token_=match.captured(1);tokenStart_=cursor.position()-token_.size();
    emit completionsRequested(token_);fillCompletion();
}

void AgentComposer::fillCompletion()
{
    popup_->clear();if(token_.isEmpty() || composing_ || !editor_->hasFocus()){popup_->hide();return;}
    const auto kind=token_.startsWith('/')?QStringLiteral("command"):QStringLiteral("mention");const auto query=token_.mid(1);
    for(const auto &value:completions_){const auto item=value.toObject();if(item.value("kind").toString()!=kind)continue;const auto label=item.value("label").toString(),description=item.value("description").toString();if(!query.isEmpty() && !label.contains(query,Qt::CaseInsensitive) && !description.contains(query,Qt::CaseInsensitive) && !item.value("insertText").toString().contains(query,Qt::CaseInsensitive))continue;
        auto *row=new QListWidgetItem(label+(description.isEmpty()?QString():QStringLiteral("  ·  ")+description),popup_);row->setData(Qt::UserRole,item);row->setToolTip(description);if(popup_->count()==12)break;}
    if(!popup_->count()){popup_->hide();return;}
    popup_->setCurrentRow(0);popup_->setFixedHeight(qMin(300,popup_->sizeHintForRow(0)*popup_->count()+8));const auto rect=editor_->cursorRect();const auto anchor=editor_->viewport()->mapToGlobal(rect.topLeft());popup_->move(anchor.x(),anchor.y()-popup_->height()-6);popup_->show();
}

void AgentComposer::acceptCompletion()
{
    if(!popup_->currentItem())return;
    const auto item=popup_->currentItem()->data(Qt::UserRole).toJsonObject();popup_->hide();const auto action=item.value("action").toString();
    auto cursor=editor_->textCursor();cursor.setPosition(tokenStart_);cursor.setPosition(editor_->textCursor().position(),QTextCursor::KeepAnchor);cursor.insertText(item.value("insertText").toString()+(action.isEmpty()?QStringLiteral(" "):QString()));editor_->setTextCursor(cursor);token_.clear();popup_->hide();
    if(!action.isEmpty())emit mentionActionRequested(action);
    editor_->setFocus();
}

bool AgentComposer::eventFilter(QObject *object,QEvent *event)
{
    if(object!=editor_)return QWidget::eventFilter(object,event);
    if(event->type()==QEvent::InputMethod){auto *input=static_cast<QInputMethodEvent*>(event);const bool wasComposing=composing_;composing_=!input->preeditString().isEmpty();if(composing_)popup_->hide();if(wasComposing && !composing_ && !input->commitString().isEmpty()){committing_=true;QTimer::singleShot(0,this,[this]{committing_=false;});}return false;}
    if(event->type()==QEvent::FocusOut){popup_->hide();return false;}
    if(event->type()!=QEvent::KeyPress)return QWidget::eventFilter(object,event);
    auto *key=static_cast<QKeyEvent*>(event);const bool enter=key->key()==Qt::Key_Return || key->key()==Qt::Key_Enter;
    if(composing_)return false;
    if(enter && committing_){key->accept();return true;}
    if(popup_->isVisible()){
        if(key->key()==Qt::Key_Escape){popup_->hide();return true;}
        if(key->key()==Qt::Key_Down || key->key()==Qt::Key_Up){popup_->setCurrentRow((popup_->currentRow()+(key->key()==Qt::Key_Down?1:-1)+popup_->count())%popup_->count());return true;}
        if(enter || key->key()==Qt::Key_Tab){acceptCompletion();return true;}
    }
    if(enter){if(key->modifiers().testFlag(Qt::ControlModifier) || key->modifiers().testFlag(Qt::ShiftModifier))editor_->insertPlainText("\n");else if(!key->isAutoRepeat())emit submitted();key->accept();return true;}
    return QWidget::eventFilter(object,event);
}
}
