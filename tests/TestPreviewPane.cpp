#include "ui/PreviewPane.h"
#include "ui/CompareView.h"

#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>
#include <QLineF>

#include <cmath>

using namespace vsr;

class TestPreviewPane final : public QObject {
    Q_OBJECT

private slots:
    void wheelAnchorsSourcePixelAndWorksForImages() {
        PreviewPane pane("Test","Fit");pane.resize(900,600);pane.show();pane.setVideoSize({1920,1080});
        auto *surface=pane.surface();const QPointF at(surface->width()*.7,surface->height()*.55);
        const auto coordinate=[&]{const auto fit=QSizeF(QSize(1920,1080).scaled(surface->size(),Qt::KeepAspectRatio));const auto scaled=fit*pane.zoom();const auto pan=pane.pan();const QPointF origin((surface->width()-scaled.width())/2+pan.x()*std::max(0.,scaled.width()-surface->width())/2,(surface->height()-scaled.height())/2+pan.y()*std::max(0.,scaled.height()-surface->height())/2);return QPointF((at.x()-origin.x())/scaled.width(),(at.y()-origin.y())/scaled.height());};
        pane.adoptView(2,0,0);const auto before=coordinate();QWheelEvent wheel(at,surface->mapToGlobal(at.toPoint()),{},QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);QApplication::sendEvent(surface,&wheel);
        QVERIFY(QLineF(before,coordinate()).length()<1e-5);
        QImage image({1920,1080},QImage::Format_ARGB32);image.fill(Qt::red);pane.setImage(image);pane.adoptView(1,0,0);QApplication::sendEvent(surface,&wheel);QCOMPARE(pane.zoom(),1.25f);
        pane.setImage({});pane.adoptView(1,0,0);QApplication::sendEvent(surface,&wheel);QCOMPARE(pane.zoom(),1.25f);
    }
    void leftDragPansOnlyAfterZoom()
    {
        PreviewPane pane(QStringLiteral("Test"), QStringLiteral("Fit"));
        pane.resize(640, 400);
        pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        QWidget *surface = pane.surface();
        QSignalSpy changes(&pane, &PreviewPane::viewChanged);

        const QPointF center(surface->width() / 2.0, surface->height() / 2.0);
        QWheelEvent wheel(center, surface->mapToGlobal(center.toPoint()), {}, QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(surface, &wheel);
        QCOMPARE(pane.zoom(), 1.25f);

        QMouseEvent press(QEvent::MouseButtonPress, center, surface->mapToGlobal(center.toPoint()),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(surface, &press);
        const QPointF moved = center + QPointF(80.0, 30.0);
        QMouseEvent move(QEvent::MouseMove, moved, surface->mapToGlobal(moved.toPoint()),
                         Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(surface, &move);
        QMouseEvent release(QEvent::MouseButtonRelease, moved, surface->mapToGlobal(moved.toPoint()),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(surface, &release);

        QVERIFY(changes.count() >= 2);
        const auto last = changes.last();
        QVERIFY(std::abs(last.at(1).toFloat()) > 0.01f);
        QVERIFY(std::abs(last.at(2).toFloat()) > 0.01f);
    }

    void letterboxedVideoUsesEqualPixelPanSensitivity()
    {
        PreviewPane pane(QStringLiteral("Wide video"),QStringLiteral("Fit"));
        pane.resize(640,700);pane.setVideoSize(QSize(3840,2160));pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        QWidget *surface=pane.surface();pane.adoptView(3,0,0);
        const QPointF start(surface->width()/2.0,surface->height()/2.0),end=start+QPointF(8,8);
        QSignalSpy changes(&pane,&PreviewPane::viewChanged);
        QMouseEvent press(QEvent::MouseButtonPress,start,surface->mapToGlobal(start.toPoint()),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QMouseEvent move(QEvent::MouseMove,end,surface->mapToGlobal(end.toPoint()),Qt::NoButton,Qt::LeftButton,Qt::NoModifier);
        QCoreApplication::sendEvent(surface,&press);QCoreApplication::sendEvent(surface,&move);
        QVERIFY(!changes.isEmpty());
        const QSize fitted=QSize(3840,2160).scaled(surface->size(),Qt::KeepAspectRatio);
        const double x=changes.last()[1].toDouble()*(fitted.width()*3-surface->width())/2;
        const double y=changes.last()[2].toDouble()*(fitted.height()*3-surface->height())/2;
        QVERIFY(std::abs(x-8)<.01);QVERIFY(std::abs(y-8)<.01);
    }

    void dragDistanceIsIndependentOfZoom()
    {
        PreviewPane pane(QStringLiteral("Test"), QStringLiteral("Fit"));
        pane.resize(640, 400);
        pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        QWidget *surface = pane.surface();
        const QPointF center(surface->width() / 2.0, surface->height() / 2.0);

        const auto dragAtZoom = [&](float zoom) {
            pane.adoptView(zoom, 0.0f, 0.0f);
            QSignalSpy changes(&pane, &PreviewPane::viewChanged);
            QMouseEvent press(QEvent::MouseButtonPress, center, surface->mapToGlobal(center.toPoint()),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(surface, &press);
            const QPointF moved = center + QPointF(5.0, 3.0);
            QMouseEvent move(QEvent::MouseMove, moved, surface->mapToGlobal(moved.toPoint()),
                             Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(surface, &move);
            QMouseEvent release(QEvent::MouseButtonRelease, moved, surface->mapToGlobal(moved.toPoint()),
                                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(surface, &release);
            if (changes.isEmpty())
                return QPointF{};
            const auto last = changes.last();
            return QPointF(last.at(1).toFloat() * (zoom - 1.0f),
                           last.at(2).toFloat() * (zoom - 1.0f));
        };

        const QPointF displacementAt2x = dragAtZoom(2.0f);
        const QPointF displacementAt8x = dragAtZoom(8.0f);
        QVERIFY(!displacementAt2x.isNull());
        QVERIFY(!displacementAt8x.isNull());
        const QString values = QStringLiteral("2x=(%1,%2), 8x=(%3,%4)")
            .arg(displacementAt2x.x()).arg(displacementAt2x.y())
            .arg(displacementAt8x.x()).arg(displacementAt8x.y());
        QVERIFY2(std::abs(displacementAt2x.x() - displacementAt8x.x()) < 0.001f, qPrintable(values));
        QVERIFY2(std::abs(displacementAt2x.y() - displacementAt8x.y()) < 0.001f, qPrintable(values));
    }

    void wheelStillWorksWhenDeliveredToPaneAfterResize()
    {
        PreviewPane pane(QStringLiteral("Test"), QStringLiteral("实时"));
        pane.resize(640, 400);
        pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        QSignalSpy redraws(&pane, &PreviewPane::redrawRequested);
        pane.resize(960, 540);
        QTRY_VERIFY(!redraws.isEmpty());

        const QPointF center(pane.width() / 2.0, pane.height() / 2.0);
        QWheelEvent wheel(center, pane.mapToGlobal(center.toPoint()), {}, QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&pane, &wheel);
        QCOMPARE(pane.zoom(), 1.25f);
    }

    void resizeBurstCoalescesNativeRedraw()
    {
        PreviewPane pane(QStringLiteral("Test"), QStringLiteral("实时"));
        pane.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pane));
        QSignalSpy redraws(&pane, &PreviewPane::redrawRequested);
        for (int i = 0; i < 12; ++i)
            pane.resize(640 + i * 10, 360 + i * 5);
        QTest::qWait(100);
        QCOMPARE(redraws.size(), 1);
    }

    void sliderModeClipsProcessedPaneAtDivider()
    {
        CompareView view;
        view.resize(1000, 600);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
        view.setMode(CompareMode::Slider);
        view.setSplitRatio(0.35);
        QCoreApplication::processEvents();

        QCOMPARE(view.sourcePane()->geometry(), QRect(0, 0, 1000, 600));
        QCOMPARE(view.processedPane()->geometry(), QRect(-350, 0, 1000, 600));
        QCOMPARE(view.sourcePane()->parentWidget()->geometry(), QRect(0, 0, 350, 600));
        QCOMPARE(view.processedPane()->parentWidget()->geometry(), QRect(350, 0, 650, 600));
        QVERIFY(view.processedPane()->mask().isEmpty());
        const auto sourceId = view.sourcePane()->surface()->winId();
        const auto processedId = view.processedPane()->surface()->winId();
        for (double ratio : {0.05, 0.9, 0.5}) {
            view.setSplitRatio(ratio);
            QCoreApplication::processEvents();
            QVERIFY(!view.sourcePane()->parentWidget()->geometry().intersects(
                view.processedPane()->parentWidget()->geometry()));
            QCOMPARE(view.sourcePane()->surface()->winId(), sourceId);
            QCOMPARE(view.processedPane()->surface()->winId(), processedId);
        }
        view.setSplitRatio(0.35);
        QCOMPARE(view.splitRatio(), 0.35);
    }
};

QTEST_MAIN(TestPreviewPane)
#include "TestPreviewPane.moc"
