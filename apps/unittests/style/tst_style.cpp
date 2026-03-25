#include <QtTest/QTest>

#include <QCoreApplication>
#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include "modules/app/defence/crashguard.h"

class tst_Style : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void source_noStreamingOperatorChains();
};

namespace {

QStringList sourceRoots()
{
    const QString root = QString::fromUtf8( VCSTREAM_SOURCE_DIR );
    return QStringList{
        QDir( root ).filePath( QStringLiteral( "apps" ) ),
        QDir( root ).filePath( QStringLiteral( "modules" ) ),
    };
}

bool shouldSkipPath( const QString &absPath )
{
    return absPath.contains( QStringLiteral( "/third_party/" ) )
        || absPath.contains( QStringLiteral( "/build/" ) )
        || absPath.endsWith( QStringLiteral( "/apps/unittests/style/tst_style.cpp" ) );
}

bool isQtTestRowExceptionLine( const QString &line )
{
    return line.contains( QStringLiteral( "QTest::newRow" ) )
        || line.contains( QStringLiteral( "QTest::addRow" ) );
}

}

void tst_Style::source_noStreamingOperatorChains()
{
    const QStringList roots = sourceRoots();
    QStringList failures;

    for ( const QString &root : roots ) {
        QDirIterator it( root, QStringList{ QStringLiteral( "*.cpp" ), QStringLiteral( "*.h" ) }, QDir::Files, QDirIterator::Subdirectories );
        while ( it.hasNext() ) {
            const QString path = it.next();
            if ( shouldSkipPath( path ) ) {
                continue;
            }

            QFile f( path );
            if ( !f.open( QIODevice::ReadOnly ) ) {
                failures.append( QStringLiteral( "%1: failed to open" ).arg( path ) );
                continue;
            }

            const QByteArray bytes = f.readAll();
            const QString text = QString::fromUtf8( bytes );
            const QStringList lines = text.split( QLatin1Char( '\n' ) );

            for ( int i = 0; i < lines.size(); ++i ) {
                const int lineNo = i + 1;
                const QString line = lines.at( i );

                if ( isQtTestRowExceptionLine( line ) ) {
                    continue;
                }

                // Ban stream/chaining uses of operator<<. Bitshifts are allowed.
                // This is a best-effort text gate; do not add new exceptions unless
                // they are strictly unavoidable due to third-party APIs.
                if ( line.contains( QStringLiteral( "qDebug() <<" ) )
                    || line.contains( QStringLiteral( "qInfo() <<" ) )
                    || line.contains( QStringLiteral( "qWarning() <<" ) )
                    || line.contains( QStringLiteral( "qCritical() <<" ) )
                    || line.contains( QStringLiteral( "qFatal() <<" ) )
                    || line.contains( QStringLiteral( "std::cout <<" ) )
                    || line.contains( QStringLiteral( "std::cerr <<" ) )
                    || line.contains( QStringLiteral( "std::clog <<" ) ) ) {
                    failures.append( QStringLiteral( "%1:%2: forbidden streaming operator chain" ).arg( path ).arg( lineNo ) );
                    continue;
                }

                // Catch common Qt streaming/container chaining patterns without tripping on bitshifts.
                if ( line.contains( QStringLiteral( "<<" ) )
                    && ( line.contains( QLatin1Char( '"' ) )
                         || line.contains( QStringLiteral( "QStringLiteral" ) )
                         || line.contains( QStringLiteral( "QLatin1String" ) )
                         || line.contains( QStringLiteral( "QTextStream" ) )
                         || line.contains( QStringLiteral( "QDataStream" ) )
                         || line.contains( QStringLiteral( "QStringList" ) ) ) ) {
                    failures.append( QStringLiteral( "%1:%2: suspicious operator<< usage (stream/chaining)" ).arg( path ).arg( lineNo ) );
                    continue;
                }
            }
        }
    }

    if ( !failures.isEmpty() ) {
        QFAIL( qPrintable( failures.join( QLatin1Char( '\n' ) ) ) );
    }
}

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "tst_style main", [argc, argv]() -> int {
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;

        QCoreApplication app( localArgc, localArgv );
        tst_Style tc;
        const int result = QTest::qExec( &tc, localArgc, localArgv );
        return result;
    } );

    return exitCode;
}

#include "tst_style.moc"
