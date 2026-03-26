#include <QtTest/QTest>

// QML smoke test policy:
// - Any Qt/QML warning fails the test.
// - This test automatically sweeps interactive controls authored under `qrc:/qml/*`:
//   - hover all interactive controls (covers ToolTip / delayed hover help)
//   - click most interactive controls
// - Interactive controls under `qrc:/qml/*` MUST have stable `objectName` values.
// - If a control is unsafe to auto-click in CI (engine reload, external links, process exit, etc.),
//   set `property bool testSkipActivate: true` on it; it will still be hovered.

#include <QCoreApplication>
#include <QColor>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFont>
#include <cmath>
#include <QGuiApplication>
#include <QObject>
#include <QPalette>
#include <QPoint>
#include <cmath>
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
#include <QWheelEvent>
#include <QString>
#include <QUrl>
#include <QtGlobal>

#include "modules/app/platform/qtshims.h"

#include <QQmlContext>
#include <QQmlEngine>

#include <QSet>

#include <algorithm>

#include "modules/app/defence/crashguard.h"
#include "modules/app/lifecycle/appsupervisor.h"
#include "modules/ui/colour/oklchutil.h"
#include "modules/ui/placement/windowplacement.h"
#include "modules/ui/theme/accentimageprovider.h"
#include "modules/ui/theme/themeiconimageprovider.h"

