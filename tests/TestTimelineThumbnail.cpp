#include "player/TimelineThumbnail.h"
#include <QTest>
#include <QTemporaryDir>
#include <QScopeGuard>
class TestTimelineThumbnail final:public QObject {
    Q_OBJECT
private slots:
    void independentFrameAndCancellation(){
        const auto ff=qEnvironmentVariable("VSR_TEST_FFMPEG");if(ff.isEmpty())QSKIP("Set VSR_TEST_FFMPEG for single-frame decode correctness test");
        QTemporaryDir tmp;QImage fixture(16,16,QImage::Format_RGB32);fixture.fill(QColor(80,120,200));const auto file=tmp.filePath("frame.png");QVERIFY(fixture.save(file));
        const auto old=qgetenv("VSR_THUMBNAIL_FFMPEG");qputenv("VSR_THUMBNAIL_FFMPEG",ff.toUtf8());const auto restore=qScopeGuard([&]{qputenv("VSR_THUMBNAIL_FFMPEG",old);});
        QSlider slider(Qt::Horizontal);slider.setRange(0,100000);slider.setMouseTracking(true);slider.resize(400,30);slider.show();QString source=file;
        vsr::TimelineThumbnail preview(&slider,[&]{return source;},[]{return qint64(10000000);},[]{return true;},nullptr);
        QLabel *label=nullptr;for(auto *w:QApplication::topLevelWidgets())if(w->objectName()=="playerTimelineThumbnail")label=qobject_cast<QLabel*>(w);QVERIFY(label);QTest::mouseMove(&slider,QPoint(100,15));
        QTRY_VERIFY_WITH_TIMEOUT(!label->pixmap().isNull(),15000);QVERIFY(label->pixmap().width()<=240);QVERIFY(label->pixmap().height()<=135);
        QEvent leave(QEvent::Leave);QApplication::sendEvent(&slider,&leave);QVERIFY(!label->isVisible());
        source=tmp.filePath("missing.mkv");QTest::mouseMove(&slider,QPoint(200,15));QTest::qWait(300);QVERIFY(!label->isVisible());
    }
};
QTEST_MAIN(TestTimelineThumbnail)
#include "TestTimelineThumbnail.moc"
