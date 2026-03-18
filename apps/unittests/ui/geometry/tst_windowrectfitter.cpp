#include <QtTest/QTest>

#include <QObject>
#include <QRect>
#include <QSize>
#include <QVector>

#include "modules/ui/geometry/windowrectfitter.h"

class tst_WindowRectFitter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fit_singleMonitor_unchangedWhenContained();
    void fit_singleMonitor_movesToContain();
    void fit_lShape_allowsWideSpanningRect();
    void fit_resizesToLargestContainedSize();
    void fit_degraded_keepsTopEdgeVisible();
};

void tst_WindowRectFitter::fit_singleMonitor_unchangedWhenContained()
{
    const QVector<QRect> allowed { QRect( 0, 0, 1920, 1080 ) };
    const QRect desired( 100, 100, 800, 600 );
    const QSize minSize( 320, 240 );

    const windowrectfitter::FitResult r = windowrectfitter::fit( desired, allowed, minSize, 32, 160 );

    QCOMPARE( r.outcome, windowrectfitter::Outcome::Unchanged );
    QCOMPARE( r.rect, desired );
    QCOMPARE( r.fullyContained, true );
}

void tst_WindowRectFitter::fit_singleMonitor_movesToContain()
{
    const QVector<QRect> allowed { QRect( 0, 0, 1920, 1080 ) };
    const QRect desired( 1800, 100, 400, 400 );
    const QSize minSize( 320, 240 );

    const windowrectfitter::FitResult r = windowrectfitter::fit( desired, allowed, minSize, 32, 160 );

    QCOMPARE( r.outcome, windowrectfitter::Outcome::Moved );
    QCOMPARE( r.fullyContained, true );
    QCOMPARE( r.rect.size(), desired.size() );
    QCOMPARE( r.rect.x(), 1520 );
    QCOMPARE( r.rect.y(), 100 );
}

void tst_WindowRectFitter::fit_lShape_allowsWideSpanningRect()
{
    const QRect landscape( 0, 0, 1920, 1080 );
    const QRect portrait( 1920, 0, 1080, 1920 );
    const QVector<QRect> allowed { landscape, portrait };
    const QRect desired( 0, 0, 3000, 1080 );
    const QSize minSize( 800, 540 );

    const windowrectfitter::FitResult r = windowrectfitter::fit( desired, allowed, minSize, 32, 160 );

    QCOMPARE( r.outcome, windowrectfitter::Outcome::Unchanged );
    QCOMPARE( r.rect, desired );
    QCOMPARE( r.fullyContained, true );
}

void tst_WindowRectFitter::fit_resizesToLargestContainedSize()
{
    const QVector<QRect> allowed { QRect( 0, 0, 640, 480 ) };
    const QRect desired( 0, 0, 800, 600 );
    const QSize minSize( 320, 240 );

    const windowrectfitter::FitResult r = windowrectfitter::fit( desired, allowed, minSize, 32, 160 );

    QCOMPARE( r.outcome, windowrectfitter::Outcome::Resized );
    QCOMPARE( r.fullyContained, true );
    QCOMPARE( r.rect, QRect( 0, 0, 640, 480 ) );
}

void tst_WindowRectFitter::fit_degraded_keepsTopEdgeVisible()
{
    const QVector<QRect> allowed { QRect( 0, 0, 640, 480 ) };
    const QRect desired( 100, 100, 800, 540 );
    const QSize minSize( 800, 540 );

    const windowrectfitter::FitResult r = windowrectfitter::fit( desired, allowed, minSize, 32, 160 );

    QCOMPARE( r.outcome, windowrectfitter::Outcome::Degraded );
    QCOMPARE( r.rect.size(), minSize );

    QCOMPARE( r.rect.y(), 0 );
    QCOMPARE( r.rect.x(), 0 );
}

QTEST_MAIN( tst_WindowRectFitter )

#include "tst_windowrectfitter.moc"