namespace {

QVector<QString> g_messages;
QtMessageHandler g_previousHandler;
QScopedPointer<QTemporaryDir> g_settingsDir;

QString formatMessage( const QtMsgType type, const QMessageLogContext &context, const QString &message )
{
    Q_UNUSED( type );

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

void wheelItemCenter( QQuickWindow &window, QQuickItem *item, const int angleDeltaY )
{
    const QPointF scenePoint = item->mapToScene( QPointF( item->width() / 2.0, item->height() / 2.0 ) );
    const QPoint pixelDelta( 0, 0 );
    const QPoint angleDelta( 0, angleDeltaY );
    QWheelEvent event(
        scenePoint,
        scenePoint,
        pixelDelta,
        angleDelta,
        Qt::NoButton,
        Qt::NoModifier,
        Qt::NoScrollPhase,
        false );

    QCoreApplication::sendEvent( item, &event );
    QCoreApplication::sendEvent( &window, &event );
}

void wheelAtItemCenterViaWindow( QQuickWindow &window, QQuickItem *item, const int angleDeltaY )
{
    const QPointF scenePoint = item->mapToScene( QPointF( item->width() / 2.0, item->height() / 2.0 ) );
    const QPoint pixelDelta( 0, 0 );
    const QPoint angleDelta( 0, angleDeltaY );
    QWheelEvent event(
        scenePoint,
        scenePoint,
        pixelDelta,
        angleDelta,
        Qt::NoButton,
        Qt::NoModifier,
        Qt::NoScrollPhase,
        false );

    QCoreApplication::sendEvent( &window, &event );
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

bool isFromProjectQml( QObject *obj )
{
    bool out;

    out = false;

    if ( obj != nullptr ) {
        const QQmlContext *ctx = QQmlEngine::contextForObject( obj );
        if ( ctx != nullptr ) {
            const QUrl baseUrl = ctx->baseUrl();
            if ( baseUrl.isValid() ) {
                out = baseUrl.toString().startsWith( QStringLiteral( "qrc:/qml/" ) );
            }
        }
    }

    return out;
}

bool hasSignal( const QMetaObject *metaObject, const char *signature )
{
    bool out;

    out = false;

    if ( metaObject != nullptr ) {
        out = ( metaObject->indexOfSignal( signature ) >= 0 );
    }

    return out;
}

bool isInteractive( QObject *obj )
{
    bool out;
    const QMetaObject *m;

    out = false;
    m = nullptr;

    if ( obj != nullptr ) {
        m = obj->metaObject();
        out =
            hasSignal( m, "clicked()" )
            || hasSignal( m, "toggled(bool)" )
            || hasSignal( m, "activated(int)" )
            || hasSignal( m, "editingFinished()" );
    }

    return out;
}

QRectF sceneRect( QQuickItem *item )
{
    if ( item == nullptr ) {
        return QRectF();
    }

    const QPointF tl = item->mapToScene( QPointF( 0.0, 0.0 ) );
    const QPointF br = item->mapToScene( QPointF( item->width(), item->height() ) );
    return QRectF( tl, br ).normalized();
}

QRectF scenePaintedTextRect( QQuickItem *textItem )
{
    if ( textItem == nullptr ) {
        return QRectF();
    }

    const QString className = QString::fromLatin1( textItem->metaObject()->className() );
    if ( !className.startsWith( QStringLiteral( "QQuickText" ), Qt::CaseSensitive ) ) {
        return sceneRect( textItem );
    }

    const qreal paintedW = textItem->property( "paintedWidth" ).toReal();
    const qreal paintedH = textItem->property( "paintedHeight" ).toReal();
    if ( paintedW < 2.0 || paintedH < 2.0 ) {
        return QRectF();
    }

    const qreal leftPadding = textItem->property( "leftPadding" ).toReal();
    const qreal rightPadding = textItem->property( "rightPadding" ).toReal();
    const qreal topPadding = textItem->property( "topPadding" ).toReal();
    const qreal bottomPadding = textItem->property( "bottomPadding" ).toReal();

    const qreal availableW = std::max( 0.0, textItem->width() - leftPadding - rightPadding );
    const qreal availableH = std::max( 0.0, textItem->height() - topPadding - bottomPadding );

    const int hAlign = textItem->property( "horizontalAlignment" ).toInt();
    const int vAlign = textItem->property( "verticalAlignment" ).toInt();

    qreal x = leftPadding;
    if ( hAlign == Qt::AlignHCenter ) {
        x = leftPadding + ( availableW - paintedW ) / 2.0;
    } else if ( hAlign == Qt::AlignRight ) {
        x = leftPadding + ( availableW - paintedW );
    }

    qreal y = topPadding;
    if ( vAlign == Qt::AlignVCenter ) {
        y = topPadding + ( availableH - paintedH ) / 2.0;
    } else if ( vAlign == Qt::AlignBottom ) {
        y = topPadding + ( availableH - paintedH );
    }

    const QPointF tl = textItem->mapToScene( QPointF( x, y ) );
    const QPointF br = textItem->mapToScene( QPointF( x + paintedW, y + paintedH ) );
    return QRectF( tl, br ).normalized();
}

bool isEffectivelyVisible( QQuickItem *item )
{
    if ( item == nullptr ) {
        return false;
    }
    if ( !item->isVisible() ) {
        return false;
    }
    if ( item->opacity() < 0.2 ) {
        return false;
    }

    return true;
}

void collectItemsDepthFirst( QQuickItem *root, QVector<QQuickItem *> &out );
QVector<QQuickItem *> discoverInteractiveProjectItems( QQuickItem *rootItem );
QString stableKeyForItem( QQuickItem *item );
bool itemCentreIsInsideWindow( QQuickWindow &window, QQuickItem *item );

qreal relativeLuminance( const QColor &c )
{
    auto toLinear = []( const qreal u ) {
        if ( u <= 0.03928 ) {
            return u / 12.92;
        }
        return std::pow( ( u + 0.055 ) / 1.055, 2.4 );
    };

    const qreal r = toLinear( c.redF() );
    const qreal g = toLinear( c.greenF() );
    const qreal b = toLinear( c.blueF() );

    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

qreal contrastRatio( const QColor &a, const QColor &b )
{
    const qreal la = relativeLuminance( a );
    const qreal lb = relativeLuminance( b );
    const qreal l1 = std::max( la, lb );
    const qreal l2 = std::min( la, lb );
    return ( l1 + 0.05 ) / ( l2 + 0.05 );
}

QColor findBackgroundColourForItem( QQuickWindow &window, QQuickItem *item )
{
    Q_UNUSED( window );

    QQuickItem *cur;

    cur = ( item != nullptr ) ? item->parentItem() : nullptr;
    while ( cur != nullptr ) {
        const QVariant colourVar = cur->property( "color" );
        if ( colourVar.isValid() ) {
            const QColor c = colourVar.value<QColor>();
            if ( c.isValid() && c.alphaF() > 0.10 ) {
                return c;
            }
        }

        const QVariant surfaceVar = cur->property( "surfaceColour" );
        if ( surfaceVar.isValid() ) {
            const QColor c = surfaceVar.value<QColor>();
            if ( c.isValid() && c.alphaF() > 0.10 ) {
                return c;
            }
        }

        cur = cur->parentItem();
    }

    return QGuiApplication::palette().color( QPalette::Window );
}

void verifyReadableTextContrast( QQuickWindow &window, QQuickItem *rootItem, const char *step )
{
    QVector<QQuickItem *> all;
    collectItemsDepthFirst( rootItem, all );

    for ( QQuickItem *item : all ) {
        if ( item == nullptr ) {
            continue;
        }
        if ( !isFromProjectQml( item ) ) {
            continue;
        }

        const QString className = QString::fromLatin1( item->metaObject()->className() );
        if ( !className.startsWith( QStringLiteral( "QQuickText" ), Qt::CaseSensitive ) ) {
            continue;
        }
        if ( !isEffectivelyVisible( item ) ) {
            continue;
        }

        const QString text = item->property( "text" ).toString().trimmed();
        if ( text.isEmpty() ) {
            continue;
        }

        const QVariant colourVar = item->property( "color" );
        if ( !colourVar.isValid() ) {
            continue;
        }

        const QColor fg = colourVar.value<QColor>();
        if ( !fg.isValid() ) {
            continue;
        }
        if ( fg.alphaF() < 0.50 ) {
            continue;
        }

        const QColor bg = findBackgroundColourForItem( window, item );
        if ( !bg.isValid() ) {
            continue;
        }

        const qreal ratio = contrastRatio( fg, bg );
        if ( ratio < 3.0 ) {
            const QQmlContext *ctx = QQmlEngine::contextForObject( item );
            const QString baseUrl = ( ctx != nullptr ) ? ctx->baseUrl().toString() : QString();

            const QString message =
                QStringLiteral( "Low text contrast at %1: ratio=%2 text=%3 baseUrl=%4 objectName=%5" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( ratio, 0, 'f', 2 )
                    .arg( text )
                    .arg( baseUrl )
                    .arg( item->objectName() );

            QFAIL( qPrintable( message ) );
        }
    }
}

void verifyInkStrokeContracts( QQuickWindow &window, QQuickItem *rootItem, const char *step )
{
    QVector<QQuickItem *> all;
    QVector<QQuickItem *> inkStrokes;
    QVector<QQuickItem *> closeButtons;

    collectItemsDepthFirst( rootItem, all );

    for ( QQuickItem *item : all ) {
        if ( item == nullptr ) {
            continue;
        }
        if ( !isFromProjectQml( item ) ) {
            continue;
        }

        const QVariant inkVar = item->property( "vcInkStroke" );
        if ( inkVar.isValid() && inkVar.toBool() ) {
            inkStrokes.append( item );
        }

        const QString name = item->objectName();
        if ( !name.isEmpty() && name.contains( QStringLiteral( "CloseButton" ), Qt::CaseSensitive ) ) {
            closeButtons.append( item );
        }
    }

    for ( QQuickItem *stroke : inkStrokes ) {
        const QVariant colourVar = stroke->property( "color" );
        if ( !colourVar.isValid() ) {
            continue;
        }

        const QColor fg = colourVar.value<QColor>();
        if ( !fg.isValid() || fg.alphaF() < 0.80 ) {
            continue;
        }

        const QColor bg = findBackgroundColourForItem( window, stroke );
        const qreal ratio = contrastRatio( fg, bg );
        if ( ratio < 3.0 ) {
            const QQmlContext *ctx = QQmlEngine::contextForObject( stroke );
            const QString baseUrl = ( ctx != nullptr ) ? ctx->baseUrl().toString() : QString();

            const QString message =
                QStringLiteral( "Low ink-stroke contrast at %1: ratio=%2 baseUrl=%3 objectName=%4" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( ratio, 0, 'f', 2 )
                    .arg( baseUrl )
                    .arg( stroke->objectName() );
            QFAIL( qPrintable( message ) );
        }
    }

    for ( QQuickItem *button : closeButtons ) {
        bool found = false;

        QVector<QQuickItem *> sub;
        collectItemsDepthFirst( button, sub );
        for ( QQuickItem *child : sub ) {
            if ( child == nullptr ) {
                continue;
            }
            const QVariant inkVar = child->property( "vcInkStroke" );
            if ( inkVar.isValid() && inkVar.toBool() ) {
                found = true;
                break;
            }
        }

        if ( !found ) {
            const QQmlContext *ctx = QQmlEngine::contextForObject( button );
            const QString baseUrl = ( ctx != nullptr ) ? ctx->baseUrl().toString() : QString();
            const QString message =
                QStringLiteral( "Close button missing ink-stroke icon at %1: baseUrl=%2 objectName=%3" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( baseUrl )
                    .arg( button->objectName() );
            QFAIL( qPrintable( message ) );
        }
    }
}

void verifyInteractiveItemsContainedWithinParents( QQuickWindow &window, QQuickItem *rootItem, const char *step )
{
    const QVector<QQuickItem *> items = discoverInteractiveProjectItems( rootItem );

    for ( QQuickItem *item : items ) {
        const QString key = stableKeyForItem( item );
        if ( key.isEmpty() ) {
            continue;
        }

        const QVariant skip = item->property( "testSkipLayoutSanity" );
        if ( skip.isValid() && skip.toBool() ) {
            continue;
        }

        if ( !itemCentreIsInsideWindow( window, item ) ) {
            continue;
        }

        QQuickItem *parent = item->parentItem();
        if ( parent == nullptr ) {
            continue;
        }

        const QRectF childRect = sceneRect( item );
        const QRectF parentRect = sceneRect( parent );

        const QRectF parentInner = parentRect.adjusted( -2.0, -2.0, 2.0, 2.0 );
        if ( !parentInner.contains( childRect ) ) {
            const QString message =
                QStringLiteral( "Interactive item overflows parent at %1: item=%2 parent=%3" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( key )
                    .arg( parent->objectName() );
            QFAIL( qPrintable( message ) );
        }
    }
}

void verifyScrollBarsAreNotDegenerate( QQuickWindow &window, QQuickItem *rootItem, const char *step )
{
    Q_UNUSED( window );

    QVector<QQuickItem *> all;
    collectItemsDepthFirst( rootItem, all );

    for ( QQuickItem *item : all ) {
        if ( item == nullptr ) {
            continue;
        }
        if ( !isFromProjectQml( item ) ) {
            continue;
        }

        const QString baseUrl = QQmlEngine::contextForObject( item ) != nullptr
            ? QQmlEngine::contextForObject( item )->baseUrl().toString()
            : QString();
        if ( baseUrl != QStringLiteral( "qrc:/qml/VcScrollBar.qml" ) ) {
            continue;
        }

        const QString className = QString::fromLatin1( item->metaObject()->className() );
        const bool isQmlScrollBar = className.startsWith( QStringLiteral( "QQuickScrollBar" ), Qt::CaseSensitive );
        if ( !isQmlScrollBar ) {
            continue;
        }

        if ( !item->isVisible() ) {
            continue;
        }

        // If it is visible, it must look like a scroll bar, not a tiny dot.
        const QRectF r = sceneRect( item );
        if ( r.width() < 8.0 || r.height() < 60.0 ) {
            const QString message =
                QStringLiteral( "Degenerate scrollbar geometry at %1: w=%2 h=%3 objectName=%4" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( r.width(), 0, 'f', 1 )
                    .arg( r.height(), 0, 'f', 1 )
                    .arg( item->objectName() );
            QFAIL( qPrintable( message ) );
        }

        // It should be anchored near the right edge of its parent.
        const QRectF pr = ( item->parentItem() != nullptr ) ? sceneRect( item->parentItem() ) : QRectF();
        if ( pr.isValid() && r.right() < pr.right() - 6.0 ) {
            const QString message =
                QStringLiteral( "Scrollbar not near right edge at %1: x=%2 right=%3 parentRight=%4" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( r.left(), 0, 'f', 1 )
                    .arg( r.right(), 0, 'f', 1 )
                    .arg( pr.right(), 0, 'f', 1 );
            QFAIL( qPrintable( message ) );
        }
    }
}

static void verifyUtilityActionsOrderAndPlacement(
    QQuickWindow &window,
    QQuickItem *rootItem,
    const QString &schedulingObjectName,
    const QString &supportObjectName,
    const QString &preferencesObjectName,
    const char *step )
{
    QQuickItem *schedulingButton;
    QQuickItem *supportButton;
    QQuickItem *preferencesButton;

    schedulingButton = findItemByObjectNameRecursive( rootItem, schedulingObjectName );
    supportButton = findItemByObjectNameRecursive( rootItem, supportObjectName );
    preferencesButton = findItemByObjectNameRecursive( rootItem, preferencesObjectName );

    QVERIFY2( schedulingButton != nullptr, qPrintable( schedulingObjectName ) );
    QVERIFY2( supportButton != nullptr, qPrintable( supportObjectName ) );
    QVERIFY2( preferencesButton != nullptr, qPrintable( preferencesObjectName ) );

    const QRectF rs = sceneRect( schedulingButton );
    const QRectF rSup = sceneRect( supportButton );
    const QRectF rp = sceneRect( preferencesButton );
    QVERIFY2( rs.isValid(), "scheduling rect" );
    QVERIFY2( rSup.isValid(), "support rect" );
    QVERIFY2( rp.isValid(), "preferences rect" );

    // Left-to-right ordering: Scheduling, Support, Preferences.
    QVERIFY2( rs.center().x() < rSup.center().x(), step );
    QVERIFY2( rSup.center().x() < rp.center().x(), step );

    // Rough top-right placement.
    QVERIFY2( rp.right() > ( window.width() - 80 ), step );
    QVERIFY2( rp.top() < 90.0, step );

    // Keep on the same row.
    QVERIFY2( qAbs( rs.center().y() - rp.center().y() ) < 24.0, step );
    QVERIFY2( qAbs( rSup.center().y() - rp.center().y() ) < 24.0, step );
}

void verifyNoDirectDefaultControls( QQuickItem *rootItem, const char *step )
{
    QVector<QQuickItem *> all;
    collectItemsDepthFirst( rootItem, all );

    const QStringList forbiddenPrefixes = {
        QStringLiteral( "QQuickButton" ),
        QStringLiteral( "QQuickToolButton" ),
        QStringLiteral( "QQuickTextField" ),
        QStringLiteral( "QQuickComboBox" ),
        QStringLiteral( "QQuickCheckBox" ),
        QStringLiteral( "QQuickTabBar" ),
        QStringLiteral( "QQuickTabButton" ),
        QStringLiteral( "QQuickFrame" ),
        QStringLiteral( "QQuickPane" ),
        QStringLiteral( "QQuickItemDelegate" )
    };

    for ( QQuickItem *item : all ) {
        if ( item == nullptr ) {
            continue;
        }

        const QQmlContext *ctx = QQmlEngine::contextForObject( item );
        const QString baseUrl = ( ctx != nullptr ) ? ctx->baseUrl().toString() : QString();
        if ( !baseUrl.startsWith( QStringLiteral( "qrc:/qml/" ) ) ) {
            continue;
        }

        if ( baseUrl.startsWith( QStringLiteral( "qrc:/qml/Vc" ) ) ) {
            continue;
        }

        const QString className = QString::fromLatin1( item->metaObject()->className() );
        for ( const QString &prefix : forbiddenPrefixes ) {
            if ( className.startsWith( prefix, Qt::CaseSensitive ) ) {
                const QString parentName = ( item->parentItem() != nullptr ) ? item->parentItem()->objectName() : QString();
                const QString message =
                    QStringLiteral( "Direct default control detected (%1) at %2; baseUrl=%3 parentObjectName=%4 objectName=%5" )
                        .arg( prefix )
                        .arg( QString::fromLatin1( step ) )
                        .arg( baseUrl )
                        .arg( parentName )
                        .arg( item->objectName() );
                QFAIL( qPrintable( message ) );
            }
        }
    }
}

void verifyNoVisibleTextOverlaps( QQuickItem *rootItem, const char *step )
{
    QVector<QQuickItem *> all;
    QVector<QQuickItem *> texts;

    collectItemsDepthFirst( rootItem, all );

    for ( QQuickItem *item : all ) {
        if ( item == nullptr ) {
            continue;
        }
        if ( !isFromProjectQml( item ) ) {
            continue;
        }

        const QString className = QString::fromLatin1( item->metaObject()->className() );
        if ( !className.startsWith( QStringLiteral( "QQuickText" ), Qt::CaseSensitive ) ) {
            continue;
        }
        if ( !isEffectivelyVisible( item ) ) {
            continue;
        }

        const QString text = item->property( "text" ).toString();
        if ( text.trimmed().isEmpty() ) {
            continue;
        }
        if ( item->width() < 4.0 || item->height() < 4.0 ) {
            continue;
        }

        texts.append( item );
    }

    for ( int i = 0; i < texts.size(); ++i ) {
        QQuickItem *a = texts[i];
        const QRectF ra = scenePaintedTextRect( a );
        if ( ra.isEmpty() ) {
            continue;
        }
        for ( int j = i + 1; j < texts.size(); ++j ) {
            QQuickItem *b = texts[j];

            if ( a == b ) {
                continue;
            }
            if ( a->parentItem() == b || b->parentItem() == a ) {
                continue;
            }

            const QRectF rb = scenePaintedTextRect( b );
            if ( rb.isEmpty() ) {
                continue;
            }
            const QRectF inter = ra.intersected( rb );
            if ( inter.width() <= 2.0 || inter.height() <= 2.0 ) {
                continue;
            }

            const QQmlContext *ctxA = QQmlEngine::contextForObject( a );
            const QQmlContext *ctxB = QQmlEngine::contextForObject( b );
            const QString baseA = ( ctxA != nullptr ) ? ctxA->baseUrl().toString() : QString();
            const QString baseB = ( ctxB != nullptr ) ? ctxB->baseUrl().toString() : QString();

            const QString message =
                QStringLiteral( "Overlapping visible text items at %1: A(text=%2 baseUrl=%3) B(text=%4 baseUrl=%5)" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( a->property( "text" ).toString() )
                    .arg( baseA )
                    .arg( b->property( "text" ).toString() )
                    .arg( baseB );

            QFAIL( qPrintable( message ) );
        }
    }
}

namespace {

QQuickItem *nearestAncestorWithObjectNameSubstring( QQuickItem *item, const QString &needle )
{
    QQuickItem *cur;

    cur = item;
    while ( cur != nullptr ) {
        if ( cur->objectName().contains( needle, Qt::CaseSensitive ) ) {
            return cur;
        }
        cur = cur->parentItem();
    }

    return nullptr;
}

QQuickItem *nearestAncestorWithClassPrefix( QQuickItem *item, const QString &prefix )
{
    QQuickItem *cur;

    cur = item;
    while ( cur != nullptr ) {
        const QString className = QString::fromLatin1( cur->metaObject()->className() );
        if ( className.startsWith( prefix, Qt::CaseSensitive ) ) {
            return cur;
        }
        cur = cur->parentItem();
    }

    return nullptr;
}

QString qmlBaseUrlForItem( QQuickItem *item )
{
    const QQmlContext *ctx;

    ctx = ( item != nullptr ) ? QQmlEngine::contextForObject( item ) : nullptr;
    return ( ctx != nullptr ) ? ctx->baseUrl().toString() : QString();
}

bool shouldIgnoreTextOverlapParticipation( QQuickItem *textItem )
{
    const QString baseUrl = qmlBaseUrlForItem( textItem );

    // Help tips are overlays by design and can cover underlying UI.
    if ( baseUrl.contains( QStringLiteral( "/HelpTip.qml" ), Qt::CaseSensitive ) ) {
        return true;
    }

    return false;
}

QString textOverlapLayerKey( QQuickItem *textItem )
{
    if ( textItem == nullptr ) {
        return QStringLiteral( "main" );
    }

    if ( nearestAncestorWithClassPrefix( textItem, QStringLiteral( "QQuickPopupItem" ) ) != nullptr ) {
        QQuickItem *popupItem = nearestAncestorWithClassPrefix( textItem, QStringLiteral( "QQuickPopupItem" ) );
        return QStringLiteral( "popup:%1" ).arg( QString::number( reinterpret_cast<quintptr>( popupItem ) ) );
    }

    // Treat modal overlays as separate layers so they don't fail on intentional coverage.
    QQuickItem *overlay = nearestAncestorWithObjectNameSubstring( textItem, QStringLiteral( "Overlay" ) );
    if ( overlay != nullptr ) {
        const QString name = overlay->objectName();
        if ( !name.isEmpty() ) {
            return QStringLiteral( "overlay:%1" ).arg( name );
        }
    }

    return QStringLiteral( "main" );
}

}

void verifyNoVisibleTextOverlapsIgnoringOverlays( QQuickItem *rootItem, const char *step )
{
    QVector<QQuickItem *> all;
    QVector<QQuickItem *> texts;

    collectItemsDepthFirst( rootItem, all );

    for ( QQuickItem *item : all ) {
        if ( item == nullptr ) {
            continue;
        }
        if ( !isFromProjectQml( item ) ) {
            continue;
        }

        const QString className = QString::fromLatin1( item->metaObject()->className() );
        if ( !className.startsWith( QStringLiteral( "QQuickText" ), Qt::CaseSensitive ) ) {
            continue;
        }
        if ( !isEffectivelyVisible( item ) ) {
            continue;
        }
        if ( shouldIgnoreTextOverlapParticipation( item ) ) {
            continue;
        }

        const QString text = item->property( "text" ).toString();
        if ( text.trimmed().isEmpty() ) {
            continue;
        }
        if ( item->width() < 4.0 || item->height() < 4.0 ) {
            continue;
        }

        texts.append( item );
    }

    for ( int i = 0; i < texts.size(); ++i ) {
        QQuickItem *a = texts[i];
        const QString layerA = textOverlapLayerKey( a );
        const QRectF ra = scenePaintedTextRect( a );
        if ( ra.isEmpty() ) {
            continue;
        }

        for ( int j = i + 1; j < texts.size(); ++j ) {
            QQuickItem *b = texts[j];

            if ( a == b ) {
                continue;
            }
            if ( a->parentItem() == b || b->parentItem() == a ) {
                continue;
            }

            const QString layerB = textOverlapLayerKey( b );
            if ( layerA != layerB ) {
                continue;
            }

            const QRectF rb = scenePaintedTextRect( b );
            if ( rb.isEmpty() ) {
                continue;
            }
            const QRectF inter = ra.intersected( rb );
            if ( inter.width() <= 2.0 || inter.height() <= 2.0 ) {
                continue;
            }

            const QString baseA = qmlBaseUrlForItem( a );
            const QString baseB = qmlBaseUrlForItem( b );

            const QString message =
                QStringLiteral( "Overlapping visible text items at %1 layer=%2: A(text=%3 baseUrl=%4) B(text=%5 baseUrl=%6)" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( layerA )
                    .arg( a->property( "text" ).toString() )
                    .arg( baseA )
                    .arg( b->property( "text" ).toString() )
                    .arg( baseB );

            QFAIL( qPrintable( message ) );
        }
    }
}

void verifyNoInteractiveOverlaps( QQuickWindow &window, QQuickItem *rootItem, const char *step )
{
    const QVector<QQuickItem *> items = discoverInteractiveProjectItems( rootItem );

    auto nearestAncestorWithObjectNameSubstring = []( QQuickItem *item, const QString &needle ) {
        QQuickItem *cur;

        cur = item;
        while ( cur != nullptr ) {
            if ( cur->objectName().contains( needle, Qt::CaseSensitive ) ) {
                return cur;
            }
            cur = cur->parentItem();
        }

        return static_cast<QQuickItem *>( nullptr );
    };

    auto nearestAncestorWithClassPrefix = []( QQuickItem *item, const QString &prefix ) {
        QQuickItem *cur;

        cur = item;
        while ( cur != nullptr ) {
            const QString className = QString::fromLatin1( cur->metaObject()->className() );
            if ( className.startsWith( prefix, Qt::CaseSensitive ) ) {
                return cur;
            }
            cur = cur->parentItem();
        }

        return static_cast<QQuickItem *>( nullptr );
    };

    auto interactiveOverlapLayerKey = [&nearestAncestorWithObjectNameSubstring, &nearestAncestorWithClassPrefix]( QQuickItem *item ) {
        if ( item == nullptr ) {
            return QStringLiteral( "main" );
        }

        QQuickItem *popupItem = nearestAncestorWithClassPrefix( item, QStringLiteral( "QQuickPopupItem" ) );
        if ( popupItem != nullptr ) {
            return QStringLiteral( "popup:%1" ).arg( QString::number( reinterpret_cast<quintptr>( popupItem ) ) );
        }

        QQuickItem *overlay = nearestAncestorWithObjectNameSubstring( item, QStringLiteral( "Overlay" ) );
        if ( overlay != nullptr ) {
            const QString name = overlay->objectName();
            if ( !name.isEmpty() ) {
                return QStringLiteral( "overlay:%1" ).arg( name );
            }
        }

        return QStringLiteral( "main" );
    };

    for ( int i = 0; i < items.size(); ++i ) {
        QQuickItem *a = items[i];
        const QRectF ra = sceneRect( a );
        const QString layerA = interactiveOverlapLayerKey( a );
        for ( int j = i + 1; j < items.size(); ++j ) {
            QQuickItem *b = items[j];

            if ( a == b ) {
                continue;
            }

            const QString layerB = interactiveOverlapLayerKey( b );
            if ( layerA != layerB ) {
                continue;
            }

            const QRectF rb = sceneRect( b );
            const QRectF inter = ra.intersected( rb );
            if ( inter.width() <= 1.0 || inter.height() <= 1.0 ) {
                continue;
            }

            const QString keyA = stableKeyForItem( a );
            const QString keyB = stableKeyForItem( b );

            const QString message =
                QStringLiteral( "Overlapping interactive items at %1 layer=%2: A=%3 B=%4 window=%5x%6" )
                    .arg( QString::fromLatin1( step ) )
                    .arg( layerA )
                    .arg( keyA )
                    .arg( keyB )
                    .arg( window.width() )
                    .arg( window.height() );
            QFAIL( qPrintable( message ) );
        }
    }
}

bool shouldAttemptActivate( QObject *obj )
{
    bool out;

    out = true;

    if ( obj != nullptr ) {
        const QVariant skip = obj->property( "testSkipActivate" );
        if ( skip.isValid() ) {
            out = !skip.toBool();
        }
    }

    return out;
}

void collectItemsDepthFirst( QQuickItem *root, QVector<QQuickItem *> &out )
{
    if ( root == nullptr ) {
        return;
    }

    out.append( root );

    const QList<QQuickItem *> children = root->childItems();
    for ( QQuickItem *child : children ) {
        collectItemsDepthFirst( child, out );
    }
}

QVector<QQuickItem *> discoverInteractiveProjectItems( QQuickItem *rootItem )
{
    QVector<QQuickItem *> all;
    QVector<QQuickItem *> out;

    collectItemsDepthFirst( rootItem, all );

    for ( QQuickItem *item : all ) {
        if ( item == nullptr ) {
            continue;
        }
        if ( !isFromProjectQml( item ) ) {
            continue;
        }
        if ( !isInteractive( item ) ) {
            continue;
        }
        if ( item->width() < 6.0 || item->height() < 6.0 ) {
            continue;
        }
        if ( !item->isVisible() ) {
            continue;
        }
        if ( !item->isEnabled() ) {
            continue;
        }

        out.append( item );
    }

    return out;
}

QString stableKeyForItem( QQuickItem *item )
{
    QString out;

    out = QString();

    if ( item != nullptr ) {
        out = item->objectName();
    }

    return out;
}

bool itemCentreIsInsideWindow( QQuickWindow &window, QQuickItem *item )
{
    bool out;

    out = false;

    if ( item != nullptr ) {
        const QPointF scenePoint = item->mapToScene( QPointF( item->width() / 2.0, item->height() / 2.0 ) );
        out =
            ( scenePoint.x() >= 1.0 )
            && ( scenePoint.y() >= 1.0 )
            && ( scenePoint.x() <= window.width() - 2.0 )
            && ( scenePoint.y() <= window.height() - 2.0 );
    }

    return out;
}

bool isDeprioritisedClickTarget( const QString &key )
{
    if ( key == QStringLiteral( "disconnectButton" ) ) {
        return true;
    }
    if ( key.contains( QStringLiteral( "CloseButton" ), Qt::CaseSensitive ) ) {
        return true;
    }
    if ( key.contains( QStringLiteral( "closeButton" ), Qt::CaseSensitive ) ) {
        return true;
    }

    return false;
}

void runAutomaticInteractionSweep( QQuickWindow &window, QQuickItem *rootItem )
{
    QSet<QString> hovered;
    QSet<QString> clicked;

    const int clickBudget = 200;
    int clickCount;

    clickCount = 0;

    for ( int iteration = 0; iteration < clickBudget; ++iteration ) {
        const QVector<QQuickItem *> items = discoverInteractiveProjectItems( rootItem );
        QVector<QQuickItem *> sorted;

        sorted = items;
        std::sort( sorted.begin(), sorted.end(), []( QQuickItem *a, QQuickItem *b ) {
            return stableKeyForItem( a ) < stableKeyForItem( b );
        } );

        for ( QQuickItem *item : sorted ) {
            const QString key = stableKeyForItem( item );

            if ( key.isEmpty() ) {
                const QQmlContext *ctx = QQmlEngine::contextForObject( item );
                const QString baseUrl = ( ctx != nullptr ) ? ctx->baseUrl().toString() : QString();
                const QString className = QString::fromLatin1( item->metaObject()->className() );
                const QString text = item->property( "text" ).toString();
                const QString parentName = ( item->parentItem() != nullptr ) ? item->parentItem()->objectName() : QString();

                const QString message =
                    QStringLiteral( "Interactive project QML item is missing objectName: class=%1 baseUrl=%2 parentObjectName=%3 text=%4" )
                        .arg( className )
                        .arg( baseUrl )
                        .arg( parentName )
                        .arg( text );

                QFAIL( qPrintable( message ) );
            }

            if ( !hovered.contains( key ) ) {
                const QPointF posBeforeHover = sceneTopLeft( item );

                if ( itemCentreIsInsideWindow( window, item ) ) {
                    moveMouseToItemCenter( window, item );
                    QTest::qWait( 350 );
                    failIfAnyWarnings( "auto-hover" );
                    verifyScenePositionUnchanged( item, posBeforeHover, 1.0, "auto-hover" );
                    clearMessages();
                }

                hovered.insert( key );
            }
        }

        {
            QQuickItem *toClick;
            QString clickKey;

            toClick = nullptr;

            for ( QQuickItem *item : sorted ) {
                const QString key = stableKeyForItem( item );
                if ( clicked.contains( key ) ) {
                    continue;
                }
                if ( !shouldAttemptActivate( item ) ) {
                    clicked.insert( key );
                    continue;
                }
                if ( isDeprioritisedClickTarget( key ) ) {
                    continue;
                }
                if ( !itemCentreIsInsideWindow( window, item ) ) {
                    continue;
                }

                toClick = item;
                clickKey = key;
                break;
            }

            if ( toClick == nullptr ) {
                for ( QQuickItem *item : sorted ) {
                    const QString key = stableKeyForItem( item );
                    if ( clicked.contains( key ) ) {
                        continue;
                    }
                    if ( !shouldAttemptActivate( item ) ) {
                        clicked.insert( key );
                        continue;
                    }
                    if ( !itemCentreIsInsideWindow( window, item ) ) {
                        continue;
                    }

                    toClick = item;
                    clickKey = key;
                    break;
                }
            }

            if ( toClick == nullptr ) {
                break;
            }

            clickItemCenter( window, toClick );
            QTest::qWait( 60 );
            failIfAnyWarnings( "auto-click" );
            clearMessages();

            clicked.insert( clickKey );
            clickCount += 1;
        }
    }

    QVERIFY( clickCount >= 1 );
}

}

class tst_QmlUi : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void navigation_joinAndHost_emitNoWarnings();
    void preferences_categorySwitchingShowsExpectedPane();
    void themePreferences_propagatePaletteRolesToPages();
    void style_projectDoesNotUseDefaultControlsDirectly();
    void style_projectDoesNotUseUnsupportedCanvasApis();
    void style_displayFontIsNotUsedAtSmallSizes();
    void layout_noOverlapsInKeyScreens();
    void appearancePreferences_extremes_noLayoutGlitches();
    void text_noOverlapsInKeyFlows_ignoringOverlays();
    void layout_textContrastIsReadableInKeyScreens();
    void preferences_scrollbarAndWheelWork();
    void preferences_wheelOverComboBox_doesNotChangeSelection();
    void preferences_clickInsidePanel_doesNotClose();
    void landing_popups_openAndCloseWithoutWarnings();
    void app_utilityActions_areConsistentAcrossPages();
    void themePresetAccents_mapToExplicitHighlightColours();
    void themePresetAccents_shiftInDarkMode();
    void landing_primaryButtons_haveReadableTextInVictorianAndLight();
    void preferences_accentChange_repaintsComboIndicator();
};

void tst_QmlUi::preferences_accentChange_repaintsComboIndicator()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "victorian" ) );
    }

    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupOpenPreferences" ), true );
    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupPreferencesCategoryIndex" ), 0 );

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 60 );
    failIfAnyWarnings( "accent-indicator-post-load" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    QQuickItem *overlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesOverlay" ) );
    QVERIFY2( overlay != nullptr, "preferencesOverlay" );
    QTRY_VERIFY_WITH_TIMEOUT( overlay->isVisible(), 1000 );

    QQuickItem *combo = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesAccentCombo" ) );
    QVERIFY2( combo != nullptr, "preferencesAccentCombo" );

    // The preferences page is scrollable; ensure the combo is on-screen before we expect repaints.
    {
        QQuickItem *scrollView = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesGeneralScrollView" ) );
        QVERIFY2( scrollView != nullptr, "preferencesGeneralScrollView" );

        QObject *flickableObj = scrollView->property( "contentItem" ).value<QObject *>();
        QQuickItem *flickable = qobject_cast<QQuickItem *>( flickableObj );
        QVERIFY2( flickable != nullptr, "preferencesGeneralScrollView flickable" );

        const qreal contentH = flickable->property( "contentHeight" ).toReal();
        const qreal viewH = flickable->height();
        const qreal maxY = std::max( 0.0, contentH - viewH );

        const QPointF comboCentreInFlickable = combo->mapToItem( flickable, QPointF( combo->width() / 2.0, combo->height() / 2.0 ) );
        qreal desired = comboCentreInFlickable.y() - ( viewH / 2.0 );
        if ( desired < 0.0 ) {
            desired = 0.0;
        }
        if ( desired > maxY ) {
            desired = maxY;
        }

        flickable->setProperty( "contentY", desired );
        QCoreApplication::processEvents();
        QTest::qWait( 80 );
    }

    QVERIFY2( itemCentreIsInsideWindow( *window, combo ), "preferencesAccentCombo should be on-screen" );

    QQuickItem *indicator = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesAccentComboIndicator" ) );
    QVERIFY2( indicator != nullptr, "preferencesAccentComboIndicator" );

    QTRY_VERIFY_WITH_TIMEOUT( indicator->property( "paintSerial" ).toInt() > 0, 1000 );
    const int before = indicator->property( "paintSerial" ).toInt();

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "blue" ) );
    }

    QTRY_VERIFY_WITH_TIMEOUT( indicator->property( "paintSerial" ).toInt() > before, 1000 );
    failIfAnyWarnings( "accent-indicator-repaint" );
    clearMessages();
}

void tst_QmlUi::initTestCase()
{
    qputenv( "VCSTREAM_DISABLE_MEDIA_ENUM", "1" );

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
    OklchUtil oklchUtil;
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

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
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

    runAutomaticInteractionSweep( *window, rootItem );
}

void tst_QmlUi::preferences_categorySwitchingShowsExpectedPane()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );

    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 50 );
    failIfAnyWarnings( "preferences-open-post-load" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QQuickItem *preferencesButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingPreferencesButton" ) );
    QVERIFY2( preferencesButton != nullptr, "landingPreferencesButton" );

    clickItemCenter( *window, preferencesButton );
    QTest::qWait( 50 );

    QQuickItem *generalPane = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesGeneralPane" ) );
    QQuickItem *devicesCategory = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesCategoryDevices" ) );
    QVERIFY2( generalPane != nullptr, "preferencesGeneralPane" );
    QVERIFY2( devicesCategory != nullptr, "preferencesCategoryDevices" );

    QVERIFY( generalPane->property( "visible" ).toBool() );
    failIfAnyWarnings( "preferences-general-pane-visible" );
    clearMessages();

    clickItemCenter( *window, devicesCategory );
    QTest::qWait( 50 );

    QQuickItem *devicesPane = nullptr;
    QQuickItem *deviceRefreshButton = nullptr;
    QTRY_VERIFY_WITH_TIMEOUT( ( devicesPane = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesDevicesPane" ) ) ) != nullptr, 1000 );
    QTRY_VERIFY_WITH_TIMEOUT( ( deviceRefreshButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesDeviceRefreshButton" ) ) ) != nullptr, 1000 );

    QVERIFY( devicesPane->property( "visible" ).toBool() );
    QVERIFY( deviceRefreshButton->property( "visible" ).toBool() );
    failIfAnyWarnings( "preferences-devices-pane-visible" );
    clearMessages();
}

void tst_QmlUi::themePreferences_propagatePaletteRolesToPages()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );

    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 50 );
    failIfAnyWarnings( "theme-post-load" );
    clearMessages();

    QObject *landingPage = rootObject->findChild<QObject *>( QStringLiteral( "landingPage" ) );
    QObject *joinRoomButton = rootObject->findChild<QObject *>( QStringLiteral( "joinRoomButton" ) );
    QVERIFY2( landingPage != nullptr, "landingPage" );
    QVERIFY2( joinRoomButton != nullptr, "joinRoomButton" );

    const QColor initialHighlight = landingPage->property( "resolvedHighlightColour" ).value<QColor>();
    const QColor initialButton = landingPage->property( "resolvedButtonColour" ).value<QColor>();

    supervisor.preferences()->setProperty( "accent", QStringLiteral( "green" ) );
    supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );

    QTRY_VERIFY_WITH_TIMEOUT( landingPage->property( "resolvedHighlightColour" ).value<QColor>() != initialHighlight, 1000 );
    QTRY_VERIFY_WITH_TIMEOUT( landingPage->property( "resolvedButtonColour" ).value<QColor>() != initialButton, 1000 );

    const QColor landingHighlight = landingPage->property( "resolvedHighlightColour" ).value<QColor>();
    const QColor landingButton = landingPage->property( "resolvedButtonColour" ).value<QColor>();

    clickItemCenter( *window, qobject_cast<QQuickItem *>( joinRoomButton ) );

    QObject *disconnectButton = rootObject->findChild<QObject *>( QStringLiteral( "disconnectButton" ) );
    QObject *shellPage = rootObject->findChild<QObject *>( QStringLiteral( "shellPage" ) );
    QVERIFY2( disconnectButton != nullptr, "disconnectButton" );
    QVERIFY2( shellPage != nullptr, "shellPage" );

    const QColor shellHighlight = shellPage->property( "resolvedHighlightColour" ).value<QColor>();
    const QColor shellButton = shellPage->property( "resolvedButtonColour" ).value<QColor>();

    QCOMPARE( shellHighlight, landingHighlight );
    QCOMPARE( shellButton, landingButton );
    failIfAnyWarnings( "theme-shell-palette" );
    clearMessages();
}

void tst_QmlUi::style_projectDoesNotUseDefaultControlsDirectly()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 50 );
    failIfAnyWarnings( "style-post-load" );
    clearMessages();

    verifyNoDirectDefaultControls( window->contentItem(), "style-default-controls" );
}

