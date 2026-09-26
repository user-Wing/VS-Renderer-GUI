#include "backend/FrameTimeline.h"
#include "backend/StartupWarmup.h"
#include <QFile>
#include <QTimer>
#include <QTemporaryDir>
#include "backend/ThreeFpApi.h"
#include "backend/ThreeFpPlayer.h"
#include "backend/VapourSynthFrameServer.h"
#include "ui/PreviewPane.h"
#include "ui/CompareView.h"
#include <QWheelEvent>

#include <QSignalSpy>
#include <QTest>
#include <QWidget>

using namespace vsr;

class TestFrameBridge final : public QObject {
    Q_OBJECT

private slots:
    void startupWarmsDecodeAndBothRenderers()
    {
        CompareView view;
        view.resize(1000, 600);
        view.sourcePane()->setSurfaceActive(true);
        view.processedPane()->setSurfaceActive(true);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QTemporaryDir directory;
        ThreeFpApi api;
        QVERIFY2(api.available(), qPrintable(api.errorString()));
        ThreeFpPlayer source(api, view.sourcePane()->surface());
        ThreeFpPlayer processed(api, view.processedPane()->surface());
        QElapsedTimer construction;
        construction.start();
        VapourSynthFrameServer server;
        QVERIFY(construction.elapsed() < 250);
        QElapsedTimer heartbeatClock;
        heartbeatClock.start();
        qint64 previousBeat = 0, maxGap = 0;
        QTimer heartbeat;
        connect(&heartbeat, &QTimer::timeout, this, [&] {
            const auto now = heartbeatClock.elapsed(); maxGap = std::max(maxGap, now-previousBeat); previousBeat=now;
        });
        heartbeat.start(10);
        StartupWarmup warmup(&source, &processed, &server);
        QSignalSpy done(&warmup, &StartupWarmup::finished);
        QElapsedTimer elapsed;
        elapsed.start();
        warmup.start();
        QTRY_VERIFY_WITH_TIMEOUT(!done.isEmpty(), 35000);
        QVERIFY2(done.first().at(0).toBool(), qPrintable(done.first().at(1).toString()));
        QVERIFY(!warmup.isRunning());
        QVERIFY(source.snapshot().presentedVideoFrames > 0);
        QVERIFY(processed.snapshot().swapChainPresents > 0);
        qInfo() << "Startup warmup ms:" << elapsed.elapsed() << "UI maximum heartbeat gap ms:" << maxGap;
        QVERIFY(maxGap < 1000);
        const QString next = directory.filePath(QStringLiteral("first-user-video.mkv"));
        QVERIFY(QFile::copy(QStringLiteral(":/startup/warmup.mkv"), next));
        QFile::setPermissions(next, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        elapsed.restart();
        QVERIFY(source.openFile(next));
        QTRY_VERIFY_WITH_TIMEOUT(source.snapshot().state == ThreeFpState::Ready, 10000);
        QVERIFY(source.seekFrame(0));
        QTRY_VERIFY_WITH_TIMEOUT(source.snapshot().presentedVideoFrames > 0, 10000);
        qInfo() << "First import after warmup ms:" << elapsed.elapsed();
        warmup.releaseScript();
        QVERIFY(!warmup.ownsScript());
    }

    void mapsDisplayedTimeToAbsoluteFrame()
    {
        constexpr std::int64_t fpsNumerator = 30000;
        constexpr std::int64_t fpsDenominator = 1001;
        constexpr std::int64_t totalFrames = 30000;
        const auto position = static_cast<std::int64_t>(std::llround(
            10000000.0L * 20000 * fpsDenominator / fpsNumerator));

        QCOMPARE(frameAtPosition100ns(position, totalFrames, fpsNumerator, fpsDenominator), 20000);
        QCOMPARE(frameAtPosition100ns(position, 60000, fpsNumerator * 2, fpsDenominator), 40000);
    }

    void loadsSourceAndProcessedTimingsSeparately()
    {
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY2(server.available(), qPrintable(server.errorString()));
        QSignalSpy loaded(&server, &VapourSynthFrameServer::scriptLoaded);
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);
        server.loadScript(QStringLiteral(
            "import vapoursynth as vs\n"
            "src = vs.core.std.BlankClip(width=64, height=48, format=vs.YUV420P8, "
            "length=30000, fpsnum=30000, fpsden=1001)\n"
            "clip = vs.core.std.Trim(src, first=100, last=200)\n"
            "src.set_output(1)\n"
            "clip.set_output(0)\n"), QStringLiteral("source-timing-test.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));

        const auto processed = qvariant_cast<VapourSynthClipInfo>(loaded.first().at(0));
        const auto source = qvariant_cast<VapourSynthClipInfo>(loaded.first().at(1));
        QCOMPARE(source.totalFrames, 30000);
        QCOMPARE(source.fpsNumerator, 30000);
        QCOMPARE(source.fpsDenominator, 1001);
        QCOMPARE(processed.totalFrames, 101);
    }

    void rendersWhenNativeSurfaceReplacesPlaceholder()
    {
        PreviewPane pane(QStringLiteral("处理后"), QStringLiteral("VS · 待渲染"));
        pane.resize(640, 400);
        pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        QVERIFY(!pane.surface()->isVisible());

        ThreeFpApi api;
        QVERIFY2(api.available(), qPrintable(api.errorString()));
        ThreeFpPlayer player(api, pane.surface());
        QVERIFY2(player.ready(), qPrintable(player.lastError()));
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::Nearest,
                                            ThreeFpScalingAlgorithm::Lanczos3));
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::Bilinear,
                                            ThreeFpScalingAlgorithm::Lanczos3));
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::Bicubic,
                                            ThreeFpScalingAlgorithm::Lanczos3));
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::Lanczos3,
                                            ThreeFpScalingAlgorithm::Lanczos3));
        QVERIFY(player.setScalingAlgorithms(ThreeFpScalingAlgorithm::Jinc2,
                                            ThreeFpScalingAlgorithm::Lanczos3));

        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY2(server.available(), qPrintable(server.errorString()));
        QSignalSpy loaded(&server, &VapourSynthFrameServer::scriptLoaded);
        QSignalSpy frames(&server, &VapourSynthFrameServer::frameReady);
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);
        server.loadScript(QStringLiteral(
            "import vapoursynth as vs\n"
            "clip = vs.core.std.BlankClip(width=64, height=48, format=vs.YUV420P8, "
            "length=1, color=[16, 128, 128])\n"
            "clip.set_output(0)\n"), QStringLiteral("placeholder-switch-test.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));

        pane.setSurfaceActive(true);
        QVERIFY(pane.surface()->isVisible());
        player.redraw();
        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        const auto frame = qvariant_cast<VapourSynthFrame>(frames.first().first());
        QVERIFY2(player.submitFrame(frame), qPrintable(player.lastError()));
        QTest::qWait(100);
        QCOMPARE(player.snapshot().frameIndex, 0);
        QVERIFY2(player.resetVideoOutput(), qPrintable(player.lastError()));
        QVERIFY2(player.submitFrame(frame), qPrintable(player.lastError()));
        QTest::qWait(100);
        QCOMPARE(player.snapshot().frameIndex, 0);

        loaded.clear();
        frames.clear();
        errors.clear();
        server.loadScript(QStringLiteral(
            "import vapoursynth as vs\n"
            "clip = vs.core.std.BlankClip(width=128, height=72, format=vs.YUV444P16, "
            "length=1, color=[4096, 32768, 32768])\n"
            "clip.set_output(0)\n"), QStringLiteral("changed-chain-geometry-test.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        const auto changedFrame = qvariant_cast<VapourSynthFrame>(frames.first().first());
        QCOMPARE(changedFrame.width, 128);
        QCOMPARE(changedFrame.height, 72);
        QVERIFY2(player.resetVideoOutput(), qPrintable(player.lastError()));
        QVERIFY2(player.submitFrame(changedFrame), qPrintable(player.lastError()));
        QTest::qWait(100);
        QCOMPARE(player.snapshot().videoWidth, 128u);
        QCOMPARE(player.snapshot().videoHeight, 72u);
    }

    void rendersBundledFilterChain()
    {
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY2(server.available(), qPrintable(server.errorString()));
        QSignalSpy loaded(&server, &VapourSynthFrameServer::scriptLoaded);
        QSignalSpy frames(&server, &VapourSynthFrameServer::frameReady);
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);

        const QString script = QStringLiteral(
            "import vapoursynth as vs\n"
            "core = vs.core\n"
            "clip = core.std.BlankClip(width=64, height=48, format=vs.YUV420P8, length=1)\n"
            "clip = core.vszip.Bilateral(clip, sigmaS=3.0, sigmaR=0.02)\n"
            "clip = core.nlm_ispc.NLMeans(clip, d=0, wmode=3, h=3)\n"
            "clip = core.vszip.Deband(clip, range=12, thr=[96 / 255.0, 48 / 255.0, 48 / 255.0], grain=[0, 0])\n"
            "clip = core.cas.CAS(clip, sharpness=0.5)\n"
            "clip = core.fmtc.bitdepth(clip, bits=16, dmode=3)\n"
            "clip.set_output(0)\n");
        server.loadScript(script, QStringLiteral("bundled-filter-chain-test.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));

        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        const auto frame = qvariant_cast<VapourSynthFrame>(frames.first().first());
        QCOMPARE(frame.format, ThreeFpExternalPixelFormat::Yuv420P16);
    }

    void coalescesFrameRequestsToNewest()
    {
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY2(server.available(), qPrintable(server.errorString()));
        QSignalSpy loaded(&server, &VapourSynthFrameServer::scriptLoaded);
        QSignalSpy frames(&server, &VapourSynthFrameServer::frameReady);
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);
        server.loadScript(QStringLiteral(
            "import time\n"
            "import vapoursynth as vs\n"
            "clip = vs.core.std.BlankClip(width=64, height=48, format=vs.YUV420P8, length=5)\n"
            "def slow_first(n, f):\n"
            "    if n == 0: time.sleep(0.15)\n"
            "    return f\n"
            "clip = vs.core.std.ModifyFrame(clip, clips=clip, selector=slow_first)\n"
            "clip.set_output(0)\n"), QStringLiteral("latest-frame-test.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));

        server.requestFrame(0);
        server.requestFrame(4);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        QTest::qWait(250);
        QCOMPARE(frames.size(), 1);
        QCOMPARE(qvariant_cast<VapourSynthFrame>(frames.first().first()).frameIndex, 4);
    }

    void reportsMissingNamespacePrecisely()
    {
        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY2(server.available(), qPrintable(server.errorString()));
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);
        server.loadScript(QStringLiteral(
            "import vapoursynth as vs\n"
            "clip = vs.core.definitely_missing.Filter()\n"
            "clip.set_output(0)\n"), QStringLiteral("missing-plugin-test.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!errors.isEmpty(), 10000);
        const QString message = errors.first().first().toString();
        QVERIFY2(message.contains(QStringLiteral("namespace “definitely_missing”")), qPrintable(message));
    }

    void rendersVapourSynthFrameThroughThreeFp()
    {
        CompareView view;
        view.resize(1000, 600);
        view.sourcePane()->setSurfaceActive(true);
        view.processedPane()->setSurfaceActive(true);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        QWidget &surface = *view.sourcePane()->surface();

        ThreeFpApi api;
        QVERIFY2(api.available(), qPrintable(api.errorString()));
        QVERIFY(api.apiVersion() >= 14);
        ThreeFpPlayer player(api, &surface);
        QVERIFY2(player.ready(), qPrintable(player.lastError()));
        ThreeFpPlayer processed(api, view.processedPane()->surface());
        QVERIFY2(processed.ready(), qPrintable(processed.lastError()));
        connect(view.sourcePane(), &PreviewPane::redrawRequested, &player, &ThreeFpPlayer::redraw);
        connect(view.processedPane(), &PreviewPane::redrawRequested, &processed, &ThreeFpPlayer::redraw);
        connect(view.sourcePane(), &PreviewPane::viewChanged, &player, &ThreeFpPlayer::setView);

        VapourSynthFrameServer server;
        QTRY_VERIFY_WITH_TIMEOUT(!server.initializing(), 15000);
        QVERIFY2(server.available(), qPrintable(server.errorString()));
        QSignalSpy loaded(&server, &VapourSynthFrameServer::scriptLoaded);
        QSignalSpy frames(&server, &VapourSynthFrameServer::frameReady);
        QSignalSpy errors(&server, &VapourSynthFrameServer::errorOccurred);

        const QString script = QStringLiteral(
            "import vapoursynth as vs\n"
            "clip = vs.core.std.BlankClip(width=64, height=48, format=vs.YUV420P8, "
            "length=2, fpsnum=24, fpsden=1, color=[16, 128, 128])\n"
            "clip.set_output(0)\n");
        server.loadScript(script, QStringLiteral("frame-bridge-test.vpy"));
        QTRY_VERIFY_WITH_TIMEOUT(!loaded.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));

        server.requestFrame(0);
        QTRY_VERIFY_WITH_TIMEOUT(!frames.isEmpty() || !errors.isEmpty(), 10000);
        QVERIFY2(errors.isEmpty(), errors.isEmpty() ? "" : qPrintable(errors.first().first().toString()));
        const auto frame = qvariant_cast<VapourSynthFrame>(frames.first().first());
        QCOMPARE(frame.width, 64);
        QCOMPARE(frame.height, 48);
        QCOMPARE(frame.frameIndex, 0);
        QCOMPARE(static_cast<unsigned char>(frame.planes[0].front()), 16u);
        QVERIFY2(player.submitFrame(frame), qPrintable(player.lastError()));
        QVERIFY2(processed.submitFrame(frame), qPrintable(processed.lastError()));
        const auto sourceId = surface.winId();
        const auto processedId = view.processedPane()->surface()->winId();
        for (const auto mode : {CompareMode::Slider, CompareMode::SideBySide, CompareMode::Slider}) {
            view.setMode(mode);
            for (double split : {0.05, 0.5, 0.95}) {
                const auto sourcePresents = player.snapshot().swapChainPresents;
                const auto processedPresents = processed.snapshot().swapChainPresents;
                view.setSplitRatio(split);
                QCoreApplication::processEvents();
                player.redraw();
                processed.redraw();
                QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents > sourcePresents, 3000);
                QTRY_VERIFY_WITH_TIMEOUT(processed.snapshot().swapChainPresents > processedPresents, 3000);
                QCOMPARE(surface.winId(), sourceId);
                QCOMPARE(view.processedPane()->surface()->winId(), processedId);
            }
        }
        QTest::qWait(100);
        for (const QSize size : {QSize(913, 517), QSize(1200, 700), QSize(777, 433)}) {
            const auto presents = player.snapshot().swapChainPresents;
            view.resize(size);
            QCoreApplication::processEvents();
            player.redraw();
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents > presents, 3000);
            const auto beforeZoom = player.snapshot().swapChainPresents;
            const QPointF center(surface.rect().center());
            QWheelEvent wheel(center, surface.mapToGlobal(center.toPoint()), {}, QPoint(0, 120),
                Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            const float previousZoom = view.sourcePane()->zoom();
            QCoreApplication::sendEvent(&surface, &wheel);
            QVERIFY(view.sourcePane()->zoom() > previousZoom);
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents > beforeZoom, 3000);
            player.setView(1.0f, 0.0f, 0.0f);
        }
        for (bool fullscreen : {true, false}) {
            const auto presents = player.snapshot().swapChainPresents;
            if (fullscreen) view.showFullScreen(); else view.showNormal();
            QCoreApplication::processEvents();
            player.redraw();
            QTRY_VERIFY_WITH_TIMEOUT(player.snapshot().swapChainPresents > presents, 3000);
        }
        QCOMPARE(player.snapshot().frameIndex, 0);
        ThreeFpPixelProbe sample{};
        QVERIFY2(player.samplePixel(surface.width() / 2, surface.height() / 2, sample),
                 qPrintable(player.lastError()));
        QVERIFY(sample.red >= 0.0f && sample.red < 0.1f);
        QVERIFY(sample.green >= 0.0f && sample.green < 0.1f);
        QVERIFY(sample.blue >= 0.0f && sample.blue < 0.1f);
    }
};

QTEST_MAIN(TestFrameBridge)
#include "TestFrameBridge.moc"
