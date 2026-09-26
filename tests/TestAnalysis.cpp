#include "ui/AnalysisPage.h"
#include "ui/MultiCompareView.h"
#include "ui/PreviewPane.h"
#include "app/MainWindow.h"
#include "app/Style.h"
#include "backend/VapourSynthFrameServer.h"
#include "backend/ThreeFpPlayer.h"
#include "graph/FilterGraph.h"
#include "graph/VpyScriptBuilder.h"
#include <QSignalSpy>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QProcess>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QTest>
#include <QToolButton>

using namespace vsr;
class ViewEventCounter final : public QObject {
public:
    int hides = 0;
    int resizes = 0;
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::Hide) ++hides;
        if (event->type() == QEvent::Resize) ++resizes;
        return false;
    }
};

class TestAnalysis final : public QObject {
    Q_OBJECT
private slots:
    void directComparison()
    {
        QTemporaryDir dir;
        QString ffmpeg = QCoreApplication::applicationDirPath() + "/runtime/ffmpeg/ffmpeg.exe";
        if (!QFileInfo::exists(ffmpeg)) ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg.exe"));
        if (ffmpeg.isEmpty()) ffmpeg = QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe");
        for (int i = 0; i < 2; ++i) {
            QProcess process;
            process.start(ffmpeg, {"-v", "error", "-f", "lavfi", "-i", QString("testsrc2=size=320x180:rate=%1:duration=6").arg(i ? 30 : 24), "-c:v", "libx264", "-preset", "ultrafast", "-y", dir.filePath(QString("%1.mkv").arg(i))});
            QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));
            QVERIFY(process.waitForFinished(15000));
            QCOMPARE(process.exitCode(), 0);
        }
        ThreeFpApi api;
        QVERIFY2(api.available(), qPrintable(api.errorString()));
        AnalysisPage page(api);
        page.resize(1280, 700);
        page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        page.openFiles({dir.filePath("0.mkv"), dir.filePath("1.mkv")});
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(), 15000);
        QVERIFY(page.snapshot(0).presentedVideoFrames > 0);
        QVERIFY(page.snapshot(1).presentedVideoFrames > 0);
        page.seekGlobal(20000000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(), 10000);
        QVERIFY(std::abs(page.snapshot(0).position100ns - 20000000) < 500000);
        const auto left = page.snapshot(0).position100ns;
        page.alignVideo(1, 1, false);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(1) > 0, 10000);
        QCOMPARE(page.snapshot(0).position100ns, left);
        QVERIFY(page.offset(1) > 200000 && page.offset(1) < 500000);
        page.alignVideo(1, -1, false);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && std::abs(page.offset(1)) < 10000, 10000);
        page.alignVideo(1, 1, true);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(1) > 9000000, 10000);
        QCOMPARE(page.snapshot(0).position100ns, left);
        const auto offset = page.offset(1);
        page.seekGlobal(10000000);
        page.seekGlobal(15000000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && std::abs(page.snapshot(0).position100ns - 15000000) < 500000, 10000);
        QVERIFY(std::abs(page.snapshot(1).position100ns - page.snapshot(0).position100ns - offset) < 500000);
        page.stepGlobal(1);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.snapshot(0).position100ns > 15300000, 10000);
        QCOMPARE(page.offset(1), offset);
        page.findChild<QSlider *>("analysisTimeline1")->setValue(60000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.snapshot(1).position100ns > 35000000, 10000);
        QVERIFY(std::abs(page.snapshot(1).position100ns - page.snapshot(0).position100ns - offset) < 500000);
        page.togglePlayback();
        QTRY_VERIFY_WITH_TIMEOUT(page.snapshot(0).state == ThreeFpState::Playing, 5000);
        QTest::qWait(600);
        page.pause();
        QTRY_VERIFY(page.snapshot(0).state == ThreeFpState::Paused);
        QTRY_VERIFY(page.snapshot(1).state == ThreeFpState::Paused);
        auto *view = page.findChild<MultiCompareView *>();
        const auto handleA = view->pane(0)->surface()->winId();
        page.findChild<QComboBox *>("analysisLayout")->setCurrentIndex(1);
        view->setDivision(0.23,0.5);
        page.resize(1100, 620);
        QTest::qWait(150);
        const auto presents = page.snapshot(0).swapChainPresents;
        view->pane(0)->adoptView(2.0f, 0, 0);
        emit view->pane(0)->viewChanged(2.0f, 0, 0);
        QTRY_VERIFY(page.snapshot(0).swapChainPresents > presents);
        QCOMPARE(view->pane(0)->surface()->winId(), handleA);
        page.showFullScreen();
        QTest::qWait(150);
        page.showNormal();
        page.resize(1280, 700);
        QTest::qWait(150);
        page.grab().save(QCoreApplication::applicationDirPath() + "/analysis-page.png");
        page.seekGlobal(59000000);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(), 10000);
        page.togglePlayback();
        QTRY_VERIFY_WITH_TIMEOUT(page.snapshot(0).state != ThreeFpState::Playing && page.snapshot(1).state != ThreeFpState::Playing, 5000);
    }
    void multipleSourcesAndLayouts()
    {
        QTemporaryDir dir;
        const QString mp4=dir.filePath("source.mp4");
        QProcess process;
        process.start(QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=4","-c:v","libx264","-preset","ultrafast","-y",mp4});
        QVERIFY(process.waitForFinished(15000)); QCOMPARE(process.exitCode(),0);
        ThreeFpApi api;
        AnalysisPage page(api); page.resize(1440,900); page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        for(auto *slider:page.findChildren<QSlider *>()) QVERIFY(!slider->isVisible());
        QStringList paths; for(int i=0;i<9;++i) paths.append(mp4);
        page.openFiles(paths);
        QCOMPARE(page.videoCount(),9);
        QCOMPARE(page.visibleVideos().size(),9);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),20000);
        for(int i=0;i<9;++i) QVERIFY(page.snapshot(i).presentedVideoFrames>0);
        int visibleRows=0;for(auto *slider:page.findChildren<QSlider *>())if(slider->isVisible())++visibleRows;
        QCOMPARE(visibleRows,4);
        page.alignVideo(8,1,true);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(8)>9000000,10000);
        const auto offset=page.offset(8);
        page.setMode(2);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        page.selectSource(0,8);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QCOMPARE(page.visibleVideos().first(),8);QCOMPARE(page.offset(8),offset);
        page.selectSource(1,8);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QCOMPARE(page.visibleVideos(),QList<int>({1,8}));QCOMPARE(page.offset(8),offset);
        page.setMode(3);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        auto *view=page.findChild<MultiCompareView *>();
        auto *layout=page.findChild<QComboBox *>("analysisLayout");
        layout->setCurrentIndex(layout->findData(MultiCompareView::ThreeLeft));
        const auto before=view->cellRect(0);
        auto *center=view->findChild<QWidget *>("comparisonDivider4");
        QVERIFY(center->isVisible());
        const QPoint local=center->rect().center();
        QTest::mousePress(center,Qt::LeftButton,Qt::NoModifier,local);
        const QPoint target=view->mapToGlobal(QPoint(view->width()*.6,view->height()*.7));
        QMouseEvent move(QEvent::MouseMove,center->mapFromGlobal(target),target,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(center,&move);QTest::mouseRelease(center,Qt::LeftButton);
        QVERIFY(view->cellRect(0).width()>before.width());
        QVERIFY(view->cellRect(1).height()>view->height()*.6);
        page.setMode(4);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        view->setDivision(.4,.6);QCOMPARE(view->cellRect(0).right()+1,view->cellRect(1).left());
        layout->setCurrentIndex(layout->findData(MultiCompareView::FourWipe));
        QTest::qWait(100);
        for (int slot = 0; slot < 4; ++slot) {
            auto *pane = view->pane(page.visibleVideos()[slot]);
            auto *surface = pane->surface();
            const QPoint target = view->cellRect(slot).center() + QPoint(0,28);
            const QPoint global = view->mapToGlobal(target);
            QMouseEvent hover(QEvent::MouseMove, surface->mapFromGlobal(global), global, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(surface, &hover);
            auto *label = pane->findChild<QLabel *>("pixelReadout");
            QVERIFY(label->isVisible());
            QVERIFY(label->text().contains("RGB"));
            QVERIFY(view->cellRect(slot).translated(0,28).contains(QRect(label->mapTo(view,QPoint(0,0)),label->size())));
        }
        auto *color = page.findChild<QToolButton *>("colorProcessing");
        auto *nearest = color->menu()->actions().first();
        nearest->trigger(); QVERIFY(nearest->isChecked());
        QVERIFY(color->toolTip().contains("Nearest"));
        auto *retained = view->pane(8);
        const auto hwnd = retained->surface()->winId();
        QTest::mouseClick(view->findChild<QToolButton *>("removeVideo0"),Qt::LeftButton);
        QCOMPARE(page.videoCount(),8);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QCOMPARE(view->pane(7),retained);
        QCOMPARE(retained->surface()->winId(),hwnd);
        QCOMPARE(page.offset(7),offset);
        page.openFiles({mp4});
        QCOMPARE(page.videoCount(),9);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QVERIFY(page.snapshot(8).presentedVideoFrames>0);
        page.setMode(9);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QTest::qWait(100);page.grab().save(QCoreApplication::applicationDirPath()+"/nine-way-analysis.png");
        while(page.videoCount()) page.removeVideo(0);
        QCOMPARE(page.visibleVideos().size(),0);
        for(auto *slider:page.findChildren<QSlider *>()) QVERIFY(!slider->isVisible());
        page.openFiles({mp4});
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        QVERIFY(page.snapshot(0).presentedVideoFrames>0);
    }
    void enhancementFiltersExecute()
    {
        QTemporaryDir dir;const QString input=dir.filePath("input.mkv");
        QVERIFY(QFile::copy(QStringLiteral(":/startup/warmup.mkv"),input));
        FilterGraph graph;
        for(const auto &id:{"deband","deblock","nlmeans","dering","sharpen_edges","crispen_edges","thin_edges","enhance_detail"}) QVERIFY(graph.add(id)>=0);
        const auto script=VpyScriptBuilder::build(input,SourceFilter::Ffms2,graph);
        QVERIFY(script.errors.isEmpty());
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),15000);QVERIFY(server.available());
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
        server.loadScript(script.script,dir.filePath("filters.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
        QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),15000);
        QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        QVERIFY(!frames.isEmpty());
    }
    void firstFrame2160p()
    {
        QTemporaryDir dir;const QString input=dir.filePath("2160p.mp4");QProcess process;
        process.start(QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=3840x2160:rate=24:duration=1","-c:v","libx264","-preset","ultrafast","-y",input});
        QVERIFY(process.waitForFinished(20000));QCOMPARE(process.exitCode(),0);
        ThreeFpApi api; AnalysisPage page(api);page.resize(1200,800);page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));
        QElapsedTimer elapsed;elapsed.start();page.openFiles({input});
        QTRY_VERIFY_WITH_TIMEOUT(page.snapshot(0).presentedVideoFrames>0,15000);
        qInfo()<<"2160p first frame ms:"<<elapsed.elapsed();
        QVERIFY(elapsed.elapsed()<5000);
        page.openFiles({input});
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
        auto *view=page.findChild<MultiCompareView *>();
        auto *layout=page.findChild<QComboBox *>("analysisLayout");
        layout->setCurrentIndex(layout->findData(MultiCompareView::Wipe));
        QTest::qWait(150);
        auto *handle=view->findChild<QWidget *>("comparisonDivider0");
        ViewEventCounter counter;
        handle->installEventFilter(&counter);
        view->pane(0)->surface()->installEventFilter(&counter);
        view->pane(1)->surface()->installEventFilter(&counter);
        const auto size=view->pane(0)->surface()->size();
        const auto sourceFrame=page.snapshot(0).framePts;
        QTest::mousePress(handle,Qt::LeftButton,Qt::NoModifier,handle->rect().center());
        elapsed.restart();
        for(int step=0;step<100;++step) {
            const QPoint global=view->mapToGlobal(QPoint(200+step*5,200));
            QMouseEvent move(QEvent::MouseMove,handle->mapFromGlobal(global),global,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(handle,&move);
            QCoreApplication::processEvents();
            QCOMPARE(QWidget::mouseGrabber(),handle);
        }
        qInfo()<<"2160p AB 100 drag updates ms:"<<elapsed.elapsed();
        QVERIFY(elapsed.elapsed()<2000);
        QTest::mouseRelease(handle,Qt::LeftButton);
        QCOMPARE(counter.hides,0);QCOMPARE(counter.resizes,0);
        QCOMPARE(view->pane(0)->surface()->size(),size);
        QCOMPARE(view->pane(1)->surface()->size(),size);
        QCOMPARE(page.snapshot(0).framePts,sourceFrame);
    }
    void chromaAndScalingRender()
    {
        ThreeFpApi api;
        PreviewPane pane(QStringLiteral("Shader test"),QStringLiteral("Test"));pane.resize(640,480);pane.setSurfaceActive(true);pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        ThreeFpPlayer player(api,pane.surface());QVERIFY(player.ready());
        VapourSynthFrame frame;frame.width=64;frame.height=64;frame.format=ThreeFpExternalPixelFormat::Yuv420P8;
        frame.colorRange=2;frame.colorMatrix=1;frame.totalFrames=1;frame.duration100ns=400000;
        frame.planes[0]=QByteArray(64*64,char(128));frame.strides[0]=64;
        for(int p=1;p<3;++p){frame.planes[p].resize(32*32);frame.strides[p]=32;
            for(int y=0;y<32;++y)for(int x=0;x<32;++x)frame.planes[p][y*32+x]=char(((x+y+p)%2)?80:180);}
        QVERIFY(player.submitFrame(frame));
        QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().presentedVideoFrames>0,10000);
        double nearest=0,linear=0;
        for(int kernel=0;kernel<=10;++kernel){
            const auto presents=player.snapshot().swapChainPresents;
            QVERIFY2(player.setChromaAlgorithm(kernel),qPrintable(player.lastError()));
            QVERIFY(player.submitFrame(frame));
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents>presents,5000);
            double value=0;
            for(int x=201;x<210;++x){ThreeFpPixelProbe probe{};QVERIFY(player.samplePixel(x,203,probe));value+=probe.red+probe.blue*2;}
            if(kernel==0)nearest=value;if(kernel==1)linear=value;
        }
        QVERIFY(std::abs(nearest-linear)>0.001);
        for(auto algorithm:{ThreeFpScalingAlgorithm::Spline36,ThreeFpScalingAlgorithm::SuperXbrSinglePass}){
            const auto presents=player.snapshot().swapChainPresents;
            QVERIFY(player.setScalingAlgorithms(algorithm,ThreeFpScalingAlgorithm::Lanczos3));
            player.setView(2,0,0);QVERIFY(player.submitFrame(frame));
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents>presents,5000);
        }
    }
    void wipeUsesSharedCanvasWithoutResizingOrLosingCapture()
    {
        MultiCompareView view;
        view.resize(960,628);
        view.setSources({"A","B","C","D"});
        for (int i=0;i<4;++i) view.pane(i)->setSurfaceActive(true);
        view.setSlots({0,1,2},MultiCompareView::ThreeNormal);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QCOMPARE(view.cellRect(0).width(),320);
        QCOMPARE(view.cellRect(1).width(),320);
        QCOMPARE(view.cellRect(2).width(),320);
        for (auto layout : {MultiCompareView::Wipe, MultiCompareView::ThreeRow,
                            MultiCompareView::ThreeColumn, MultiCompareView::ThreeLeft,
                            MultiCompareView::ThreeRight, MultiCompareView::FourWipe}) {
            const int count = layout == MultiCompareView::Wipe ? 2 : layout == MultiCompareView::FourWipe ? 4 : 3;
            QList<int> assignments;
            for (int i=0;i<count;++i) assignments.append(i);
            view.setSlots(assignments,layout);
            QTest::qWait(60);
            for (int i=0;i<count;++i) {
                QCOMPARE(view.pane(i)->surface()->size(),QSize(960,600));
                QCOMPARE(view.pane(i)->surface()->mapTo(&view,QPoint()),QPoint(0,28));
            }
            const int handleId = layout == MultiCompareView::ThreeColumn ? 2 :
                (layout == MultiCompareView::ThreeLeft || layout == MultiCompareView::ThreeRight || layout == MultiCompareView::FourWipe) ? 4 : 0;
            auto *handle = view.findChild<QWidget *>(QStringLiteral("comparisonDivider%1").arg(handleId));
            QVERIFY(handle->isVisible());
            ViewEventCounter counter;
            handle->installEventFilter(&counter);
            QList<WId> handles;
            for (int i=0;i<count;++i) {
                view.pane(i)->surface()->installEventFilter(&counter);
                handles.append(view.pane(i)->surface()->winId());
            }
            QTest::mousePress(handle,Qt::LeftButton,Qt::NoModifier,handle->rect().center());
            QCOMPARE(QWidget::mouseGrabber(),handle);
            QElapsedTimer elapsed;elapsed.start();
            for (int step=0;step<100;++step) {
                const QPoint global=view.mapToGlobal(QPoint(250+step*2,200+step));
                QMouseEvent move(QEvent::MouseMove,handle->mapFromGlobal(global),global,Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
                QApplication::sendEvent(handle,&move);
                QCOMPARE(QWidget::mouseGrabber(),handle);
            }
            qInfo()<<"Wipe mode"<<int(layout)<<"100 drag updates ms:"<<elapsed.elapsed();
            QVERIFY(elapsed.elapsed()<1500);
            QTest::mouseRelease(handle,Qt::LeftButton);
            QCOMPARE(counter.hides,0);
            QCOMPARE(counter.resizes,0);
            int area=0;
            for (int i=0;i<count;++i) {
                QCOMPARE(view.pane(i)->surface()->size(),QSize(960,600));
                QCOMPARE(view.pane(i)->surface()->mapTo(&view,QPoint()),QPoint(0,28));
                QCOMPARE(view.pane(i)->surface()->winId(),handles[i]);
                area += view.cellRect(i).width()*view.cellRect(i).height();
                for (int j=0;j<i;++j) QVERIFY(!view.cellRect(i).intersects(view.cellRect(j)));
            }
            QCOMPARE(area,960*600);
        }
        view.setSlots({0,1,2,3},MultiCompareView::Four);
        QVERIFY(!view.isWipe());
        QCOMPARE(view.cellRect(0).size(),QSize(480,314));
    }

    void navigation()
    {
        MainWindow window;
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *rail = window.findChild<QWidget *>("pageNavigation");
        QVERIFY(rail);
        QCOMPARE(rail->width(), 44);
        auto *toggle = window.findChild<QAction *>("navigationToggle");
        QVERIFY(toggle);
        toggle->trigger();
        QCOMPARE(rail->width(), 180);
        const auto buttons = rail->findChildren<QToolButton *>();
        QCOMPARE(buttons.size(), 2);
        buttons[1]->click();
        QVERIFY(window.findChild<AnalysisPage *>()->isVisible());
        buttons[0]->click();
        QVERIFY(!window.findChild<AnalysisPage *>()->isVisible());
        toggle->trigger();
        QCOMPARE(rail->width(), 44);
        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<QProgressBar *>()->isHidden(),15000);
        buttons[1]->click();
        toggle->trigger();
        QTest::qWait(150);
        window.grab().save(QCoreApplication::applicationDirPath() + "/analysis-navigation.png");
    }
};
int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setStyleSheet(vsr::applicationStyleSheet());
    TestAnalysis test;
    return QTest::qExec(&test, argc, argv);
}
#include "TestAnalysis.moc"
