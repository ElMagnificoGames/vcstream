#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QString>
#include <QUrl>

#include "modules/app/lifecycle/appsupervisor.h"

int main( int argc, char **argv )
{
    int exitCode;
    QGuiApplication app( argc, argv );
    QQmlApplicationEngine engine;
    const QUrl mainUrl( QStringLiteral( "qrc:/qml/main.qml" ) );
    AppSupervisor appSupervisor;

    exitCode = 0;

    QCoreApplication::setApplicationName( QStringLiteral( "vcstream" ) );
    QCoreApplication::setApplicationVersion( QStringLiteral( VCSTREAM_VERSION_STRING ) );

    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &appSupervisor );

    QObject::connect( &app, &QCoreApplication::aboutToQuit, &appSupervisor, &AppSupervisor::shutdown );

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [mainUrl]( QObject *obj, const QUrl &objUrl ) {
            if ( !obj && ( mainUrl == objUrl ) ) {
                QCoreApplication::exit( -1 );
            }
        },
        Qt::QueuedConnection );

    engine.load( mainUrl );

    exitCode = app.exec();

    return exitCode;
}
