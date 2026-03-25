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
    const int w = ( requestedSize.width() > 0 ) ? requestedSize.width() : 24;
    const int h = ( requestedSize.height() > 0 ) ? requestedSize.height() : 24;
    const QSize outSize( w, h );

    if ( size != nullptr ) {
        *size = outSize;
    }

    const QIcon icon = QIcon::fromTheme( id );
    if ( icon.isNull() ) {
        QImage empty( outSize, QImage::Format_ARGB32_Premultiplied );
        empty.fill( Qt::transparent );
        return empty;
    }

    const QPixmap pix = icon.pixmap( outSize );
    if ( pix.isNull() ) {
        QImage empty( outSize, QImage::Format_ARGB32_Premultiplied );
        empty.fill( Qt::transparent );
        return empty;
    }

    return pix.toImage();
}
