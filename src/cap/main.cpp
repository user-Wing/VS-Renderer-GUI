#include <QtWidgets>
#include <QtGui/qscreen_platform.h>
#include <QAbstractNativeEventFilter>
#include <QProcess>
#include <QSettings>
#include <QImageReader>
#include <QStandardPaths>
#include <QPointer>
#include <QDateTime>
#include <QSaveFile>
#include <QAbstractTextDocumentLayout>
#include <windows.h>
#include <dwmapi.h>
#include <functional>
#include <cmath>
#include <cstring>
#include "DesktopCapture.h"
#include "UiAccess.h"

static QString defaultDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/VS Cap";
}

using cap::savePng;

static void copyImage(const QImage &image, unsigned attempt=0, unsigned request=0)
{
    static unsigned latest=0;
    if(!attempt)request=++latest;
    if(request!=latest)return;
    auto *mime=new QMimeData;
    auto preview=cap::preview(image);if(image.text("capHdr")=="scRGB-FP16")preview.setText("capHdr",{});mime->setImageData(preview);
    QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);
    preview.save(&buffer,"PNG");mime->setData("image/png",png);
    if(image.text("capHdr")=="scRGB-FP16") {
        QByteArray raw;QDataStream stream(&raw,QIODevice::WriteOnly);
        stream<<qint32(image.width())<<qint32(image.height());
        for(int y=0;y<image.height();++y)raw.append(reinterpret_cast<const char*>(image.constScanLine(y)),image.width()*16);
        mime->setData("application/x-vscap-scrgb",raw);
    }
    QApplication::clipboard()->setMimeData(mime);
    // Clipboard viewers can briefly hold OpenClipboard while processing a copy.
    if(!QApplication::clipboard()->ownsClipboard() && attempt<4)
        QTimer::singleShot(100,qApp,[image,attempt,request]{copyImage(image,attempt+1,request);});
}

class CaptureTextBox final : public QWidget {
    QRect bounds_;
    QPoint dragOrigin_, startPosition_;
    bool dragging_ = false;
public:
    QTextEdit *editor;
    QColor ink;
    std::function<void()> selected, changed, beforeMove, moved, undo, cancel;
    CaptureTextBox(QWidget *parent, const QRect &bounds, const QFont &font, const QColor &color)
        : QWidget(parent), bounds_(bounds), ink(color) {
        setObjectName("capTextBox");setCursor(Qt::SizeAllCursor);
        setToolTip(QStringLiteral("框内编辑文字，拖动虚线边缘移动"));
        editor=new QTextEdit(this);editor->setObjectName("capTextEditor");
        editor->setAcceptRichText(false);editor->setUndoRedoEnabled(false);
        editor->setFrameShape(QFrame::NoFrame);editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        editor->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);editor->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        editor->document()->setDocumentMargin(0);editor->setFont(font);
        editor->setStyleSheet("QTextEdit{background:transparent;border:0;padding:0;}");
        editor->viewport()->setAutoFillBackground(false);editor->installEventFilter(this);
        connect(editor->document(),&QTextDocument::contentsChanged,this,[this]{fit();if(changed)changed();});
        setInk(color);fit();
    }
    void setInk(const QColor &color) {
        ink=color;auto palette=editor->palette();palette.setColor(QPalette::Text,color);editor->setPalette(palette);
    }
    void fit() {
        auto *document=editor->document();document->setTextWidth(-1);
        const int boxWidth=qMin(bounds_.width(),qMax(90,int(std::ceil(document->idealWidth()))+12));
        document->setTextWidth(qMax(1,boxWidth-12));
        const int boxHeight=qMin(bounds_.height(),qMax(36,int(std::ceil(document->size().height()))+12));
        resize(boxWidth,boxHeight);editor->setGeometry(rect().adjusted(6,6,-6,-6));moveWithin(pos());
    }
    void moveWithin(const QPoint &point) {
        const auto old=geometry();move(qBound(bounds_.left(),point.x(),bounds_.right()-width()+1),
             qBound(bounds_.top(),point.y(),bounds_.bottom()-height()+1));
        parentWidget()->update(old.united(geometry()).adjusted(-3,-3,3,3));
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        if(object==editor){
            if(event->type()==QEvent::FocusIn && selected)selected();
            if(event->type()==QEvent::ShortcutOverride && static_cast<QKeyEvent*>(event)->matches(QKeySequence::Undo)){event->accept();return true;}
            if(event->type()==QEvent::KeyPress){
                auto *key=static_cast<QKeyEvent*>(event);
                if(key->matches(QKeySequence::Undo)){if(undo)undo();return true;}
                if(key->key()==Qt::Key_Escape){if(cancel)cancel();return true;}
            }
        }
        return QWidget::eventFilter(object,event);
    }
    void mousePressEvent(QMouseEvent *event) override {
        if(event->button()!=Qt::LeftButton)return;
        if(selected)selected();dragOrigin_=event->globalPosition().toPoint();startPosition_=pos();dragging_=false;
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if(!event->buttons().testFlag(Qt::LeftButton))return;
        const auto delta=event->globalPosition().toPoint()-dragOrigin_;if(delta.isNull())return;
        if(!dragging_){if(beforeMove)beforeMove();dragging_=true;}
        moveWithin(startPosition_+delta);
    }
    void mouseReleaseEvent(QMouseEvent *) override {if(dragging_ && moved)moved();dragging_=false;}
};

