#include "modules/ui/theme/themeiconimageprovider.h"

#include <QIcon>
#include <QImage>
#include <QPixmap>
#include <QSize>

ThemeIconImageProvider::ThemeIconImageProvider()
    : QQuickImageProvider( QQuickImageProvider::Image )
{
}

QImage ThemeIconImageProvider::requestImage( const QString &id, QSize *size, const QSize &requestedSize )
{
    QImage out;
    int w;
    int h;

    w = ( requestedSize.width() > 0 ) ? requestedSize.width() : 24;
    h = ( requestedSize.height() > 0 ) ? requestedSize.height() : 24;
    const QSize outSize( w, h );

    out = QImage( outSize, QImage::Format_ARGB32_Premultiplied );
    out.fill( Qt::transparent );

    if ( size != nullptr ) {
        *size = outSize;
    }

    const QIcon icon = QIcon::fromTheme( id );
    if ( !icon.isNull() ) {
        const QPixmap pix = icon.pixmap( outSize );
        if ( !pix.isNull() ) {
            out = pix.toImage();
        }
    }

    return out;
}
