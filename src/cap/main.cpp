#include <QtWidgets>
#include <QAbstractNativeEventFilter>
#include <QProcess>
#include <QSettings>
#include <QImageWriter>
#include <QImageReader>
#include <QStandardPaths>
#include <QPointer>
#include <QDateTime>
#include <windows.h>
#include <functional>
#include <cmath>
#include <cstring>
#include "DesktopCapture.h"

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
    mime->setImageData(cap::preview(image));
    QByteArray png;QBuffer buffer(&png);buffer.open(QIODevice::WriteOnly);
    cap::pngPixels(image).save(&buffer,"PNG");mime->setData("image/png",png);
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

class PinWindow final : public QWidget {
    QImage image_;
    QPoint drag_;
    QWidget *bar_=nullptr;
public:
    explicit PinWindow(QImage image) : image_(std::move(image)) {
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_DeleteOnClose);
        resize(image_.size().boundedTo(QSize(960, 720)));
        move(QCursor::pos() - QPoint(width()/2, height()/2));
        show();
    }
    void showToolBar() {
        if(!bar_) {
            bar_=new QWidget(this);auto *row=new QHBoxLayout(bar_);row->setContentsMargins(4,4,4,4);
            auto *copy=new QPushButton(QStringLiteral("复制"),bar_);row->addWidget(copy);connect(copy,&QPushButton::clicked,this,[this]{copyImage(image_);});
            auto *save=new QPushButton(QStringLiteral("保存"),bar_);row->addWidget(save);connect(save,&QPushButton::clicked,this,[this]{
                QSettings settings;const auto path=QFileDialog::getSaveFileName(this,QStringLiteral("保存贴图"),settings.value("save/lastDirectory",defaultDirectory()).toString(),"PNG (*.png)");
                if(!path.isEmpty() && savePng(image_,path))settings.setValue("save/lastDirectory",QFileInfo(path).absolutePath());
            });
            auto *destroy=new QPushButton(QStringLiteral("销毁"),bar_);row->addWidget(destroy);connect(destroy,&QPushButton::clicked,this,&QWidget::close);
            bar_->adjustSize();
        }
        bar_->move(qMax(1,width()-bar_->width()-2),qMax(1,height()-bar_->height()-2));bar_->show();bar_->raise();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(20, 20, 20));
        p.drawImage(rect().adjusted(1,1,-1,-1), cap::preview(image_));
        p.setPen(QPen(QColor(70, 135, 200), 1));
        p.drawRect(rect().adjusted(0,0,-1,-1));
    }
    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::LeftButton) drag_ = e->globalPosition().toPoint() - pos();
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if (e->buttons() & Qt::LeftButton) move(e->globalPosition().toPoint() - drag_);
    }
    void contextMenuEvent(QContextMenuEvent *e) override {
        QMenu m;
        auto *showBar = m.addAction(QStringLiteral("显示工具栏"));
        auto *save = m.addAction(QStringLiteral("保存 PNG..."));
        auto *del = m.addAction(QStringLiteral("销毁贴图"));
        const auto selected = m.exec(e->globalPos());
        if (selected == del) close();
        if (selected == save) {
            const auto path = QFileDialog::getSaveFileName(this, QStringLiteral("保存贴图"), defaultDirectory(), "PNG (*.png)");
            if (!path.isEmpty() && !savePng(image_, path))
                QMessageBox::warning(this, QStringLiteral("VS Cap"), QStringLiteral("PNG 写入失败"));
        }
        if (selected == showBar) showToolBar();
    }
};

