#include "modules/ui/colour/oklchcolour.h"

#include <cmath>
#include <QtMath>

namespace {

double clamp01( const double v )
{
    if ( v < 0.0 ) {
        return 0.0;
    }
    if ( v > 1.0 ) {
        return 1.0;
    }
    return v;
}

double clampHueDegrees( const double degrees )
{
    double out;

    out = std::fmod( degrees, 360.0 );
    if ( out < 0.0 ) {
        out += 360.0;
    }

    return out;
}

double srgbEncode( const double linear )
{
    if ( linear <= 0.0 ) {
        return 0.0;
    }
    if ( linear >= 1.0 ) {
        return 1.0;
    }

    if ( linear <= 0.0031308 ) {
        return 12.92 * linear;
    }

    return 1.055 * std::pow( linear, 1.0 / 2.4 ) - 0.055;
}

double srgbDecode( const double u )
{
    if ( u <= 0.0 ) {
        return 0.0;
    }
    if ( u >= 1.0 ) {
        return 1.0;
    }

    if ( u <= 0.04045 ) {
        return u / 12.92;
    }

    return std::pow( ( u + 0.055 ) / 1.055, 2.4 );
}

struct LinearRgb final
{
    double r;
    double g;
    double b;
};

LinearRgb oklabToLinearSrgb( const double L, const double a, const double b )
{
    LinearRgb out;

    const double l_ = L + 0.3963377774 * a + 0.2158037573 * b;
    const double m_ = L - 0.1055613458 * a - 0.0638541728 * b;
    const double s_ = L - 0.0894841775 * a - 1.2914855480 * b;

    const double l = l_ * l_ * l_;
    const double m = m_ * m_ * m_;
    const double s = s_ * s_ * s_;

    out.r = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s;
    out.g = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s;
    out.b = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s;

    return out;
}

struct Oklab final
{
    double L;
    double a;
    double b;
};

Oklab linearSrgbToOklab( const LinearRgb &linear )
{
    // https://bottosson.github.io/posts/oklab/
    const double l = 0.4122214708 * linear.r + 0.5363325363 * linear.g + 0.0514459929 * linear.b;
    const double m = 0.2119034982 * linear.r + 0.6806995451 * linear.g + 0.1073969566 * linear.b;
    const double s = 0.0883024619 * linear.r + 0.2817188376 * linear.g + 0.6299787005 * linear.b;

    const double l_ = std::cbrt( l );
    const double m_ = std::cbrt( m );
    const double s_ = std::cbrt( s );

    Oklab out;
    out.L = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_;
    out.a = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_;
    out.b = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_;
    return out;
}

bool linearRgbIn01( const LinearRgb &c )
{
    return ( c.r >= 0.0 ) && ( c.r <= 1.0 ) && ( c.g >= 0.0 ) && ( c.g <= 1.0 ) && ( c.b >= 0.0 ) && ( c.b <= 1.0 );
}

}

namespace oklchcolour {

Oklch clampOklch( const Oklch &oklch )
{
    Oklch out;

    out = oklch;
    out.lightness = clamp01( out.lightness );

    if ( out.chroma < 0.0 ) {
        out.chroma = 0.0;
    }
    if ( out.chroma > 1.0 ) {
        out.chroma = 1.0;
    }

    out.hueDegrees = clampHueDegrees( out.hueDegrees );

    return out;
}

bool isInSrgbGamut( const Oklch &oklch )
{
    const Oklch clamped = clampOklch( oklch );
    const double hRad = qDegreesToRadians( clamped.hueDegrees );
    const double a = clamped.chroma * std::cos( hRad );
    const double b = clamped.chroma * std::sin( hRad );
    const LinearRgb linear = oklabToLinearSrgb( clamped.lightness, a, b );

    return linearRgbIn01( linear );
}

Oklch fitToSrgbGamut( const Oklch &oklch )
{
    Oklch out;
    Oklch clamped;
    double low;
    double high;

    out = clampOklch( oklch );
    clamped = out;

    if ( isInSrgbGamut( clamped ) ) {
        return out;
    }

    low = 0.0;
    high = clamped.chroma;

    for ( int i = 0; i < 22; ++i ) {
        const double mid = ( low + high ) / 2.0;
        Oklch probe;

        probe = clamped;
        probe.chroma = mid;

        if ( isInSrgbGamut( probe ) ) {
            low = mid;
        } else {
            high = mid;
        }
    }

    out.chroma = low;
    return out;
}

QColor toSrgbFitted( const Oklch &oklch )
{
    const Oklch fitted = fitToSrgbGamut( oklch );
    const double hRad = qDegreesToRadians( fitted.hueDegrees );
    const double a = fitted.chroma * std::cos( hRad );
    const double b = fitted.chroma * std::sin( hRad );
    const LinearRgb linear = oklabToLinearSrgb( fitted.lightness, a, b );

    const double r = srgbEncode( linear.r );
    const double g = srgbEncode( linear.g );
    const double bl = srgbEncode( linear.b );

    return QColor::fromRgbF( clamp01( r ), clamp01( g ), clamp01( bl ), 1.0 );
}

Oklch fromSrgb( const QColor &srgb )
{
    Oklch out;

    const double r = srgbDecode( srgb.redF() );
    const double g = srgbDecode( srgb.greenF() );
    const double b = srgbDecode( srgb.blueF() );

    const LinearRgb linear = { r, g, b };
    const Oklab lab = linearSrgbToOklab( linear );

    out.lightness = clamp01( lab.L );
    out.chroma = std::sqrt( lab.a * lab.a + lab.b * lab.b );

    const double hue = std::atan2( lab.b, lab.a );
    out.hueDegrees = clampHueDegrees( qRadiansToDegrees( hue ) );

    return out;
}

}