void tst_QmlUi::style_projectDoesNotUseUnsupportedCanvasApis()
{
    // Scan the embedded QML resources to avoid depending on a checkout-relative path.
    const QStringList patterns = QStringList{ QStringLiteral( "*.qml" ) };
    QDirIterator it( QStringLiteral( ":/qml" ), patterns, QDir::Files, QDirIterator::Subdirectories );

    QVERIFY2( it.hasNext(), "Expected QML resources under :/qml" );

    while ( it.hasNext() ) {
        const QString path = it.next();
        QFile f( path );
        QVERIFY2( f.open( QIODevice::ReadOnly ), qPrintable( path ) );

        const QString text = QString::fromUtf8( f.readAll() );

        // Canvas2D context API support varies across Qt versions.
        // `reset()` is a common footgun that can crash/throw at runtime.
        QVERIFY2( !text.contains( QStringLiteral( "context.reset(" ) ), qPrintable( QStringLiteral( "Unsupported Canvas API used in %1" ).arg( path ) ) );
    }
}

void tst_QmlUi::style_displayFontIsNotUsedAtSmallSizes()
{
    // Libre Caslon Display is a display cut. It is intended for large sizes.
    // Guard against using the display font family alongside small UI heading sizes.
    const QStringList patterns = QStringList{ QStringLiteral( "*.qml" ) };
    QDirIterator it( QStringLiteral( ":/qml" ), patterns, QDir::Files, QDirIterator::Subdirectories );

    QVERIFY2( it.hasNext(), "Expected QML resources under :/qml" );

    while ( it.hasNext() ) {
        const QString path = it.next();
        QFile f( path );
        QVERIFY2( f.open( QIODevice::ReadOnly ), qPrintable( path ) );

        const QString text = QString::fromUtf8( f.readAll() );

        const QStringList lines = text.split( QLatin1Char( '\n' ) );

        bool inLabel;
        qsizetype braceDepth;
        bool sawHeadingFamily;
        bool sawSmallFont;

        inLabel = false;
        braceDepth = 0;
        sawHeadingFamily = false;
        sawSmallFont = false;

        for ( int i = 0; i < lines.size(); ++i ) {
            const QString line = lines[i];

            if ( !inLabel && line.contains( QStringLiteral( "Label" ) ) && line.contains( QLatin1Char( '{' ) ) ) {
                inLabel = true;
                braceDepth = 0;
                sawHeadingFamily = false;
                sawSmallFont = false;
            }

            if ( inLabel ) {
                braceDepth += line.count( QLatin1Char( '{' ) );
                braceDepth -= line.count( QLatin1Char( '}' ) );

                if ( line.contains( QStringLiteral( "headingFontFamily" ) ) ) {
                    sawHeadingFamily = true;
                }

                // Consider these "small" for a display face.
                if ( line.contains( QStringLiteral( "fontHeadingPx" ) )
                    || line.contains( QStringLiteral( "fontBasePx" ) )
                    || line.contains( QStringLiteral( "fontSmallPx" ) ) ) {
                    sawSmallFont = true;
                }

                if ( braceDepth <= 0 ) {
                    if ( sawHeadingFamily && sawSmallFont ) {
                        const QString message =
                            QStringLiteral( "Display font used with small size in %1" ).arg( path );
                        QFAIL( qPrintable( message ) );
                    }

                    inLabel = false;
                }
            }
        }
    }
}

