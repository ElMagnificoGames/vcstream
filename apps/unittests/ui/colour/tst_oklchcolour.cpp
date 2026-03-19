#include <QtTest/QTest>

#include <QColor>
#include <QCoreApplication>

#include "modules/app/defence/crashguard.h"
#include "modules/ui/colour/oklchcolour.h"

class tst_OklchColour : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toSrgbFitted_returnsValidRgb_data();
    void toSrgbFitted_returnsValidRgb();
    void fitToSrgbGamut_producesInGamut();
};

void tst_OklchColour::toSrgbFitted_returnsValidRgb_data()
{
    QTest::addColumn<double>( "L" );
    QTest::addColumn<double>( "C" );
    QTest::addColumn<double>( "h" );

    QTest::newRow( "neutral" ) << 0.6 << 0.0 << 0.0;
    QTest::newRow( "blue" ) << 0.65 << 0.18 << 260.0;
    QTest::newRow( "red" ) << 0.65 << 0.18 << 20.0;
    QTest::newRow( "light" ) << 0.92 << 0.10 << 120.0;
    QTest::newRow( "dark" ) << 0.10 << 0.10 << 280.0;
    QTest::newRow( "high-chroma" ) << 0.65 << 0.9 << 330.0;
}

void tst_OklchColour::toSrgbFitted_returnsValidRgb()
{
    QFETCH( double, L );
    QFETCH( double, C );
    QFETCH( double, h );

    oklchcolour::Oklch in;
    in.lightness = L;
    in.chroma = C;
    in.hueDegrees = h;

    const QColor out = oklchcolour::toSrgbFitted( in );

    QVERIFY( out.isValid() );
    QVERIFY( out.alpha() == 255 );
    QVERIFY( out.red() >= 0 && out.red() <= 255 );
    QVERIFY( out.green() >= 0 && out.green() <= 255 );
    QVERIFY( out.blue() >= 0 && out.blue() <= 255 );
}

void tst_OklchColour::fitToSrgbGamut_producesInGamut()
{
    oklchcolour::Oklch in;
    in.lightness = 0.65;
    in.chroma = 1.0;
    in.hueDegrees = 330.0;

    const oklchcolour::Oklch fitted = oklchcolour::fitToSrgbGamut( in );
    QVERIFY( oklchcolour::isInSrgbGamut( fitted ) );
}

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "tst_oklchcolour main", [argc, argv]() -> int {
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;

        QCoreApplication app( localArgc, localArgv );
        tst_OklchColour tc;
        const int result = QTest::qExec( &tc, localArgc, localArgv );
        return result;
    } );

    return exitCode;
}

#include "tst_oklchcolour.moc"
