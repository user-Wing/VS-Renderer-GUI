#include "ui/AgentMarkdownView.h"
#include <QAbstractTextDocumentLayout>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextTable>
#include <QTimer>
#include <limits>

namespace vsr {
namespace {
constexpr int detailBlock=QTextFormat::UserProperty+1;
constexpr int bubbleBackground=QTextFormat::UserProperty+2;
QString content(const QJsonObject &message)
{
    const auto value=message.value(message.contains("displayContent")?"displayContent":"content");if(value.isString())return value.toString();QStringList parts;
    for(const auto &entry:value.toArray()){const auto block=entry.toObject();if(block.value("type")==QJsonValue("text"))parts.append(block.value("text").toString());else if(block.value("type")==QJsonValue("image_url"))parts.append(QStringLiteral("[本地图像]"));}return parts.join('\n');
}
bool failed(const QString &content)
{
    const auto result=QJsonDocument::fromJson(content.toUtf8()).object();const auto error=result.value("error");
    return result.value("ok")==QJsonValue(false) || result.value("isError")==QJsonValue(true)
        || (!error.isUndefined() && !error.isNull() && error!=QJsonValue(false) && error!=QJsonValue(""));
}
QString errorHint(const QString &content)
{
    const auto error=QJsonDocument::fromJson(content.toUtf8()).object().value("error").toString();
    return error.isEmpty()?QStringLiteral("工具返回错误"):error.simplified().left(100);
}
int nearestOccurrence(const QString &text,const QString &needle,int expected)
{
    int best=-1,distance=std::numeric_limits<int>::max();
    for(int found=text.indexOf(needle);!needle.isEmpty() && found>=0;found=text.indexOf(needle,found+1))
        if(qAbs(found-expected)<distance){best=found;distance=qAbs(found-expected);}
    return best;
}
// Keep selections and the visible paragraph stable when summary text grows.
int mappedPosition(const QString &before,const QString &after,int position)
{
    position=qBound(0,position,int(before.size()));int found=nearestOccurrence(after,before.mid(position,80),position);
    if(found<0){const int start=qMax(0,position-80);found=nearestOccurrence(after,before.mid(start,position-start),start);if(found>=0)found+=position-start;}
    return found<0?qMin(position,int(after.size())):found;
}
}
AgentMarkdownView::AgentMarkdownView(QWidget *parent):QTextBrowser(parent)
{
    setObjectName("agentTranscript");setOpenLinks(false);setOpenExternalLinks(false);setFrameShape(QFrame::NoFrame);setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto font=QTextBrowser::font();if(font.pixelSize()>0)font.setPixelSize(qMax(14,font.pixelSize()+1));else font.setPointSizeF(qMax(10.5,font.pointSizeF()+.5));setFont(font);
    connect(this,&QTextBrowser::anchorClicked,this,[this](const QUrl &url){if(url.scheme()=="process" || url.scheme()=="detail"){const auto key=url.scheme()+":"+url.path();if(expanded_.contains(key))expanded_.remove(key);else expanded_.insert(key);render(false);}else if(url.scheme()=="https" || url.scheme()=="http")QDesktopServices::openUrl(url);});
}

void AgentMarkdownView::showMessages(const QJsonArray &messages,const QString &streamText,const QJsonArray &pending,bool busy)
{
    bool continuation=messages.size()>=messages_.size();
    for(int i=0;continuation && i<messages_.size();++i)continuation=messages[i]==messages_[i];
    if(!continuation || (messages.isEmpty() && streamText.isEmpty())){expanded_.clear();setTextCursor(QTextCursor(document()));verticalScrollBar()->setValue(verticalScrollBar()->maximum());}
    messages_=messages;streamText_=streamText;pending_=pending;busy_=busy;render();
}

bool AgentMarkdownView::event(QEvent *event)
{
    const auto type=event->type();const bool result=QTextBrowser::event(event);
    if((type==QEvent::PaletteChange || type==QEvent::StyleChange || type==QEvent::Resize) && !scheduled_){scheduled_=true;QTimer::singleShot(0,this,[this]{scheduled_=false;render();});}return result;
}

void AgentMarkdownView::paintEvent(QPaintEvent *event)
{
    // The first root block anchors document coordinates to the styled viewport,
    // including its padding and current scroll position.
    const auto first=document()->begin();QTextCursor origin(first);
    const auto offset=QPointF(cursorRect(origin).topLeft())-document()->documentLayout()->blockBoundingRect(first).topLeft();
    QPainter painter(viewport());painter.setRenderHint(QPainter::Antialiasing);painter.setPen(Qt::NoPen);
    for(auto *frame:document()->rootFrame()->childFrames())if(frame->format().hasProperty(bubbleBackground)){
        // Qt locates this borderless table at its cell content origin while its
        // bounding size includes cell padding; move the painted edge outward.
        const auto padding=qobject_cast<QTextTable*>(frame)->format().cellPadding();
        const auto rect=document()->documentLayout()->frameBoundingRect(frame).translated(offset-QPointF(padding,padding));
        if(rect.intersects(viewport()->rect())){painter.setBrush(frame->format().property(bubbleBackground).value<QColor>());painter.drawRoundedRect(rect,16,16);}
    }
    painter.end();QTextBrowser::paintEvent(event);
}

void AgentMarkdownView::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)selecting_=true;QTextBrowser::mousePressEvent(event);
}