class CaptureOverlay final : public QWidget {
    QImage canvas_, display_;
    bool displayDirty_ = true, embedded_ = false, constrainShape_ = false;
    std::function<void()> positionExternalBar_;
    QRect selection_, hoveredWindow_, pressedWindow_;
    QList<QRect> windows_;
    QPoint origin_, last_;
    bool selecting_ = true, dragging_ = false;
    bool movingSelection_ = false;
    int resizeEdges_ = 0;
    QRect startSelection_;
    enum Tool { Selection, Pen, Rect, Ellipse, Text, Line, Pan } tool_ = Selection;
    QColor color_ = Qt::red;
    int lineMode_ = 0;
    int penWidth_ = 3;
    bool filled_ = false;
    QComboBox *options_ = nullptr;
    QSpinBox *penWidthInput_ = nullptr;
    QToolButton *strokeButton_ = nullptr, *fillButton_ = nullptr;
    QFontComboBox *fontFamily_ = nullptr;
    QSpinBox *fontSize_ = nullptr;
    QToolButton *bold_ = nullptr, *italic_ = nullptr;
    QWidget *fontOptions_ = nullptr;
    QButtonGroup *tools_ = nullptr;
    QFont textFont_;
    QList<CaptureTextBox*> texts_;
    QPointer<CaptureTextBox> activeText_, typingText_;
    struct TextState { QString text; QFont font; QColor ink; QPoint position; };
    QHash<CaptureTextBox*,TextState> textStates_;
    bool restoringText_ = false;
    QPushButton *colorButton_ = nullptr, *undoButton_ = nullptr;
    struct Edit { QRect area; QImage pixels; QPointer<CaptureTextBox> text; TextState before; bool created=false; };
    QList<Edit> history_;
    QWidget *bar_ = nullptr;
    std::function<bool(QImage, bool, bool, bool)> output_;
public:
    CaptureOverlay(QScreen *screen, QImage shot, std::function<bool(QImage,bool,bool,bool)> output, bool embedded=false)
        : canvas_(std::move(shot)), embedded_(embedded), output_(std::move(output)) {
        setWindowFlags(embedded_?Qt::Widget:Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_DeleteOnClose,!embedded_);
        setAttribute(Qt::WA_OpaquePaintEvent);
        setMouseTracking(true);
        setCursor(Qt::CrossCursor);
        if(embedded_){resize(canvas_.size());selecting_=false;selection_=rect();tool_=Pen;}
        else {
            setGeometry(screen->geometry());
            MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
            if(const auto *native=screen->nativeInterface<QNativeInterface::QWindowsScreen>())GetMonitorInfoW(native->handle(),&monitor);
            struct Scan { CaptureOverlay *self; QScreen *screen; RECT monitor; } scan{this,screen,monitor.rcMonitor};
            EnumWindows([](HWND hwnd,LPARAM param)->BOOL {
                auto *data=reinterpret_cast<Scan*>(param);
                if(!IsWindowVisible(hwnd) || IsIconic(hwnd))return TRUE;
                BOOL cloaked=FALSE;
                if(SUCCEEDED(DwmGetWindowAttribute(hwnd,DWMWA_CLOAKED,&cloaked,sizeof(cloaked))) && cloaked)return TRUE;
                RECT native{};
                if(FAILED(DwmGetWindowAttribute(hwnd,DWMWA_EXTENDED_FRAME_BOUNDS,&native,sizeof(native))))
                    if(!GetWindowRect(hwnd,&native))return TRUE;
                const double dpr=data->screen->devicePixelRatio();
                QRect window(qRound((native.left-data->monitor.left)/dpr),qRound((native.top-data->monitor.top)/dpr),
                             qRound((native.right-native.left)/dpr),qRound((native.bottom-native.top)/dpr));
                window=window.intersected(data->self->rect());
                if(window.width()>24 && window.height()>24 && window!=data->self->rect())data->self->windows_.append(window);
                return TRUE;
            },reinterpret_cast<LPARAM>(&scan));
            const auto pos=QCursor::pos()-geometry().topLeft();
            for(const auto &window:windows_)if(window.contains(pos)){hoveredWindow_=window;break;}
        }
        bar_ = new QWidget(this);bar_->setObjectName("capToolBar");bar_->setCursor(Qt::ArrowCursor);
        qApp->installEventFilter(this);
        auto *layout = new QHBoxLayout(bar_);
        layout->setContentsMargins(4,4,4,4);
        layout->setSpacing(3);
        auto add = [this, layout](const QString &name, std::function<void()> fn) {
            auto *button = new QPushButton(name, bar_);
            button->setMinimumWidth(35);
            QObject::connect(button, &QPushButton::clicked, bar_, std::move(fn));
            layout->addWidget(button);
            return button;
        };
        colorButton_=add(QStringLiteral("颜色"), [this] {
            const auto c = QColorDialog::getColor(color_, this);
            if(c.isValid()){if(tool_==Text && activeText_)rememberText(activeText_);color_=c;if(tool_==Text && activeText_){activeText_->setInk(c);textStates_[activeText_]=textState(activeText_);}updateColor();}
        });
        options_=new QComboBox(bar_);options_->setObjectName("capToolOptions");layout->addWidget(options_);
        penWidthInput_=new QSpinBox(bar_);penWidthInput_->setObjectName("capPenWidth");penWidthInput_->setRange(1,512);penWidthInput_->setPrefix(QStringLiteral("粗细 "));penWidthInput_->setSuffix(" px");penWidthInput_->setValue(penWidth_);layout->addWidget(penWidthInput_);
        connect(penWidthInput_,&QSpinBox::valueChanged,this,[this](int width){penWidth_=width;});
        textFont_.setPointSize(18);
        fontOptions_=new QWidget(bar_);auto *fonts=new QHBoxLayout(fontOptions_);fonts->setContentsMargins(0,0,0,0);fonts->setSpacing(3);
        fontFamily_=new QFontComboBox(fontOptions_);fontFamily_->setObjectName("capFontFamily");fontFamily_->setEditable(true);fontFamily_->setFixedWidth(160);
        fontFamily_->completer()->setCaseSensitivity(Qt::CaseInsensitive);fontFamily_->completer()->setCompletionMode(QCompleter::PopupCompletion);
        fontFamily_->setToolTip(QStringLiteral("字体（可输入名称并自动补全）"));fonts->addWidget(fontFamily_);
        fontSize_=new QSpinBox(fontOptions_);fontSize_->setObjectName("capFontSize");fontSize_->setRange(1,512);fontSize_->setPrefix(QStringLiteral("字号 "));fontSize_->setValue(18);fonts->addWidget(fontSize_);
        const auto styleButton=[&](const QString &name,const QString &tip){auto *button=new QToolButton(fontOptions_);button->setText(name);button->setToolTip(tip);button->setCheckable(true);fonts->addWidget(button);return button;};
        bold_=styleButton("B",QStringLiteral("加粗"));bold_->setObjectName("capFontBold");italic_=styleButton("I",QStringLiteral("斜体"));italic_->setObjectName("capFontItalic");layout->addWidget(fontOptions_);
        connect(fontFamily_,&QFontComboBox::currentFontChanged,this,[this](const QFont &font){textFont_.setFamily(font.family());applyTextFont();});
        connect(fontSize_,&QSpinBox::valueChanged,this,[this](int size){textFont_.setPointSize(size);applyTextFont();});
        connect(bold_,&QToolButton::toggled,this,[this](bool checked){textFont_.setBold(checked);applyTextFont();});
        connect(italic_,&QToolButton::toggled,this,[this](bool checked){textFont_.setItalic(checked);applyTextFont();});
        tools_=new QButtonGroup(this);tools_->setExclusive(true);
        const auto tool=[&](const QString &name,Tool value){
            auto *button=add(name,[this,value]{typingText_=nullptr;tool_=value;updateOptions();});
            button->setCheckable(true);button->setChecked(value==tool_);tools_->addButton(button,value);
        };
        if(!embedded_)tool(QStringLiteral("选区"),Selection);
        if(embedded_)tool(QStringLiteral("拖动"),Pan);
        tool(QStringLiteral("画笔"),Pen);
        strokeButton_=new QToolButton(bar_);strokeButton_->setObjectName("capShapeWidth");strokeButton_->setText(QStringLiteral("●"));strokeButton_->setToolTip(QStringLiteral("滚轮调节矩形/圆形线条粗细"));strokeButton_->installEventFilter(this);layout->addWidget(strokeButton_);
        fillButton_=new QToolButton(bar_);fillButton_->setObjectName("capShapeFill");fillButton_->setText(QStringLiteral("■"));fillButton_->setToolTip(QStringLiteral("纯色填充"));fillButton_->setCheckable(true);layout->addWidget(fillButton_);
        connect(fillButton_,&QToolButton::toggled,this,[this](bool enabled){filled_=enabled;});
        tool(QStringLiteral("矩形"),Rect);
        tool(QStringLiteral("圆形"),Ellipse);tool(QStringLiteral("线"),Line);tool(QStringLiteral("文字"),Text);
        tools_->button(Rect)->setToolTip(QStringLiteral("按住 Ctrl 画正方形"));tools_->button(Ellipse)->setToolTip(QStringLiteral("按住 Ctrl 画圆形"));
        connect(options_,&QComboBox::currentIndexChanged,this,[this](int index){if(tool_==Line)lineMode_=index;else penWidth_=options_->currentData().toInt();});
        undoButton_=add(QStringLiteral("撤销"),[this]{undo();});undoButton_->setEnabled(false);
        if(!embedded_){auto *undoShortcut=new QShortcut(QKeySequence::Undo,this);connect(undoShortcut,&QShortcut::activated,this,[this]{undo();});}
        add(QStringLiteral("保存"), [this] {
            QSettings preferences(QCoreApplication::applicationDirPath()+"/vs-cap.ini",QSettings::IniFormat);
            deliver(false,preferences.value("save/format","png").toString()=="avif");
        })->setToolTip(QStringLiteral("使用设置中的默认格式保存"));
        add(QStringLiteral("PNG"), [this] {deliver(false,false);});
        add(QStringLiteral("AVIF"), [this] {deliver(false,true);});
        add(QStringLiteral("贴图"), [this] {deliver(true,false);});
        add(QStringLiteral("✓"), [this] {if(embedded_)bar_->hide();else deliver(false,false,true);})->setToolTip(embedded_?QStringLiteral("隐藏工具栏"):QStringLiteral("复制到剪贴板"));
        bar_->setStyleSheet("QWidget{background:#252831;} QPushButton,QComboBox,QToolButton{color:white;background:#3b414e;padding:5px;border:1px solid transparent;} QPushButton:checked,QToolButton:checked{border:2px solid #78baff;background:#24496a;} QPushButton:disabled{color:#858585;} QSpinBox{color:white;background:#3b414e;padding:4px 24px 4px 5px;border:1px solid transparent;min-height:24px;} QSpinBox::up-button{subcontrol-origin:border;subcontrol-position:top right;width:20px;height:16px;} QSpinBox::down-button{subcontrol-origin:border;subcontrol-position:bottom right;width:20px;height:16px;}");
        updateColor();updateOptions();
        bar_->adjustSize();
        bar_->hide();
        if(!embedded_){show();raise();activateWindow();}
    }
    QWidget *takeToolBar(QWidget *parent,std::function<void()> position) {
        positionExternalBar_=std::move(position);bar_->setParent(parent,Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);bar_->hide();return bar_;
    }
    void exportImage(bool pin,bool avif,bool copy=false){deliver(pin,avif,copy);}
    void undoEdit(){undo();}
    bool isPanTool() const { return tool_==Pan; }
private:
    TextState textState(CaptureTextBox *box) const {return {box->editor->toPlainText(),box->editor->font(),box->ink,box->pos()};}
    void selectText(CaptureTextBox *box) {
        if(activeText_!=box)typingText_=nullptr;
        activeText_=box;textFont_=box->editor->font();color_=box->ink;tool_=Text;tools_->button(Text)->setChecked(true);
        const QSignalBlocker family(fontFamily_),size(fontSize_),bold(bold_),italic(italic_);
        fontFamily_->setCurrentFont(textFont_);fontSize_->setValue(textFont_.pointSize());bold_->setChecked(textFont_.bold());italic_->setChecked(textFont_.italic());
        updateColor();updateOptions();
    }
    void rememberText(CaptureTextBox *box,bool typing=false) {
        if(restoringText_)return;
        if(!typing || typingText_!=box){Edit edit;edit.text=box;edit.before=textStates_.value(box);history_.append(edit);trimHistory();}
        typingText_=typing?box:nullptr;undoButton_->setEnabled(true);
    }
    void applyTextFont() {
        if(!activeText_ || tool_!=Text)return;
        rememberText(activeText_);restoringText_=true;activeText_->editor->setFont(textFont_);activeText_->fit();restoringText_=false;
        textStates_[activeText_]=textState(activeText_);
    }
    void addText(const QPoint &point) {
        auto *box=new CaptureTextBox(this,rect(),textFont_,color_);box->moveWithin(point);texts_.append(box);textStates_[box]=textState(box);
        box->selected=[this,box]{selectText(box);};box->undo=[this]{undo();};
        box->cancel=[this]{if(embedded_)bar_->hide();else close();};
        box->changed=[this,box]{rememberText(box,true);textStates_[box]=textState(box);};
        box->beforeMove=[this,box]{rememberText(box);};
        box->moved=[this,box]{textStates_[box]=textState(box);};
        Edit edit;edit.text=box;edit.created=true;history_.append(edit);trimHistory();typingText_=nullptr;undoButton_->setEnabled(true);
        box->show();box->raise();bar_->raise();selectText(box);box->editor->setFocus();
    }
    void updateColor(){colorButton_->setStyleSheet(QString("border-bottom:3px solid %1;").arg(color_.name()));}
    void updateOptions(){
        for(auto *box:texts_)box->setAttribute(Qt::WA_TransparentForMouseEvents,tool_==Selection || tool_==Pan);
        const QSignalBlocker block(options_);options_->clear();
        if(tool_==Line){options_->addItems({QStringLiteral("直线"),QStringLiteral("箭头"),QStringLiteral("虚线")});options_->setCurrentIndex(lineMode_);}
        else{for(int width:{2,3,5,8})options_->addItem(QStringLiteral("%1 px").arg(width),width);if(options_->findData(penWidth_)<0)options_->addItem(QStringLiteral("%1 px").arg(penWidth_),penWidth_);options_->setCurrentIndex(options_->findData(penWidth_));}
        const QSignalBlocker width(penWidthInput_);penWidthInput_->setValue(penWidth_);
        options_->setVisible(tool_!=Selection && tool_!=Text && tool_!=Pen && tool_!=Pan);penWidthInput_->setVisible(tool_==Pen || tool_==Line);fontOptions_->setVisible(tool_==Text);
        strokeButton_->setVisible(tool_==Rect || tool_==Ellipse);fillButton_->setVisible(tool_==Rect || tool_==Ellipse);positionBar();
    }
    void remember(const QRect &area){
        const auto pixels=pixelRect(area).intersected(canvas_.rect());if(pixels.isEmpty())return;
        Edit edit;edit.area=pixels;edit.pixels=canvas_.copy(pixels);history_.append(edit);typingText_=nullptr;trimHistory();
        undoButton_->setEnabled(true);
    }
    void trimHistory(){
        qint64 bytes=0;for(const auto &edit:history_)bytes+=edit.pixels.sizeInBytes();
        while(history_.size()>1 && (history_.size()>32 || bytes>128*1024*1024)){bytes-=history_.first().pixels.sizeInBytes();history_.removeFirst();}
    }
    void undo(){
        if(dragging_ || history_.isEmpty())return;
        const auto edit=history_.takeLast();typingText_=nullptr;
        if(edit.text){
            auto *box=edit.text.data();restoringText_=true;
            if(edit.created){texts_.removeOne(box);textStates_.remove(box);if(activeText_==box)activeText_=nullptr;box->hide();box->deleteLater();setFocus();}
            else{box->editor->setPlainText(edit.before.text);box->editor->setFont(edit.before.font);box->setInk(edit.before.ink);box->fit();box->moveWithin(edit.before.position);textStates_[box]=textState(box);selectText(box);}
            restoringText_=false;
        }else{QPainter painter(&canvas_);painter.setCompositionMode(QPainter::CompositionMode_Source);painter.drawImage(edit.area.topLeft(),edit.pixels);displayDirty_=true;}
        undoButton_->setEnabled(!history_.isEmpty());update();
    }
    QRect shapeRect() const {
        QPoint to=last_;
        if(constrainShape_ && (tool_==Rect || tool_==Ellipse)){
            const auto delta=to-origin_;const int size=qMin(qAbs(delta.x()),qAbs(delta.y()));
            to=origin_+QPoint(delta.x()<0?-size:size,delta.y()<0?-size:size);
        }
        return QRect(origin_,to).normalized();
    }
    void drawLine(QPainter &p,const QPoint &from,const QPoint &to){
        p.drawLine(from,to);if(lineMode_!=1 || from==to)return;
        const QPointF direction=QPointF(to-from);const double length=std::hypot(direction.x(),direction.y());
        const QPointF unit=direction/length,normal(-unit.y(),unit.x());const double size=qMax(12,penWidth_*4);
        p.drawLine(QPointF(to),QPointF(to)-unit*size+normal*size*.5);p.drawLine(QPointF(to),QPointF(to)-unit*size-normal*size*.5);
    }
    void positionBar() {
        bar_->adjustSize();if(positionExternalBar_){positionExternalBar_();return;}
        const int x = qBound(0, selection_.right() - bar_->width(), qMax(0,width() - bar_->width()));
        const int y = qBound(0, selection_.bottom() + 8, height() - bar_->height());
        bar_->move(x,y);
    }
    void deliver(bool pin, bool avif, bool copy = false) {
        if(selection_.isEmpty()) return;
        QImage composite=canvas_;
        if(!texts_.isEmpty()){
            QPainter painter(&composite);painter.scale(double(canvas_.width())/width(),double(canvas_.height())/height());
            for(auto *box:texts_){
                painter.save();painter.translate(box->pos()+QPoint(6,6));painter.setClipRect(box->editor->viewport()->rect(),Qt::IntersectClip);
                QAbstractTextDocumentLayout::PaintContext context;context.palette.setColor(QPalette::Text,annotationInk(box->ink));
                box->editor->document()->documentLayout()->draw(&painter,context);painter.restore();
            }
        }
        const QImage crop = composite.copy(pixelRect(selection_.intersected(rect())));
        if(copy) copyImage(crop);
        const bool accepted=embedded_ && copy?true:output_(crop,pin,avif,copy);
        if(!embedded_ && accepted)close();
    }
protected:
    bool eventFilter(QObject *object,QEvent *event) override {
        auto *widget=qobject_cast<QWidget*>(object);
        if(object==strokeButton_ && event->type()==QEvent::Wheel){
            const auto steps=static_cast<QWheelEvent*>(event)->angleDelta().y()/120;
            penWidth_=qBound(1,penWidth_+steps,512);penWidthInput_->setValue(penWidth_);updateOptions();event->accept();return true;
        }
        if(event->type()==QEvent::MouseButtonPress && widget && (widget==this || isAncestorOf(widget) || widget==bar_ || bar_->isAncestorOf(widget))){
            const auto boxes=texts_;
            for(auto *box:boxes)if(box->editor->toPlainText().isEmpty() && widget!=box && !box->isAncestorOf(widget)){
                history_.removeIf([box](const Edit &edit){return edit.text==box;});texts_.removeOne(box);textStates_.remove(box);
                if(activeText_==box)activeText_=nullptr;if(typingText_==box)typingText_=nullptr;
                update(box->geometry());box->hide();box->deleteLater();undoButton_->setEnabled(!history_.isEmpty());
            }
        }
        return QWidget::eventFilter(object,event);
    }
    QRect pixelRect(const QRect &area) const {
        const double sx=double(canvas_.width())/width(),sy=double(canvas_.height())/height();
        return QRect(qRound(area.x()*sx),qRound(area.y()*sy),qRound(area.width()*sx),qRound(area.height()*sy));
    }
    QColor annotationInk(QColor ink) const {
        if(canvas_.text("capHdr")=="scRGB-FP16") {
            const auto linear=[](float c){return c<=.04045f?c/12.92f:std::pow((c+.055f)/1.055f,2.4f);};
            ink=QColor::fromRgbF(linear(ink.redF()),linear(ink.greenF()),linear(ink.blueF()));
        }
        return ink;
    }
    void annotationPainter(QPainter &p) {
        p.scale(double(canvas_.width())/width(),double(canvas_.height())/height());
        p.setPen(QPen(annotationInk(color_),penWidth_,tool_==Line && lineMode_==2?Qt::DashLine:Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
        p.setBrush(filled_ && (tool_==Rect || tool_==Ellipse)?QBrush(annotationInk(color_)):Qt::NoBrush);
    }
    int selectionEdges(const QPoint &point) const {
        if(embedded_ || selecting_ || selection_.isEmpty() || !selection_.adjusted(-6,-6,6,6).contains(point))return 0;
        int edges=0;
        if(qAbs(point.x()-selection_.left())<=6)edges|=1;
        else if(qAbs(point.x()-selection_.right())<=6)edges|=2;
        if(qAbs(point.y()-selection_.top())<=6)edges|=4;
        else if(qAbs(point.y()-selection_.bottom())<=6)edges|=8;
        return edges;
    }
    void adjustSelection(const QPoint &point) {
        if(movingSelection_){
            const auto delta=point-origin_;
            selection_=startSelection_.translated(qBound(-startSelection_.left(),delta.x(),width()-1-startSelection_.right()),qBound(-startSelection_.top(),delta.y(),height()-1-startSelection_.bottom()));
        }else{
            selection_=startSelection_;
            if(resizeEdges_&1)selection_.setLeft(qBound(0,point.x(),startSelection_.right()-3));
            if(resizeEdges_&2)selection_.setRight(qBound(startSelection_.left()+3,point.x(),width()-1));
            if(resizeEdges_&4)selection_.setTop(qBound(0,point.y(),startSelection_.bottom()-3));
            if(resizeEdges_&8)selection_.setBottom(qBound(startSelection_.top()+3,point.y(),height()-1));
        }
        positionBar();update();
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        if(displayDirty_){display_=cap::preview(canvas_);displayDirty_=false;}
        p.drawImage(rect(), display_);
        if(!embedded_)p.fillRect(rect(), QColor(0,0,0,105));
        if(!embedded_ && selecting_ && !dragging_ && !hoveredWindow_.isEmpty()){
            p.drawImage(hoveredWindow_,display_,pixelRect(hoveredWindow_));
            p.setPen(QPen(QColor(110,190,255),2));p.setBrush(Qt::NoBrush);p.drawRect(hoveredWindow_.adjusted(1,1,-1,-1));
        }
        if (!embedded_ && !selection_.isEmpty()) {
            p.drawImage(selection_, display_, pixelRect(selection_));
            p.setPen(QPen(Qt::white,1,Qt::DashLine));
            p.drawRect(selection_);
            p.setPen(QColor(70,135,200));p.setBrush(Qt::white);
            for(const auto point:{selection_.topLeft(),QPoint(selection_.center().x(),selection_.top()),selection_.topRight(),QPoint(selection_.right(),selection_.center().y()),selection_.bottomRight(),QPoint(selection_.center().x(),selection_.bottom()),selection_.bottomLeft(),QPoint(selection_.left(),selection_.center().y())})p.drawRect(QRect(point-QPoint(3,3),QSize(7,7)));
            p.setBrush(Qt::NoBrush);
        }
        p.setPen(QPen(Qt::white,2,Qt::DashLine));
        for(auto *box:texts_)p.drawRect(box->geometry().adjusted(2,2,-3,-3));
        if (dragging_ && !selecting_ && !resizeEdges_ && !movingSelection_ && (tool_==Rect || tool_==Ellipse || tool_==Line)) {
            p.setPen(QPen(color_,penWidth_,tool_==Line && lineMode_==2?Qt::DashLine:Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
            p.setBrush(filled_ && (tool_==Rect || tool_==Ellipse)?QBrush(color_):Qt::NoBrush);
            const QRect drawRect=shapeRect();
            if(tool_==Rect)p.drawRect(drawRect);else if(tool_==Ellipse)p.drawEllipse(drawRect);else drawLine(p,origin_,last_);
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        if(e->button()==Qt::RightButton){if(!embedded_)close();return;}
        if(e->button()!=Qt::LeftButton) return;
        origin_=last_=e->pos();pressedWindow_=hoveredWindow_;constrainShape_=e->modifiers().testFlag(Qt::ControlModifier);
        resizeEdges_=selectionEdges(e->pos());movingSelection_=!selecting_ && tool_==Selection && selection_.contains(e->pos()) && !resizeEdges_;
        if(resizeEdges_ || movingSelection_){startSelection_=selection_;dragging_=true;return;}
        if(!selecting_ && tool_==Selection)return;
        dragging_=true;
        if(selecting_) {selection_=QRect(origin_,QSize(1,1));bar_->hide();}
        else if(tool_==Text) {
            dragging_=false;
            addText(e->pos());
        }else if(!selecting_ && tool_==Pen){
            remember(rect());
        }
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(!dragging_){
            if(selecting_ && !embedded_){hoveredWindow_={};for(const auto &window:windows_)if(window.contains(e->pos())){hoveredWindow_=window;break;}update();}
            const int edges=selectionEdges(e->pos());
            setCursor((edges==5 || edges==10)?Qt::SizeFDiagCursor:(edges==6 || edges==9)?Qt::SizeBDiagCursor:(edges&3)?Qt::SizeHorCursor:(edges&12)?Qt::SizeVerCursor:(!selecting_ && tool_==Selection && selection_.contains(e->pos()))?Qt::SizeAllCursor:Qt::CrossCursor);
            return;
        }
        last_=e->pos();constrainShape_=e->modifiers().testFlag(Qt::ControlModifier);
        if(resizeEdges_ || movingSelection_){adjustSelection(last_);return;}
        if(selecting_) selection_=QRect(origin_,last_).normalized();
        else if(tool_==Pen){
            QPainter p(&canvas_);annotationPainter(p);
            p.drawLine(origin_,last_);origin_=last_;displayDirty_=true;
        }
        update();
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if(e->button()!=Qt::LeftButton || !dragging_)return;
        dragging_=false;last_=e->pos();constrainShape_=e->modifiers().testFlag(Qt::ControlModifier);
        if(resizeEdges_ || movingSelection_){adjustSelection(last_);resizeEdges_=0;movingSelection_=false;return;}
        if(selecting_){
            selection_=QRect(origin_,last_).normalized().intersected(rect());
            if(selection_.width()<4 || selection_.height()<4){
                selection_=pressedWindow_;if(selection_.isEmpty()){update();return;}
            }
            selecting_=false; positionBar();bar_->show();bar_->raise();
        }else if(tool_==Rect || tool_==Ellipse || tool_==Line){
            const int margin=qMax(40,penWidth_*5+2);remember(shapeRect().adjusted(-margin,-margin,margin,margin).intersected(rect()));
            QPainter p(&canvas_);annotationPainter(p);
            const QRect r=shapeRect();
            if(tool_==Rect)p.drawRect(r);else if(tool_==Ellipse)p.drawEllipse(r);else drawLine(p,origin_,last_);displayDirty_=true;
        }
        update();
    }
    void keyPressEvent(QKeyEvent *e) override {
        if(e->key()==Qt::Key_Escape){if(embedded_ && bar_)bar_->hide();else close();}
        else if(e->key()==Qt::Key_Control){constrainShape_=true;update();}
        else QWidget::keyPressEvent(e);
    }
    void keyReleaseEvent(QKeyEvent *e) override {
        if(e->key()==Qt::Key_Control){constrainShape_=false;update();}
        else QWidget::keyReleaseEvent(e);
    }
};

class PinView final : public QGraphicsView {
    QPoint drag_;
public:
    std::function<bool()> editing;
    std::function<void(int)> zoom;
    std::function<void(const QPoint&)> menu;
    explicit PinView(QWidget *parent):QGraphicsView(parent){
        setFrameShape(QFrame::NoFrame);setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setRenderHint(QPainter::SmoothPixmapTransform,false);setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setAlignment(Qt::AlignLeft | Qt::AlignTop);setTransformationAnchor(QGraphicsView::NoAnchor);setResizeAnchor(QGraphicsView::NoAnchor);
    }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if(event->button()==Qt::LeftButton && !editing()){drag_=event->globalPosition().toPoint()-parentWidget()->pos();event->accept();}
        else QGraphicsView::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) override {
        if(event->buttons().testFlag(Qt::LeftButton) && !editing()){parentWidget()->move(event->globalPosition().toPoint()-drag_);event->accept();}
        else QGraphicsView::mouseMoveEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent *event) override {if(!editing())event->accept();else QGraphicsView::mouseReleaseEvent(event);}
    void wheelEvent(QWheelEvent *event) override {zoom(event->angleDelta().y());event->accept();}
    void contextMenuEvent(QContextMenuEvent *event) override {menu(event->globalPos());event->accept();}
};

class PinWindow final : public QWidget {
    static constexpr int borderWidth_ = 2;
    CaptureOverlay *canvas_;
    PinView *view_;
    QWidget *bar_ = nullptr;
    QMenu *menu_ = nullptr;
    double zoom_ = 1;
public:
    explicit PinWindow(QImage image,std::function<bool(QImage,bool,bool,bool)> output) {
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);setAttribute(Qt::WA_DeleteOnClose);
        setWindowTitle(QStringLiteral("VS Cap 贴图"));
        canvas_=new CaptureOverlay(qApp->primaryScreen(),std::move(image),std::move(output),true);
        canvas_->setObjectName("capPinCanvas");
        auto *undoShortcut=new QShortcut(QKeySequence::Undo,this);connect(undoShortcut,&QShortcut::activated,this,[this]{canvas_->undoEdit();});
        view_=new PinView(this);view_->setObjectName("capPinView");auto *scene=new QGraphicsScene(view_);view_->setScene(scene);
        scene->addWidget(canvas_);scene->setSceneRect(canvas_->rect());
        bar_=canvas_->takeToolBar(this,[this]{positionBar();});
        view_->editing=[this]{return bar_->isVisible() && !canvas_->isPanTool();};view_->zoom=[this](int delta){if(delta)setZoom(zoom_*std::pow(1.2,delta/120.0));};
        view_->menu=[this](const QPoint &point){menu_->exec(point);};
        menu_=new QMenu(this);menu_->setObjectName("capPinMenu");
        menu_->addAction(QStringLiteral("显示工具栏"),this,[this]{if(bar_->isVisible())bar_->hide();else showToolBar();});
        connect(menu_,&QMenu::aboutToShow,this,[this]{menu_->actions().first()->setText(bar_->isVisible()?QStringLiteral("隐藏工具栏"):QStringLiteral("显示工具栏"));});
        menu_->addAction(QStringLiteral("复制"),this,[this]{canvas_->exportImage(false,false,true);});
        menu_->addAction(QStringLiteral("保存为PNG"),this,[this]{canvas_->exportImage(false,false);});
        menu_->addAction(QStringLiteral("保存为AVIF"),this,[this]{canvas_->exportImage(false,true);});
        menu_->addAction(QStringLiteral("销毁"),this,&QWidget::close);
        auto *targetScreen=QGuiApplication::screenAt(QCursor::pos());if(!targetScreen)targetScreen=qApp->primaryScreen();
        const auto available=targetScreen->availableGeometry();
        // Match image pixels to physical screen pixels even with desktop DPI scaling.
        setZoom(qMin(1.0/targetScreen->devicePixelRatio(),qMin(double(available.width()-2*borderWidth_)/canvas_->width(),double(available.height()-2*borderWidth_)/canvas_->height())));
        const auto position=QCursor::pos()-QPoint(width()/2,height()/2);
        move(qBound(available.left(),position.x(),available.right()-width()+1),qBound(available.top(),position.y(),available.bottom()-height()+1));show();
    }
    void showToolBar(){positionBar();bar_->show();bar_->raise();}
private:
    void positionBar(){
        if(!bar_)return;bar_->adjustSize();const auto screen=screenGeometry();
        const int x=qBound(screen.left(),geometry().right()-bar_->width()+1,qMax(screen.left(),screen.right()-bar_->width()+1));
        const int below=geometry().bottom()+6;const int y=below+bar_->height()<=screen.bottom()?below:qMax(screen.top(),geometry().top()-bar_->height()-6);
        bar_->move(x,y);
    }
    QRect screenGeometry() const {return screen()?screen()->availableGeometry():qApp->primaryScreen()->availableGeometry();}
    void setZoom(double value){
        const auto center=geometry().center();zoom_=qBound(.01,value,8.0);
        view_->setTransform(QTransform::fromScale(zoom_,zoom_));
        resize(qMax(1,qRound(canvas_->width()*zoom_))+2*borderWidth_,qMax(1,qRound(canvas_->height()*zoom_))+2*borderWidth_);
        view_->setGeometry(rect().adjusted(borderWidth_,borderWidth_,-borderWidth_,-borderWidth_));move(center-QPoint(width()/2,height()/2));positionBar();
    }
protected:
    void paintEvent(QPaintEvent *) override {QPainter painter(this);painter.fillRect(rect(),QColor(70,135,200));}
    void moveEvent(QMoveEvent *event) override {positionBar();QWidget::moveEvent(event);}
    void contextMenuEvent(QContextMenuEvent *event) override {menu_->exec(event->globalPos());}
};


class CapApp final : public QObject, public QAbstractNativeEventFilter {
    friend class TestCapWorkflow;
    QSettings settings_;
    QSystemTrayIcon tray_;
    QList<QPointer<PinWindow>> pins_;
    int hotA_ = 0, hotT_ = 0;
public:
    CapApp() : settings_(QCoreApplication::applicationDirPath()+"/vs-cap.ini", QSettings::IniFormat) {
        auto icon=QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
        tray_.setIcon(icon);tray_.setToolTip("VS Cap");
        auto *menu=new QMenu;
        menu->addAction(QStringLiteral("截图"),this,[this]{capture();});
        menu->addAction(QStringLiteral("贴图（剪贴板）"),this,[this]{pastePin();});
        menu->addAction(QStringLiteral("设置"),this,[this]{configure();});
        menu->addSeparator();
        menu->addAction(QStringLiteral("退出"),qApp,&QApplication::quit);
        tray_.setContextMenu(menu);
        connect(&tray_,&QSystemTrayIcon::activated,this,[this](QSystemTrayIcon::ActivationReason why) {
            if(why==QSystemTrayIcon::Trigger)capture();
        });
        tray_.show();
        QApplication::instance()->installNativeEventFilter(this);
        bindHotkeys(settings_.value("hotkeys/capture","Alt+A").toString(),
                    settings_.value("hotkeys/pin","Alt+T").toString());
        if(!settings_.value("general/minimizeToTray",true).toBool())QTimer::singleShot(0,this,[this]{configure();});
    }
    ~CapApp() override {
        UnregisterHotKey(nullptr,1001);
        UnregisterHotKey(nullptr,1002);
    }
    bool nativeEventFilter(const QByteArray &, void *message, qintptr *) override {
        auto *m=static_cast<MSG*>(message);
        if(m->message != WM_HOTKEY)return false;
        if(m->wParam==1001){capture();return true;}
        if(m->wParam==1002){pastePin();return true;}
        return false;
    }
private:
    void applyTopPriority(QWidget *widget) {
        if(!settings_.value("general/uiAccess",false).toBool())return;
        if(!cap::hasUiAccess()){
            tray_.showMessage("VS Cap",QStringLiteral("UIAccess 未启用：重启截图工具并确认 Windows 管理员授权后生效。"),QSystemTrayIcon::Information);
            return;
        }
        SetWindowPos(reinterpret_cast<HWND>(widget->winId()),HWND_TOPMOST,0,0,0,0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    static bool sequence(const QString &value, UINT &mod, UINT &key) {
        QKeySequence seq=QKeySequence::fromString(value,QKeySequence::PortableText);
        if(seq.isEmpty() || seq.count()!=1)return false;
        const auto comb=seq[0];const auto k=comb.key();
        const auto m=comb.keyboardModifiers();
        mod=MOD_NOREPEAT;
        if(m&Qt::AltModifier)mod|=MOD_ALT;
        if(m&Qt::ControlModifier)mod|=MOD_CONTROL;
        if(m&Qt::ShiftModifier)mod|=MOD_SHIFT;
        if(m&Qt::MetaModifier)mod|=MOD_WIN;
        if(mod==MOD_NOREPEAT)return false;
        if(k>=Qt::Key_A && k<=Qt::Key_Z)key=UINT('A'+k-Qt::Key_A);
        else if(k>=Qt::Key_0 && k<=Qt::Key_9)key=UINT('0'+k-Qt::Key_0);
        else if(k>=Qt::Key_F1 && k<=Qt::Key_F12)key=UINT(VK_F1+k-Qt::Key_F1);
        else return false;
        return true;
    }
    bool bindHotkeys(const QString &capture, const QString &pin) {
        UINT m1=0,k1=0,m2=0,k2=0;
        if(!sequence(capture,m1,k1) || !sequence(pin,m2,k2) || (m1==m2 && k1==k2))return false;
        UnregisterHotKey(nullptr,1001);UnregisterHotKey(nullptr,1002);
        const bool a=RegisterHotKey(nullptr,1001,m1,k1);
        const bool b=RegisterHotKey(nullptr,1002,m2,k2);
        if(!a || !b){
            UnregisterHotKey(nullptr,1001);UnregisterHotKey(nullptr,1002);
            tray_.showMessage("VS Cap",QStringLiteral("快捷键冲突或已被占用，请打开设置更换"),QSystemTrayIcon::Warning);
            return false;
        }
        hotA_=1001;hotT_=1002;return true;
    }
    void addPin(QImage image) {
        pins_.removeIf([](const QPointer<PinWindow> &pin){return pin.isNull();});
        auto *pin=new PinWindow(std::move(image),[this](QImage shot,bool paste,bool avif,bool copy){return output(std::move(shot),paste,avif,copy);});
        const int index=pins_.size();
        if(index) {
            const auto area=pin->screen()->availableGeometry();
            const int shift=index%8*26;
            pin->move(qBound(area.left(),pin->x()+shift,qMax(area.left(),area.right()-pin->width()+1)),
                      qBound(area.top(),pin->y()+shift,qMax(area.top(),area.bottom()-pin->height()+1)));
        }
        pins_.append(pin);
        applyTopPriority(pin);
    }
    QString makePath(const QString &ext,bool automatic=false) {
        const auto dir=settings_.value(automatic?"save/defaultDirectory":"save/lastDirectory",defaultDirectory()).toString();
        return QDir(dir).filePath(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz")+"."+ext);
    }
    bool saveCapturePng(const QImage &image,const QString &path) {
        if(!savePng(image,path))return false;
        if(image.text("capHdr")!="scRGB-FP16")return true;
        const auto info=QFileInfo(path);
        const auto sdrPath=QDir(info.absolutePath()).filePath(info.completeBaseName()+"_SDR.png");
        return savePng(cap::preview(image),sdrPath);
    }
    bool saveHdrCapture(const QImage &image) {
        if(image.text("capHdr")!="scRGB-FP16")return true;
        const auto path=makePath("png",true);
        return QDir().mkpath(QFileInfo(path).absolutePath()) && saveCapturePng(image,path);
    }
    void storeDir(const QString &path) {
        settings_.setValue("save/lastDirectory",QFileInfo(path).absolutePath());settings_.sync();
    }
    bool startAvif(const QImage &image, const QString &path) {
        const QString work=QDir(QDir::tempPath()).filePath("vscap-"+QUuid::createUuid().toString(QUuid::WithoutBraces));
        if(!QDir().mkpath(work))return false;
        const QString input=QDir(work).filePath("input.png"),encoded=QDir(work).filePath("result.avif");
        if(!savePng(cap::pngPixels(image),input)){
            QDir(work).removeRecursively();tray_.showMessage("VS Cap",QStringLiteral("AVIF 输入图像写入失败"));return false;
        }
        const QString app=QCoreApplication::applicationDirPath();
        QString encoder=QDir(app).filePath("runtime/awj/AWJ.exe");
        if(!QFileInfo::exists(encoder))encoder=QDir(app).filePath("AWJ.exe");
        if(!QFileInfo::exists(encoder)){
            // Source-tree builds use the same pinned AWJimage 1.1.0 as Player.
            encoder=QDir(app).filePath("../../.deps/awj/1.1.0/AWJ.exe");
        }
        if(!QFileInfo::exists(encoder)){
            QDir(work).removeRecursively();tray_.showMessage("VS Cap",QStringLiteral("未找到 AWJ.exe（runtime/awj）"));return false;
        }
        const bool hdr=image.text("capHdr")=="scRGB-FP16";
        QStringList args{"-i",input,"-o",work,"--format","avif","--template","result",
            "--quality",QString::number(settings_.value("avif/quality",70).toInt()),
            "--avif-encoder","aom","--avif-color-representation","yuv",
            "--chroma",(image.width()%2 || image.height()%2)?"444":"420",
            "--bit-depth",hdr?"12":"10","--speed","6",
            "--alpha","auto","--image-size-limit","none","--keep-metadata","--no-summary","--no-log"};
        if(hdr)args<< "--color-primaries"<<"9"<<"--transfer-characteristics"<<"16"
                    <<"--matrix-coefficients"<<"9"<<"--color-range"<<"1";
        auto *process=new QProcess(this);process->setWorkingDirectory(work);
        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process,&QProcess::errorOccurred,this,[this,process,work](QProcess::ProcessError error){
            if(error!=QProcess::FailedToStart)return;
            QDir(work).removeRecursively();
            tray_.showMessage("VS Cap",QStringLiteral("无法启动 AWJ：")+process->errorString());
            process->deleteLater();
        });
        connect(process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,
                [this,process,work,encoded,path](int code,QProcess::ExitStatus status){
                    bool ok=code==0 && status==QProcess::NormalExit && QFileInfo::exists(encoded);
                    if(ok){
                        QFile source(encoded);QSaveFile destination(path);
                        ok=source.open(QIODevice::ReadOnly) && destination.open(QIODevice::WriteOnly);
                        while(ok && !source.atEnd()){
                            const auto bytes=source.read(4*1024*1024);
                            ok=!bytes.isEmpty() && destination.write(bytes)==bytes.size();
                        }
                        if(ok)ok=destination.commit();
                        else destination.cancelWriting();
                    }
                    const auto message=QString::fromUtf8(process->readAll().right(400));
                    QDir(work).removeRecursively();
                    tray_.showMessage("VS Cap",ok?QStringLiteral("AWJ AVIF 已保存：")+path:
                        QStringLiteral("AWJ AVIF 编码失败：")+message,QSystemTrayIcon::Information);
                    process->deleteLater();
                });
        process->start(encoder,args);
        return true;
    }
    bool output(QImage image, bool pin, bool avif, bool copy) {
        if(image.isNull())return false;
        if(pin) {addPin(std::move(image));return true;}
        if(copy)return true;
        const auto ext=avif?"avif":"png";
        const auto path=QFileDialog::getSaveFileName(nullptr,QStringLiteral("保存截图"),makePath(ext),
                         avif?"AVIF (*.avif)":"PNG (*.png)");
        if(path.isEmpty())return false;
        storeDir(path);
        if(avif)return startAvif(image,path);
        if(saveCapturePng(image,path))return true;
        tray_.showMessage("VS Cap",QStringLiteral("PNG 写入失败"),QSystemTrayIcon::Warning);return false;
    }
    void capture() {
        QScreen *screen=QGuiApplication::screenAt(QCursor::pos());
        if(!screen)screen=QGuiApplication::primaryScreen();
        if(!screen)return;
        QString error;
        const QImage shot=cap::captureScreen(screen,&error);
        if(shot.isNull()){tray_.showMessage("VS Cap",error,QSystemTrayIcon::Warning);return;}
        if(!saveHdrCapture(shot))tray_.showMessage("VS Cap",QStringLiteral("HDR 截图自动保存失败，请检查默认保存路径"),QSystemTrayIcon::Warning);
        auto *overlay=new CaptureOverlay(screen,shot,[this](QImage shot,bool pin,bool avif,bool copy){
            return output(std::move(shot),pin,avif,copy);
        });
        applyTopPriority(overlay);
    }
    void pastePin() {
        QImage pic=QApplication::clipboard()->image();
        const auto raw=QApplication::clipboard()->mimeData()->data("application/x-vscap-scrgb");
        if(raw.size()>=8) {
            QDataStream stream(raw);qint32 w=0,h=0;stream>>w>>h;
            if(w>0 && h>0 && qint64(w)*h*16==raw.size()-8) {
                pic=QImage(w,h,QImage::Format_RGBA32FPx4);
                for(int y=0;y<h;++y)memcpy(pic.scanLine(y),raw.constData()+8+qint64(y)*w*16,w*16);
                pic.setColorSpace(QColorSpace::SRgbLinear);pic.setText("capHdr","scRGB-FP16");
            }
        }
        const bool hadImage=!pic.isNull();
        if(hadImage)addPin(std::move(pic));
        int added=0;
        for(const auto &url:QApplication::clipboard()->mimeData()->urls()){
            if(!url.isLocalFile())continue;
            QImageReader reader(url.toLocalFile());reader.setAutoTransform(true);
            const auto next=reader.read();if(next.isNull())continue;
            addPin(next);++added;
        }
        if(!hadImage && !added)tray_.showMessage("VS Cap",QStringLiteral("剪贴板中没有可读取的图片或图片文件"));
    }
    void configure() {
        QDialog dialog;
        dialog.setWindowTitle(QStringLiteral("VS Cap 设置"));
        QFormLayout layout(&dialog);
        QKeySequenceEdit screenshot(QKeySequence(settings_.value("hotkeys/capture","Alt+A").toString()));
        QKeySequenceEdit pin(QKeySequence(settings_.value("hotkeys/pin","Alt+T").toString()));
        QCheckBox autostart(QStringLiteral("开机自启"));
        QCheckBox trayMode(QStringLiteral("启动时最小化到托盘"));
        QCheckBox uiAccess(QStringLiteral("UIAccess 高优先级置顶（重启后生效，需要管理员授权）"));
        uiAccess.setObjectName("capUiAccess");uiAccess.setChecked(settings_.value("general/uiAccess",false).toBool());
        QComboBox format;format.setObjectName("capDefaultFormat");format.addItem("PNG","png");format.addItem("AVIF · AWJ","avif");
        format.setCurrentIndex(qMax(0,format.findData(settings_.value("save/format","png").toString())));
        QSpinBox quality;quality.setObjectName("capAvifQuality");quality.setRange(1,100);quality.setValue(settings_.value("avif/quality",70).toInt());quality.setSuffix(QStringLiteral("（推荐 70）"));
        QWidget saveLocation;QHBoxLayout saveLayout(&saveLocation);saveLayout.setContentsMargins(0,0,0,0);
        QLineEdit saveDirectory(settings_.value("save/defaultDirectory",defaultDirectory()).toString());saveDirectory.setObjectName("capDefaultSaveDirectory");
        QPushButton browse(QStringLiteral("浏览…"));saveLayout.addWidget(&saveDirectory);saveLayout.addWidget(&browse);
        connect(&browse,&QPushButton::clicked,&dialog,[&]{const auto path=QFileDialog::getExistingDirectory(&dialog,QStringLiteral("截图默认保存路径"),saveDirectory.text());if(!path.isEmpty())saveDirectory.setText(path);});
        autostart.setChecked(settings_.value("general/autostart",false).toBool());
        trayMode.setChecked(settings_.value("general/minimizeToTray",true).toBool());
        layout.addRow(QStringLiteral("截图快捷键"),&screenshot);
        layout.addRow(QStringLiteral("贴图快捷键"),&pin);
        layout.addRow(QStringLiteral("截图默认保存路径"),&saveLocation);
        layout.addRow(QStringLiteral("默认保存格式"),&format);
        layout.addRow(QStringLiteral("AWJ AVIF 质量"),&quality);
        layout.addRow(&uiAccess);
        layout.addRow(&autostart);layout.addRow(&trayMode);
        QLabel note(QStringLiteral("PNG 总是 16bit×4 通道；HDR PNG 同时生成 SDR 预览。AWJ 使用 AOM、默认质量 70。UIAccess 默认关闭，启用后下次启动会请求 Windows 管理员授权，无需驱动或安装证书。"));
        note.setWordWrap(true);layout.addRow(&note);
        QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
        layout.addRow(&buttons);
        connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
        connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
        if(dialog.exec()!=QDialog::Accepted)return;
        const QString a=screenshot.keySequence().toString(QKeySequence::PortableText);
        const QString b=pin.keySequence().toString(QKeySequence::PortableText);
        if(!bindHotkeys(a,b)){
            QMessageBox::warning(nullptr,QStringLiteral("VS Cap"),QStringLiteral("快捷键无效或与其它程序冲突"));
            bindHotkeys(settings_.value("hotkeys/capture","Alt+A").toString(),
                        settings_.value("hotkeys/pin","Alt+T").toString());
            return;
        }
        settings_.setValue("hotkeys/capture",a);
        settings_.setValue("hotkeys/pin",b);
        settings_.setValue("save/defaultDirectory",saveDirectory.text().trimmed().isEmpty()?defaultDirectory():QDir::cleanPath(saveDirectory.text().trimmed()));
        settings_.setValue("general/autostart",autostart.isChecked());
        settings_.setValue("general/minimizeToTray",trayMode.isChecked());
        settings_.setValue("save/format",format.currentData().toString());
        settings_.setValue("avif/quality",quality.value());
        settings_.setValue("general/uiAccess",uiAccess.isChecked());
        settings_.sync();
        QSettings startup("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",QSettings::NativeFormat);
        if(autostart.isChecked())startup.setValue("VS Cap",QDir::toNativeSeparators("\""+QCoreApplication::applicationFilePath()+"\""));
        else startup.remove("VS Cap");
    }
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    if(app.arguments().contains("--capture-test")) {
        QString error;const auto image=cap::captureScreen(app.primaryScreen(),&error);
        qInfo()<<"CAPTURE"<<image.size()<<image.depth()<<image.text("capHdr")<<error;
        return image.isNull()?1:0;
    }
    if (app.arguments().contains("--self-test")) {
        const QString path=QDir(QDir::tempPath()).filePath("vs-cap-test-16bit.png");
        QImage gradient(32,32,QImage::Format_RGBA64);
        for(int y=0;y<32;++y) for(int x=0;x<32;++x)
            gradient.setPixelColor(x,y,QColor::fromRgba64(qRgba64(x*2114,y*2114,32768,65535)));
        const bool saved=savePng(gradient,path);
        const QImage output=QImageReader(path).read();
        QFile::remove(path);
        qInfo().noquote()<<"CAP_TEST_PNG16"<<(saved && output.depth()==64 && output.pixelColor(10,10)==gradient.pixelColor(10,10))
                         <<"DEPTH"<<output.depth();
        return saved && output.depth()==64 && output.pixelColor(10,10)==gradient.pixelColor(10,10) ? 0:1;
    }
    app.setQuitOnLastWindowClosed(false);
    app.setApplicationName("VS Cap");
    const int waitArgument=app.arguments().indexOf("--wait-parent");
    if(waitArgument>=0){const auto parent=OpenProcess(SYNCHRONIZE,FALSE,app.arguments().value(waitArgument+1).toULong());if(parent){WaitForSingleObject(parent,10000);CloseHandle(parent);}}
    if(const auto running=OpenMutexW(SYNCHRONIZE,FALSE,L"Local\\VSRenderer-VSCap")){CloseHandle(running);return 0;}
    QSettings preferences(QCoreApplication::applicationDirPath()+"/vs-cap.ini",QSettings::IniFormat);
    if((preferences.value("general/uiAccess",false).toBool() || app.arguments().contains("--uiaccess-launch")) && !cap::hasUiAccess()){
        QString error;if(cap::restartWithUiAccess(&error))return 0;
        QMessageBox::warning(nullptr,"VS Cap",QStringLiteral("UIAccess 启动失败，继续使用普通置顶。\n")+error);
    }
    HANDLE instance=CreateMutexW(nullptr,FALSE,L"Local\\VSRenderer-VSCap");
    if(GetLastError()==ERROR_ALREADY_EXISTS){if(instance)CloseHandle(instance);return 0;}
    CapApp controller;
    const int result=app.exec();if(instance)CloseHandle(instance);return result;
}
