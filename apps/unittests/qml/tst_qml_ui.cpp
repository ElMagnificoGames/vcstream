#include <QtTest/QTest>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QPoint>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QLatin1Char>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRect>
#include <QScopedPointer>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QTemporaryDir>
#include <QVector>
#include <QString>
#include <QUrl>
#include <QtGlobal>

#include "modules/app/lifecycle/appsupervisor.h"
#include "modules/ui/placement/windowplacement.h"

namespace {

QVector<QString> g_messages;
QtMessageHandler g_previousHandler;
QScopedPointer<QTemporaryDir> g_settingsDir;

QString formatMessage( const QtMsgType type, const QMessageLogContext &context, const QString &message )
{
    const QString file = context.file ? QString::fromLatin1( context.file ) : QString();
    const QString function = context.function ? QString::fromLatin1( context.function ) : QString();

    return QStringLiteral( "%1:%2 %3 %4" )
        .arg( file )
        .arg( context.line )
        .arg( function )
        .arg( message );
}

void messageHandler( const QtMsgType type, const QMessageLogContext &context, const QString &message )
{
    if ( type == QtWarningMsg || type == QtCriticalMsg || type == QtFatalMsg ) {
        g_messages.append( formatMessage( type, context, message ) );
    }

    if ( g_previousHandler ) {
        g_previousHandler( type, context, message );
    }
}

QString joinedMessages()
{
    QString out;

    for ( const QString &m : g_messages ) {
        out.append( m );
        out.append( QLatin1Char( '\n' ) );
    }

    return out;
}

void clearMessages()
{
    g_messages.clear();
}

QQuickItem *findItemByObjectNameRecursive( QQuickItem *rootItem, const QString &name )
{
    QQuickItem *out;

    out = nullptr;

    if ( rootItem != nullptr ) {
        if ( rootItem->objectName() == name ) {
            out = rootItem;
        } else {
            const QList<QQuickItem *> children = rootItem->childItems();
            for ( QQuickItem *child : children ) {
                if ( out == nullptr ) {
                    QQuickItem *found = findItemByObjectNameRecursive( child, name );
                    if ( found != nullptr ) {
                        out = found;
                    }
                }
            }
        }
    }

    return out;
}

void clickItemCenter( QQuickWindow &window, QQuickItem *item )
{
    const QPointF scenePoint = item->mapToScene( QPointF( item->width() / 2.0, item->height() / 2.0 ) );
    const QPoint clickPoint = scenePoint.toPoint();

    QTest::mouseClick( &window, Qt::LeftButton, Qt::NoModifier, clickPoint );
}

void moveMouseToItemCenter( QQuickWindow &window, QQuickItem *item )
{
    const QPointF scenePoint = item->mapToScene( QPointF( item->width() / 2.0, item->height() / 2.0 ) );
    const QPoint movePoint = scenePoint.toPoint();

    QTest::mouseMove( &window, movePoint );
}

QPointF sceneTopLeft( QQuickItem *item )
{
    return item->mapToScene( QPointF( 0.0, 0.0 ) );
}

void verifyScenePositionUnchanged( QQuickItem *item, const QPointF &before, const qreal tolerance, const char *context )
{
    const QPointF after = sceneTopLeft( item );
    const qreal dx = qAbs( after.x() - before.x() );
    const qreal dy = qAbs( after.y() - before.y() );

    if ( dx > tolerance || dy > tolerance ) {
        const QString message =
            QStringLiteral( "%1 moved: dx=%2 dy=%3" )
                .arg( QString::fromLatin1( context ) )
                .arg( dx )
                .arg( dy );
        QFAIL( qPrintable( message ) );
    }
}

void failIfAnyWarnings( const char *step )
{
    if ( !g_messages.isEmpty() ) {
        const QString message =
            QStringLiteral( "step=%1\n" ).arg( QString::fromLatin1( step ) )
            + joinedMessages();

        QFAIL( qPrintable( message ) );
    }
}

}

class tst_QmlUi : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void navigation_joinAndHost_emitNoWarnings();
};

void tst_QmlUi::initTestCase()
{
    g_previousHandler = qInstallMessageHandler( messageHandler );

    g_settingsDir.reset( new QTemporaryDir() );
    QVERIFY( g_settingsDir->isValid() );

    QCoreApplication::setOrganizationName( QStringLiteral( "vcstream-tests" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "tst_qml_ui" ) );

    QSettings::setDefaultFormat( QSettings::IniFormat );
    QSettings::setPath( QSettings::IniFormat, QSettings::UserScope, g_settingsDir->path() );

    {
        QSettings settings;
        settings.clear();
        settings.sync();
    }
}

void tst_QmlUi::cleanupTestCase()
{
    qInstallMessageHandler( g_previousHandler );
    g_previousHandler = nullptr;
    g_settingsDir.reset();
}

