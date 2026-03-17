#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QString>
#include <QUrl>

int main( int argc, char **argv )
{
    int exitCode;
    QGuiApplication app( argc, argv );
    QQmlApplicationEngine engine;
    const QUrl mainUrl( QStringLiteral( "qrc:/qml/main.qml" ) );

    exitCode = 0;

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