void tst_QmlUi::layout_noOverlapsInKeyScreens()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "custom" ) );
        supervisor.preferences()->setProperty( "customAccentHueDegrees", 170.0 );
        supervisor.preferences()->setProperty( "customAccentChroma", 0.20 );
        supervisor.preferences()->setProperty( "customAccentLightness", 0.62 );
    }

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 80 );

    // Bundled Victorian fonts should register and become the app font when the
    // default font preset is `victorian`.
    {
        const QString victorianBody = supervisor.victorianBodyFontFamily();
        QVERIFY2( !victorianBody.isEmpty(), "Victorian bundled body font should be available" );

        const QFont windowFont = rootObject->property( "font" ).value<QFont>();
        QCOMPARE( windowFont.family(), victorianBody );
    }

    failIfAnyWarnings( "layout-post-load" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    verifyNoVisibleTextOverlaps( rootItem, "landing" );
    verifyNoInteractiveOverlaps( *window, rootItem, "landing" );
    verifyInteractiveItemsContainedWithinParents( *window, rootItem, "landing" );

    QQuickItem *preferencesButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingPreferencesButton" ) );
    QVERIFY2( preferencesButton != nullptr, "landingPreferencesButton" );
    clickItemCenter( *window, preferencesButton );
    QTest::qWait( 80 );
    failIfAnyWarnings( "layout-open-preferences" );
    clearMessages();

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "preferences" );
    verifyNoInteractiveOverlaps( *window, rootItem, "preferences" );
    verifyInteractiveItemsContainedWithinParents( *window, rootItem, "preferences" );
    verifyScrollBarsAreNotDegenerate( *window, rootItem, "preferences" );

    QQuickItem *closeButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesCloseButton" ) );
    QVERIFY2( closeButton != nullptr, "preferencesCloseButton" );
    clickItemCenter( *window, closeButton );
    QTest::qWait( 80 );

    QQuickItem *joinRoomButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "joinRoomButton" ) );
    QVERIFY2( joinRoomButton != nullptr, "joinRoomButton" );
    clickItemCenter( *window, joinRoomButton );
    QTest::qWait( 120 );
    failIfAnyWarnings( "layout-go-shell" );
    clearMessages();

    verifyNoVisibleTextOverlaps( rootItem, "shell" );
    verifyNoInteractiveOverlaps( *window, rootItem, "shell" );
    verifyInteractiveItemsContainedWithinParents( *window, rootItem, "shell" );
}

