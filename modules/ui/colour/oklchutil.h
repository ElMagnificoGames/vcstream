#ifndef MODULES_UI_COLOUR_OKLCHUTIL_H
#define MODULES_UI_COLOUR_OKLCHUTIL_H

#include <QColor>
#include <QObject>

class OklchUtil final : public QObject
{
    Q_OBJECT

public:
    explicit OklchUtil( QObject *parent = nullptr );

    Q_INVOKABLE QColor oklchToSrgbFitted( double lightness, double chroma, double hueDegrees ) const;

    // Adjust an existing sRGB colour in OKLCH space (then fit back into sRGB).
    // This is intended for mode-aware tuning of explicit preset colours.
    // - dLightness: additive delta in [ -1..+1 ] (clamped after applying)
    // - chromaScale: multiplicative scale (clamped after applying)
    Q_INVOKABLE QColor adjustSrgbInOklchFitted( const QColor &srgb, double dLightness, double chromaScale ) const;
};

#endif
