#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QString>
#include <QUrl>
#include <QWindow>

#include "modules/app/defence/crashguard.h"
#include "modules/app/lifecycle/appsupervisor.h"
#include "modules/ui/colour/oklchutil.h"
#include "modules/ui/placement/windowplacement.h"
#include "modules/ui/theme/accentimageprovider.h"

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "vcstream main", [argc, argv]() -> int {
        int guardedExitCode;
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;

        QCoreApplication::setOrganizationName( QStringLiteral( "ElMagnificoGames" ) );
        QCoreApplication::setApplicationName( QStringLiteral( "vcstream" ) );

        QGuiApplication app( localArgc, localArgv );
        const QUrl mainUrl( QStringLiteral( "qrc:/qml/main.qml" ) );
        QWindow *mainWindow;

        guardedExitCode = 0;
        mainWindow = nullptr;

        QCoreApplication::setApplicationVersion( QStringLiteral( VCSTREAM_VERSION_STRING ) );

        AppSupervisor appSupervisor;

        OklchUtil oklchUtil;
        QQmlApplicationEngine engine;

        QObject::connect( &app, &QCoreApplication::aboutToQuit, &appSupervisor, &AppSupervisor::shutdown );

        QObject::connect( &engine, &QQmlApplicationEngine::warnings, &engine, []( const QList<QQmlError> &warnings ) {
            for ( const QQmlError &e : warnings ) {
                qWarning() << e;
            }
        } );

        engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
        engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
        engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &appSupervisor );

        QObject::connect( &engine, &QQmlApplicationEngine::objectCreated, &app,
            [&mainWindow, mainUrl]( QObject *obj, const QUrl &objUrl ) {
                if ( !obj && ( mainUrl == objUrl ) ) {
                    QCoreApplication::exit( -1 );
                }

                if ( obj && ( mainUrl == objUrl ) ) {
                    QWindow *window;

                    window = qobject_cast<QWindow *>( obj );
                    if ( window ) {
                        mainWindow = window;
                        windowplacement::restoreAndRepairMainWindowPlacement( window );
                    }
                }
            },
            Qt::QueuedConnection );

        QObject::connect( &app, &QCoreApplication::aboutToQuit, &app, [&mainWindow]() {
            if ( mainWindow != nullptr ) {
                windowplacement::saveMainWindowPlacement( mainWindow );
            }
        } );

        engine.load( mainUrl );

        guardedExitCode = app.exec();

        return guardedExitCode;
    } );

    return exitCode;
}