void tst_QmlUi::appearancePreferences_extremes_noLayoutGlitches()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        const QStringList families = QFontDatabase().families();
        const QString family = families.isEmpty() ? QString() : families.first();

        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "green" ) );
        supervisor.preferences()->setProperty( "fontPreset", QStringLiteral( "custom" ) );
        supervisor.preferences()->setProperty( "fontFamily", family );
        supervisor.preferences()->setProperty( "fontScalePercent", 150 );
        supervisor.preferences()->setProperty( "density", QStringLiteral( "spacious" ) );
        supervisor.preferences()->setProperty( "zoomPercent", 200 );
    }

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );

    window->resize( 1600, 1000 );
    QCoreApplication::processEvents();

    QTest::qWait( 120 );
    failIfAnyWarnings( "appearance-post-load" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    verifyNoVisibleTextOverlaps( rootItem, "appearance-landing" );
    verifyNoInteractiveOverlaps( *window, rootItem, "appearance-landing" );
    verifyInteractiveItemsContainedWithinParents( *window, rootItem, "appearance-landing" );

    {
        QQuickItem *schedulingButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingSchedulingButton" ) );
        QQuickItem *schedulingOverlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "schedulingOverlay" ) );
        QVERIFY2( schedulingButton != nullptr, "landingSchedulingButton" );
        QVERIFY2( schedulingOverlay != nullptr, "schedulingOverlay" );

        clickItemCenter( *window, schedulingButton );
        QTRY_VERIFY_WITH_TIMEOUT( schedulingOverlay->isVisible(), 1000 );
        failIfAnyWarnings( "appearance-open-scheduling" );
        clearMessages();

        verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "appearance-scheduling" );
        verifyNoInteractiveOverlaps( *window, rootItem, "appearance-scheduling" );
        verifyInteractiveItemsContainedWithinParents( *window, rootItem, "appearance-scheduling" );

        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, QPoint( 2, 2 ) );
        QTRY_VERIFY_WITH_TIMEOUT( !schedulingOverlay->isVisible(), 1000 );
    }

    {
        QQuickItem *supportButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingSupportButton" ) );
        QQuickItem *supportOverlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "supportOverlay" ) );
        QVERIFY2( supportButton != nullptr, "landingSupportButton" );
        QVERIFY2( supportOverlay != nullptr, "supportOverlay" );

        clickItemCenter( *window, supportButton );
        QTRY_VERIFY_WITH_TIMEOUT( supportOverlay->isVisible(), 1000 );
        failIfAnyWarnings( "appearance-open-support" );
        clearMessages();

        verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "appearance-support" );
        verifyNoInteractiveOverlaps( *window, rootItem, "appearance-support" );
        verifyInteractiveItemsContainedWithinParents( *window, rootItem, "appearance-support" );
        verifyScrollBarsAreNotDegenerate( *window, rootItem, "appearance-support" );

        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, QPoint( 2, 2 ) );
        QTRY_VERIFY_WITH_TIMEOUT( !supportOverlay->isVisible(), 1000 );
    }

    QQuickItem *preferencesButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingPreferencesButton" ) );
    QVERIFY2( preferencesButton != nullptr, "landingPreferencesButton" );
    clickItemCenter( *window, preferencesButton );
    QTest::qWait( 120 );
    failIfAnyWarnings( "appearance-open-preferences" );
    clearMessages();

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "appearance-preferences" );
    verifyNoInteractiveOverlaps( *window, rootItem, "appearance-preferences" );
    verifyInteractiveItemsContainedWithinParents( *window, rootItem, "appearance-preferences" );
    verifyScrollBarsAreNotDegenerate( *window, rootItem, "appearance-preferences" );

    QQuickItem *closeButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesCloseButton" ) );
    QVERIFY2( closeButton != nullptr, "preferencesCloseButton" );
    clickItemCenter( *window, closeButton );
    QTest::qWait( 120 );

    QQuickItem *joinRoomButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "joinRoomButton" ) );
    QVERIFY2( joinRoomButton != nullptr, "joinRoomButton" );
    clickItemCenter( *window, joinRoomButton );
    QTest::qWait( 200 );
    failIfAnyWarnings( "appearance-go-shell" );
    clearMessages();

    verifyNoVisibleTextOverlaps( rootItem, "appearance-shell" );
    verifyNoInteractiveOverlaps( *window, rootItem, "appearance-shell" );
    verifyInteractiveItemsContainedWithinParents( *window, rootItem, "appearance-shell" );
}

