#include <QtTest/QTest>

#include <QCoreApplication>
#include <QHash>
#include <QPointer>
#include <QSignalSpy>
#include <QTimer>

#include "modules/app/defence/crashguard.h"
#include "modules/ui/fonts/fontpreviewsafetycache.h"
#include "modules/ui/fonts/fontpreviewsafetychecker.h"

class FakeFontPreviewSafetyChecker final : public FontPreviewSafetyChecker
{
    Q_OBJECT

public:
    struct Plan
    {
        bool willFinish;
        bool ok;
        int delayMs;
    };

    explicit FakeFontPreviewSafetyChecker( QObject *parent = nullptr )
        : FontPreviewSafetyChecker( parent )
    {
    }

    void setPlan( const QString &family, const int pixelSize, const Plan &plan )
    {
        m_plans.insert( qMakePair( family, pixelSize ), plan );
    }

    int startCount( const QString &family, const int pixelSize ) const
    {
        return m_starts.value( qMakePair( family, pixelSize ), 0 );
    }

    void startCheck( const QString &family, const int pixelSize ) override
    {
        const Key key = qMakePair( family, pixelSize );
        m_starts[ key ] = m_starts.value( key, 0 ) + 1;

        const Plan plan = m_plans.value( key, Plan { false, false, 0 } );
        if ( !plan.willFinish ) {
            return;
        }

        QTimer::singleShot( plan.delayMs, this, [this, family, pixelSize, plan]() {
            Q_EMIT checkFinished( family, pixelSize, plan.ok );
        } );
    }

private:
    using Key = QPair<QString, int>;
    QHash<Key, Plan> m_plans;
    QHash<Key, int> m_starts;
};

class tst_FontPreviewSafetyCache : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void safeCheck_updatesStatusAndCachesResult();
    void timeout_thenLateFinish_freesCapacity();
    void blocked_canBeRetriedByRequestingAgain();
    void timedOut_isNotRetriedAutomatically();
};

void tst_FontPreviewSafetyCache::safeCheck_updatesStatusAndCachesResult()
{
    FakeFontPreviewSafetyChecker checker;
    const FontPreviewSafetyCache::Config cfg { 50, 8 };
    FontPreviewSafetyCache cache( &checker, cfg );

    const QString family = QStringLiteral( "FakeSafe" );
    const int px = 12;

    checker.setPlan( family, px, FakeFontPreviewSafetyChecker::Plan { true, true, 1 } );

    QSignalSpy spy( &cache, &FontPreviewSafetyCache::statusChanged );

    QCOMPARE( cache.statusForFamily( family, px ), FontPreviewSafetyCache::Status::NeverChecked );
    cache.requestCheck( family, px );

    QTRY_VERIFY( spy.count() >= 1 );
    QCOMPARE( cache.statusForFamily( family, px ), FontPreviewSafetyCache::Status::Checking );

    QTRY_COMPARE( cache.statusForFamily( family, px ), FontPreviewSafetyCache::Status::Safe );

    // Should not start a second time when already Safe.
    cache.requestCheck( family, px );
    QCOMPARE( checker.startCount( family, px ), 1 );
}

void tst_FontPreviewSafetyCache::timeout_thenLateFinish_freesCapacity()
{
    FakeFontPreviewSafetyChecker checker;
    const FontPreviewSafetyCache::Config cfg { 10, 1 };
    FontPreviewSafetyCache cache( &checker, cfg );

    const QString slowFamily = QStringLiteral( "FakeSlow" );
    const QString otherFamily = QStringLiteral( "Other" );
    const int px = 12;

    // Finish well after timeout.
    checker.setPlan( slowFamily, px, FakeFontPreviewSafetyChecker::Plan { true, true, 250 } );

    QSignalSpy spy( &cache, &FontPreviewSafetyCache::statusChanged );

    cache.requestCheck( slowFamily, px );

    // Ensure we actually see the TimedOut transition (it can be transient if the late
    // finish comes in very quickly).
    QTRY_VERIFY( spy.count() >= 2 );

    bool sawTimedOut = false;
    for ( const auto &args : spy ) {
        if ( args.size() >= 3 && args.at( 2 ).toInt() == static_cast<int>( FontPreviewSafetyCache::Status::TimedOut ) ) {
            sawTimedOut = true;
            break;
        }
    }
    QVERIFY( sawTimedOut );

    // With maxStuckChecks=1, new checks are now blocked.
    cache.requestCheck( otherFamily, px );
    QCOMPARE( cache.statusForFamily( otherFamily, px ), FontPreviewSafetyCache::Status::Blocked );

    // When the slow check eventually finishes, capacity is freed.
    QTRY_COMPARE( cache.statusForFamily( slowFamily, px ), FontPreviewSafetyCache::Status::Safe );

    // Retrying the blocked key should now proceed.
    checker.setPlan( otherFamily, px, FakeFontPreviewSafetyChecker::Plan { true, true, 1 } );
    cache.requestCheck( otherFamily, px );
    QTRY_COMPARE( cache.statusForFamily( otherFamily, px ), FontPreviewSafetyCache::Status::Safe );
}

void tst_FontPreviewSafetyCache::blocked_canBeRetriedByRequestingAgain()
{
    FakeFontPreviewSafetyChecker checker;
    const FontPreviewSafetyCache::Config cfg { 10, 1 };
    FontPreviewSafetyCache cache( &checker, cfg );

    const QString familyA = QStringLiteral( "A" );
    const QString familyB = QStringLiteral( "B" );
    const int px = 12;

    // A never finishes; it will time out and consume capacity.
    checker.setPlan( familyA, px, FakeFontPreviewSafetyChecker::Plan { false, false, 0 } );
    cache.requestCheck( familyA, px );
    QTRY_COMPARE( cache.statusForFamily( familyA, px ), FontPreviewSafetyCache::Status::TimedOut );

    cache.requestCheck( familyB, px );
    QCOMPARE( cache.statusForFamily( familyB, px ), FontPreviewSafetyCache::Status::Blocked );

    // Even if we request again, it stays Blocked until capacity changes.
    cache.requestCheck( familyB, px );
    QCOMPARE( cache.statusForFamily( familyB, px ), FontPreviewSafetyCache::Status::Blocked );
}

void tst_FontPreviewSafetyCache::timedOut_isNotRetriedAutomatically()
{
    FakeFontPreviewSafetyChecker checker;
    const FontPreviewSafetyCache::Config cfg { 10, 8 };
    FontPreviewSafetyCache cache( &checker, cfg );

    const QString family = QStringLiteral( "Slow" );
    const int px = 12;

    checker.setPlan( family, px, FakeFontPreviewSafetyChecker::Plan { true, true, 50 } );
    cache.requestCheck( family, px );
    QTRY_COMPARE( cache.statusForFamily( family, px ), FontPreviewSafetyCache::Status::TimedOut );

    // Requesting again while TimedOut does not re-run the check.
    cache.requestCheck( family, px );
    QCOMPARE( checker.startCount( family, px ), 1 );
}

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "tst_fontpreviewsafetycache main", [argc, argv]() -> int {
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;

        QCoreApplication app( localArgc, localArgv );
        tst_FontPreviewSafetyCache tc;
        const int result = QTest::qExec( &tc, localArgc, localArgv );
        return result;
    } );

    return exitCode;
}

#include "tst_fontpreviewsafetycache.moc"