void AgentMarkdownView::mouseReleaseEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)selecting_=false;QTextBrowser::mouseReleaseEvent(event);if(renderPending_)QTimer::singleShot(0,this,[this]{render();});
}

void AgentMarkdownView::render(bool followBottom)
{
    if(selecting_){renderPending_=true;return;}renderPending_=false;
    const int position=verticalScrollBar()->value();const auto selection=textCursor();const bool selected=selection.hasSelection();const int selectionAnchor=selection.anchor(),selectionPosition=selection.position();const auto before=document()->toRawText();const auto top=cursorForPosition(QPoint(0,0));const int topPosition=top.position(),topOffset=cursorRect(top).top();
    const bool bottom=followBottom && verticalScrollBar()->maximum()-position<48 && !selected;
    const bool dark=palette().color(QPalette::Text).lightness()>128;const QColor text=dark?QColor("#f0f2f6"):QColor("#202733"),code=dark?QColor("#242b35"):QColor("#edf1f7");
    document()->clear();document()->setDefaultFont(font());document()->setDocumentMargin(12);auto option=document()->defaultTextOption();option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);document()->setDefaultTextOption(option);
    QTextCursor cursor(document());const auto markdown=QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectGitHub)|QTextDocument::MarkdownNoHTML;
    const auto append=[&](const QString &content){cursor.insertMarkdown(content,markdown);cursor.movePosition(QTextCursor::End);cursor.insertBlock(QTextBlockFormat());cursor.setCharFormat({});};
    const auto bubble=[&](const QString &content,bool waiting){QTextTableFormat table;table.setAlignment(Qt::AlignRight);table.setWidth(QTextLength(QTextLength::FixedLength,qMin(qRound(viewport()->width()*.82),qMax(180,fontMetrics().horizontalAdvance(content.left(1000).simplified())+32))));table.setBorder(0);table.setCellPadding(13);table.setCellSpacing(0);table.setProperty(bubbleBackground,waiting?QColor("#37536c"):QColor("#224e86"));auto *frame=cursor.insertTable(1,1,table);auto body=frame->cellAt(0,0).firstCursorPosition();body.insertMarkdown(content+(waiting?QStringLiteral("\n\n*等待 Agent 接收*"):QString()),markdown);auto end=frame->cellAt(0,0).lastCursorPosition();body=frame->cellAt(0,0).firstCursorPosition();body.setPosition(end.position(),QTextCursor::KeepAnchor);QTextCharFormat white;white.setForeground(QColor("#f7fbff"));body.mergeCharFormat(white);cursor.movePosition(QTextCursor::End);cursor.insertBlock();cursor.insertBlock();};
    const auto control=[&](const QString &label,const QString &key,bool bold=false){QTextCharFormat format;format.setAnchor(true);format.setAnchorHref(key);format.setForeground(text);format.setFontWeight(bold?QFont::DemiBold:QFont::Normal);cursor.insertText(label,format);cursor.insertBlock(QTextBlockFormat());cursor.setCharFormat({});};
    const auto details=[&](const QString &value){
        const auto json=QJsonDocument::fromJson(value.toUtf8());QString body=(json.isObject() || json.isArray())?QString::fromUtf8(json.toJson(QJsonDocument::Indented)):value;const bool truncated=body.size()>16000;
        QTextTableFormat format;format.setWidth(QTextLength(QTextLength::PercentageLength,100));format.setBorder(1);format.setCellPadding(11);format.setBackground(code);auto *table=cursor.insertTable(1,1,format);QTextTableCellFormat background;background.setBackground(code);table->cellAt(0,0).setFormat(background);
        auto cell=table->cellAt(0,0).firstCursorPosition();QTextBlockFormat compact;compact.setProperty(detailBlock,true);cell.setBlockFormat(compact);QTextCharFormat monospace;monospace.setFontFamilies({"Consolas"});monospace.setForeground(text);cell.insertText(body.left(16000),monospace);
        cursor.movePosition(QTextCursor::End);cursor.insertBlock(QTextBlockFormat());cursor.setCharFormat({});if(truncated)append(QStringLiteral("*显示已截断；完整内容保留在会话导出中。*"));
    };
    struct Turn { int user=-1;QList<int> rows; };QList<Turn> turns;
    for(int row=0;row<messages_.size();++row){const auto message=messages_[row].toObject();const auto role=message.value("role").toString();if(role=="system")continue;
        if(role=="user" && message.value("source").toString().isEmpty()){turns.append(Turn{row,{}});continue;}
        if(turns.isEmpty())turns.append(Turn{});turns.last().rows.append(row);
    }
    for(int turnIndex=0;turnIndex<turns.size();++turnIndex){
        const auto &turn=turns[turnIndex];if(turn.user>=0)bubble(content(messages_[turn.user].toObject()),false);
        int finalRow=-1,lastAssistant=-1,calls=0,errors=0,notifications=0;QString hint;QHash<QString,int> results;QSet<QString> callIds;
        for(int row:turn.rows){const auto message=messages_[row].toObject();const auto role=message.value("role").toString();
            if(role=="assistant"){lastAssistant=row;for(const auto &item:message.value("tool_calls").toArray()){++calls;callIds.insert(item.toObject().value("id").toString());}}
            if(role=="tool"){results.insert(message.value("tool_call_id").toString(),row);if(failed(content(message))){++errors;if(hint.isEmpty())hint=errorHint(content(message));}}
            if(role=="user")++notifications;
        }
        if(lastAssistant>=0 && messages_[lastAssistant].toObject().value("tool_calls").toArray().isEmpty())finalRow=lastAssistant;
        for(auto it=results.cbegin();it!=results.cend();++it)if(!callIds.contains(it.key()))++calls;
        bool hasProcess=false;for(int row:turn.rows)if(row!=finalRow){const auto message=messages_[row].toObject();hasProcess|=!content(message).isEmpty() || !message.value("tool_calls").toArray().isEmpty();}
        if(hasProcess){
            const auto key="process:"+QString::number(turn.user);const bool expanded=expanded_.contains(key);
            QString label=(expanded?QStringLiteral("▾ "):QStringLiteral("▸ "))+QStringLiteral("执行过程 · %1 次工具调用").arg(calls);
            if(notifications)label+=QStringLiteral(" · %1 条任务通知").arg(notifications);
            if(errors)label+=QStringLiteral(" · %1 项错误：%2").arg(errors).arg(hint);
            if(busy_ && turnIndex==turns.size()-1)label+=QStringLiteral(" · 正在处理");
            control(label,key,true);
            if(expanded)for(int row:turn.rows){
                if(row==finalRow)continue;const auto message=messages_[row].toObject();const auto role=message.value("role").toString();
                if(role=="user"){const auto noticeKey="detail:"+QString::number(row)+"-notice";control((expanded_.contains(noticeKey)?QStringLiteral("▾ "):QStringLiteral("▸ "))+QStringLiteral("来自 ")+message.value("source").toString()+QStringLiteral(" · 通知内容"),noticeKey);if(expanded_.contains(noticeKey))append(content(message));continue;}
                if(role=="assistant"){
                    if(!content(message).isEmpty())append(content(message));
                    const auto tools=message.value("tool_calls").toArray();for(int i=0;i<tools.size();++i){
                        const auto call=tools[i].toObject(),function=call.value("function").toObject();const auto id=call.value("id").toString();const int resultRow=results.value(id,-1);const bool failure=resultRow>=0 && failed(content(messages_[resultRow].toObject()));
                        append(QStringLiteral("**%1 · %2**").arg(resultRow<0?QStringLiteral("结果尚未返回"):failure?QStringLiteral("执行失败"):QStringLiteral("已完成"),function.value("name").toString()));
                        const auto argsKey=QString("detail:%1-%2-args").arg(row).arg(i);control((expanded_.contains(argsKey)?QStringLiteral("▾ "):QStringLiteral("▸ "))+QStringLiteral("参数"),argsKey);if(expanded_.contains(argsKey))details(function.value("arguments").toString());
                        if(resultRow>=0){const auto resultKey=QString("detail:%1-%2-result").arg(row).arg(i);control((expanded_.contains(resultKey)?QStringLiteral("▾ "):QStringLiteral("▸ "))+QStringLiteral("结果"),resultKey);if(expanded_.contains(resultKey))details(content(messages_[resultRow].toObject()));}
                    }
                }else if(role=="tool" && !callIds.contains(message.value("tool_call_id").toString())){
                    const auto resultKey="detail:"+QString::number(row)+"-result";control((failed(content(message))?QStringLiteral("执行失败"):QStringLiteral("工具结果"))+QStringLiteral(" · 查看结果"),resultKey);if(expanded_.contains(resultKey))details(content(message));
                }
            }
        }
        if(finalRow>=0 && !content(messages_[finalRow].toObject()).isEmpty())append(content(messages_[finalRow].toObject()));
    }
    if(!streamText_.isEmpty())append(streamText_);
    for(int i=0;i<pending_.size();++i){const auto entry=pending_[i].toObject();const auto source=entry.value("source").toString();if(source.isEmpty())bubble(content(entry),true);else{const auto key="process:pending-"+entry.value("id").toString(QString::number(i));control((expanded_.contains(key)?QStringLiteral("▾ "):QStringLiteral("▸ "))+QStringLiteral("来自 %1 · 等待接收").arg(source),key);if(expanded_.contains(key))append(content(entry));}}
    if(messages_.isEmpty() && pending_.isEmpty() && streamText_.isEmpty())append(QStringLiteral("## 让 Agent 帮你完成工作\n\n描述视频处理任务，或输入 **/** 选择命令、**@** 引用技能和工作区。\n\n可查询滤镜、调整处理链、导入导出项目与预设，或让 Agent 自动安排独立子任务。"));
    struct Fence { int first=-1,last=-1;QColor background; };QList<Fence> fences;Fence fence;
    for(auto block=document()->begin();block.isValid();block=block.next()){
        QTextCursor c(block);auto format=block.blockFormat();const bool fenced=format.hasProperty(QTextFormat::BlockCodeFence) || format.hasProperty(QTextFormat::BlockCodeLanguage);
        const bool compact=fenced || format.boolProperty(detailBlock);format.setLineHeight(compact?135:155,QTextBlockFormat::ProportionalHeight);format.setTopMargin(compact?0:format.headingLevel()?15:2);format.setBottomMargin(compact?0:format.headingLevel()?11:c.currentList()?7:12);format.setNonBreakableLines(false);
        if(fenced){format.clearBackground();format.setLeftMargin(0);format.setRightMargin(0);QTextCharFormat codeFont;codeFont.setFont(font());codeFont.setFontFamilies({"Consolas"});c.select(QTextCursor::BlockUnderCursor);c.mergeCharFormat(codeFont);if(fence.first<0){fence.first=block.position();fence.background=c.currentTable() && c.currentTable()->format().border()==0?QColor("#1d3d66"):code;}if(!block.text().isEmpty())fence.last=block.position()+block.length()-1;}
        else if(fence.first>=0){if(fence.last>=fence.first)fences.append(fence);fence={};}
        c.setBlockFormat(format);if(!c.currentTable()){c.select(QTextCursor::BlockUnderCursor);QTextCharFormat color;color.setForeground(text);c.mergeCharFormat(color);}
    }
    if(fence.first>=0 && fence.last>=fence.first)fences.append(fence);
    for(auto it=fences.crbegin();it!=fences.crend();++it){QTextCursor codeCursor(document());codeCursor.setPosition(it->first);codeCursor.setPosition(it->last,QTextCursor::KeepAnchor);QTextFrameFormat format;format.setBackground(it->background);format.setPadding(11);format.setTopMargin(8);format.setBottomMargin(14);format.setBorder(1);format.setBorderBrush(dark?QColor("#63758d"):QColor("#9ba9bc"));codeCursor.insertFrame(format);}
    const auto tables=document()->rootFrame()->childFrames();for(auto *frame:tables)if(auto *table=qobject_cast<QTextTable*>(frame)){auto format=table->format();if(format.border()>0){format.setCellPadding(8);format.setCellSpacing(0);format.setBorder(1);format.setBorderBrush(dark?QColor("#73829a"):QColor("#8b99ad"));table->setFormat(format);}}
    const auto after=document()->toRawText();
    if(selected){const int start=qMin(selectionAnchor,selectionPosition),length=qAbs(selectionAnchor-selectionPosition);const int found=nearestOccurrence(after,before.mid(start,length),start);QTextCursor restored(document());if(found>=0){restored.setPosition(found+(selectionAnchor>selectionPosition?length:0));restored.setPosition(found+(selectionPosition>selectionAnchor?length:0),QTextCursor::KeepAnchor);}else restored.setPosition(mappedPosition(before,after,selectionPosition));setTextCursor(restored);}
    if(bottom)verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    else {QTextCursor restored(document());restored.setPosition(mappedPosition(before,after,topPosition));verticalScrollBar()->setValue(verticalScrollBar()->value()+cursorRect(restored).top()-topOffset);}
}
}
