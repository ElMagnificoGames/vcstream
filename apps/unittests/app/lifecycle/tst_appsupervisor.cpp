#include <QtTest/QTest>

#include <QCoreApplication>
#include <QObject>
#include <QSignalSpy>
#include <QScopedPointer>
#include <QSettings>
#include <QTemporaryDir>

#include "modules/app/defence/crashguard.h"
#include "modules/app/lifecycle/appsupervisor.h"

#include "apps/unittests/helpers/test_random.h"

class tst_AppSupervisor : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void appVersion_roundTripsExplicitValues();
    void appVersion_roundTripsDeterministicRandomAscii();
    void joinRoomEnabled_defaultsToFalse();
    void joinRoomEnabled_emitsChangedOnlyOnChange();
    void hostRoomEnabled_defaultsToFalse();
    void hostRoomEnabled_emitsChangedOnlyOnChange();
    void shutdown_isSafeToCall();
};

namespace {

QScopedPointer<QTemporaryDir> g_settingsDir;

}

void tst_AppSupervisor::initTestCase()
{
    g_settingsDir.reset( new QTemporaryDir() );
    QVERIFY( g_settingsDir->isValid() );

    QCoreApplication::setOrganizationName( QStringLiteral( "vcstream-tests" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "tst_appsupervisor" ) );

    QSettings::setDefaultFormat( QSettings::IniFormat );
    QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, g_settingsDir->path() );

    {
        QSettings settings;
        settings.clear();
        settings.sync();
    }
}

void tst_AppSupervisor::cleanupTestCase()
{
    g_settingsDir.reset();
}

void tst_AppSupervisor::appVersion_roundTripsExplicitValues()
{
    AppSupervisor supervisor;

    {
        const QString applicationVersionTestValue = QString();
        QCoreApplication::setApplicationVersion( applicationVersionTestValue );
        QCOMPARE( supervisor.appVersion(), applicationVersionTestValue );
    }

    {
        const QString applicationVersionTestValue = QStringLiteral( "test-only-application-version" );
        QCoreApplication::setApplicationVersion( applicationVersionTestValue );
        QCOMPARE( supervisor.appVersion(), applicationVersionTestValue );
    }

    {
        const QString applicationVersionTestValue( 1024, QLatin1Char( 'x' ) );
        QCoreApplication::setApplicationVersion( applicationVersionTestValue );
        QCOMPARE( supervisor.appVersion(), applicationVersionTestValue );
    }
}

void tst_AppSupervisor::appVersion_roundTripsDeterministicRandomAscii()
{
    const quint32 defaultSeed = 0xC0FFEEu;
    const quint32 seed = testhelpers::seedFromEnvironmentOrDefault( "VCSTREAM_DEBUG_TEST_SEED", defaultSeed );
    QRandomGenerator rng( seed );
    AppSupervisor supervisor;
    const int cases = 25;

    for ( int i = 0; i < cases; ++i ) {
        const QString applicationVersionTestValue = testhelpers::randomAsciiString( rng, 0, 64 );
        QCoreApplication::setApplicationVersion( applicationVersionTestValue );
        const QString actualVersion = supervisor.appVersion();

        if ( actualVersion != applicationVersionTestValue ) {
            const QString message =
                QStringLiteral( "Mismatch: VCSTREAM_DEBUG_TEST_SEED=%1 case=%2 expected='%3' actual='%4'" )
                    .arg( seed )
                    .arg( i )
                    .arg( applicationVersionTestValue )
                    .arg( actualVersion );

            QFAIL( qPrintable( message ) );
        }
    }
}

void tst_AppSupervisor::shutdown_isSafeToCall()
{
    AppSupervisor supervisor;

    supervisor.shutdown();
}

void tst_AppSupervisor::joinRoomEnabled_defaultsToFalse()
{
    AppSupervisor supervisor;

    QCOMPARE( supervisor.joinRoomEnabled(), false );
}

void tst_AppSupervisor::joinRoomEnabled_emitsChangedOnlyOnChange()
{
    AppSupervisor supervisor;
    QSignalSpy spy( &supervisor, &AppSupervisor::joinRoomEnabledChanged );

    supervisor.setJoinRoomEnabled( false );
    QCOMPARE( spy.count(), 0 );

    supervisor.setJoinRoomEnabled( true );
    QCOMPARE( spy.count(), 1 );
    QCOMPARE( supervisor.joinRoomEnabled(), true );

    supervisor.setJoinRoomEnabled( true );
    QCOMPARE( spy.count(), 1 );

    supervisor.setJoinRoomEnabled( false );
    QCOMPARE( spy.count(), 2 );
    QCOMPARE( supervisor.joinRoomEnabled(), false );
}

void tst_AppSupervisor::hostRoomEnabled_defaultsToFalse()
{
    AppSupervisor supervisor;

    QCOMPARE( supervisor.hostRoomEnabled(), false );
}

void tst_AppSupervisor::hostRoomEnabled_emitsChangedOnlyOnChange()
{
    AppSupervisor supervisor;
    QSignalSpy spy( &supervisor, &AppSupervisor::hostRoomEnabledChanged );

    supervisor.setHostRoomEnabled( false );
    QCOMPARE( spy.count(), 0 );

    supervisor.setHostRoomEnabled( true );
    QCOMPARE( spy.count(), 1 );
    QCOMPARE( supervisor.hostRoomEnabled(), true );

    supervisor.setHostRoomEnabled( true );
    QCOMPARE( spy.count(), 1 );

    supervisor.setHostRoomEnabled( false );
    QCOMPARE( spy.count(), 2 );
    QCOMPARE( supervisor.hostRoomEnabled(), false );
}

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "tst_appsupervisor main", [argc, argv]() -> int {
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;

        QCoreApplication app( localArgc, localArgv );
        tst_AppSupervisor tc;
        const int result = QTest::qExec( &tc, localArgc, localArgv );
        return result;
    } );

    return exitCode;
}

#include "tst_appsupervisor.moc"