void tst_QmlUi::text_noOverlapsInKeyFlows_ignoringOverlays()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "custom" ) );
        supervisor.preferences()->setProperty( "fontPreset", QStringLiteral( "custom" ) );
        supervisor.preferences()->setProperty( "customAccentHueDegrees", 160.0 );
        supervisor.preferences()->setProperty( "customAccentChroma", 0.18 );
        supervisor.preferences()->setProperty( "customAccentLightness", 0.66 );
        supervisor.preferences()->setProperty( "density", QStringLiteral( "comfortable" ) );
        supervisor.preferences()->setProperty( "zoomPercent", 100 );
    }

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 80 );
    failIfAnyWarnings( "textflow-post-load" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-landing" );

    QQuickItem *preferencesButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingPreferencesButton" ) );
    QVERIFY2( preferencesButton != nullptr, "landingPreferencesButton" );
    clickItemCenter( *window, preferencesButton );
    QTest::qWait( 120 );
    failIfAnyWarnings( "textflow-open-preferences" );
    clearMessages();

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-preferences" );

    // Open the font picker popup and verify overlaps inside the popup layer.
    {
        QQuickItem *pickerButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesFontFamilyPickerButton" ) );
        QVERIFY2( pickerButton != nullptr, "preferencesFontFamilyPickerButton" );
        clickItemCenter( *window, pickerButton );
        QTest::qWait( 160 );

        QQuickItem *searchField = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesFontFamilyPickerSearchField" ) );
        QVERIFY2( searchField != nullptr, "preferencesFontFamilyPickerSearchField" );

        // Guard against first-open rendering glitches.
        {
            QQuickItem *scrollBar = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesFontFamilyPickerScrollBar" ) );
            QVERIFY2( scrollBar != nullptr, "preferencesFontFamilyPickerScrollBar" );

            if ( scrollBar->isVisible() ) {
                const QRectF r = sceneRect( scrollBar );
                QVERIFY2( r.width() >= 8.0 && r.height() >= 60.0, "font picker scrollbar geometry" );

                const double minimumSize = scrollBar->property( "minimumSize" ).toDouble();
                const double effectiveSize = scrollBar->property( "vcEffectiveSize" ).toDouble();
                QVERIFY2( std::isfinite( minimumSize ) && minimumSize > 0.0, "scrollbar minimumSize" );
                QVERIFY2( std::isfinite( effectiveSize ) && effectiveSize >= minimumSize, "scrollbar vcEffectiveSize" );

                const double thumbLen = std::max( 6.0, r.height() * effectiveSize );
                QVERIFY2( thumbLen >= 7.0, "font picker thumb should not be dot-sized" );
            }
        }

        // The popup list should remain within the window so the scroll range is reachable.
        {
            QQuickItem *list = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesFontFamilyPickerList" ) );
            QVERIFY2( list != nullptr, "preferencesFontFamilyPickerList" );

            const QRectF listRect = sceneRect( list );
            const QRectF winRect( 0.0, 0.0, window->width(), window->height() );

            // Allow a small tolerance for borders/shadows.
            const QRectF tolWin = winRect.adjusted( -2.0, -2.0, 2.0, 2.0 );
            QVERIFY2( tolWin.contains( listRect ), "font picker list should be fully on-screen" );
        }

        // Ensure the popup itself leaves a small bottom margin.
        {
            QQuickItem *bg = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesFontFamilyPickerPopupBackground" ) );
            QVERIFY2( bg != nullptr, "preferencesFontFamilyPickerPopupBackground" );

            const QRectF bgRect = sceneRect( bg );
            QVERIFY2( bgRect.bottom() <= window->height() - 4.0, "font picker popup should not sit flush with window bottom" );
        }

        // Ensure the list frame fits inside the popup background.
        {
            QQuickItem *bg = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesFontFamilyPickerPopupBackground" ) );
            QVERIFY2( bg != nullptr, "preferencesFontFamilyPickerPopupBackground" );
            QQuickItem *frame = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesFontFamilyPickerListFrame" ) );
            QVERIFY2( frame != nullptr, "preferencesFontFamilyPickerListFrame" );

            const QRectF bgRect = sceneRect( bg );
            const QRectF frameRect = sceneRect( frame );
            const QRectF insetBg = bgRect.adjusted( 0.0, 0.0, 0.0, 1.0 );
            QVERIFY2( insetBg.contains( frameRect ), "font picker list frame should not overflow popup" );
        }

        verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-font-picker" );

        // Close the popup.
        QTest::keyClick( window, Qt::Key_Escape );
        QTest::qWait( 80 );
        clearMessages();
    }

    QQuickItem *devicesCategory = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesCategoryDevices" ) );
    QVERIFY2( devicesCategory != nullptr, "preferencesCategoryDevices" );
    clickItemCenter( *window, devicesCategory );
    QTest::qWait( 120 );
    failIfAnyWarnings( "textflow-preferences-devices" );
    clearMessages();

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-preferences-devices" );

    QQuickItem *closeButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesCloseButton" ) );
    QVERIFY2( closeButton != nullptr, "preferencesCloseButton" );
    clickItemCenter( *window, closeButton );
    QTest::qWait( 120 );

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-landing-after-preferences" );

    QQuickItem *joinRoomButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "joinRoomButton" ) );
    QVERIFY2( joinRoomButton != nullptr, "joinRoomButton" );
    clickItemCenter( *window, joinRoomButton );
    QTest::qWait( 200 );
    failIfAnyWarnings( "textflow-go-shell" );
    clearMessages();

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-shell" );

    // Open the source inspector (modal overlay) and verify text overlaps within layers.
    {
        QObject *shellPage = rootObject->findChild<QObject *>( QStringLiteral( "shellPage" ) );
        QVERIFY2( shellPage != nullptr, "shellPage" );

        shellPage->setProperty( "selectedSourceIndex", 0 );
        shellPage->setProperty( "selectedSourceName", QStringLiteral( "Camera" ) );
        shellPage->setProperty( "selectedSourceExportEnabled", false );
        shellPage->setProperty( "sourceInspectorOpen", true );

        QCoreApplication::processEvents();
        QTest::qWait( 80 );

        QQuickItem *inspectorOverlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorOverlay" ) );
        QVERIFY2( inspectorOverlay != nullptr, "sourceInspectorOverlay" );
        QTRY_VERIFY_WITH_TIMEOUT( inspectorOverlay->property( "visible" ).toBool(), 1000 );

        verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-source-inspector" );

        QQuickItem *inspectorClose = findItemByObjectNameRecursive( rootItem, QStringLiteral( "sourceInspectorCloseButton" ) );
        QVERIFY2( inspectorClose != nullptr, "sourceInspectorCloseButton" );
        clickItemCenter( *window, inspectorClose );
        QTest::qWait( 160 );
    }

    verifyNoVisibleTextOverlapsIgnoringOverlays( rootItem, "textflow-shell-after-inspector" );
}

