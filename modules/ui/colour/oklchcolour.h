#ifndef MODULES_UI_COLOUR_OKLCHCOLOUR_H
#define MODULES_UI_COLOUR_OKLCHCOLOUR_H

#include <QColor>

namespace oklchcolour {

struct Oklch final
{
    double lightness;
    double chroma;
    double hueDegrees;
};

QColor toSrgbFitted( const Oklch &oklch );
Oklch fromSrgb( const QColor &srgb );
bool isInSrgbGamut( const Oklch &oklch );
Oklch fitToSrgbGamut( const Oklch &oklch );

Oklch clampOklch( const Oklch &oklch );

}

#endif
