#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQmlEngine>
#include <QString>
#include <QUrl>
#include <QWindow>

#include <QtGlobal>

#include "modules/app/defence/crashguard.h"
#include "modules/app/platform/qtshims.h"
#include "modules/app/lifecycle/appsupervisor.h"
#include "modules/ui/colour/oklchutil.h"
#include "modules/ui/placement/windowplacement.h"
#include "modules/ui/theme/accentimageprovider.h"
#include "modules/ui/theme/themeiconimageprovider.h"
#include "modules/ui/fonts/fontpreviewsafetycache.h"

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

        qtshims::applyBeforeQGuiApplication();

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

        qmlRegisterUncreatableType<FontPreviewSafetyCache>( "VcStream", 1, 0, "FontPreviewSafety",
            QStringLiteral( "FontPreviewSafety is exposed via AppSupervisor.fontPreviewSafetyCache." ) );

        QObject::connect( &app, &QCoreApplication::aboutToQuit, &appSupervisor, &AppSupervisor::shutdown );

        QObject::connect( &engine, &QQmlApplicationEngine::warnings, &engine, []( const QList<QQmlError> &warnings ) {
            for ( const QQmlError &e : warnings ) {
                const QByteArray msgUtf8 = e.toString().toUtf8();
                qWarning( "%s", msgUtf8.constData() );
            }
        } );

        engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
        engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
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
