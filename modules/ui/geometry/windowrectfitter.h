#ifndef MODULES_UI_GEOMETRY_WINDOWRECTFITTER_H
#define MODULES_UI_GEOMETRY_WINDOWRECTFITTER_H

#include <QRect>
#include <QSize>
#include <QVector>

namespace windowrectfitter {

enum class Outcome {
    Unchanged,
    Moved,
    Resized,
    Degraded,
    Fallback,
};

struct FitResult {
    QRect rect;
    Outcome outcome;
    bool fullyContained;
};

FitResult fit( const QRect &desired,
               const QVector<QRect> &allowedRects,
               const QSize &minimumSize,
               int titleBarHeight,
               int titleBarGripWidth );

}

#endif
