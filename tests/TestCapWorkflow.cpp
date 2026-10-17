#define main capApplicationMain
#include "../src/cap/main.cpp"
#undef main
#include <QtTest>

class TestCapWorkflow : public QObject {
    Q_OBJECT
private slots:
    void defaultJxlCaptureAndPin(){
        const auto path=QCoreApplication::applicationDirPath()+"/vs-cap.ini";QFile originalFile(path);const bool existed=originalFile.exists();QByteArray original;
        if(existed){QVERIFY(originalFile.open(QIODevice::ReadOnly));original=originalFile.readAll();originalFile.close();}
        const auto restore=qScopeGuard([&]{if(existed){QFile file(path);if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))file.write(original);}else QFile::remove(path);});
        QTemporaryDir directory;{QSettings settings(path,QSettings::IniFormat);settings.clear();settings.setValue("save/defaultDirectory",directory.path());settings.setValue("hotkeys/capture","Ctrl+Alt+Shift+F11");settings.setValue("hotkeys/pin","Ctrl+Alt+Shift+F12");}
        CapApp controller;QCOMPARE(controller.settings_.value("save/format","jxl").toString(),QString("jxl"));
        QImage hdr(80,50,QImage::Format_RGBA32FPx4);hdr.fill(QColor::fromRgbF(.5,.6,1.5));hdr.setColorSpace(QColorSpace::SRgbLinear);hdr.setText("capHdr","scRGB-FP16");hdr.setText("capSdrWhiteNits","480");
        controller.saveHdrCaptureAsync(hdr);QTRY_VERIFY_WITH_TIMEOUT(controller.findChildren<QFutureWatcherBase*>().isEmpty(),10000);const auto files=QDir(directory.path()).entryList({"*.jxl"},QDir::Files);QCOMPARE(files.size(),2);QCOMPARE(QDir(directory.path()).entryList(QDir::Files).size(),2);
        const auto decoded=vsr::readJxl(directory.filePath(files.first()));QCOMPARE(decoded.colorSpace(),QColorSpace(QColorSpace::Bt2100Pq));QCOMPARE(decoded.text("capSdrWhiteNits"),QString("480"));const auto expected=cap::pngPixels(hdr);for(int y=0;y<hdr.height();++y)QVERIFY(std::memcmp(decoded.constScanLine(y),expected.constScanLine(y),hdr.width()*8)==0);
        const auto sdr=vsr::readJxl(directory.filePath(files.last()));QCOMPARE(sdr.colorSpace(),QColorSpace(QColorSpace::SRgb));QCOMPARE(sdr,cap::preview(hdr));
        auto *mime=new QMimeData;mime->setUrls({QUrl::fromLocalFile(directory.filePath(files.first()))});QApplication::clipboard()->setMimeData(mime);QTest::qWait(150);controller.pastePin();QCOMPARE(controller.pins_.size(),1);controller.pins_.last()->close();
    }
    void captureWhileSettingsModal(){
        QDialog settings;QLineEdit unsaved(QStringLiteral("keep pending changes"),&settings);bool verified=false;
        QTimer::singleShot(0,&settings,[&]{
            const auto closeSettings=qScopeGuard([&]{settings.reject();});
            QCOMPARE(qApp->activeModalWidget(),&settings);
            QImage source(640,480,QImage::Format_RGB32);source.fill(Qt::white);
            QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[](QImage,bool,QString,bool){return false;});
            const auto closeOverlay=qScopeGuard([&]{if(overlay)delete overlay;});
            QTRY_COMPARE_WITH_TIMEOUT(qApp->activeModalWidget(),static_cast<QWidget*>(overlay),1000);
            QTest::mousePress(overlay->windowHandle(),Qt::LeftButton,Qt::NoModifier,{20,20});
            QTest::mouseRelease(overlay->windowHandle(),Qt::LeftButton,Qt::NoModifier,{300,250});
            QVERIFY(overlay->findChild<QWidget*>("capToolBar")->isVisible());
            QTest::keyClick(overlay->windowHandle(),Qt::Key_Escape);
            QTRY_VERIFY(!overlay || !overlay->isVisible());
            QCOMPARE(qApp->activeModalWidget(),&settings);QCOMPARE(unsaved.text(),QStringLiteral("keep pending changes"));verified=true;
        });
        settings.exec();QVERIFY(verified);
    }
    void shapeFillAndWheelWidth(){
        QImage source(640,480,QImage::Format_RGBA8888);source.fill(Qt::white);QImage delivered;
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,QString,bool){delivered=image;return false;});
        const auto close=qScopeGuard([&]{if(overlay)overlay->close();});
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,{20,20});QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,{500,350});
        for(auto *button:overlay->findChildren<QPushButton*>())if(button->text()==QStringLiteral("矩形"))button->click();
        auto *stroke=overlay->findChild<QToolButton*>("capShapeWidth"),*fill=overlay->findChild<QToolButton*>("capShapeFill");
        auto *width=overlay->findChild<QSpinBox*>("capPenWidth");QVERIFY(stroke && fill && width);QVERIFY(stroke->isCheckable());QVERIFY(stroke->isChecked());QVERIFY(!fill->isChecked());QVERIFY(!fill->icon().isNull());QCOMPARE(fill->iconSize(),QSize(20,20));QCOMPARE(fill->height(),stroke->height());const int before=width->value();
        QWheelEvent wheel(QPointF(4,4),QPointF(stroke->mapToGlobal(QPoint(4,4))),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(stroke,&wheel);QCOMPARE(width->value(),before+1);QVERIFY(stroke->isChecked());fill->click();QVERIFY(fill->isChecked());QVERIFY(!stroke->isChecked());
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,{100,100});QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,{300,250});
        overlay->exportImage(false,"png");QVERIFY(!delivered.isNull());
        const auto ink=delivered.pixelColor(qRound((200-20)*640.0/overlay->width()),qRound((170-20)*480.0/overlay->height()));
        QVERIFY(ink.red()>240 && ink.green()<20);
        stroke->click();QVERIFY(stroke->isChecked());QVERIFY(!fill->isChecked());
        QApplication::sendEvent(stroke,&wheel);QCOMPARE(width->value(),before+2);
        for(auto *button:overlay->findChildren<QPushButton*>())if(button->text()==QStringLiteral("圆形"))button->click();
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,{340,100});QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,{460,240});
        overlay->exportImage(false,"png");const auto blank=delivered.pixelColor(qRound((400-20)*640.0/overlay->width()),qRound((170-20)*480.0/overlay->height()));QVERIFY(blank.green()>240);
        QImage pinImage(200,150,QImage::Format_RGBA8888);pinImage.fill(Qt::white);
        QPointer<PinWindow> pin=new PinWindow(pinImage,[](QImage,bool,QString,bool){return true;});
        const auto closePin=qScopeGuard([&]{if(pin)pin->close();});pin->showToolBar();
        QPushButton *pan=nullptr;for(auto *button:pin->findChildren<QPushButton*>())if(button->text()==QStringLiteral("拖动"))pan=button;
        QVERIFY(pan);pan->click();QVERIFY(pan->isChecked());
        auto *view=static_cast<PinView*>(pin->findChild<QGraphicsView*>("capPinView"));QVERIFY(view);QVERIFY(!view->editing());
    }
    void desktopPixelsAndOverlay() {
        QWidget marker;
        marker.setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        marker.setStyleSheet("background:white;");
        marker.setGeometry(QRect(qApp->primaryScreen()->geometry().center()-QPoint(150,150),QSize(300,300)));
        marker.show();
        QVERIFY(QTest::qWaitForWindowExposed(&marker));
        QTest::qWait(500);
        QString error;
        const auto desktop=cap::captureScreen(marker.screen(),&error);
        QVERIFY2(!desktop.isNull(),qPrintable(error));
        const auto point=(marker.geometry().center()-marker.screen()->geometry().topLeft())*marker.devicePixelRatioF();
        const auto pixel=desktop.pixelColor(point);
        const auto display=cap::preview(desktop);
        qInfo()<<"marker"<<pixel<<"preview"<<display.pixelColor(point)<<"format"<<desktop.format();
        QVERIFY(pixel.redF()>.1f && pixel.greenF()>.1f && pixel.blueF()>.1f);
        QVERIFY(display.pixelColor(point).red()>80);
        QImage delivered;
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(marker.screen(),desktop,
            [&](QImage image,bool,QString,bool){delivered=image;return true;});
        const auto close=qScopeGuard([&]{if(overlay)overlay->close();});
        QVERIFY(QTest::qWaitForWindowExposed(overlay));
        const auto local=marker.geometry().center()-overlay->geometry().topLeft();
        QVERIFY(overlay->grab().toImage().pixelColor(local*overlay->devicePixelRatioF()).red()>40);
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,local-QPoint(40,40));
        QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,local+QPoint(40,40));
        for(auto *button:overlay->findChildren<QPushButton*>())if(button->text()==QStringLiteral("✓"))button->click();
        QVERIFY(!delivered.isNull());
        QVERIFY(cap::preview(delivered).pixelColor(delivered.rect().center()).red()>80);
        QTemporaryDir dir;const auto path=dir.filePath("marker.png");
        QVERIFY(savePng(delivered,path));
        QVERIFY(QImageReader(path).read().pixelColor(delivered.rect().center()).redF()>.1f);
    }
    void selectAnnotateAndCopy() {
        QImage source(640,480,QImage::Format_RGBA32FPx4);
        source.fill(QColor::fromRgbF(1,1,1));source.setColorSpace(QColorSpace::SRgbLinear);
        source.setText("capHdr","scRGB-FP16");
        QImage delivered;bool copied=false;
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,
            [&](QImage image,bool pin,QString format,bool copy){delivered=image;copied=copy;return !pin && format!="avif";});
        QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(50,50));
        QTest::mouseMove(overlay,QPoint(350,250));
        QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(350,250));
        const auto buttons=overlay->findChildren<QPushButton*>();QCOMPARE(buttons.size(),14);
        QPushButton *rectangle=nullptr,*check=nullptr;
        for(auto *button:buttons){QVERIFY(button->isVisible());if(button->text()==QStringLiteral("矩形"))rectangle=button;if(button->text()==QStringLiteral("✓"))check=button;}
        QVERIFY(rectangle && check);rectangle->click();
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));
        QTest::mouseMove(overlay,QPoint(250,200));
        QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(250,200));
        check->click();QVERIFY(copied);QVERIFY(!delivered.isNull());
        QCOMPARE(delivered.format(),QImage::Format_RGBA32FPx4);
        QCOMPARE(delivered.text("capHdr"),QString("scRGB-FP16"));
        bool ink=false,original=false;
        for(int y=0;y<delivered.height();++y)for(int x=0;x<delivered.width();++x){
            const auto color=delivered.pixelColor(x,y);ink|=color.redF()>.9f && color.greenF()<.1f;original|=color.greenF()>.9f;
        }
        QVERIFY(ink && original);
        const auto png=QImage::fromData(QApplication::clipboard()->mimeData()->data("image/png"));
        QCOMPARE(png.depth(),64);QCOMPARE(png.size(),delivered.size());QCOMPARE(png.pixelColor(10,10),cap::preview(delivered).pixelColor(10,10));QVERIFY(png.text("capHdr").isEmpty());
        QTRY_VERIFY(!overlay);
    }
    void selectedToolsLineAndUndo() {
        QImage source(640,480,QImage::Format_RGBA32FPx4);source.fill(QColor::fromRgbF(.4,.5,.6));
        source.setColorSpace(QColorSpace::SRgbLinear);source.setText("capHdr","scRGB-FP16");
        QImage delivered;QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,QString,bool){delivered=image;return true;});
        const auto close=qScopeGuard([&]{if(overlay)overlay->close();});QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(50,50));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(450,350));
        const auto button=[&](const QString &name){for(auto *b:overlay->findChildren<QPushButton*>())if(b->text()==name)return b;return static_cast<QPushButton*>(nullptr);};
        auto *line=button(QStringLiteral("线")),*rectangle=button(QStringLiteral("矩形"));QVERIFY(line && rectangle);
        auto *options=overlay->findChild<QComboBox*>("capToolOptions");QVERIFY(options);line->click();QVERIFY(line->isChecked());QVERIFY(!rectangle->isChecked());
        QCOMPARE(options->itemText(0),QStringLiteral("直线"));QCOMPARE(options->itemText(1),QStringLiteral("箭头"));options->setCurrentIndex(1);
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));QTest::mouseMove(overlay,QPoint(300,230));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(300,230));
        QVERIFY(button(QStringLiteral("撤销"))->isEnabled());overlay->grab().save("build/acceptance-1.0.10/cap-selected-arrow.png");
        QTest::keyClick(overlay,Qt::Key_Z,Qt::ControlModifier);QTRY_VERIFY(!button(QStringLiteral("撤销"))->isEnabled());
        rectangle->click();QVERIFY(rectangle->isChecked());QVERIFY(!line->isChecked());QVERIFY(options->currentText().contains("px"));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(110,120));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(300,230));button(QStringLiteral("撤销"))->click();
        auto *copy=button(QStringLiteral("✓"));QVERIFY(copy);copy->click();QVERIFY(!delivered.isNull());
        for(int y=0;y<delivered.height();++y)for(int x=0;x<delivered.width();++x){const auto pixel=delivered.pixelColor(x,y);QVERIFY(std::abs(pixel.redF()-.4f)<1e-5);QVERIFY(std::abs(pixel.greenF()-.5f)<1e-5);QVERIFY(std::abs(pixel.blueF()-.6f)<1e-5);}
    }
    void selectionHandlesAndFinalCrop() {
        QImage source(800,600,QImage::Format_RGB32);
        for(int y=0;y<source.height();++y)for(int x=0;x<source.width();++x)source.setPixelColor(x,y,QColor(x%256,y%256,80));
        struct Resize {QPoint from,to;QRect crop;};
        for(const auto &resize:QList<Resize>{{{100,100},{80,80},{80,80,321,221}},{{250,100},{250,80},{100,80,301,221}},{{400,100},{420,80},{100,80,321,221}},{{400,200},{420,200},{100,100,321,201}},{{400,300},{420,320},{100,100,321,221}},{{250,300},{250,320},{100,100,301,221}},{{100,300},{80,320},{80,100,321,221}},{{100,200},{80,200},{80,100,321,201}}}){
            QImage delivered;QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,QString,bool){delivered=image;return true;});
            const auto close=qScopeGuard([&]{if(overlay)overlay->close();});overlay->resize(source.size());QVERIFY(QTest::qWaitForWindowExposed(overlay));
            QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(400,300));
            QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,resize.from);QTest::mouseMove(overlay,resize.to);QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,resize.to);
            overlay->exportImage(false,"png",true);QCOMPARE(delivered,source.copy(resize.crop));
        }
        source.fill(Qt::white);QImage delivered;
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,QString,bool){delivered=image;return true;});
        const auto close=qScopeGuard([&]{if(overlay)overlay->close();});overlay->resize(source.size());QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(400,300));
        const auto button=[&](const QString &name){for(auto *b:overlay->findChildren<QPushButton*>())if(b->text()==name)return b;return static_cast<QPushButton*>(nullptr);};
        QVERIFY(button(QStringLiteral("选区"))->isChecked());button(QStringLiteral("线"))->click();overlay->findChild<QComboBox*>("capToolOptions")->setCurrentIndex(1);
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(50,150));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(600,150));
        button(QStringLiteral("画笔"))->click();QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(50,260));QTest::mouseMove(overlay,QPoint(600,260));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(600,260));
        button(QStringLiteral("文字"))->click();QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(430,200));
        auto *box=static_cast<CaptureTextBox*>(overlay->findChild<QWidget*>("capTextBox"));QVERIFY(box);QTest::keyClicks(box->editor,"Outside");const auto textPosition=box->pos();
        button(QStringLiteral("选区"))->click();QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(250,200));QTest::mouseMove(overlay,QPoint(570,200));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(570,200));
        QCOMPARE(box->pos(),textPosition);QCOMPARE(box->editor->toPlainText(),QString("Outside"));
        overlay->exportImage(false,"png",true);QCOMPARE(delivered.size(),QSize(301,201));
        QCOMPARE(delivered.pixelColor(100,50),QColor(Qt::red));QCOMPARE(delivered.pixelColor(100,160),QColor(Qt::red));QCOMPARE(delivered.pixelColor(250,50),QColor(Qt::white));
        bool textInk=false;for(int y=106;y<145;++y)for(int x=16;x<120;++x)textInk|=delivered.pixelColor(x,y).green()<100;QVERIFY(textInk);
    }
    void clipboardAndSaveDirectories() {
        const auto settingsPath=QCoreApplication::applicationDirPath()+"/vs-cap.ini";
        QFile settingsFile(settingsPath);const bool existed=settingsFile.exists();QByteArray original;
        if(existed){QVERIFY(settingsFile.open(QIODevice::ReadOnly));original=settingsFile.readAll();settingsFile.close();}
        const auto restore=qScopeGuard([&]{if(existed){QFile file(settingsPath);if(file.open(QIODevice::WriteOnly|QIODevice::Truncate))file.write(original);}else QFile::remove(settingsPath);});
        QTemporaryDir dir;const auto hdrDir=dir.filePath("hdr"),manualDir=dir.filePath("manual");QVERIFY(QDir().mkpath(manualDir));
        {QSettings settings(settingsPath,QSettings::IniFormat);settings.clear();settings.setValue("hotkeys/capture","Ctrl+Alt+Shift+F11");settings.setValue("hotkeys/pin","Ctrl+Alt+Shift+F12");settings.setValue("save/defaultDirectory",hdrDir);settings.setValue("save/lastDirectory",manualDir);settings.sync();}
        CapApp controller;QImage sdr(32,32,QImage::Format_RGBA8888);sdr.fill(Qt::white);
        controller.saveHdrCaptureAsync(sdr);controller.output(sdr,false,"png",true);QVERIFY(!QFileInfo::exists(hdrDir));QVERIFY(QDir(manualDir).entryList(QDir::Files).isEmpty());
        QImage hdr(32,32,QImage::Format_RGBA32FPx4);hdr.fill(QColor::fromRgbF(.4,.5,1.5));hdr.setColorSpace(QColorSpace::SRgbLinear);hdr.setText("capHdr","scRGB-FP16");
        controller.settings_.setValue("save/format","png");controller.saveHdrCaptureAsync(hdr);QTRY_VERIFY_WITH_TIMEOUT(controller.findChildren<QFutureWatcherBase*>().isEmpty(),10000);
        const auto files=QDir(hdrDir).entryList(QDir::Files);QCOMPARE(files.size(),2);
        const auto pngs=QDir(hdrDir).entryList({"*.png"},QDir::Files);QCOMPARE(pngs.size(),2);QCOMPARE(QImageReader(QDir(hdrDir).filePath(pngs.first())).read().depth(),64);
        const auto sdrPath=QDir(hdrDir).filePath(pngs.last());QVERIFY(sdrPath.endsWith("_SDR.png"));
        QCOMPARE(QImageReader(sdrPath).read().pixelColor(10,10),cap::preview(hdr).pixelColor(10,10));
        QVERIFY(!QFile::exists(sdrPath+".json"));QVERIFY(!QFile::exists(sdrPath+".scrgb-f32"));
        QVERIFY(!QFile::exists(QDir(hdrDir).filePath(pngs.first()+".scrgb-f32")));
        controller.output(hdr,false,"png",true);QCOMPARE(QDir(hdrDir).entryList(QDir::Files),files);QVERIFY(QDir(manualDir).entryList(QDir::Files).isEmpty());
        QCOMPARE(controller.settings_.value("save/lastDirectory").toString(),manualDir);
        auto large=hdr.scaled(2560,1440);QElapsedTimer automaticTimer;automaticTimer.start();
        controller.settings_.setValue("save/format","png");controller.saveHdrCaptureAsync(large);QVERIFY(automaticTimer.elapsed()<100);
        QVERIFY(!controller.findChildren<QFutureWatcherBase*>().isEmpty());
        bool responsive=false;QTimer::singleShot(0,&controller,[&]{responsive=true;});
        QTRY_VERIFY_WITH_TIMEOUT(responsive,500);
        QTRY_VERIFY_WITH_TIMEOUT(controller.findChildren<QFutureWatcherBase*>().isEmpty(),10000);
        QCOMPARE(QDir(hdrDir).entryList(QDir::Files).size(),4);
        const bool nativeDisabled=QCoreApplication::testAttribute(Qt::AA_DontUseNativeDialogs);QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs,true);
        const auto restoreDialogs=qScopeGuard([&]{QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs,nativeDisabled);});
        QTimer watchdog;watchdog.setSingleShot(true);connect(&watchdog,&QTimer::timeout,qApp,[]{if(auto *dialog=qApp->activeModalWidget())QMetaObject::invokeMethod(dialog,"reject");});
        for(int i=0;i<2;++i){
            const auto path=QDir(manualDir).filePath(QString("manual-%1.png").arg(i));bool dialogSeen=false;
            QTimer::singleShot(50,qApp,[&]{auto *dialog=qobject_cast<QFileDialog*>(qApp->activeModalWidget());if(!dialog)return;dialogSeen=true;QCOMPARE(dialog->directory().absolutePath(),manualDir);auto *name=dialog->findChild<QLineEdit*>("fileNameEdit");QVERIFY(name);name->setText(path);QMetaObject::invokeMethod(dialog,"accept");});
            watchdog.start(2000);controller.output(sdr,false,"png",false);watchdog.stop();QVERIFY(dialogSeen);QVERIFY2(QFileInfo::exists(path),qPrintable(QDir(manualDir).entryList(QDir::Files).join(',')));
        }
        QImage capture(1000,700,QImage::Format_RGB32);capture.fill(Qt::white);
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),capture,[&](QImage image,bool pin,QString format,bool copy){return controller.output(std::move(image),pin,format,copy);});
        const auto closeOverlay=qScopeGuard([&]{if(overlay)overlay->close();});QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(40,40));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(900,500));
        const auto button=[&](const QString &name){for(auto *b:overlay->findChildren<QPushButton*>())if(b->text()==name)return b;return static_cast<QPushButton*>(nullptr);};
        button(QStringLiteral("文字"))->click();QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,120));auto *editor=overlay->findChild<QTextEdit*>("capTextEditor");QVERIFY(editor);QTest::keyClicks(editor,"Keep after cancel");
        for(const auto name:{QStringLiteral("JXL"),QStringLiteral("AVIF"),QStringLiteral("PNG")}){
            bool dialogSeen=false;QTimer::singleShot(50,qApp,[&]{auto *dialog=qobject_cast<QFileDialog*>(qApp->activeModalWidget());if(!dialog)return;dialogSeen=true;dialog->reject();});
            watchdog.start(2000);button(name)->click();watchdog.stop();QVERIFY(dialogSeen);QVERIFY(overlay && overlay->isVisible());QCOMPARE(editor->toPlainText(),QString("Keep after cancel"));
        }
        const auto retryPath=QDir(manualDir).filePath("retried.png");
        QTimer::singleShot(50,qApp,[&]{auto *dialog=qobject_cast<QFileDialog*>(qApp->activeModalWidget());QVERIFY(dialog);dialog->findChild<QLineEdit*>("fileNameEdit")->setText(retryPath);QMetaObject::invokeMethod(dialog,"accept");});
        watchdog.start(2000);button(QStringLiteral("PNG"))->click();watchdog.stop();QVERIFY(QFileInfo::exists(retryPath));QTRY_VERIFY(!overlay);
        const auto copiedPath=dir.filePath(QStringLiteral("复制 图片.png"));QVERIFY(sdr.save(copiedPath));
        const auto nonImage=dir.filePath("not-image.txt");{QFile file(nonImage);QVERIFY(file.open(QIODevice::WriteOnly));file.write("not an image");}
        auto *mime=new QMimeData;mime->setUrls({QUrl("https://example.invalid/image.png"),QUrl::fromLocalFile(nonImage),QUrl::fromLocalFile(copiedPath)});QApplication::clipboard()->setMimeData(mime);
        QTRY_VERIFY(QApplication::clipboard()->mimeData()->hasUrls());QVERIFY(QApplication::clipboard()->image().isNull());
        const auto oldPins=controller.pins_.size();controller.pastePin();QCOMPARE(controller.pins_.size(),oldPins+1);
        auto pin=controller.pins_.last();const auto closePin=qScopeGuard([&]{if(pin)pin->close();});QVERIFY(QTest::qWaitForWindowExposed(pin));
        auto *view=pin->findChild<QGraphicsView*>("capPinView");QVERIFY(view);auto *proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(view->scene()->items().first());QVERIFY(proxy);
        static_cast<CaptureOverlay*>(proxy->widget())->exportImage(false,"png",true);
        QTRY_COMPARE(QImage::fromData(QApplication::clipboard()->mimeData()->data("image/png")).convertToFormat(sdr.format()),sdr);
        QTimer::singleShot(50,qApp,[&]{auto *dialog=qApp->activeModalWidget();QVERIFY(dialog);QVERIFY(dialog->findChild<QLineEdit*>("capDefaultSaveDirectory"));QCOMPARE(dialog->findChild<QLineEdit*>("capDefaultSaveDirectory")->text(),hdrDir);QMetaObject::invokeMethod(dialog,"reject");});
        watchdog.start(2000);controller.configure();watchdog.stop();QCOMPARE(controller.settings_.value("save/defaultDirectory").toString(),hdrDir);QCOMPARE(controller.settings_.value("save/lastDirectory").toString(),manualDir);
    }
    void editableTextObjects() {
        QImage source(1000,700,QImage::Format_RGBA32FPx4);source.fill(QColor::fromRgbF(.4,.5,1.5));
        source.setColorSpace(QColorSpace::SRgbLinear);source.setText("capHdr","scRGB-FP16");
        QImage delivered;QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,QString,bool){delivered=image;return true;});
        const auto close=qScopeGuard([&]{if(overlay)overlay->close();});overlay->resize(source.size());QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(40,40));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(900,500));
        const auto button=[&](const QString &name){for(auto *b:overlay->findChildren<QPushButton*>())if(b->text()==name)return b;return static_cast<QPushButton*>(nullptr);};
        button(QStringLiteral("文字"))->click();QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,120));
        auto boxes=overlay->findChildren<QWidget*>("capTextBox");QCOMPARE(boxes.size(),1);auto *first=static_cast<CaptureTextBox*>(boxes.first());
        QVERIFY(first->isVisible());QVERIFY(first->editor->hasFocus());QVERIFY(!qApp->activeModalWidget());
        QTest::keyClicks(first->editor,"First");QCOMPARE(first->editor->toPlainText(),QString("First"));
        QTest::keyClick(first->editor,Qt::Key_Z,Qt::ControlModifier);QCOMPARE(first->editor->toPlainText(),QString());
        QTest::keyClicks(first->editor,"Hello");first->editor->insertPlainText(QStringLiteral("\n世界"));
        auto *family=overlay->findChild<QFontComboBox*>("capFontFamily");auto *size=overlay->findChild<QSpinBox*>("capFontSize");
        auto *bold=overlay->findChild<QToolButton*>("capFontBold"),*italic=overlay->findChild<QToolButton*>("capFontItalic");
        QVERIFY(family && size && bold && italic);QVERIFY(family->isVisible() && family->isEditable());
        const auto families=QFontDatabase::families();QVERIFY(!families.isEmpty());const auto name=families.contains("Arial")?QString("Arial"):families.first();
        family->completer()->setCompletionPrefix(name.left(2));QVERIFY(family->completer()->completionCount()>0);family->setCurrentFont(QFont(name));
        auto *number=size->findChild<QLineEdit*>();QVERIFY(number);number->setFocus();number->selectAll();QTest::keyClicks(number,"32");QTest::keyClick(number,Qt::Key_Enter);
        QCOMPARE(size->value(),32);bold->click();italic->click();QCOMPARE(first->editor->font().family(),name);
        QCOMPARE(first->editor->font().pointSize(),32);QVERIFY(first->editor->font().bold());QVERIFY(first->editor->font().italic());
        QTest::keyClick(first->editor,Qt::Key_Z,Qt::ControlModifier);QVERIFY(!first->editor->font().italic());italic->click();
        QVERIFY(first->editor->viewport()->height()>=first->editor->document()->size().height());
        const auto moveBox=[](CaptureTextBox *box,const QPoint &delta){
            const QPoint start(3,3);QTest::mousePress(box,Qt::LeftButton,Qt::NoModifier,start);
            QMouseEvent event(QEvent::MouseMove,QPointF(start+delta),QPointF(box->mapToGlobal(start+delta)),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(box,&event);QTest::mouseRelease(box,Qt::LeftButton,Qt::NoModifier,start);
        };
        const auto origin=first->pos();moveBox(first,QPoint(60,45));QCOMPARE(first->pos(),origin+QPoint(60,45));
        button(QStringLiteral("撤销"))->click();QCOMPARE(first->pos(),origin);QCOMPARE(first->editor->toPlainText(),QStringLiteral("Hello\n世界"));
        moveBox(first,QPoint(40,30));button(QStringLiteral("矩形"))->click();QVERIFY(!family->isVisible());
        QTest::mouseClick(first->editor->viewport(),Qt::LeftButton);QVERIFY(button(QStringLiteral("文字"))->isChecked());QVERIFY(family->isVisible());
        first->editor->selectAll();first->editor->insertPlainText(QStringLiteral("Revised\n文字"));
        QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(600,150));boxes=overlay->findChildren<QWidget*>("capTextBox");QCOMPARE(boxes.size(),2);
        auto *second=static_cast<CaptureTextBox*>(boxes.last());QTest::keyClicks(second->editor,"Other");size->setValue(14);QCOMPARE(second->editor->font().pointSize(),14);QCOMPARE(first->editor->font().pointSize(),32);
        second->editor->setPlainText(QString(250,'W'));QVERIFY(overlay->rect().contains(second->geometry()));
        QVERIFY(second->editor->document()->size().height()>QFontMetrics(second->editor->font()).height()*2);
        QTest::keyClick(second->editor,Qt::Key_Z,Qt::ControlModifier);QCOMPARE(second->editor->toPlainText(),QString("Other"));QCOMPARE(second->pos(),QPoint(600,150));
        QTest::mouseClick(first->editor->viewport(),Qt::LeftButton);QCOMPARE(size->value(),32);QVERIFY(bold->isChecked() && italic->isChecked());
        QDir().mkpath("build/acceptance-cap-text");overlay->grab().save("build/acceptance-cap-text/editable-text.png");
        button(QStringLiteral("✓"))->click();QVERIFY(!delivered.isNull());QCOMPARE(delivered.format(),QImage::Format_RGBA32FPx4);QCOMPARE(delivered.text("capHdr"),QString("scRGB-FP16"));
        bool firstInk=false,secondInk=false;
        for(int y=0;y<delivered.height();++y)for(int x=0;x<delivered.width();++x){
            const auto pixel=delivered.pixelColor(x,y);QVERIFY(!(pixel.redF()>.9 && pixel.greenF()>.9 && pixel.blueF()>.9));
            if(pixel.redF()>.9 && pixel.greenF()<.1){if(x<400)firstInk=true;else secondInk=true;}
        }
        QVERIFY(firstInk && secondInk);QCOMPARE(delivered.pixelColor(60,80),source.pixelColor(100,120));
        QVERIFY(savePng(delivered,"build/acceptance-cap-text/exported-text.png"));
    }
    void textAndDrawingUndo() {
        QImage source(800,600,QImage::Format_RGBA8888);source.fill(Qt::white);QImage delivered;
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,QString,bool){delivered=image;return true;});overlay->resize(source.size());
        const auto close=qScopeGuard([&]{if(overlay)overlay->close();});QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(20,20));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(700,400));
        const auto button=[&](const QString &name){for(auto *b:overlay->findChildren<QPushButton*>())if(b->text()==name)return b;return static_cast<QPushButton*>(nullptr);};
        button(QStringLiteral("文字"))->click();QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));
        auto *box=overlay->findChild<QTextEdit*>("capTextEditor");QTest::keyClicks(box,"Undo me");button(QStringLiteral("矩形"))->click();
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(350,150));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(500,250));
        button(QStringLiteral("撤销"))->click();QCOMPARE(box->toPlainText(),QString("Undo me"));
        QTest::keyClick(box,Qt::Key_Z,Qt::ControlModifier);QCOMPARE(box->toPlainText(),QString());
        QTest::keyClick(box,Qt::Key_Z,Qt::ControlModifier);QTRY_VERIFY(overlay->findChildren<QTextEdit*>("capTextEditor").isEmpty());
        QVERIFY(!button(QStringLiteral("撤销"))->isEnabled());button(QStringLiteral("PNG"))->click();QCOMPARE(delivered,source.copy(QRect(QPoint(20,20),QPoint(700,400))));
    }
    void toolbarShapesAndTextFrames() {
        QImage source(1000,700,QImage::Format_RGBA8888);source.fill(QColor(70,80,90));
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[](QImage,bool,QString,bool){return true;});overlay->resize(source.size());
        const auto close=qScopeGuard([&]{if(overlay)overlay->close();});QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(40,40));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(900,500));
        const auto button=[&](const QString &name){for(auto *b:overlay->findChildren<QPushButton*>())if(b->text()==name)return b;return static_cast<QPushButton*>(nullptr);};
        QCOMPARE(overlay->findChild<QWidget*>("capToolBar")->cursor().shape(),Qt::ArrowCursor);
        QCOMPARE(button(QStringLiteral("✓"))->toolTip(),QStringLiteral("复制到剪贴板"));
        QVERIFY(button(QStringLiteral("矩形"))->toolTip().contains("Ctrl"));QVERIFY(button(QStringLiteral("圆形"))->toolTip().contains("Ctrl"));
        button(QStringLiteral("画笔"))->click();auto *width=overlay->findChild<QSpinBox*>("capPenWidth");QVERIFY(width && width->isVisible());QCOMPARE(width->value(),3);QCOMPARE(width->suffix(),QString(" px"));
        auto *number=width->findChild<QLineEdit*>();number->selectAll();QTest::keyClicks(number,"7");QTest::keyClick(number,Qt::Key_Enter);QCOMPARE(width->value(),7);
        const auto inkBounds=[&]{
            const auto image=overlay->grab().toImage().scaled(overlay->size(),Qt::IgnoreAspectRatio,Qt::FastTransformation);QRect bounds;
            for(int y=50;y<490;++y)for(int x=50;x<890;++x){const auto pixel=image.pixelColor(x,y);if(pixel.red()>230 && pixel.green()<60)bounds=bounds.united(QRect(x,y,1,1));}
            return bounds;
        };
        for(const auto &tool:{QStringLiteral("矩形"),QStringLiteral("圆形")}){
            button(tool)->click();QCOMPARE(overlay->findChild<QComboBox*>("capToolOptions")->currentText(),QString("7 px"));QTest::mousePress(overlay,Qt::LeftButton,Qt::ControlModifier,QPoint(100,100));
            QTest::mouseRelease(overlay,Qt::LeftButton,Qt::ControlModifier,QPoint(300,200));const auto ink=inkBounds();
            QVERIFY(!ink.isEmpty());QVERIFY(qAbs(ink.width()-ink.height())<=1);QVERIFY(ink.width()<120);button(QStringLiteral("撤销"))->click();
        }
        button(QStringLiteral("线"))->click();QVERIFY(width->isVisible());QCOMPARE(width->value(),7);width->setValue(9);
        button(QStringLiteral("画笔"))->click();QCOMPARE(width->value(),9);button(QStringLiteral("线"))->click();QCOMPARE(width->value(),9);
        auto *options=overlay->findChild<QComboBox*>("capToolOptions");QCOMPARE(options->count(),3);options->setCurrentText(QStringLiteral("虚线"));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(400,100));
        auto image=overlay->grab().toImage().scaled(overlay->size(),Qt::IgnoreAspectRatio,Qt::FastTransformation);int transitions=0;bool previous=false;
        for(int x=100;x<=400;++x){const auto pixel=image.pixelColor(x,100);const bool ink=pixel.red()>230 && pixel.green()<60;if(ink!=previous)++transitions;previous=ink;}
        QVERIFY(transitions>6);button(QStringLiteral("撤销"))->click();
        button(QStringLiteral("文字"))->click();auto *size=overlay->findChild<QSpinBox*>("capFontSize");
        QStyleOptionSpinBox option;option.initFrom(size);option.buttonSymbols=QAbstractSpinBox::UpDownArrows;option.stepEnabled=QAbstractSpinBox::StepUpEnabled | QAbstractSpinBox::StepDownEnabled;
        const auto up=size->style()->subControlRect(QStyle::CC_SpinBox,&option,QStyle::SC_SpinBoxUp,size),down=size->style()->subControlRect(QStyle::CC_SpinBox,&option,QStyle::SC_SpinBoxDown,size);
        QVERIFY(!up.isEmpty() && !down.isEmpty() && !up.intersects(down));QTest::mouseClick(size,Qt::LeftButton,Qt::NoModifier,up.center());QCOMPARE(size->value(),19);
        QTest::mouseClick(size,Qt::LeftButton,Qt::NoModifier,down.center());QCOMPARE(size->value(),18);
        QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(100,120));QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(300,120));
        QTRY_COMPARE(overlay->findChildren<QWidget*>("capTextBox").size(),1);auto *box=static_cast<CaptureTextBox*>(overlay->findChild<QWidget*>("capTextBox"));QCOMPARE(box->pos(),QPoint(300,120));
        QTest::keyClicks(box->editor,"Frame");QTest::mouseClick(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(600,120));QTest::mouseClick(button(QStringLiteral("画笔")),Qt::LeftButton);
        QTRY_COMPARE(overlay->findChildren<QWidget*>("capTextBox").size(),1);QCOMPARE(box->editor->toPlainText(),QString("Frame"));
        const auto globalStart=box->mapToGlobal(QPoint(3,3));QTest::mousePress(box,Qt::LeftButton,Qt::NoModifier,QPoint(3,3));
        for(int step=1;step<=6;++step){
            const auto global=globalStart-QPoint(step*10,step*5);
            QMouseEvent event(QEvent::MouseMove,QPointF(box->mapFromGlobal(global)),QPointF(global),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);QApplication::sendEvent(box,&event);QTest::qWait(10);
            image=overlay->grab().toImage().scaled(overlay->size(),Qt::IgnoreAspectRatio,Qt::FastTransformation).copy(box->geometry());
            const auto white=[&](int x,int y){const auto pixel=image.pixelColor(x,y);return pixel.red()>230 && pixel.green()>230 && pixel.blue()>230;};
            int top=0,bottom=0,left=0,right=0;
            for(int x=2;x<image.width()-2;++x){top+=white(x,2);bottom+=white(x,image.height()-3);}
            for(int y=2;y<image.height()-2;++y){left+=white(2,y);right+=white(image.width()-3,y);}
            QVERIFY2(top>2 && bottom>2 && left>2 && right>2,qPrintable(QString("frame %1: top %2 bottom %3 left %4 right %5").arg(step).arg(top).arg(bottom).arg(left).arg(right)));
        }
        QTest::mouseRelease(box,Qt::LeftButton,Qt::NoModifier,QPoint(3,3));QDir().mkpath("build/acceptance-cap-interaction");overlay->grab().save("build/acceptance-cap-interaction/text-frame.png");
    }
    void pinAspectZoomAndEditing() {
        QImage source(1600,400,QImage::Format_RGBA8888);source.fill(Qt::white);
        {QPainter painter(&source);for(int y=0;y<400;y+=10)for(int x=0;x<1600;x+=10)if((x/10+y/10)%2)painter.fillRect(x,y,10,10,Qt::black);}
        QImage delivered;bool avif=false;
        QPointer<PinWindow> pin=new PinWindow(source,[&](QImage image,bool,QString format,bool){delivered=image;avif=format=="avif";return true;});
        const auto close=qScopeGuard([&]{if(pin)pin->close();});QVERIFY(QTest::qWaitForWindowExposed(pin));
        auto *view=pin->findChild<QGraphicsView*>("capPinView");QVERIFY(view);const double originalZoom=1.0/pin->screen()->devicePixelRatio();
        QCOMPARE(view->size(),QSize(qRound(1600*originalZoom),qRound(400*originalZoom)));QCOMPARE(view->transform().m11(),originalZoom);QCOMPARE(view->transform().m11(),view->transform().m22());
        QWheelEvent wheel(QPointF(view->viewport()->rect().center()),QPointF(view->viewport()->mapToGlobal(view->viewport()->rect().center())),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(view->viewport(),&wheel);QTest::qWait(20);QCOMPARE(view->size(),QSize(qRound(1920*originalZoom),qRound(480*originalZoom)));QCOMPARE(view->transform().m11(),view->transform().m22());
        QVERIFY(!view->renderHints().testFlag(QPainter::SmoothPixmapTransform));
        const auto pixels=view->viewport()->grab().toImage();int black=0,white=0;
        for(int y=3;y<pixels.height()-3;++y)for(int x=3;x<pixels.width()-3;++x){const auto pixel=pixels.pixelColor(x,y);QVERIFY(pixel==QColor(Qt::black) || pixel==QColor(Qt::white));black+=pixel==QColor(Qt::black);white+=pixel==QColor(Qt::white);}
        QVERIFY(black>100 && white>100);
        auto *menu=pin->findChild<QMenu*>("capPinMenu");QVERIFY(menu);const auto actions=menu->actions();QCOMPARE(actions.size(),6);
        QCOMPARE(actions[0]->text(),QStringLiteral("显示工具栏"));QCOMPARE(actions[1]->text(),QStringLiteral("复制"));QCOMPARE(actions[2]->text(),QStringLiteral("保存为JXL"));QCOMPARE(actions[3]->text(),QStringLiteral("保存为AVIF"));QCOMPARE(actions[4]->text(),QStringLiteral("保存为PNG"));QCOMPARE(actions[5]->text(),QStringLiteral("销毁"));
        actions[2]->trigger();QCOMPARE(delivered,source);QVERIFY(!avif);actions[3]->trigger();QCOMPARE(delivered,source);QVERIFY(avif);
        actions[1]->trigger();QTRY_COMPARE(QImage::fromData(QApplication::clipboard()->mimeData()->data("image/png")).size(),source.size());QVERIFY(pin->isVisible());
        actions[0]->trigger();auto *bar=pin->findChild<QWidget*>("capToolBar");QVERIFY(bar && bar->isVisible());QCOMPARE(bar->cursor().shape(),Qt::ArrowCursor);
        const auto button=[&](const QString &name){for(auto *b:pin->findChildren<QPushButton*>())if(b->text()==name)return b;return static_cast<QPushButton*>(nullptr);};
        for(const auto &name:{QStringLiteral("画笔"),QStringLiteral("矩形"),QStringLiteral("圆形"),QStringLiteral("线"),QStringLiteral("文字"),QStringLiteral("撤销"),QStringLiteral("PNG"),QStringLiteral("AVIF"),QStringLiteral("✓")})QVERIFY(button(name) && button(name)->isVisible());
        QCOMPARE(button(QStringLiteral("✓"))->toolTip(),QStringLiteral("隐藏工具栏"));
        menu->popup(pin->mapToGlobal(QPoint(10,10)));QTRY_VERIFY(menu->isVisible());QCOMPARE(actions[0]->text(),QStringLiteral("隐藏工具栏"));menu->hide();
        actions[0]->trigger();QVERIFY(!bar->isVisible());actions[0]->trigger();QVERIFY(bar->isVisible());
        const auto clipboardBefore=QApplication::clipboard()->mimeData()->data("image/png");
        button(QStringLiteral("✓"))->click();QVERIFY(!bar->isVisible());QVERIFY(pin->isVisible());QCOMPARE(QApplication::clipboard()->mimeData()->data("image/png"),clipboardBefore);
        menu->popup(pin->mapToGlobal(QPoint(10,10)));QTRY_VERIFY(menu->isVisible());QCOMPARE(actions[0]->text(),QStringLiteral("显示工具栏"));menu->hide();actions[0]->trigger();
        auto *proxy=qgraphicsitem_cast<QGraphicsProxyWidget*>(view->scene()->items().first());QVERIFY(proxy);
        auto *canvas=static_cast<CaptureOverlay*>(proxy->widget());QVERIFY(canvas);button(QStringLiteral("文字"))->click();
        QTest::mouseClick(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));auto *editor=canvas->findChild<QTextEdit*>("capTextEditor");QVERIFY(editor);QTest::keyClicks(editor,"Pinned text");
        actions[2]->trigger();QCOMPARE(delivered.size(),source.size());QVERIFY(delivered!=source);QVERIFY(pin->isVisible());
        QTest::keyClick(editor,Qt::Key_Escape);QVERIFY(!bar->isVisible());QVERIFY(canvas->isVisible());actions[0]->trigger();
        button(QStringLiteral("撤销"))->click();button(QStringLiteral("撤销"))->click();actions[2]->trigger();QCOMPARE(delivered,source);
        button(QStringLiteral("矩形"))->click();QTest::mousePress(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));QTest::mouseRelease(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(250,200));actions[2]->trigger();QVERIFY(delivered!=source);
        pin->activateWindow();QVERIFY(QTest::qWaitForWindowActive(pin));view->setFocus();QTest::keyClick(view,Qt::Key_Z,Qt::ControlModifier);actions[2]->trigger();QCOMPARE(delivered,source);
        QTest::mousePress(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(100,100));QTest::mouseRelease(canvas,Qt::LeftButton,Qt::NoModifier,QPoint(250,200));
        bar->activateWindow();QVERIFY(QTest::qWaitForWindowActive(bar));QTest::keyClick(bar,Qt::Key_Z,Qt::ControlModifier);actions[2]->trigger();QCOMPARE(delivered,source);
        QDir().mkpath("build/acceptance-cap-interaction");pin->grab().save("build/acceptance-cap-interaction/pin-nearest.png");bar->grab().save("build/acceptance-cap-interaction/pin-toolbar.png");
        actions[5]->trigger();QTRY_VERIFY(!pin);
    }
    void pinScreenFitAndBorders() {
        auto *screen=QGuiApplication::screenAt(QCursor::pos());if(!screen)screen=qApp->primaryScreen();
        const auto available=screen->availableGeometry();
        const auto checkBorder=[](PinWindow *pin){
            const auto image=pin->grab().toImage();const QColor blue(70,135,200);
            for(int x=0;x<image.width();++x){QCOMPARE(image.pixelColor(x,0),blue);QCOMPARE(image.pixelColor(x,image.height()-1),blue);}
            for(int y=0;y<image.height();++y){QCOMPARE(image.pixelColor(0,y),blue);QCOMPARE(image.pixelColor(image.width()-1,y),blue);}
        };
        for(const auto size:{QSize(320,180),QSize(qRound(available.width()*screen->devicePixelRatio()*2),qRound(available.height()*screen->devicePixelRatio()*2))}){
            QImage source(size,QImage::Format_RGBA8888);source.fill(Qt::white);QImage delivered;
            QPointer<PinWindow> pin=new PinWindow(source,[&](QImage image,bool,QString,bool){delivered=image;return true;});
            const auto close=qScopeGuard([&]{if(pin)pin->close();});QVERIFY(QTest::qWaitForWindowExposed(pin));
            auto *view=pin->findChild<QGraphicsView*>("capPinView");QVERIFY(view);
            QVERIFY(available.contains(pin->geometry()));QCOMPARE(view->transform().m11(),view->transform().m22());
            if(size.width()==320)QCOMPARE(view->transform().m11(),1.0/screen->devicePixelRatio());
            else{QVERIFY(view->transform().m11()<1.0/screen->devicePixelRatio());QVERIFY(pin->width()==available.width() || pin->height()==available.height());}
            checkBorder(pin);pin->move(pin->pos()+QPoint(10,10));QTest::qWait(20);checkBorder(pin);
            pin->findChild<QMenu*>("capPinMenu")->actions()[2]->trigger();QCOMPARE(delivered,source);
        }
    }
    void hdrClipboardAndPin() {
        QString error;const auto desktop=cap::captureScreen(qApp->primaryScreen(),&error);
        QVERIFY2(!desktop.isNull(),qPrintable(error));
        auto image=desktop.copy(QRect(0,0,256,256));image.setText("capHdr",desktop.text("capHdr"));
        copyImage(image);
        QTRY_VERIFY_WITH_TIMEOUT(QImage::fromData(QApplication::clipboard()->mimeData()->data("image/png")).size()==image.size(),5000);
        const auto *mime=QApplication::clipboard()->mimeData();
        QVERIFY(mime->hasImage());QVERIFY(mime->hasFormat("image/png"));
        const auto png=QImage::fromData(mime->data("image/png"));QCOMPARE(png.size(),image.size());
        if(image.text("capHdr")=="scRGB-FP16"){
            QCOMPARE(png.depth(),64);const auto raw=mime->data("application/x-vscap-scrgb");
            QCOMPARE(raw.size(),8+image.width()*image.height()*16);
            QCOMPARE(raw.mid(8,image.width()*16),QByteArray(reinterpret_cast<const char*>(image.constScanLine(0)),image.width()*16));
        }
        auto *pin=new PinWindow(image,[](QImage,bool,QString,bool){return true;});QVERIFY(pin->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        pin->showToolBar();QCOMPARE(pin->findChildren<QPushButton*>().size(),14);pin->close();
        QImage grey(64,32,QImage::Format_RGBA32FPx4);grey.fill(QColor::fromRgbF(1.2842468f,1.2842468f,1.2842468f));
        grey.setColorSpace(QColorSpace::SRgbLinear);grey.setText("capHdr","scRGB-FP16");grey.setText("capSdrWhiteNits","480");copyImage(grey);
        const auto clipboardIsGrey=[] {const auto image=QImage::fromData(QApplication::clipboard()->mimeData()->data("image/png"));return !image.isNull() && qAbs(image.pixelColor(10,10).redF()-.5)<.0001;};
        QTRY_VERIFY(clipboardIsGrey());
        QCOMPARE(QApplication::clipboard()->mimeData()->data("application/x-vscap-sdr-white-nits"),QByteArray("480"));
        CapApp controller;controller.pastePin();QCOMPARE(controller.pins_.size(),1);auto *roundTrip=controller.pins_.last().data();QVERIFY(roundTrip);
        const auto closePin=qScopeGuard([&]{if(controller.pins_.last())controller.pins_.last()->close();});
        auto *menu=roundTrip->findChild<QMenu*>("capPinMenu");QVERIFY(menu);menu->actions()[1]->trigger();QTest::qWait(150);
        QTRY_VERIFY(clipboardIsGrey());
        QCOMPARE(QApplication::clipboard()->mimeData()->data("application/x-vscap-sdr-white-nits"),QByteArray("480"));
    }
    void realHdrAvif() {
        QImage desktop(128,128,QImage::Format_RGBA32FPx4);
        desktop.fill(Qt::white);desktop.setColorSpace(QColorSpace::SRgbLinear);
        desktop.setText("capHdr","scRGB-FP16");
        auto image=desktop.copy(QRect(0,0,128,128));image.setText("capHdr",desktop.text("capHdr"));
        QTemporaryDir dir;const auto png=dir.filePath("capture.png"),avif=dir.filePath("capture.avif");QVERIFY(savePng(image,png));
        CapApp app;QVERIFY(app.startAvif(image,avif));
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo(avif).size()>0,60000);
        QFile file(avif);QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.read(64).contains("ftypavif"));



    }
};
QTEST_MAIN(TestCapWorkflow)
#include "TestCapWorkflow.moc"