class CaptureOverlay final : public QWidget {
    QImage canvas_;
    QRect selection_;
    QPoint origin_, last_;
    bool selecting_ = true, dragging_ = false;
    enum Tool { Pen, Rect, Ellipse, Text, Line } tool_ = Pen;
    QColor color_ = Qt::red;
    bool arrow_ = false;
    int penWidth_ = 3;
    QComboBox *options_ = nullptr;
    QPushButton *colorButton_ = nullptr, *undoButton_ = nullptr;
    struct Edit { QRect area; QImage pixels; };
    QList<Edit> history_;
    QWidget *bar_ = nullptr;
    std::function<void(QImage, bool, bool, bool)> output_;
public:
    CaptureOverlay(QScreen *screen, QImage shot, std::function<void(QImage,bool,bool,bool)> output)
        : canvas_(std::move(shot)), output_(std::move(output)) {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        setAttribute(Qt::WA_DeleteOnClose);
        setMouseTracking(true);
        setCursor(Qt::CrossCursor);
        setGeometry(screen->geometry());
        bar_ = new QWidget(this);
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
            if(c.isValid()){color_=c;updateColor();}
        });
        options_=new QComboBox(bar_);options_->setObjectName("capToolOptions");layout->addWidget(options_);
        auto *tools=new QButtonGroup(this);tools->setExclusive(true);
        const auto tool=[&](const QString &name,Tool value){
            auto *button=add(name,[this,value]{tool_=value;updateOptions();});
            button->setCheckable(true);button->setChecked(value==tool_);tools->addButton(button);
        };
        tool(QStringLiteral("画笔"),Pen);tool(QStringLiteral("矩形"),Rect);
        tool(QStringLiteral("圆形"),Ellipse);tool(QStringLiteral("线"),Line);tool(QStringLiteral("文字"),Text);
        connect(options_,&QComboBox::currentIndexChanged,this,[this](int index){if(tool_==Line)arrow_=index==1;else penWidth_=options_->currentData().toInt();});
        undoButton_=add(QStringLiteral("撤销"),[this]{undo();});undoButton_->setEnabled(false);
        auto *undoShortcut=new QShortcut(QKeySequence::Undo,this);connect(undoShortcut,&QShortcut::activated,this,[this]{undo();});
        add(QStringLiteral("PNG"), [this] {deliver(false,false);});
        add(QStringLiteral("AVIF"), [this] {deliver(false,true);});
        add(QStringLiteral("贴图"), [this] {deliver(true,false);});
        add(QStringLiteral("✓"), [this] {deliver(false,false,true);});
        bar_->setStyleSheet("QWidget{background:#252831;} QPushButton,QComboBox{color:white;background:#3b414e;padding:5px;border:1px solid transparent;} QPushButton:checked{border:2px solid #78baff;background:#24496a;} QPushButton:disabled{color:#858585;}");
        updateColor();updateOptions();
        bar_->adjustSize();
        bar_->hide();
        show();
        raise();
        activateWindow();
    }
