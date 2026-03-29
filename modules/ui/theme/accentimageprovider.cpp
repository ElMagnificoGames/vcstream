#include "modules/ui/theme/accentimageprovider.h"

#include "modules/ui/colour/oklchcolour.h"

#include <QColor>
#include <QImage>
#include <QLatin1Char>
#include <QString>
#include <QStringList>

namespace {

int safeDim( const int v, const int fallback )
{
    int out;

    out = fallback;

    if ( v > 0 ) {
        out = v;
    }

    return out;
}

double clamp01( const double v )
{
    double out;

    out = v;

    if ( out < 0.0 ) {
        out = 0.0;
    }
    if ( out > 1.0 ) {
        out = 1.0;
    }

    return out;
}

}

AccentImageProvider::AccentImageProvider()
    : QQuickImageProvider( QQuickImageProvider::Image )
{
}

QImage AccentImageProvider::requestImage( const QString &id, QSize *size, const QSize &requestedSize )
{
    QImage out;
    QSize outSize;

    outSize = requestedSize;
    outSize.setWidth( safeDim( outSize.width(), 320 ) );
    outSize.setHeight( safeDim( outSize.height(), 220 ) );

    if ( id.startsWith( QStringLiteral( "plane/" ) ) ) {
        const QStringList parts = id.split( QLatin1Char( '/' ) );
        double hue;

        hue = 0.0;
        if ( parts.size() >= 2 ) {
            hue = parts.at( 1 ).toDouble();
        }

        out = renderPlane( hue, outSize );
    } else if ( id == QStringLiteral( "hue" ) ) {
        out = renderHueBar( outSize );
    } else {
        out = QImage( outSize, QImage::Format_ARGB32_Premultiplied );
        out.fill( QColor( 0, 0, 0, 0 ) );
    }

    if ( size != nullptr ) {
        *size = out.size();
    }

    return out;
}

QImage AccentImageProvider::renderHueBar( const QSize &requestedSize )
{
    const int w = safeDim( requestedSize.width(), 360 );
    const int h = safeDim( requestedSize.height(), 18 );
    QImage out( w, h, QImage::Format_ARGB32_Premultiplied );

    for ( int x = 0; x < w; ++x ) {
        const double t = ( w > 1 ) ? ( static_cast<double>( x ) / static_cast<double>( w - 1 ) ) : 0.0;
        const double hue = t * 360.0;

        oklchcolour::Oklch o;
        o.lightness = 0.75;
        o.chroma = 0.16;
        o.hueDegrees = hue;

        const QColor c = oklchcolour::toSrgbFitted( o );
        const QRgb px = qRgba( c.red(), c.green(), c.blue(), 255 );

        for ( int y = 0; y < h; ++y ) {
            out.setPixel( x, y, px );
        }
    }

    return out;
}

QImage AccentImageProvider::renderPlane( const double hueDegrees, const QSize &requestedSize )
{
    const int w = safeDim( requestedSize.width(), 320 );
    const int h = safeDim( requestedSize.height(), 220 );
    QImage out( w, h, QImage::Format_ARGB32_Premultiplied );

    const double minL = 0.08;
    const double maxL = 0.95;
    const double maxC = 0.37;

    for ( int y = 0; y < h; ++y ) {
        const double ty = ( h > 1 ) ? ( static_cast<double>( y ) / static_cast<double>( h - 1 ) ) : 0.0;
        const double L = minL + ( 1.0 - ty ) * ( maxL - minL );

        for ( int x = 0; x < w; ++x ) {
            const double tx = ( w > 1 ) ? ( static_cast<double>( x ) / static_cast<double>( w - 1 ) ) : 0.0;
            const double C = clamp01( tx ) * maxC;

            oklchcolour::Oklch o;
            o.lightness = L;
            o.chroma = C;
            o.hueDegrees = hueDegrees;

            const QColor c = oklchcolour::toSrgbFitted( o );
            out.setPixel( x, y, qRgba( c.red(), c.green(), c.blue(), 255 ) );
        }
    }

    return out;
}
