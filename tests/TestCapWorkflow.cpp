#define main capApplicationMain
#include "../src/cap/main.cpp"
#undef main
#include <QtTest>

class TestCapWorkflow : public QObject {
    Q_OBJECT
private slots:
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
            [&](QImage image,bool,bool,bool){delivered=image;});
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
            [&](QImage image,bool pin,bool avif,bool copy){delivered=image;copied=copy;QVERIFY(!pin);QVERIFY(!avif);});
        QVERIFY(QTest::qWaitForWindowExposed(overlay));
        QTest::mousePress(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(50,50));
        QTest::mouseMove(overlay,QPoint(350,250));
        QTest::mouseRelease(overlay,Qt::LeftButton,Qt::NoModifier,QPoint(350,250));
        const auto buttons=overlay->findChildren<QPushButton*>();QCOMPARE(buttons.size(),11);
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
        QCOMPARE(png.depth(),64);QCOMPARE(png.size(),delivered.size());
        QTRY_VERIFY(!overlay);
    }
    void selectedToolsLineAndUndo() {
        QImage source(640,480,QImage::Format_RGBA32FPx4);source.fill(QColor::fromRgbF(.4,.5,.6));
        source.setColorSpace(QColorSpace::SRgbLinear);source.setText("capHdr","scRGB-FP16");
        QImage delivered;QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,bool,bool){delivered=image;});
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
    void editableTextObjects() {
        QImage source(1000,700,QImage::Format_RGBA32FPx4);source.fill(QColor::fromRgbF(.4,.5,1.5));
        source.setColorSpace(QColorSpace::SRgbLinear);source.setText("capHdr","scRGB-FP16");
        QImage delivered;QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,bool,bool){delivered=image;});
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
        second->editor->setPlainText(QString(250,'W'));QVERIFY(QRect(QPoint(40,40),QPoint(900,500)).contains(second->geometry()));
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
        QPointer<CaptureOverlay> overlay=new CaptureOverlay(qApp->primaryScreen(),source,[&](QImage image,bool,bool,bool){delivered=image;});overlay->resize(source.size());
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
        auto *pin=new PinWindow(image);QVERIFY(pin->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        pin->showToolBar();QCOMPARE(pin->findChildren<QPushButton*>().size(),3);pin->close();
    }
    void realHdrAvif() {
        QString error;const auto desktop=cap::captureScreen(qApp->primaryScreen(),&error);QVERIFY2(!desktop.isNull(),qPrintable(error));
        auto image=desktop.copy(QRect(0,0,128,128));image.setText("capHdr",desktop.text("capHdr"));
        QTemporaryDir dir;const auto png=dir.filePath("capture.png"),avif=dir.filePath("capture.avif");QVERIFY(savePng(image,png));
        QProcess process;process.start(QCoreApplication::applicationDirPath()+"/ffmpeg.exe",avifArguments(image,png,avif));
        QSignalSpy done(&process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished));
        QTRY_VERIFY_WITH_TIMEOUT(!done.isEmpty(),30000);QCOMPARE(process.exitCode(),0);QVERIFY(QFileInfo(avif).size()>0);
        QProcess probe;probe.start(QCoreApplication::applicationDirPath()+"/ffprobe.exe",{"-v","error","-select_streams","v:0","-show_entries","stream=width,height,pix_fmt,color_primaries,color_transfer,color_range","-of","json",avif});
        QVERIFY(probe.waitForFinished(10000));const auto stream=QJsonDocument::fromJson(probe.readAllStandardOutput()).object().value("streams").toArray().first().toObject();
        QCOMPARE(stream.value("width").toInt(),128);QCOMPARE(stream.value("pix_fmt").toString(),QString("yuv444p10le"));
        if(image.text("capHdr")=="scRGB-FP16"){QCOMPARE(stream.value("color_transfer").toString(),QString("smpte2084"));QCOMPARE(stream.value("color_primaries").toString(),QString("bt2020"));QCOMPARE(stream.value("color_range").toString(),QString("pc"));}
    }
};
QTEST_MAIN(TestCapWorkflow)
#include "TestCapWorkflow.moc"