private:
    void updateColor(){colorButton_->setStyleSheet(QString("border-bottom:3px solid %1;").arg(color_.name()));}
    void updateOptions(){
        const QSignalBlocker block(options_);options_->clear();
        if(tool_==Line){options_->addItems({QStringLiteral("直线"),QStringLiteral("箭头")});options_->setCurrentIndex(arrow_?1:0);}
        else{for(int width:{2,3,5,8})options_->addItem(QStringLiteral("%1 px").arg(width),width);options_->setCurrentIndex(options_->findData(penWidth_));}
        options_->setEnabled(tool_!=Text);positionBar();
    }
    void remember(const QRect &area){
        const auto pixels=pixelRect(area).intersected(canvas_.rect());if(pixels.isEmpty())return;
        history_.append({pixels,canvas_.copy(pixels)});qint64 bytes=0;for(const auto &edit:history_)bytes+=edit.pixels.sizeInBytes();
        while(history_.size()>1 && (history_.size()>32 || bytes>128*1024*1024)){bytes-=history_.first().pixels.sizeInBytes();history_.removeFirst();}
        undoButton_->setEnabled(true);
    }
    void undo(){
        if(dragging_ || history_.isEmpty())return;
        const auto edit=history_.takeLast();QPainter painter(&canvas_);painter.setCompositionMode(QPainter::CompositionMode_Source);painter.drawImage(edit.area.topLeft(),edit.pixels);painter.end();
        undoButton_->setEnabled(!history_.isEmpty());update();
    }
    void drawLine(QPainter &p,const QPoint &from,const QPoint &to){
        p.drawLine(from,to);if(!arrow_ || from==to)return;
        const QPointF direction=QPointF(to-from);const double length=std::hypot(direction.x(),direction.y());
        const QPointF unit=direction/length,normal(-unit.y(),unit.x());const double size=qMax(12,penWidth_*4);
        p.drawLine(QPointF(to),QPointF(to)-unit*size+normal*size*.5);p.drawLine(QPointF(to),QPointF(to)-unit*size-normal*size*.5);
    }
    void positionBar() {
        bar_->adjustSize();
        const int x = qBound(0, selection_.right() - bar_->width(), width() - bar_->width());
        const int y = qBound(0, selection_.bottom() + 8, height() - bar_->height());
        bar_->move(x,y);
    }
    void deliver(bool pin, bool avif, bool copy = false) {
        if(selection_.isEmpty()) return;
        const QImage crop = canvas_.copy(pixelRect(selection_.intersected(rect())));
        if(copy) copyImage(crop);
        output_(crop,pin,avif,copy);
        close();
    }
