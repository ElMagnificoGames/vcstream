#include "modules/ui/colour/oklchutil.h"

#include "modules/ui/colour/oklchcolour.h"

OklchUtil::OklchUtil( QObject *parent )
    : QObject( parent )
{
}

QColor OklchUtil::oklchToSrgbFitted( const double lightness, const double chroma, const double hueDegrees ) const
{
    oklchcolour::Oklch v;

    v.lightness = lightness;
    v.chroma = chroma;
    v.hueDegrees = hueDegrees;

    return oklchcolour::toSrgbFitted( v );
}

QColor OklchUtil::adjustSrgbInOklchFitted( const QColor &srgb, const double dLightness, const double chromaScale ) const
{
    oklchcolour::Oklch v;

    v = oklchcolour::fromSrgb( srgb );

    v.lightness += dLightness;
    if ( v.lightness < 0.0 ) {
        v.lightness = 0.0;
    }
    if ( v.lightness > 1.0 ) {
        v.lightness = 1.0;
    }

    v.chroma *= chromaScale;
    if ( v.chroma < 0.0 ) {
        v.chroma = 0.0;
    }
    if ( v.chroma > 1.0 ) {
        v.chroma = 1.0;
    }

    return oklchcolour::toSrgbFitted( v );
}
