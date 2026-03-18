#include <QtTest/QTest>

#include <QCoreApplication>

#include <exception>
#include <stdexcept>

#include "modules/app/defence/crashguard.h"

class tst_CrashGuard : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void runGuardedMain_catchesStdException();
    void runGuardedMain_catchesBadAlloc();
    void runGuardedMain_catchesUnknown();
};

void tst_CrashGuard::runGuardedMain_catchesStdException()
{
    const int exitCode = crashguard::runGuardedMain( "tst_CrashGuard std::exception", []() -> int {
        throw std::runtime_error( "boom" );
    } );

    QCOMPARE( exitCode, 1 );
}

void tst_CrashGuard::runGuardedMain_catchesBadAlloc()
{
    const int exitCode = crashguard::runGuardedMain( "tst_CrashGuard std::bad_alloc", []() -> int {
        throw std::bad_alloc();
    } );

    QCOMPARE( exitCode, 1 );
}

void tst_CrashGuard::runGuardedMain_catchesUnknown()
{
    const int exitCode = crashguard::runGuardedMain( "tst_CrashGuard unknown", []() -> int {
        throw 42;
    } );

    QCOMPARE( exitCode, 1 );
}

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "tst_crashguard main", [argc, argv]() -> int {
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;
        QCoreApplication app( localArgc, localArgv );
        tst_CrashGuard tc;
        const int result = QTest::qExec( &tc, localArgc, localArgv );
        return result;
    } );

    return exitCode;
}

#include "tst_crashguard.moc"