void tst_QmlUi::layout_textContrastIsReadableInKeyScreens()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "green" ) );
    }

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 80 );
    failIfAnyWarnings( "contrast-post-load" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    verifyReadableTextContrast( *window, rootItem, "landing" );
    verifyInkStrokeContracts( *window, rootItem, "landing" );

    QQuickItem *preferencesButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingPreferencesButton" ) );
    QVERIFY2( preferencesButton != nullptr, "landingPreferencesButton" );
    clickItemCenter( *window, preferencesButton );
    QTest::qWait( 80 );
    failIfAnyWarnings( "contrast-open-preferences" );
    clearMessages();

    verifyReadableTextContrast( *window, rootItem, "preferences" );
    verifyInkStrokeContracts( *window, rootItem, "preferences" );
}

void tst_QmlUi::preferences_scrollbarAndWheelWork()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "custom" ) );
        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
    }

    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupOpenPreferences" ), true );
    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupPreferencesCategoryIndex" ), 0 );
    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );

    window->resize( 360, 220 );

    QTest::qWait( 80 );
    failIfAnyWarnings( "scroll-open-preferences" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    QQuickItem *scrollView = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesGeneralScrollView" ) );
    QQuickItem *scrollBar = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesGeneralScrollBar" ) );
    QVERIFY2( scrollView != nullptr, "preferencesGeneralScrollView" );
    QVERIFY2( scrollBar != nullptr, "preferencesGeneralScrollBar" );

    QTRY_VERIFY_WITH_TIMEOUT( scrollBar->isVisible(), 1000 );
    QVERIFY( scrollBar->width() >= 8.0 );
    QVERIFY( scrollBar->height() >= 60.0 );

    const QRectF viewR = sceneRect( scrollView );
    const QRectF barR = sceneRect( scrollBar );
    QVERIFY( barR.right() >= viewR.right() - 12.0 );

    QObject *flickableObj = scrollView->property( "contentItem" ).value<QObject *>();
    QQuickItem *flickable = qobject_cast<QQuickItem *>( flickableObj );
    if ( flickable == nullptr ) {
        flickable = nullptr;
        const QList<QQuickItem *> children = scrollView->childItems();
        for ( QQuickItem *child : children ) {
            if ( child != nullptr ) {
                const QString className = QString::fromLatin1( child->metaObject()->className() );
                if ( className.startsWith( QStringLiteral( "QQuickFlickable" ) ) ) {
                    flickable = child;
                    break;
                }
            }
        }
    }
    QVERIFY2( flickable != nullptr, "preferencesGeneralScrollView flickable" );

    // Ensure pointer is over the scroll view so wheel routing matches real usage.
    moveMouseToItemCenter( *window, scrollView );

    const qreal before = flickable->property( "contentY" ).toReal();
    wheelItemCenter( *window, scrollView, -120 );
    QTRY_VERIFY_WITH_TIMEOUT( flickable->property( "contentY" ).toReal() > before + 1.0, 1000 );
    failIfAnyWarnings( "scroll-wheel" );
    clearMessages();
}

void tst_QmlUi::preferences_wheelOverComboBox_doesNotChangeSelection()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "accent", QStringLiteral( "custom" ) );
        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
    }

    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupOpenPreferences" ), true );
    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupPreferencesCategoryIndex" ), 0 );
    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );

    window->resize( 360, 220 );
    QTest::qWait( 80 );
    failIfAnyWarnings( "combo-wheel-open" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    QQuickItem *scrollView = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesGeneralScrollView" ) );
    QVERIFY2( scrollView != nullptr, "preferencesGeneralScrollView" );

    QObject *flickableObj = scrollView->property( "contentItem" ).value<QObject *>();
    QQuickItem *flickable = qobject_cast<QQuickItem *>( flickableObj );
    QVERIFY2( flickable != nullptr, "preferencesGeneralScrollView flickable" );

    QQuickItem *combo = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesAccentCombo" ) );
    QVERIFY2( combo != nullptr, "preferencesAccentCombo" );

    // The preferences page is scrollable; ensure the combo is on-screen before wheeling over it.
    {
        const qreal contentH = flickable->property( "contentHeight" ).toReal();
        const qreal viewH = flickable->height();
        const qreal maxY = std::max( 0.0, contentH - viewH );

        const QPointF comboCentreInFlickable = combo->mapToItem( flickable, QPointF( combo->width() / 2.0, combo->height() / 2.0 ) );
        qreal desired = comboCentreInFlickable.y() - ( viewH / 2.0 );
        if ( desired < 0.0 ) {
            desired = 0.0;
        }
        if ( desired > maxY ) {
            desired = maxY;
        }

        flickable->setProperty( "contentY", desired );
        QCoreApplication::processEvents();
        QTest::qWait( 80 );

        QVERIFY2( itemCentreIsInsideWindow( *window, combo ), "preferencesAccentCombo should be on-screen" );
    }

    const int beforeIndex = combo->property( "currentIndex" ).toInt();
    const qreal beforeY = flickable->property( "contentY" ).toReal();

    moveMouseToItemCenter( *window, combo );
    wheelAtItemCenterViaWindow( *window, combo, -120 );

    QTRY_VERIFY_WITH_TIMEOUT( flickable->property( "contentY" ).toReal() > beforeY + 1.0, 1000 );
    QCOMPARE( combo->property( "currentIndex" ).toInt(), beforeIndex );

    failIfAnyWarnings( "combo-wheel" );
    clearMessages();
}

void tst_QmlUi::preferences_clickInsidePanel_doesNotClose()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupOpenPreferences" ), true );
    engine.rootContext()->setContextProperty( QStringLiteral( "vcstreamStartupPreferencesCategoryIndex" ), 0 );
    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 80 );
    failIfAnyWarnings( "prefs-click-open" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    QQuickItem *overlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesOverlay" ) );
    QQuickItem *panel = findItemByObjectNameRecursive( rootItem, QStringLiteral( "preferencesPanel" ) );
    QVERIFY2( overlay != nullptr, "preferencesOverlay" );
    QVERIFY2( panel != nullptr, "preferencesPanel" );
    QVERIFY( overlay->isVisible() );

    {
        const QPointF scenePoint = panel->mapToScene( QPointF( panel->width() - 10.0, panel->height() - 10.0 ) );
        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, scenePoint.toPoint() );
        QTest::qWait( 60 );
        QVERIFY2( overlay->isVisible(), "overlay should remain open after inside click" );
    }

    {
        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, QPoint( 2, 2 ) );
        QTRY_VERIFY_WITH_TIMEOUT( !overlay->isVisible(), 1000 );
    }

    failIfAnyWarnings( "prefs-click" );
    clearMessages();
}

void tst_QmlUi::landing_popups_openAndCloseWithoutWarnings()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 80 );
    failIfAnyWarnings( "landing-popups-open" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    QQuickItem *schedulingButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingSchedulingButton" ) );
    QQuickItem *supportButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "landingSupportButton" ) );
    QQuickItem *schedulingOverlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "schedulingOverlay" ) );
    QQuickItem *supportOverlay = findItemByObjectNameRecursive( rootItem, QStringLiteral( "supportOverlay" ) );
    QQuickItem *schedulingPanel = findItemByObjectNameRecursive( rootItem, QStringLiteral( "schedulingPanel" ) );
    QQuickItem *supportPanel = findItemByObjectNameRecursive( rootItem, QStringLiteral( "supportPanel" ) );
    QQuickItem *schedulingOpenButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "schedulingOpenButton" ) );
    QQuickItem *supportTwitchButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "supportTwitchButton" ) );
    QQuickItem *supportSocialsButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "supportSocialsButton" ) );

    QVERIFY2( schedulingButton != nullptr, "landingSchedulingButton" );
    QVERIFY2( supportButton != nullptr, "landingSupportButton" );
    QVERIFY2( schedulingOverlay != nullptr, "schedulingOverlay" );
    QVERIFY2( supportOverlay != nullptr, "supportOverlay" );
    QVERIFY2( schedulingPanel != nullptr, "schedulingPanel" );
    QVERIFY2( supportPanel != nullptr, "supportPanel" );
    QVERIFY2( schedulingOpenButton != nullptr, "schedulingOpenButton" );
    QVERIFY2( supportTwitchButton != nullptr, "supportTwitchButton" );
    QVERIFY2( supportSocialsButton != nullptr, "supportSocialsButton" );

    QVERIFY( !schedulingOverlay->isVisible() );
    QVERIFY( !supportOverlay->isVisible() );
    QVERIFY( !shouldAttemptActivate( schedulingButton ) );
    QVERIFY( !shouldAttemptActivate( supportButton ) );
    QVERIFY( !shouldAttemptActivate( schedulingOpenButton ) );
    QVERIFY( !shouldAttemptActivate( supportTwitchButton ) );
    QVERIFY( !shouldAttemptActivate( supportSocialsButton ) );

    clickItemCenter( *window, schedulingButton );
    QTRY_VERIFY_WITH_TIMEOUT( schedulingOverlay->isVisible(), 1000 );
    QTest::qWait( 80 );
    failIfAnyWarnings( "landing-scheduling-open" );
    clearMessages();

    {
        const QRectF r = sceneRect( schedulingPanel );
        const QPoint p = r.center().toPoint();
        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, p );
        QTest::qWait( 60 );
        QVERIFY2( schedulingOverlay->isVisible(), "scheduling overlay should remain open after inside click" );
    }

    QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, QPoint( 2, 2 ) );
    QTRY_VERIFY_WITH_TIMEOUT( !schedulingOverlay->isVisible(), 1000 );
    failIfAnyWarnings( "landing-scheduling-close" );
    clearMessages();

    clickItemCenter( *window, supportButton );
    QTRY_VERIFY_WITH_TIMEOUT( supportOverlay->isVisible(), 1000 );
    QTest::qWait( 80 );
    failIfAnyWarnings( "landing-support-open" );
    clearMessages();

    {
        const QRectF r = sceneRect( supportPanel );
        const QPoint p = r.center().toPoint();
        QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, p );
        QTest::qWait( 60 );
        QVERIFY2( supportOverlay->isVisible(), "support overlay should remain open after inside click" );
    }

    QTest::mouseClick( window, Qt::LeftButton, Qt::NoModifier, QPoint( 2, 2 ) );
    QTRY_VERIFY_WITH_TIMEOUT( !supportOverlay->isVisible(), 1000 );
    failIfAnyWarnings( "landing-support-close" );
    clearMessages();
}

