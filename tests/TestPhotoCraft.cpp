#include "player/PhotoCraftEditor.h"
#include "player/PlayerImageTools.h"
#include "player/PlayerPng.h"
#include "ui/PreviewPane.h"
#include <QApplication>
#include <QCryptographicHash>
#include <QColorSpace>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <QUuid>

using namespace vsr;
class TestPhotoCraft final : public QObject {
    Q_OBJECT
    QTcpSocket control_;
    QTemporaryDir artifacts_{QDir::tempPath()+"/vsp-photocraft-test-XXXXXX"};
    quint16 port_ = 0;
    QByteArray token_;
    QJsonObject call(const QString &method,const QJsonObject &params={}) {
        control_.write(QJsonDocument(QJsonObject{{"id",method},{"method",method},{"params",params}}).toJson(QJsonDocument::Compact)+'\n');control_.flush();
        QElapsedTimer timer;timer.start();
        while(!control_.canReadLine() && timer.elapsed()<30000){QCoreApplication::processEvents();control_.waitForReadyRead(20);}
        const auto reply=QJsonDocument::fromJson(control_.readLine()).object();
        if(!reply.value("ok").toBool())qWarning().noquote()<<method<<QJsonDocument(reply).toJson(QJsonDocument::Compact);
        return reply;
    }
    QJsonObject inspect() {return call("ui.inspect").value("result").toObject();}
    bool connectControl() {
        if(control_.state()==QTcpSocket::ConnectedState)return true;
        control_.abort();control_.connectToHost(QHostAddress::LocalHost,port_);return control_.waitForConnected(50);
    }
    static QByteArray hash(const QString &path) {QFile file(path);if(!file.open(QIODevice::ReadOnly))return {};return QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256);}