protected:
    QRect pixelRect(const QRect &area) const {
        const double sx=double(canvas_.width())/width(),sy=double(canvas_.height())/height();
        return QRect(qRound(area.x()*sx),qRound(area.y()*sy),qRound(area.width()*sx),qRound(area.height()*sy));
    }
    void annotationPainter(QPainter &p) {
        p.scale(double(canvas_.width())/width(),double(canvas_.height())/height());
        QColor ink=color_;
        if(canvas_.text("capHdr")=="scRGB-FP16") {
            const auto linear=[](float c){return c<=.04045f?c/12.92f:std::pow((c+.055f)/1.055f,2.4f);};
            ink=QColor::fromRgbF(linear(ink.redF()),linear(ink.greenF()),linear(ink.blueF()));
        }
        p.setPen(QPen(ink,penWidth_,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        const auto display=cap::preview(canvas_);
        p.drawImage(rect(), display);
        p.fillRect(rect(), QColor(0,0,0,105));
        if (!selection_.isEmpty()) {
            p.drawImage(selection_, display, pixelRect(selection_));
            p.setPen(QPen(Qt::white,1,Qt::DashLine));
            p.drawRect(selection_);
        }
        if (dragging_ && !selecting_ && (tool_==Rect || tool_==Ellipse || tool_==Line)) {
            p.setClipRect(selection_);p.setPen(QPen(color_,penWidth_,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
            const QRect drawRect=QRect(origin_,last_).normalized();
            if(tool_==Rect)p.drawRect(drawRect);else if(tool_==Ellipse)p.drawEllipse(drawRect);else drawLine(p,origin_,last_);
        }
    }
    void mousePressEvent(QMouseEvent *e) override {
        if(e->button()==Qt::RightButton){close();return;}
        if(e->button()!=Qt::LeftButton) return;
        if(!selecting_ && !selection_.contains(e->pos()))return;
        origin_=last_=e->pos();
        dragging_=true;
        if(selecting_) {selection_=QRect(origin_,QSize(1,1));bar_->hide();}
        else if(tool_==Text && selection_.contains(e->pos())) {
            dragging_=false;
            bool ok=false;
            const QString text=QInputDialog::getText(this,QStringLiteral("插入文字"),QStringLiteral("内容"),QLineEdit::Normal,{},&ok);
            if(ok && !text.isEmpty()){
                remember(selection_);
                QPainter p(&canvas_);
                annotationPainter(p);
                p.setClipRect(selection_);
                QFont font; font.setPointSize(18); p.setFont(font);p.drawText(e->pos(),text);
                update();
            }
        }else if(!selecting_ && tool_==Pen){
            remember(selection_);
        }
    }
    void mouseMoveEvent(QMouseEvent *e) override {
        if(!dragging_)return;
        last_=e->pos();
        if(selecting_) selection_=QRect(origin_,last_).normalized();
        else if(tool_==Pen && selection_.contains(last_)){
            QPainter p(&canvas_);annotationPainter(p);p.setClipRect(selection_);
            p.drawLine(origin_,last_);origin_=last_;
        }
        update();
    }
    void mouseReleaseEvent(QMouseEvent *e) override {
        if(e->button()!=Qt::LeftButton || !dragging_)return;
        dragging_=false;last_=e->pos();
        if(selecting_){
            selection_=QRect(origin_,last_).normalized().intersected(rect());
            if(selection_.width()<4 || selection_.height()<4){selection_={};update();return;}
            selecting_=false; positionBar();bar_->show();bar_->raise();
        }else if(tool_==Rect || tool_==Ellipse || tool_==Line){
            remember(QRect(origin_,last_).normalized().adjusted(-40,-40,40,40).intersected(selection_));
            QPainter p(&canvas_);annotationPainter(p);p.setClipRect(selection_);
            const QRect r=QRect(origin_,last_).normalized().intersected(selection_);
            if(tool_==Rect)p.drawRect(r);else if(tool_==Ellipse)p.drawEllipse(r);else drawLine(p,origin_,last_);
        }
        update();
    }
    void keyPressEvent(QKeyEvent *e) override {
        if(e->key()==Qt::Key_Escape)close();
        else QWidget::keyPressEvent(e);
    }
};

static QStringList avifArguments(const QImage &image,const QString &input,const QString &output) {
    QStringList args{"-hide_banner","-y","-i",input,"-frames:v","1"};
    if(image.text("capHdr")=="scRGB-FP16")args<<"-vf"<<"scale=in_color_matrix=bt2020:out_color_matrix=bt2020:in_range=full:out_range=full,setparams=range=full:color_primaries=bt2020:color_trc=smpte2084:colorspace=bt2020nc"<<"-color_primaries"<<"bt2020"<<"-color_trc"<<"smpte2084"<<"-colorspace"<<"bt2020nc"<<"-color_range"<<"pc";
    if(image.text("capHdr")=="scRGB-FP16")args<<"-bsf:v"<<"av1_metadata=color_primaries=9:transfer_characteristics=16:matrix_coefficients=9:color_range=1";
    args<<"-c:v"<<"libaom-av1"<<"-crf"<<"18"<<"-b:v"<<"0"<<"-pix_fmt"<<"yuv444p10le"<<output;return args;
}

class CapApp final : public QObject, public QAbstractNativeEventFilter {
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
            if(why==QSystemTrayIcon::DoubleClick)capture();
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
    QString makePath(const QString &ext) {
        const auto dir=settings_.value("save/lastDirectory",defaultDirectory()).toString();
        QDir().mkpath(dir);
        return QDir(dir).filePath(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss-zzz")+"."+ext);
    }
    void storeDir(const QString &path) {
        settings_.setValue("save/lastDirectory",QFileInfo(path).absolutePath());settings_.sync();
    }
    void startAvif(const QImage &image, const QString &path) {
        const QString tmp=QDir(QDir::tempPath()).filePath("vscap-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png");
        QImageWriter writer(tmp,"png");
        if(!writer.write(cap::pngPixels(image))){tray_.showMessage("VS Cap","PNG 临时文件写入失败");return;}
        QString ffmpeg=QCoreApplication::applicationDirPath()+"/ffmpeg.exe";
        if(!QFile::exists(ffmpeg))ffmpeg="ffmpeg";
        auto *process=new QProcess(this);
        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process,&QProcess::errorOccurred,this,[this,process,tmp](QProcess::ProcessError error){
            if(error!=QProcess::FailedToStart)return;
            QFile::remove(tmp);tray_.showMessage("VS Cap",QStringLiteral("无法启动 FFmpeg：")+process->errorString());process->deleteLater();
        });
        connect(process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,
                [this,process,tmp,path](int code,QProcess::ExitStatus status){
                    QFile::remove(tmp);
                    const bool ok=code==0&&status==QProcess::NormalExit&&QFileInfo::exists(path);
                    tray_.showMessage("VS Cap", ok?QStringLiteral("AVIF 已保存：")+path:
                        QStringLiteral("AVIF 编码失败：")+process->readAll().right(400));
                    process->deleteLater();
                });
        process->start(ffmpeg,avifArguments(image,tmp,path));
    }
    void output(QImage image, bool pin, bool avif, bool copy) {
        if(image.isNull())return;
        if(pin) {auto *p=new PinWindow(image);pins_.append(p);return;}
        if(copy){
            const auto path=makePath("png");
            if(savePng(image,path))storeDir(path);
            else tray_.showMessage("VS Cap",QStringLiteral("PNG 写入失败"),QSystemTrayIcon::Warning);
            return;
        }
        const auto ext=avif?"avif":"png";
        const auto path=QFileDialog::getSaveFileName(nullptr,QStringLiteral("保存截图"),makePath(ext),
                         avif?"AVIF (*.avif)":"PNG (*.png)");
        if(path.isEmpty())return;
        storeDir(path);
        if(avif)startAvif(image,path);
        else if(!savePng(image,path))tray_.showMessage("VS Cap",QStringLiteral("PNG 写入失败"),QSystemTrayIcon::Warning);
    }
    void capture() {
        QScreen *screen=QGuiApplication::screenAt(QCursor::pos());
        if(!screen)screen=QGuiApplication::primaryScreen();
        if(!screen)return;
        QString error;
        const QImage shot=cap::captureScreen(screen,&error);
        if(shot.isNull()){tray_.showMessage("VS Cap",error,QSystemTrayIcon::Warning);return;}
        new CaptureOverlay(screen,shot,[this](QImage shot,bool pin,bool avif,bool copy){
            output(std::move(shot),pin,avif,copy);
        });
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
        if(pic.isNull()){tray_.showMessage("VS Cap",QStringLiteral("剪贴板中没有图像"));return;}
        auto *p=new PinWindow(std::move(pic));pins_.append(p);
    }
    void configure() {
        QDialog dialog;
        dialog.setWindowTitle(QStringLiteral("VS Cap 设置"));
        QFormLayout layout(&dialog);
        QKeySequenceEdit screenshot(QKeySequence(settings_.value("hotkeys/capture","Alt+A").toString()));
        QKeySequenceEdit pin(QKeySequence(settings_.value("hotkeys/pin","Alt+T").toString()));
        QCheckBox autostart(QStringLiteral("开机自启"));
        QCheckBox trayMode(QStringLiteral("启动时最小化到托盘"));
        autostart.setChecked(settings_.value("general/autostart",false).toBool());
        trayMode.setChecked(settings_.value("general/minimizeToTray",true).toBool());
        layout.addRow(QStringLiteral("截图快捷键"),&screenshot);
        layout.addRow(QStringLiteral("贴图快捷键"),&pin);
        layout.addRow(&autostart);layout.addRow(&trayMode);
        QLabel note(QStringLiteral("截取后可绘制、添加文字、复制、保存 PNG/AVIF 或贴图。"));
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
        settings_.setValue("general/autostart",autostart.isChecked());
        settings_.setValue("general/minimizeToTray",trayMode.isChecked());
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
    HANDLE instance=CreateMutexW(nullptr,FALSE,L"Local\\VSRenderer-VSCap");
    if(GetLastError()==ERROR_ALREADY_EXISTS){if(instance)CloseHandle(instance);return 0;}
    CapApp controller;
    const int result=app.exec();if(instance)CloseHandle(instance);return result;
}
