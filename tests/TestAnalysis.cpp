#include "ui/AnalysisPage.h"
#include "ui/ExportWindow.h"
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QDialog>
#include <QTimer>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QListWidget>
#include "ui/MultiCompareView.h"
#include "ui/PreviewPane.h"
#include "app/MainWindow.h"
#include "app/Style.h"
#include "backend/VapourSynthFrameServer.h"
#include "backend/ThreeFpPlayer.h"
#include "graph/FilterGraph.h"
#include "graph/PresetStore.h"
#include "ui/PresetDialog.h"
#include "ui/CompareView.h"
#include "ui/ParameterEditor.h"
#include "graph/FilterCatalog.h"
#include <QCheckBox>
#include <QFont>
#include <QGridLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QTabBar>
#include <QScopeGuard>
#include <QSettings>
#include <QStackedWidget>
#include <QMessageBox>
#include <QHeaderView>
#include <QUuid>
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
    void rendererLayoutAndParameterSizing()
    {
        const auto oldFont=qApp->font();const auto oldStyle=qApp->styleSheet();
        const auto restore=qScopeGuard([&]{qApp->setFont(oldFont);qApp->setStyleSheet(oldStyle);});
        QFont font("Segoe UI Variable");font.setPixelSize(13);qApp->setFont(font);qApp->setStyleSheet(applicationStyleSheet());
        MainWindow window;QVERIFY(window.width()>=1440);window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *view=window.findChild<CompareView *>();auto *transport=window.findChild<QWidget *>("rendererTransport");
        auto *frames=window.findChild<QWidget *>("rendererFrameControls");auto *mode=window.findChild<QComboBox *>("rendererCompareMode");
        auto *position=window.findChild<QLabel *>("rendererPosition");auto *timeline=window.findChild<QSlider *>("rendererTimeline");
        auto *source=window.findChild<QLineEdit *>("rendererSourcePath");auto *browse=window.findChild<QPushButton *>("rendererBrowse");
        auto *catalog=window.findChild<QListWidget *>("rendererCatalog");auto *sidebar=window.findChild<QWidget *>("processingSidebar");
        auto *parameters=window.findChild<QWidget *>("rendererParameterPanel");
        auto *scroll=window.findChild<QScrollArea *>("rendererParameters");
        QVERIFY(view && transport && frames && mode && position && timeline && source && browse && catalog && sidebar);
        QVERIFY(parameters && scroll);QVERIFY(!sidebar->isAncestorOf(parameters));
        auto *toggle=window.findChild<QAction *>("navigationToggle");QVERIFY(toggle);
        for(auto size:{QSize(1440,900),QSize(1280,820),QSize(1700,950)}) {
            window.resize(size);toggle->trigger();QTest::qWait(80);
            QCOMPARE(transport->mapToGlobal(QPoint(0,0)).x(),view->mapToGlobal(QPoint(0,0)).x());
            QCOMPARE(transport->width(),view->width());
            QVERIFY(std::abs(frames->mapToGlobal(frames->rect().center()).x()-view->mapToGlobal(view->rect().center()).x())<=2);
            QCOMPARE(position->mapToGlobal(QPoint()).x(),mode->mapToGlobal(QPoint()).x());
            QCOMPARE(timeline->mapToGlobal(QPoint()).x(),position->mapToGlobal(QPoint()).x());
            QCOMPARE(source->mapToGlobal(source->rect().center()).y(),browse->mapToGlobal(browse->rect().center()).y());
            QVERIFY(catalog->height()>=190);QVERIFY(sidebar->width()<360);
            QVERIFY(sidebar->mapToGlobal(QPoint(sidebar->width(),0)).x()<=parameters->mapToGlobal(QPoint()).x());
            QVERIFY(parameters->mapToGlobal(QPoint(parameters->width(),0)).x()<=view->mapToGlobal(QPoint()).x());
            QVERIFY(scroll->height()>500);
        }
        for(int i=0;i<catalog->count();++i)
            QVERIFY2(catalog->fontMetrics().horizontalAdvance(catalog->item(i)->text())+12<=catalog->viewport()->width(),qPrintable(catalog->item(i)->text()));
        auto *warmup=window.findChild<QLabel *>("rendererWarmupState");QVERIFY(warmup);
        QVERIFY(warmup->parentWidget()!=source->parentWidget());
        QTRY_COMPARE_WITH_TIMEOUT(warmup->text(),QStringLiteral("已预热"),15000);
        for(auto *action:window.findChildren<QAction *>())QVERIFY(action->text()!=QStringLiteral("实验设置"));
        for(auto *button:window.findChildren<QPushButton *>())QVERIFY(button->text()!=QStringLiteral("查看生成的 VPY"));
        window.grab().save(QCoreApplication::applicationDirPath()+"/renderer-ui-layout.png");
        QCOMPARE(scroll->viewport()->grab().toImage().pixelColor(10,scroll->viewport()->height()-10),QColor(Qt::white));
        ParameterEditor editor;editor.resize(scroll->viewport()->width(),600);editor.show();
        for(const auto &definition:FilterCatalog::all()) {
            FilterGraph graph;graph.add(definition.id);editor.setNode(&definition,&graph.nodes().first());QCoreApplication::processEvents();
            for(const auto &parameter:definition.parameters) {
                auto *label=editor.findChild<QLabel *>("parameterLabel_"+parameter.id);auto *field=editor.findChild<QWidget *>("parameter_"+parameter.id);
                QVERIFY(label && field);QVERIFY2(!label->geometry().intersects(field->geometry()),qPrintable(definition.id+":"+parameter.id));
                if(parameter.type==ParameterType::Boolean)QVERIFY(std::abs(field->geometry().right()-(editor.width()-9))<=1);
                if(auto *combo=qobject_cast<QComboBox *>(field)) {
                    QVERIFY(combo->width()>=combo->fontMetrics().horizontalAdvance(combo->currentText())+34);
                    QVERIFY(combo->width()<220);
                }
            }
            if(definition.id=="mvtools" || definition.id=="rife")editor.grab().save(QCoreApplication::applicationDirPath()+"/renderer-parameters-"+definition.id+".png");
            const auto image=editor.grab().toImage();
            QCOMPARE(image.pixelColor(4,editor.height()-4),QColor(Qt::white));
            auto *description=editor.findChildren<QLabel *>().first();
            QCOMPARE(image.pixelColor(description->geometry().right(),description->geometry().top()),QColor(Qt::white));
        }
    }
    void exportTabsShowSelection()
    {
        ExportWindow window;window.show();auto *tabs=window.findChild<QTabWidget *>("exportTabs");QVERIFY(tabs);QCOMPARE(tabs->count(),3);
        const auto sample=[&](int selected,int tab){tabs->setCurrentIndex(selected);QTest::qWait(40);const auto rectangle=tabs->tabBar()->tabRect(tab);return tabs->tabBar()->grab().toImage().pixelColor(rectangle.left()+10,rectangle.top()+10);};
        const auto selected=sample(0,0),idle=sample(1,0);QVERIFY(selected!=idle);QVERIFY(selected.blue()>selected.red());
        tabs->setCurrentIndex(0);QCoreApplication::processEvents();
        auto *command=window.findChild<QPlainTextEdit *>("exportCommand");QVERIFY(command);
        QCOMPARE(tabs->tabBar()->mapToGlobal(tabs->tabBar()->tabRect(0).topLeft()).x(),command->mapToGlobal(QPoint()).x());
        tabs->setCurrentIndex(2);QCoreApplication::processEvents();
        auto *queue=window.findChild<QTreeWidget *>("encodingQueue");QVERIFY(queue);
        const auto header=queue->header()->grab().toImage().scaled(queue->header()->size(),Qt::IgnoreAspectRatio,Qt::FastTransformation);
        for(int section=0;section<4;++section) {
            const int boundary=queue->header()->sectionViewportPosition(section)+queue->header()->sectionSize(section)-1;
            if(boundary<header.width()) QCOMPARE(header.pixelColor(boundary,8),QColor("#999999"));
        }
        tabs->setCurrentIndex(2);window.grab().save(QCoreApplication::applicationDirPath()+"/renderer-export-tabs.png");
    }
    void rendererSettingsPageAndVpyWarning()
    {
        const auto oldOrganization=QCoreApplication::organizationName(),oldApplication=QCoreApplication::applicationName();
        const auto restoreNames=qScopeGuard([&]{QCoreApplication::setOrganizationName(oldOrganization);QCoreApplication::setApplicationName(oldApplication);});
        QCoreApplication::setOrganizationName("VSRendererTests");QCoreApplication::setApplicationName("renderer-settings-page-test");
        QSettings settings;const auto saved=settings.value("sourceFilter");
        const auto restore=qScopeGuard([&]{if(saved.isValid())settings.setValue("sourceFilter",saved);else settings.remove("sourceFilter");});
        MainWindow window;window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *pages=window.findChild<QStackedWidget *>();auto *button=window.findChild<QToolButton *>("applicationSettings");
        auto *page=window.findChild<QWidget *>("rendererSettingsPage");QVERIFY(pages && button && page);
        button->click();QCOMPARE(pages->currentWidget(),page);QVERIFY(button->isChecked());QVERIFY(!QApplication::activeModalWidget());
        auto *source=window.findChild<QComboBox *>("rendererSettingsSource");
        auto *apply=window.findChild<QPushButton *>("rendererApplySettings");QVERIFY(source && apply);
        const int choice=1-source->currentIndex();source->setCurrentIndex(choice);apply->click();
        QCOMPARE(window.findChild<QComboBox *>("sourceFilter")->currentIndex(),choice);QCOMPARE(settings.value("sourceFilter").toInt(),choice);
        for(auto *nav:window.findChild<QWidget *>("pageNavigation")->findChildren<QToolButton *>())
            if(nav->text()==QStringLiteral("图像分析比对"))nav->click();
        QVERIFY(qobject_cast<AnalysisPage *>(pages->currentWidget()));QCOMPARE(pages->currentIndex(),1);QVERIFY(!button->isChecked());
        button->click();QCOMPARE(pages->currentWidget(),page);QCOMPARE(pages->currentIndex(),2);
        page->grab().save(QCoreApplication::applicationDirPath()+"/renderer-settings-page.png");
        for(auto *nav:window.findChild<QWidget *>("pageNavigation")->findChildren<QToolButton *>())
            if(nav->text()==QStringLiteral("VS 实时渲染"))nav->click();
        QCOMPARE(pages->currentIndex(),0);
        bool warningSeen=false;int warningWidth=0;bool titleFits=false;
        QTimer::singleShot(0,&window,[&] {
            auto *warning=window.findChild<QMessageBox *>("rendererVpyWarning");
            if(warning) {
                warningSeen=true;warningWidth=warning->width();
                titleFits=warning->fontMetrics().horizontalAdvance(warning->windowTitle())+100<warningWidth;
                warning->grab().save(QCoreApplication::applicationDirPath()+"/renderer-vpy-warning.png");
                warning->accept();
            }
        });
        bool actionFound=false;
        for(auto *action:window.findChildren<QAction *>())if(action->text()==QStringLiteral("查看 VPY")){actionFound=true;action->trigger();break;}
        QVERIFY(actionFound);QVERIFY(warningSeen);QVERIFY(warningWidth>=520);QVERIFY(titleFits);
    }
    void rendererLoadsPresetDuringStartup()
    {
        QTemporaryDir dir; const auto source = dir.filePath("source.mkv"); QVERIFY(QFile::copy(":/startup/warmup.mkv",source));
        FilterGraph graph; const int row = graph.add("resize"); graph.setParameter(row,"width",160); graph.setParameter(row,"height",90);
        const auto path = dir.filePath("preset.vpy"); QVERIFY(PresetStore::write(path,PresetStore::create(graph,SourceFilter::Ffms2,source,"test")));
        MainWindow window; window.show(); QMimeData mime; mime.setUrls({QUrl::fromLocalFile(path)});
        QDragEnterEvent enter(QPoint(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier); QApplication::sendEvent(&window,&enter); QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier); QApplication::sendEvent(&window,&drop); QVERIFY(drop.isAccepted());
        const auto rendered = [&window] { for (auto *player : window.findChildren<ThreeFpPlayer *>()) if (player->snapshot().videoWidth == 160 && player->snapshot().presentedVideoFrames > 0) return true; return false; };
        QTRY_VERIFY_WITH_TIMEOUT(rendered(),15000);
    }
    void presetManagementActions()
    {
        const QString name = "__test_" + QUuid::createUuid().toString(QUuid::Id128);
        const QString target = QDir(PresetStore::directory()).filePath(name + ".vpy");
        const QString renamed = QDir(PresetStore::directory()).filePath(name + "_renamed.vpy");
        QString loaded;
        PresetDialog dialog([](const QString &note) { return PresetStore::create(FilterGraph(), SourceFilter::Ffms2, "Z:/missing.mkv", note); },
                            [&loaded](const QString &path) { loaded = path; }, [] {});
        dialog.show(); auto *input = dialog.findChild<QLineEdit *>("presetName"); QVERIFY(input); input->setText(name);
        const auto click = [&dialog](const QString &title) {
            for (auto *button : dialog.findChildren<QPushButton *>()) if (button->text() == title) { button->click(); return true; }
            return false;
        };
        QVERIFY(click(QStringLiteral("保存当前"))); QVERIFY(QFileInfo::exists(target));
        auto *list = dialog.findChild<QListWidget *>("presetList"); QVERIFY(list);
        for (int i = 0; i < list->count(); ++i) if (list->item(i)->text() == name) list->setCurrentRow(i);
        QVERIFY(click(QStringLiteral("读取"))); QCOMPARE(loaded,target);
        input->setText(name + "_renamed"); QVERIFY(click(QStringLiteral("变更名称"))); QVERIFY(QFileInfo::exists(renamed)); QVERIFY(!QFileInfo::exists(target));
        QVERIFY(!PresetStore::metadata(PresetStore::load(renamed).script).isEmpty());
        dialog.grab().save("build/mingw-release/preset-management-layout.png");
        QVERIFY(QFile::remove(renamed));
    }
    void portablePreset()
    {
        QTemporaryDir dir;
        const QString source = dir.filePath("source.mkv");
        QVERIFY(QFile::copy(":/startup/warmup.mkv", source));
        FilterGraph graph;
        const int row = graph.add("resize");
        graph.setParameter(row, "width", 160); graph.setParameter(row, "height", 90);
        const auto text = PresetStore::create(graph, SourceFilter::Ffms2, "Z:/missing.mkv", QStringLiteral("测试预设"));
        const QString preset = dir.filePath("滤镜.vpy");
        QVERIFY(PresetStore::write(preset, text));
        const auto loaded = PresetStore::load(preset, source);
        QCOMPARE(PresetStore::metadata(loaded.script).value("note").toString(), QStringLiteral("测试预设"));
        QCOMPARE(PresetStore::graph(PresetStore::metadata(loaded.script)).nodes().size(), 1);
        VapourSynthFrameServer server;
        QSignalSpy ready(&server, &VapourSynthFrameServer::scriptLoaded);
        QSignalSpy frames(&server, &VapourSynthFrameServer::frameReady);
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY(server.available());
        server.loadScript(loaded.script, preset);
        QTRY_VERIFY_WITH_TIMEOUT(ready.count() || errors.count(), 15000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        const auto info = qvariant_cast<VapourSynthClipInfo>(ready.first().first());
        QCOMPARE(info.width, 160); QCOMPARE(info.height, 90);
        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(frames.count() || errors.count(), 10000);
        QVERIFY(errors.isEmpty()); QVERIFY(!frames.isEmpty());
    }
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
        auto *color = page.findChild<QComboBox *>("colorProcessing");
        color->setCurrentIndex(0); QCOMPARE(color->currentData().toInt(),0);
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
    void expandedFiltersExecute()
    {
        QTemporaryDir dir; const QString input=dir.filePath("input.mkv");
        QVERIFY(QFile::copy(QStringLiteral(":/startup/warmup.mkv"),input));
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),15000); QVERIFY(server.available());
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded), errors(&server,&VapourSynthFrameServer::errorOccurred), frames(&server,&VapourSynthFrameServer::frameReady);
        for(const auto &id:{"temporal_median","flux_t","flux_st","smart_median","iq_mean","degrain_median","cnr4","ccd","dct_filter","temporal_soften","vertical_cleaner","clahe","descale","rife"}) {
            FilterGraph graph; const int row=graph.add(id); QVERIFY(row>=0);
            if(QString(id)=="descale") {graph.setParameter(row,"width",64);graph.setParameter(row,"height",36);}
            const auto script=VpyScriptBuilder::build(input,SourceFilter::Ffms2,graph);
            QVERIFY(script.errors.isEmpty());
            loaded.clear();errors.clear();frames.clear();
            server.loadScript(script.script,dir.filePath("filters.vpy"));
            QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
            QVERIFY2(errors.isEmpty(),qPrintable(QString(id)+": "+(errors.isEmpty()?QString():errors.first().first().toString())));
            server.requestFrame(1);
            QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),20000);
            QVERIFY2(errors.isEmpty(),qPrintable(QString(id)+": "+(errors.isEmpty()?QString():errors.first().first().toString())));
            QVERIFY(!frames.isEmpty());
            qInfo()<<"Verified filter"<<id;
            if(QString(id)=="rife") {
                const auto info=qvariant_cast<VapourSynthClipInfo>(loaded.first().first()); const auto sourceInfo=qvariant_cast<VapourSynthClipInfo>(loaded.first().at(1)); QCOMPARE(info.totalFrames,sourceInfo.totalFrames*2);
                graph.setParameter(row,"model",QStringLiteral("4.26 Heavy"));
                loaded.clear();errors.clear();frames.clear();
                server.loadScript(VpyScriptBuilder::build(input,SourceFilter::Ffms2,graph).script,dir.filePath("heavy.vpy"));
                QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                server.requestFrame(1);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),20000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                QVERIFY(!frames.isEmpty());
            }
        }
    }
    void compositionExportsCanvas()
    {
        QTemporaryDir dir; QStringList paths;
        const QString ffmpeg=QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe");
        const QStringList colors{"red","lime","blue","white"};
        for(int i=0;i<4;++i) {
            const QString path=dir.filePath(QString("%1.mkv").arg(i));paths<<path;
            QProcess process;process.start(ffmpeg,{"-v","error","-f","lavfi","-i",QString("color=%1:size=%2:rate=24:duration=2").arg(colors[i],i==3?"320x180":"160x90"),"-c:v","ffv1","-y",path});
            QVERIFY(process.waitForFinished(15000));QCOMPARE(process.exitCode(),0);
        }
        ThreeFpApi api;AnalysisPage page(api);page.resize(1700,1000);page.show();
        QVERIFY(QTest::qWaitForWindowExposed(&page));page.openFiles(paths);
        QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),15000);
        auto *layout=page.findChild<QComboBox *>("analysisLayout");
        layout->setCurrentIndex(layout->findData(MultiCompareView::FourWipe));
        auto *view=page.findChild<MultiCompareView *>();view->setDivision(.35,.65);
        page.alignVideo(1,1,true);QTRY_VERIFY_WITH_TIMEOUT(!page.busy() && page.offset(1)>9000000,10000);
        const auto script=page.compositionScript();QVERIFY2(script.errors.isEmpty(),qPrintable(script.errors.join(";")));
        QFile saved(dir.filePath("composition.vpy"));QVERIFY(saved.open(QIODevice::WriteOnly));saved.write(script.script.toUtf8());saved.close();
        QProcess pipe;pipe.start(QCoreApplication::applicationDirPath()+"/runtime/python/Lib/site-packages/vapoursynth/vspipe.exe",{"--info",saved.fileName(),"-"});
        QVERIFY(pipe.waitForFinished(15000));QVERIFY2(pipe.exitCode()==0,pipe.readAllStandardError().constData());
        QVERIFY(pipe.readAllStandardOutput().contains("Width: 320"));
        QTest::mouseClick(page.findChild<QPushButton *>("exportComparison"),Qt::LeftButton);
        ExportWindow *window=nullptr;
        for(auto *widget:QApplication::topLevelWidgets()) if(auto *candidate=qobject_cast<ExportWindow *>(widget)) window=candidate;
        QVERIFY(window);
        const QString output=dir.filePath("canvas.mkv");
        QString overwriteError;
        QVERIFY(!window->addJob("ffmpeg -y -i <输入文件> -c:v ffv1 <输出文件>",paths[2],paths[0],script.script,1,&overwriteError));
        QVERIFY(overwriteError.contains(QStringLiteral("覆盖")));
        for(const auto &parameter:QStringList{"-vf scale=160:90","-filter:v:0 crop=160:90","-s 160x90","-filter scale=160:90","-vf scale\\_cuda=160:90"}) {
            QString error;QVERIFY(!window->addJob("ffmpeg -y -i <输入文件> "+parameter+" -c:v ffv1 <输出文件>",output,paths[2],script.script,1,&error));
            QVERIFY(error.contains(QStringLiteral("分辨率")));
        }
        window->findChild<QPlainTextEdit *>("exportCommand")->setPlainText("ffmpeg -y -i <输入文件> -map 0:v -c:v ffv1 <输出文件>");
        window->findChild<QLineEdit *>("singleOutput")->setText(output);
        QTest::mouseClick(window->findChild<QPushButton *>("exportSingle"),Qt::LeftButton);
        QTRY_VERIFY_WITH_TIMEOUT(!page.exportBusy() && QFileInfo(output).size()>1000,30000);
        auto *table=window->findChild<QTreeWidget *>();QCOMPARE(table->topLevelItem(0)->text(2),QStringLiteral("100%"));
        QProcess decode;decode.start(ffmpeg,{"-v","error","-i",output,"-frames:v","1","-pix_fmt","rgb24","-f","rawvideo","pipe:1"});
        QVERIFY(decode.waitForFinished(15000));QCOMPARE(decode.exitCode(),0);
        const QByteArray rgb=decode.readAllStandardOutput();QCOMPARE(rgb.size(),320*180*3);
        const QList<QPoint> probes{{50,60},{240,60},{50,145},{240,145}};
        for(int i=0;i<4;++i) {
            const int index=(probes[i].y()*320+probes[i].x())*3;
            const int r=uchar(rgb[index]),g=uchar(rgb[index+1]),b=uchar(rgb[index+2]);
            QVERIFY2(i==0?r>200&&g<40&&b<40:i==1?g>200&&r<40&&b<40:i==2?b>200&&r<40&&g<40:r>200&&g>200&&b>200,qPrintable(QString("slot %1: %2 %3 %4").arg(i).arg(r).arg(g).arg(b)));
        }
        page.grab().save(QCoreApplication::applicationDirPath()+"/comparison-export-layout.png");
        auto *trackSource=page.findChild<QComboBox *>("analysisSource0");
        const int shortWidth=trackSource->width();
        const QString longName=QString(90,QLatin1Char('W'))+QStringLiteral(".mkv");
        trackSource->setItemText(trackSource->currentIndex(),longName);
        page.refreshLayout();
        QVERIFY(trackSource->width()>shortWidth);
        QVERIFY(page.findChild<QSlider *>("analysisTimeline0")->width()>=160);
        QCOMPARE(trackSource->toolTip(),longName);
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(),15000);
        QSignalSpy loaded(&server,&VapourSynthFrameServer::scriptLoaded),errors(&server,&VapourSynthFrameServer::errorOccurred),frames(&server,&VapourSynthFrameServer::frameReady);
        for(int count:{2,3,4,9}) {
            if(count==9)page.openFiles({paths[0],paths[1],paths[2],paths[3],paths[0]});
            page.setMode(count);QTRY_VERIFY_WITH_TIMEOUT(!page.busy(),10000);
            for(int option=0;option<layout->count();++option) {
                layout->setCurrentIndex(option);
                QTest::qWait(50);
                auto *pane=view->pane(page.visibleVideos().first());
                pane->adoptView(1.25f,.2f,-.1f);
                const auto composition=page.compositionScript();
                const int mode=layout->currentData().toInt();
                const bool supported=mode==MultiCompareView::Wipe || mode==MultiCompareView::ThreeLeft || mode==MultiCompareView::ThreeRight || mode==MultiCompareView::Four || mode==MultiCompareView::FourWipe;
                if(!supported) {
                    QVERIFY(composition.errors.join(';').contains(QStringLiteral("不支持导出")));
                    QTest::mouseClick(page.findChild<QPushButton *>("exportComparison"),Qt::LeftButton);
                    bool visibleError=false;for(auto *label:page.findChildren<QLabel *>())if(label->isVisible() && label->text().contains(QStringLiteral("不支持导出")))visibleError=true;
                    QVERIFY(visibleError);continue;
                }
                QVERIFY2(composition.errors.isEmpty(),qPrintable(composition.errors.join(";")));
                loaded.clear();errors.clear();frames.clear();
                server.loadScript(composition.script,dir.filePath("layout.vpy"));
                QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                server.requestFrame(1);QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(),15000);
                QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
                const auto frame=qvariant_cast<VapourSynthFrame>(frames.first().first());
                QCOMPARE(frame.width,320);QCOMPARE(frame.height,180);
            }
        }
        window->close();
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

    void rifePreviewPresentsIntermediateFrames()
    {
        QTemporaryDir dir;const QString input=dir.filePath("rife.mkv");
        QProcess encode;encode.start(QStringLiteral("C:/PortableSoft/FFmpegFreeUI ReadyToRun x64/ffmpeg.exe"),
            {"-v","error","-f","lavfi","-i","testsrc2=size=320x180:rate=24:duration=4","-c:v","libx264","-preset","ultrafast","-y",input});
        QVERIFY(encode.waitForFinished(15000));QCOMPARE(encode.exitCode(),0);
        MainWindow window;window.show();QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY_WITH_TIMEOUT(window.findChild<QProgressBar *>()->isHidden(),15000);
        QMimeData mime;mime.setUrls({QUrl::fromLocalFile(input)});
        QDragEnterEvent enter(QPoint(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&window,&enter);QVERIFY(enter.isAccepted());
        QDropEvent drop(QPointF(400,300),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&window,&drop);QVERIFY(drop.isAccepted());
        bool found=false;
        for(auto *list:window.findChildren<QListWidget *>()) for(int i=0;i<list->count();++i)
            if(list->item(i)->data(Qt::UserRole).toString()=="rife") {list->setCurrentRow(i);found=true;}
        QVERIFY(found);
        for(auto *button:window.findChildren<QPushButton *>()) if(button->text()==QStringLiteral("添加到处理链"))button->click();
        auto *server=window.findChild<VapourSynthFrameServer *>();QVERIFY(server);
        QSignalSpy loaded(server,&VapourSynthFrameServer::scriptLoaded),frames(server,&VapourSynthFrameServer::frameReady),errors(server,&VapourSynthFrameServer::errorOccurred);
        for(auto *action:window.findChildren<QAction *>()) if(action->text()==QStringLiteral("生成并验证")) action->trigger();
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(),15000);
        QVERIFY2(errors.isEmpty(),errors.isEmpty()?"":qPrintable(errors.first().first().toString()));
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty(),15000);
        QPushButton *play=nullptr;
        for(auto *button:window.findChildren<QPushButton *>()) if(button->text()==QStringLiteral("播放"))play=button;
        QVERIFY(play);frames.clear();play->click();QTest::qWait(700);play->click();
        int intermediate=0;
        for(const auto &entry:frames) if(qvariant_cast<VapourSynthFrame>(entry.first()).frameIndex%2==1)++intermediate;
        qInfo()<<"RIFE preview submitted frames"<<frames.size()<<"intermediate frames"<<intermediate;
        QVERIFY(frames.size()>8);QVERIFY(intermediate>2);
        window.close();
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
        QCOMPARE(buttons.size(), 3);
        QVERIFY(!window.findChild<QComboBox *>("sourceFilter")->isVisible());
        bool settingsShown=false;
        QTimer::singleShot(0,&window,[&settingsShown] {
            auto *dialog=qobject_cast<QDialog *>(QApplication::activeModalWidget());
            if(dialog) { settingsShown=dialog->windowTitle()==QStringLiteral("设置") && dialog->findChild<QComboBox *>()->count()==2; dialog->reject(); }
        });
        window.findChild<QToolButton *>("applicationSettings")->click();
        QVERIFY(settingsShown);
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
