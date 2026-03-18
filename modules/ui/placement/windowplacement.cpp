#include "modules/ui/placement/windowplacement.h"

#include <QGuiApplication>
#include <QPoint>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QVector>
#include <QWindow>

#include "modules/ui/geometry/windowrectfitter.h"

namespace {

const int kTitleBarHeight = 32;
const int kTitleBarGripWidth = 160;

const QString kSettingsWindowXKey = QStringLiteral( "ui/mainWindow/x" );
const QString kSettingsWindowYKey = QStringLiteral( "ui/mainWindow/y" );
const QString kSettingsWindowWidthKey = QStringLiteral( "ui/mainWindow/width" );
const QString kSettingsWindowHeightKey = QStringLiteral( "ui/mainWindow/height" );

QVector<QRect> availableGeometries()
{
    QVector<QRect> out;
    const QList<QScreen *> screens = QGuiApplication::screens();

    for ( QScreen *screen : screens ) {
        if ( screen ) {
            out.append( screen->availableGeometry() );
        }
    }

    return out;
}

QScreen *currentScreenOrPrimary( QWindow *window )
{
    QScreen *screen;

    screen = window ? window->screen() : nullptr;
    if ( screen == nullptr ) {
        screen = QGuiApplication::primaryScreen();
    }

    return screen;
}

QPoint centredPosition( const QRect &avail, const QSize &windowSize )
{
    const int x = avail.x() + ( avail.width() - windowSize.width() ) / 2;
    const int y = avail.y() + ( avail.height() - windowSize.height() ) / 2;
    return QPoint( x, y );
}

}

namespace windowplacement {

void restoreAndRepairMainWindowPlacement( QWindow *window )
{
    QSettings settings;
    const bool hasX = settings.contains( kSettingsWindowXKey );
    const bool hasY = settings.contains( kSettingsWindowYKey );
    const bool hasW = settings.contains( kSettingsWindowWidthKey );
    const bool hasH = settings.contains( kSettingsWindowHeightKey );
    const QSize minimumSize = window->minimumSize();

    if ( hasW && hasH ) {
        const int desiredWidth = settings.value( kSettingsWindowWidthKey, window->width() ).toInt();
        const int desiredHeight = settings.value( kSettingsWindowHeightKey, window->height() ).toInt();

        if ( desiredWidth > 0 && desiredHeight > 0 ) {
            window->resize( desiredWidth, desiredHeight );
        }
    }

    if ( window->width() < minimumSize.width() || window->height() < minimumSize.height() ) {
        window->resize( qMax( window->width(), minimumSize.width() ), qMax( window->height(), minimumSize.height() ) );
    }

    if ( hasX && hasY ) {
        const int desiredX = settings.value( kSettingsWindowXKey, window->x() ).toInt();
        const int desiredY = settings.value( kSettingsWindowYKey, window->y() ).toInt();

        window->setPosition( desiredX, desiredY );
    } else {
        QScreen *screen;

        screen = currentScreenOrPrimary( window );
        if ( screen != nullptr ) {
            const QRect avail = screen->availableGeometry();
            if ( !avail.isEmpty() ) {
                window->setPosition( centredPosition( avail, window->size() ) );
            }
        }
    }

    {
        const QVector<QRect> allowedRects = availableGeometries();
        const QRect desired = window->geometry();
        const windowrectfitter::FitResult fitResult =
            windowrectfitter::fit( desired, allowedRects, minimumSize, kTitleBarHeight, kTitleBarGripWidth );

        window->setGeometry( fitResult.rect );
    }
}

void saveMainWindowPlacement( QWindow *window )
{
    QSettings settings;
    settings.setValue( kSettingsWindowXKey, window->x() );
    settings.setValue( kSettingsWindowYKey, window->y() );
    settings.setValue( kSettingsWindowWidthKey, window->width() );
    settings.setValue( kSettingsWindowHeightKey, window->height() );
    settings.sync();
}

}