private slots:
    void initTestCase() {
        QVERIFY2(QFileInfo::exists(PhotoCraftEditor::executablePath()),"Build the pinned PhotoCraft runtime before native integration tests");
        QVERIFY(artifacts_.isValid());artifacts_.setAutoRemove(false);qInfo().noquote()<<"PhotoCraft evidence:"<<artifacts_.path();
        QTcpServer port;QVERIFY(port.listen(QHostAddress::LocalHost,0));port_=port.serverPort();port.close();
        token_=QUuid::createUuid().toByteArray(QUuid::Id128)+QUuid::createUuid().toByteArray(QUuid::Id128);
        qputenv("PHOTOCRAFT_CONTROL_PORT",QByteArray::number(port_));qputenv("PHOTOCRAFT_CONTROL_TOKEN",token_);
        qputenv("PHOTOCRAFT_AUTOMATION_READ_ROOT",artifacts_.path().toUtf8());qputenv("PHOTOCRAFT_AUTOMATION_WRITE_ROOT",artifacts_.path().toUtf8());
        qputenv("PHOTOCRAFT_LOCALE","zh-CN");
    }
    void nativeEntryMultidocumentPrecisionAndTools() {
        const auto cleanup=qScopeGuard([this]{if(control_.state()==QTcpSocket::ConnectedState){call("app.quit");control_.waitForDisconnected(5000);}control_.abort();});
        QImage image(320,240,QImage::Format_RGBA64);image.setColorSpace(QColorSpace::SRgb);
        for(int y=0;y<image.height();++y)for(int x=0;x<image.width();++x)image.setPixelColor(x,y,QColor::fromRgba64(x*200,y*260,32000,65535));
        image.setPixelColor(0,0,QColor::fromRgba64(1001,2003,3005,65535));
        const auto first=artifacts_.filePath("中文 one.png"),second=artifacts_.filePath("two.png");
        QVERIFY(writeScreenshotPng(first,image,{}).isEmpty());QVERIFY(image.save(second));const auto original=hash(first);
        PreviewPane pane("Image","");pane.setImage(image);PlayerImageTools toolbar(&pane);toolbar.setSource(first);
        auto *button=toolbar.findChild<QToolButton *>("imageTool_edit");QVERIFY(button);QVERIFY(!button->isEnabled());toolbar.setReady(true);QSignalSpy errors(&toolbar,&PlayerImageTools::errorOccurred);
        button->click();button->click();
        QTRY_VERIFY_WITH_TIMEOUT(connectControl(),30000);QVERIFY(call("auth",{{"token",QString::fromLatin1(token_)}}).value("ok").toBool());
        QVERIFY(call("engine.execute",{{"command","prefs.set"},{"params",QJsonObject{{"path","interface.language"},{"value","zh-hans"}}}}).value("ok").toBool());
        QTRY_COMPARE_WITH_TIMEOUT(inspect().value("session").toObject().value("documents").toArray().size(),1,20000);
        QCOMPARE(inspect().value("document").toObject().value("depth").toInt(),16);
        toolbar.setSource(second);toolbar.setReady(true);button->click();
        QTRY_COMPARE_WITH_TIMEOUT(inspect().value("session").toObject().value("documents").toArray().size(),2,20000);
        toolbar.setSource(first);toolbar.setReady(true);button->click();QTRY_COMPARE(inspect().value("session").toObject().value("active").toInt(),0);
        QCOMPARE(inspect().value("session").toObject().value("documents").toArray().size(),2);
        QVERIFY(call("app.save",{{"path","roundtrip16.png"}}).value("ok").toBool());
        const QImage saved(artifacts_.filePath("roundtrip16.png"));QCOMPARE(saved.pixelColor(0,0).rgba64(),image.pixelColor(0,0).rgba64());QCOMPARE(saved.colorSpace(),image.colorSpace());
        QImage floating(3,2,QImage::Format_RGBA32FPx4);floating.setColorSpace(QColorSpace::SRgbLinear);
        for(int y=0;y<2;++y)for(int x=0;x<3;++x){auto *p=reinterpret_cast<float *>(floating.scanLine(y))+x*4;p[0]=-.25f;p[1]=12.5f;p[2]=.50001f;p[3]=.4f;}
        pane.setImage(floating);pane.setImageTransform(QTransform(0,1,-1,0,0,0));toolbar.setSource(artifacts_.filePath("decoded.avif"));toolbar.setReady(true);button->click();
        QTRY_COMPARE_WITH_TIMEOUT(inspect().value("document").toObject().value("depth").toInt(),32,20000);
        const auto hdr=inspect().value("document").toObject();QCOMPARE(hdr.value("width").toInt(),2);QCOMPARE(hdr.value("height").toInt(),3);
        QVERIFY(call("app.save",{{"path","roundtrip32.tif"}}).value("ok").toBool());
        pane.setImage(image);pane.setImageTransform({});toolbar.setSource(first);toolbar.setReady(true);button->click();QTRY_COMPARE(inspect().value("document").toObject().value("depth").toInt(),16);
        QVERIFY(call("ui.set",{{"tool","brush"}}).value("ok").toBool());
        const QJsonArray stroke{QJsonObject{{"kind","down"},{"x",80},{"y",90}},QJsonObject{{"kind","move"},{"x",140},{"y",100}},QJsonObject{{"kind","up"},{"x",140},{"y",100}}};
        QVERIFY(call("ui.pointer",{{"events",stroke}}).value("ok").toBool());QVERIFY(inspect().value("document").toObject().value("canUndo").toBool());
        QVERIFY(call("engine.execute",{{"command","type.create"},{"params",QJsonObject{{"x",20},{"y",50},{"text","VSP / PhotoCraft"},{"size",24},{"color","#ffffff"}}}}).value("ok").toBool());
        QVERIFY(call("app.save",{{"path","layers.psd"}}).value("ok").toBool());
        QVERIFY(call("ui.screenshot",{{"path","native-ui.png"}}).value("ok").toBool());QVERIFY(QFileInfo::exists(artifacts_.filePath("native-ui.png")));
        QVERIFY(call("app.open",{{"path","layers.psd"}}).value("ok").toBool());
        QCOMPARE(inspect().value("document").toObject().value("depth").toInt(),16);
        QCOMPARE(inspect().value("document").toObject().value("layers").toArray().size(),2);
        QVERIFY(call("engine.execute",{{"command","image.crop"},{"params",QJsonObject{{"x",10},{"y",10},{"width",160},{"height",120}}}}).value("ok").toBool());
        QCOMPARE(inspect().value("document").toObject().value("width").toInt(),160);
        QVERIFY(call("engine.execute",{{"command","edit.undo"}}).value("ok").toBool());QCOMPARE(inspect().value("document").toObject().value("width").toInt(),320);
        QCOMPARE(hash(first),original);QCOMPARE(errors.count(),0);
        const auto large=artifacts_.filePath("24mp.png");
        {QImage pixels(6000,4000,QImage::Format_RGBA64);pixels.setColorSpace(QColorSpace::SRgb);pixels.fill(QColor::fromRgba64(30001,20003,40005,65535));QVERIFY(writeScreenshotPng(large,pixels,{}).isEmpty());}
        QElapsedTimer benchmark;benchmark.start();PhotoCraftEditor::instance()->open(large);
        QTRY_COMPARE_WITH_TIMEOUT(inspect().value("document").toObject().value("width").toInt(),6000,30000);const auto openMs=benchmark.restart();
        QVERIFY(call("engine.execute",{{"command","image.imageSize"},{"params",QJsonObject{{"width",3000},{"height",2000},{"resample","bicubic"}}}}).value("ok").toBool());const auto resizeMs=benchmark.elapsed();
        QCOMPARE(inspect().value("document").toObject().value("height").toInt(),2000);
        qInfo()<<"24 MP native open ms:"<<openMs<<"bicubic resize ms:"<<resizeMs;
    }
    void streamedImportPrecision() {
        const auto cleanup=qScopeGuard([this]{if(connectControl())call("app.quit");control_.abort();});
        for(auto format:{QImage::Format_RGBA8888,QImage::Format_RGBA64,QImage::Format_RGBA32FPx4}){
            QImage image(32769,2,format);image.setColorSpace(QColorSpace::SRgbLinear);image.fill(QColor::fromRgba64(12001,23003,34005,45007));
            const auto path=artifacts_.filePath(QString("wide-%1.vspimage").arg(int(format)));
            QVERIFY(PlayerImageTools::writeEditorImport(image,path,"Wide VSP").isEmpty());PhotoCraftEditor::instance()->open(path);
            QTRY_VERIFY_WITH_TIMEOUT(connectControl(),30000);if(format==QImage::Format_RGBA8888)QVERIFY(call("auth",{{"token",QString::fromLatin1(token_)}}).value("ok").toBool());
            const int bits=format==QImage::Format_RGBA8888?8:format==QImage::Format_RGBA64?16:32;
            QTRY_COMPARE_WITH_TIMEOUT(inspect().value("document").toObject().value("depth").toInt(),bits,30000);
            QCOMPARE(inspect().value("document").toObject().value("width").toInt(),32769);
            const auto output=QString("wide-roundtrip-%1.%2").arg(bits).arg(bits==32?"tif":"png");
            QVERIFY(call("app.save",{{"path",output}}).value("ok").toBool());
            if(bits!=32){const QImage restored(artifacts_.filePath(output));QCOMPARE(restored.pixelColor(32768,1).rgba64(),image.pixelColor(32768,1).rgba64());QCOMPARE(restored.colorSpace(),image.colorSpace());}
            QVERIFY(!QFileInfo::exists(path+".raw"));
        }
    }
    void largeImageOriginalDecoder() {
        const auto path=qEnvironmentVariable("VSR_PHOTOCRAFT_LARGE_MANIFEST");if(path.isEmpty())QSKIP("Set VSR_PHOTOCRAFT_LARGE_MANIFEST for the 48000x32000 image");
        QFile pixels(path+".raw");QVERIFY(pixels.open(QIODevice::ReadOnly));QVERIFY(pixels.seek((qint64(20)*48000+20)*4));const auto expected=pixels.read(128*4);pixels.close();
        const auto cleanup=qScopeGuard([this]{if(connectControl())call("app.quit");control_.abort();});
        QElapsedTimer elapsed;elapsed.start();PhotoCraftEditor::instance()->open(path);QTRY_VERIFY_WITH_TIMEOUT(connectControl(),30000);QVERIFY(call("auth",{{"token",QString::fromLatin1(token_)}}).value("ok").toBool());
        QTRY_COMPARE_WITH_TIMEOUT(inspect().value("document").toObject().value("width").toInt(),48000,240000);
        QCOMPARE(inspect().value("document").toObject().value("height").toInt(),32000);qInfo()<<"1.536 Gpixel full-resolution editor import ms"<<elapsed.elapsed();
        QVERIFY(call("ui.screenshot",{{"path","native-large.png"}}).value("ok").toBool());
        QVERIFY(inspect().value("perf").toObject().value("timings").toObject().value("gpuInfo").toObject().value("lost").isNull());
        QVERIFY(call("ui.set",{{"zoom",1.0},{"center",QJsonArray{10000,10000}}}).value("ok").toBool());QTest::qWait(300);
        QVERIFY(call("ui.screenshot",{{"path","native-large-100-percent.png"}}).value("ok").toBool());
        QVERIFY(call("engine.execute",{{"command","image.crop"},{"params",QJsonObject{{"x",20},{"y",20},{"width",128},{"height",128}}}}).value("ok").toBool());
        QVERIFY(call("app.save",{{"path","large-crop.png"}}).value("ok").toBool());
        const auto crop=QImage(artifacts_.filePath("large-crop.png")).convertToFormat(QImage::Format_RGBA8888);
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(crop.constScanLine(0)),512),expected);
        QVERIFY(call("engine.execute",{{"command","edit.undo"}}).value("ok").toBool());QCOMPARE(inspect().value("document").toObject().value("width").toInt(),48000);
    }
    void editorSurvivesPlayerProcessExit() {
        QTest::qWait(500);QProcess launcher;
        launcher.start(QCoreApplication::applicationFilePath(),{"--lifetime-fixture",artifacts_.filePath("two.png")});
        QVERIFY(launcher.waitForStarted());QTRY_VERIFY_WITH_TIMEOUT(connectControl(),30000);
        QVERIFY(call("auth",{{"token",QString::fromLatin1(token_)}}).value("ok").toBool());
        const auto cleanup=qScopeGuard([this]{call("app.quit");control_.abort();});
        QTRY_COMPARE_WITH_TIMEOUT(inspect().value("session").toObject().value("documents").toArray().size(),1,20000);
        QTRY_COMPARE_WITH_TIMEOUT(launcher.state(),QProcess::NotRunning,20000);QCOMPARE(launcher.exitCode(),0);
        QVERIFY(call("ui.inspect").value("ok").toBool());
    }
};
int main(int argc,char **argv) {
    QApplication app(argc,argv);
    if(app.arguments().value(1)=="--lifetime-fixture"){
        PhotoCraftEditor::instance()->open(app.arguments().value(2));QTimer::singleShot(10000,&app,&QCoreApplication::quit);return app.exec();
    }
    TestPhotoCraft test;return QTest::qExec(&test,argc,argv);
}
#include "TestPhotoCraft.moc"
