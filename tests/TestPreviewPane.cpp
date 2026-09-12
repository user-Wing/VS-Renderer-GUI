#include "ui/PreviewPane.h"

#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>

#include <cmath>

using namespace vsr;

class TestPreviewPane final : public QObject {
    Q_OBJECT

private slots:
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
};

QTEST_MAIN(TestPreviewPane)
#include "TestPreviewPane.moc"