void tst_QmlUi::app_utilityActions_areConsistentAcrossPages()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 80 );
    failIfAnyWarnings( "utility-actions-post-load" );
    clearMessages();

    QQuickItem *rootItem = window->contentItem();
    QVERIFY( rootItem != nullptr );

    verifyUtilityActionsOrderAndPlacement(
        *window,
        rootItem,
        QStringLiteral( "landingSchedulingButton" ),
        QStringLiteral( "landingSupportButton" ),
        QStringLiteral( "landingPreferencesButton" ),
        "utility-actions-landing" );

    QQuickItem *joinRoomButton = findItemByObjectNameRecursive( rootItem, QStringLiteral( "joinRoomButton" ) );
    QVERIFY2( joinRoomButton != nullptr, "joinRoomButton" );
    clickItemCenter( *window, joinRoomButton );
    QTest::qWait( 160 );
    failIfAnyWarnings( "utility-actions-go-shell" );
    clearMessages();

    verifyUtilityActionsOrderAndPlacement(
        *window,
        rootItem,
        QStringLiteral( "shellSchedulingButton" ),
        QStringLiteral( "shellSupportButton" ),
        QStringLiteral( "preferencesButton" ),
        "utility-actions-shell" );
}

static void verifyColourApprox( const QColor &actual, const QColor &expected, const int tolerance, const char *context )
{
    const int dr = qAbs( actual.red() - expected.red() );
    const int dg = qAbs( actual.green() - expected.green() );
    const int db = qAbs( actual.blue() - expected.blue() );

    if ( dr > tolerance || dg > tolerance || db > tolerance ) {
        const QString message =
            QStringLiteral( "%1 expected=%2 actual=%3 dr=%4 dg=%5 db=%6" )
                .arg( QString::fromLatin1( context ) )
                .arg( expected.name( QColor::HexRgb ) )
                .arg( actual.name( QColor::HexRgb ) )
                .arg( dr )
                .arg( dg )
                .arg( db );
        QFAIL( qPrintable( message ) );
    }
}

void tst_QmlUi::themePresetAccents_mapToExplicitHighlightColours()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    if ( supervisor.preferences() != nullptr ) {
        supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
    }

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 60 );

    struct Expectation {
        QString accent;
        QColor expected;
    };

    const QVector<Expectation> expectations = {
        { QStringLiteral( "red" ), QColor( QStringLiteral( "#B3263A" ) ) },
        { QStringLiteral( "orange" ), QColor( QStringLiteral( "#E07A2F" ) ) },
        { QStringLiteral( "yellow" ), QColor( QStringLiteral( "#E0B21B" ) ) },
        { QStringLiteral( "green" ), QColor( QStringLiteral( "#2FA66B" ) ) },
        { QStringLiteral( "cyan" ), QColor( QStringLiteral( "#24B3B6" ) ) },
        { QStringLiteral( "blue" ), QColor( QStringLiteral( "#3D7BEB" ) ) },
        { QStringLiteral( "pink" ), QColor( QStringLiteral( "#FC89AC" ) ) },
        { QStringLiteral( "purple" ), QColor( QStringLiteral( "#8F63F4" ) ) },
        // victorian primary accent is explicit in QML (oxblood).
        { QStringLiteral( "victorian" ), QColor( QStringLiteral( "#5B1F2A" ) ) },
    };

    QObject *paletteObj = window->property( "palette" ).value<QObject *>();
    QVERIFY2( paletteObj != nullptr, "window palette object" );

    for ( const Expectation &e : expectations ) {
        if ( supervisor.preferences() != nullptr ) {
            supervisor.preferences()->setProperty( "accent", e.accent );
        }

        QTRY_VERIFY_WITH_TIMEOUT( paletteObj->property( "highlight" ).value<QColor>().isValid(), 500 );
        const QColor actual = paletteObj->property( "highlight" ).value<QColor>();

        verifyColourApprox( actual, e.expected, 2, qPrintable( QStringLiteral( "accent=%1" ).arg( e.accent ) ) );
    }

    failIfAnyWarnings( "preset-accents" );
    clearMessages();
}

void tst_QmlUi::themePresetAccents_shiftInDarkMode()
{
    AppSupervisor supervisor;
    OklchUtil oklchUtil;
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

    engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
    engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
    engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
    engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
    engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

    QCOMPARE( engine.rootObjects().size(), 1 );

    QObject *rootObject = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
    QVERIFY( window != nullptr );
    QVERIFY( QTest::qWaitForWindowExposed( window ) );
    QTest::qWait( 60 );

    QObject *paletteObj = window->property( "palette" ).value<QObject *>();
    QVERIFY2( paletteObj != nullptr, "window palette object" );

    auto highlight = [&]() {
        return paletteObj->property( "highlight" ).value<QColor>();
    };

    // Use a couple of accents that cover different hue regions.
    const QVector<QString> accents = { QStringLiteral( "red" ), QStringLiteral( "blue" ) };

    for ( const QString &accent : accents ) {
        if ( supervisor.preferences() != nullptr ) {
            supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "light" ) );
            supervisor.preferences()->setProperty( "accent", accent );
        }
        QTRY_VERIFY_WITH_TIMEOUT( highlight().isValid(), 500 );
        const QColor light = highlight();

        if ( supervisor.preferences() != nullptr ) {
            supervisor.preferences()->setProperty( "themeMode", QStringLiteral( "dark" ) );
        }
        QTRY_VERIFY_WITH_TIMEOUT( highlight().isValid() && ( highlight() != light ), 800 );
        const QColor dark = highlight();

        const qreal lLight = relativeLuminance( light );
        const qreal lDark = relativeLuminance( dark );
        QVERIFY2( lDark > lLight + 0.02, qPrintable( QStringLiteral( "accent=%1 luminance did not increase in dark mode (light=%2 dark=%3)" )
            .arg( accent )
            .arg( lLight )
            .arg( lDark ) ) );
    }

    failIfAnyWarnings( "preset-accents-dark-shift" );
    clearMessages();
}

static qreal contrastRatioForColours( const QColor &a, const QColor &b )
{
    return contrastRatio( a, b );
}

void tst_QmlUi::landing_primaryButtons_haveReadableTextInVictorianAndLight()
{
    struct Scenario {
        QString themeMode;
        QString accent;
    };

    const QVector<Scenario> scenarios = {
        { QStringLiteral( "victorian" ), QStringLiteral( "victorian" ) },
        { QStringLiteral( "light" ), QStringLiteral( "victorian" ) },
        { QStringLiteral( "light" ), QStringLiteral( "pink" ) },
    };

    for ( const Scenario &s : scenarios ) {
        AppSupervisor supervisor;
        OklchUtil oklchUtil;
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

        if ( supervisor.preferences() != nullptr ) {
            supervisor.preferences()->setProperty( "themeMode", s.themeMode );
            supervisor.preferences()->setProperty( "accent", s.accent );
        }

        engine.addImageProvider( QStringLiteral( "vcTheme" ), new AccentImageProvider() );
        engine.addImageProvider( QStringLiteral( "theme" ), new ThemeIconImageProvider() );
        engine.rootContext()->setContextProperty( QStringLiteral( "oklchUtil" ), &oklchUtil );
        engine.rootContext()->setContextProperty( QStringLiteral( "appSupervisor" ), &supervisor );
        engine.load( QUrl( QStringLiteral( "qrc:/qml/main.qml" ) ) );

        QCOMPARE( engine.rootObjects().size(), 1 );

        QObject *rootObject = engine.rootObjects().first();
        QQuickWindow *window = qobject_cast<QQuickWindow *>( rootObject );
        QVERIFY( window != nullptr );
        QVERIFY( QTest::qWaitForWindowExposed( window ) );
        QTest::qWait( 80 );

        QQuickItem *rootItem = window->contentItem();
        QVERIFY( rootItem != nullptr );

        QQuickItem *join = findItemByObjectNameRecursive( rootItem, QStringLiteral( "joinRoomButton" ) );
        QQuickItem *host = findItemByObjectNameRecursive( rootItem, QStringLiteral( "hostRoomButton" ) );
        QVERIFY2( join != nullptr, "joinRoomButton" );
        QVERIFY2( host != nullptr, "hostRoomButton" );

        auto verifyButton = [&]( QQuickItem *button, const char *name ) {
            const QColor fill = button->property( "surfaceColour" ).value<QColor>();
            QVERIFY2( fill.isValid(), name );

            QObject *pal = button->property( "palette" ).value<QObject *>();
            QVERIFY2( pal != nullptr, name );
            const QColor fg = pal->property( "buttonText" ).value<QColor>();
            QVERIFY2( fg.isValid(), name );

            const qreal ratio = contrastRatioForColours( fg, fill );
            const qreal minRatio = ( s.themeMode == QStringLiteral( "light" ) && s.accent == QStringLiteral( "pink" ) ? 4.5 : 3.5 );
            QVERIFY2( ratio >= minRatio, qPrintable( QStringLiteral( "%1 themeMode=%2 accent=%3 contrast=%4 min=%5 fg=%6 fill=%7" )
                .arg( QString::fromLatin1( name ) )
                .arg( s.themeMode )
                .arg( s.accent )
                .arg( ratio )
                .arg( minRatio )
                .arg( fg.name( QColor::HexRgb ) )
                .arg( fill.name( QColor::HexRgb ) ) ) );
        };

        verifyButton( join, "joinRoomButton" );
        verifyButton( host, "hostRoomButton" );

        failIfAnyWarnings( "landing-button-contrast" );
        clearMessages();
    }
}


int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "tst_qml_ui main", [argc, argv]() -> int {
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;

        qtshims::applyBeforeQGuiApplication();

        QGuiApplication app( localArgc, localArgv );
        tst_QmlUi tc;
        const int result = QTest::qExec( &tc, localArgc, localArgv );
        return result;
    } );

    return exitCode;
}

#include "tst_qml_ui.moc"