void tst_QmlUi::navigation_joinAndHost_emitNoWarnings()
{
    AppSupervisor supervisor;
    QQmlApplicationEngine engine;

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::warnings,
        &engine,
        []( const QList<QQmlError> &warnings ) {
            for ( const QQmlError &e : warnings ) {
                g_messages.append( e.toString() );
            }
        } );

    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );

    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 50 );
    failIfAnyWarnings( "post-load" );
    clearMessages();

    {
        QSettings settings;
        settings.clear();
        settings.sync();

        QScreen *screen = window->screen();
        if ( screen == nullptr ) {
            screen = QGuiApplication::primaryScreen();
        }

        if ( screen != nullptr ) {
            const QRect avail = screen->availableGeometry();
            const QSize minSize = window->minimumSize();

            QVERIFY( window->y() >= avail.y() );

            if ( minSize.width() <= avail.width() && minSize.height() <= avail.height() ) {
                window->resize( minSize );
                QCoreApplication::processEvents();

                windowplacement::restoreAndRepairMainWindowPlacement( window );
                QCoreApplication::processEvents();

                const int expectedX = avail.x() + ( avail.width() - window->width() ) / 2;
                const int expectedY = avail.y() + ( avail.height() - window->height() ) / 2;
                const int tolerance = 4;

                {
                    const int actualX = window->x();
                    const QString message =
                        QStringLiteral( "centre-x mismatch: avail=(%1,%2 %3x%4) win=(%5,%6 %7x%8) expectedX=%9 actualX=%10" )
                            .arg( avail.x() )
                            .arg( avail.y() )
                            .arg( avail.width() )
                            .arg( avail.height() )
                            .arg( window->x() )
                            .arg( window->y() )
                            .arg( window->width() )
                            .arg( window->height() )
                            .arg( expectedX )
                            .arg( actualX );
                    QVERIFY2( qAbs( actualX - expectedX ) <= tolerance, qPrintable( message ) );
                }

                {
                    const int actualY = window->y();
                    const QString message =
                        QStringLiteral( "centre-y mismatch: avail=(%1,%2 %3x%4) win=(%5,%6 %7x%8) expectedY=%9 actualY=%10" )
                            .arg( avail.x() )
                            .arg( avail.y() )
                            .arg( avail.width() )
                            .arg( avail.height() )
                            .arg( window->x() )
                            .arg( window->y() )
                            .arg( window->width() )
                            .arg( window->height() )
                            .arg( expectedY )
                            .arg( actualY );
                    QVERIFY2( qAbs( actualY - expectedY ) <= tolerance, qPrintable( message ) );
                }
            }
        }
    }



    {
        QObject *metrics = rootObject->findChild<QObject *>( QStringLiteral( "uiMetrics" ) );
        QVERIFY2( metrics != nullptr, "uiMetrics" );

        const int minWidthExpected = metrics->property( "minimumWindowWidth" ).toInt();
        const int minHeightExpected = metrics->property( "minimumWindowHeight" ).toInt();
        QVERIFY( minWidthExpected > 0 );
        QVERIFY( minHeightExpected > 0 );

        QCOMPARE( window->minimumWidth(), minWidthExpected );
        QCOMPARE( window->minimumHeight(), minHeightExpected );

        window->setWidth( minWidthExpected - 1 );
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT( window->width() >= minWidthExpected, 1000 );

        window->setHeight( minHeightExpected - 1 );
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT( window->height() >= minHeightExpected, 1000 );
    }

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    {
        QQuickItem *joinButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "joinRoomButton" ) );
        QVERIFY2( joinButton != nullptr, "joinRoomButton" );
        QTRY_VERIFY_WITH_TIMEOUT( joinButton->isVisible(), 1000 );
        clickItemCenter( *window, joinButton );
        QTest::qWait( 50 );
        failIfAnyWarnings( "join-click" );
        clearMessages();

        QQuickItem *disconnectButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "disconnectButton" ) );
        QVERIFY2( disconnectButton != nullptr, "disconnectButton" );
        QTRY_VERIFY_WITH_TIMEOUT( disconnectButton->isVisible(), 1000 );

        QQuickItem *sourceRow0 = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( sourceRow0 = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceRow_0" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( sourceRow0->isVisible(), 1000 );
        clickItemCenter( *window, sourceRow0 );
        QTest::qWait( 50 );
        failIfAnyWarnings( "source-row-click" );
        clearMessages();

        QQuickItem *inspectorPanel = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( inspectorPanel = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorPanel" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( inspectorPanel->isVisible(), 1000 );

        {
            const QPointF panelPoint = inspectorPanel->mapToScene( QPointF( 10.0, 10.0 ) );
            QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, panelPoint.toPoint() );
            QTest::qWait( 50 );
            failIfAnyWarnings( "inspector-panel-click" );
            clearMessages();
            QVERIFY( inspectorPanel->isVisible() );
        }

        QQuickItem *inspectorExportToggle = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( inspectorExportToggle = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorBrowserExportToggle" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( inspectorExportToggle->isVisible(), 1000 );
        const QPointF exportTogglePosBeforeHover = sceneTopLeft( inspectorExportToggle );
        moveMouseToItemCenter( *window, inspectorExportToggle );
        QTest::qWait( 350 );
        failIfAnyWarnings( "inspector-export-hover" );
        verifyScenePositionUnchanged( inspectorExportToggle, exportTogglePosBeforeHover, 0.5, "export toggle" );
        clearMessages();

        clickItemCenter( *window, inspectorExportToggle );
        QTest::qWait( 50 );
        failIfAnyWarnings( "inspector-export-click" );
        clearMessages();

        QQuickItem *inspectorOverlay = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( inspectorOverlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorOverlay" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( inspectorOverlay->isVisible(), 1000 );
        const QPointF overlayPoint = inspectorOverlay->mapToScene( QPointF( 5.0, 5.0 ) );
        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, overlayPoint.toPoint() );
        QTest::qWait( 50 );
        failIfAnyWarnings( "inspector-dismiss-outside" );
        clearMessages();

        QTRY_VERIFY_WITH_TIMEOUT( !inspectorOverlay->isVisible(), 1000 );

        clickItemCenter( *window, disconnectButton );
        QTest::qWait( 50 );
        failIfAnyWarnings( "disconnect-click" );
        clearMessages();
    }

    {
        QQuickItem *hostButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "hostRoomButton" ) );
        QVERIFY2( hostButton != nullptr, "hostRoomButton" );
        QTRY_VERIFY_WITH_TIMEOUT( hostButton->isVisible(), 1000 );
        clickItemCenter( *window, hostButton );
        QTest::qWait( 50 );
        failIfAnyWarnings( "host-click" );
        clearMessages();

        QQuickItem *disconnectButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "disconnectButton" ) );
        QVERIFY2( disconnectButton != nullptr, "disconnectButton" );
        QTRY_VERIFY_WITH_TIMEOUT( disconnectButton->isVisible(), 1000 );

        QQuickItem *sourceRow0 = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( sourceRow0 = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceRow_0" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( sourceRow0->isVisible(), 1000 );
        clickItemCenter( *window, sourceRow0 );
        QTest::qWait( 50 );
        failIfAnyWarnings( "source-row-click" );
        clearMessages();

        QQuickItem *inspectorExportToggle = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( inspectorExportToggle = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorBrowserExportToggle" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( inspectorExportToggle->isVisible(), 1000 );
        const QPointF exportTogglePosBeforeHover = sceneTopLeft( inspectorExportToggle );
        moveMouseToItemCenter( *window, inspectorExportToggle );
        QTest::qWait( 350 );
        failIfAnyWarnings( "inspector-export-hover" );
        verifyScenePositionUnchanged( inspectorExportToggle, exportTogglePosBeforeHover, 0.5, "export toggle" );
        clearMessages();

        clickItemCenter( *window, inspectorExportToggle );
        QTest::qWait( 50 );
        failIfAnyWarnings( "inspector-export-click" );
        clearMessages();

        QQuickItem *inspectorPanel = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( inspectorPanel = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorPanel" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( inspectorPanel->isVisible(), 1000 );

        {
            const QPointF panelPoint = inspectorPanel->mapToScene( QPointF( 10.0, 10.0 ) );
            QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, panelPoint.toPoint() );
            QTest::qWait( 50 );
            failIfAnyWarnings( "inspector-panel-click" );
            clearMessages();
            QVERIFY( inspectorPanel->isVisible() );
        }

        QQuickItem *inspectorOverlay = nullptr;
        QTRY_VERIFY_WITH_TIMEOUT(
            ( inspectorOverlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorOverlay" ) ) ) != nullptr,
            1000 );
        QTRY_VERIFY_WITH_TIMEOUT( inspectorOverlay->isVisible(), 1000 );
        const QPointF overlayPoint = inspectorOverlay->mapToScene( QPointF( 5.0, 5.0 ) );
        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, overlayPoint.toPoint() );
        QTest::qWait( 50 );
        failIfAnyWarnings( "inspector-dismiss-outside" );
        clearMessages();

        QTRY_VERIFY_WITH_TIMEOUT( !inspectorOverlay->isVisible(), 1000 );

        clickItemCenter( *window, disconnectButton );
        QTest::qWait( 50 );
        failIfAnyWarnings( "disconnect-click" );
        clearMessages();
    }
}

int main( int argc, char **argv )
{
    QGuiApplication app( argc, argv );
    tst_QmlUi tc;
    const int result = QTest::qExec( &tc, argc, argv );
    return result;
}

#include "tst_qml_ui.moc"
