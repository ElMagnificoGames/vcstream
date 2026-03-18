#include "modules/ui/geometry/windowrectfitter.h"

#include <QRegion>
#include <QSet>
#include <QSpan>
#include <QtGlobal>

namespace {

struct CandidatePlacement {
    QRect rect;
    bool fullyContained;
    int visibleTopEdge;
    bool leftGripVisible;
    bool rightGripVisible;
    qint64 offScreenArea;
    qint64 movementCost;
    int absDx;
    int absDy;
};

qint64 rectArea( const QRect &r )
{
    qint64 out;

    out = static_cast<qint64>( r.width() ) * static_cast<qint64>( r.height() );

    return out;
}

qint64 regionArea( const QRegion &region )
{
    qint64 out;
    const QSpan<const QRect> rects = region.rects();

    out = 0;

    for ( const QRect &r : rects ) {
        out += rectArea( r );
    }

    return out;
}

QRegion buildAllowedRegion( const QVector<QRect> &allowedRects )
{
    QRegion out;

    for ( const QRect &r : allowedRects ) {
        if ( !r.isEmpty() ) {
            out += QRegion( r );
        }
    }

    return out;
}

bool fullyContainsRect( const QRegion &allowed, const QRect &rect )
{
    const QRegion outside = QRegion( rect ).subtracted( allowed );
    return outside.isEmpty();
}

QVector<int> candidateXPositions( const QRect &desired, const QSpan<const QRect> allowedRects, const int w )
{
    QVector<int> out;
    QSet<int> seen;

    seen.reserve( static_cast<int>( allowedRects.size() ) * 2 + 1 );

    auto appendIfNew = [&out, &seen]( const int value ) {
        if ( !seen.contains( value ) ) {
            seen.insert( value );
            out.append( value );
        }
    };

    appendIfNew( desired.x() );

    for ( const QRect &r : allowedRects ) {
        appendIfNew( r.left() );
        appendIfNew( r.right() - w + 1 );
    }

    return out;
}

QVector<int> candidateYPositions( const QRect &desired, const QSpan<const QRect> allowedRects, const int h )
{
    QVector<int> out;
    QSet<int> seen;

    seen.reserve( static_cast<int>( allowedRects.size() ) * 2 + 1 );

    auto appendIfNew = [&out, &seen]( const int value ) {
        if ( !seen.contains( value ) ) {
            seen.insert( value );
            out.append( value );
        }
    };

    appendIfNew( desired.y() );

    for ( const QRect &r : allowedRects ) {
        appendIfNew( r.top() );
        appendIfNew( r.bottom() - h + 1 );
    }

    return out;
}

CandidatePlacement scoreCandidate( const QRegion &allowed,
                                  const QRect &desired,
                                  const QRect &rect,
                                  const int titleBarHeight,
                                  const int titleBarGripWidth )
{
    CandidatePlacement out;
    const QRect topEdge( rect.x(), rect.y(), rect.width(), 1 );
    const QRect leftGrip( rect.x(), rect.y(), titleBarGripWidth, titleBarHeight );
    const QRect rightGrip( rect.right() - titleBarGripWidth + 1, rect.y(), titleBarGripWidth, titleBarHeight );
    const QRegion rectRegion( rect );
    const QRegion outside = rectRegion.subtracted( allowed );
    const int dx = rect.x() - desired.x();
    const int dy = rect.y() - desired.y();

    out.rect = rect;
    out.fullyContained = fullyContainsRect( allowed, rect );
    out.visibleTopEdge = static_cast<int>( regionArea( QRegion( topEdge ).intersected( allowed ) ) );
    out.leftGripVisible = fullyContainsRect( allowed, leftGrip );
    out.rightGripVisible = fullyContainsRect( allowed, rightGrip );
    out.offScreenArea = regionArea( outside );
    out.movementCost = static_cast<qint64>( dx ) * static_cast<qint64>( dx ) + static_cast<qint64>( dy ) * static_cast<qint64>( dy );
    out.absDx = qAbs( dx );
    out.absDy = qAbs( dy );

    return out;
}

bool isBetterContained( const CandidatePlacement &a, const CandidatePlacement &b )
{
    bool out;

    if ( a.movementCost != b.movementCost ) {
        out = ( a.movementCost < b.movementCost );
    } else if ( a.absDx != b.absDx ) {
        out = ( a.absDx < b.absDx );
    } else if ( a.absDy != b.absDy ) {
        out = ( a.absDy < b.absDy );
    } else if ( a.rect.x() != b.rect.x() ) {
        out = ( a.rect.x() < b.rect.x() );
    } else {
        out = ( a.rect.y() < b.rect.y() );
    }

    return out;
}

bool isBetterDegraded( const CandidatePlacement &a, const CandidatePlacement &b )
{
    bool out;
    const int aGrip = ( a.leftGripVisible || a.rightGripVisible ) ? 1 : 0;
    const int bGrip = ( b.leftGripVisible || b.rightGripVisible ) ? 1 : 0;

    if ( a.visibleTopEdge != b.visibleTopEdge ) {
        out = ( a.visibleTopEdge > b.visibleTopEdge );
    } else if ( aGrip != bGrip ) {
        out = ( aGrip > bGrip );
    } else if ( a.offScreenArea != b.offScreenArea ) {
        out = ( a.offScreenArea < b.offScreenArea );
    } else if ( a.movementCost != b.movementCost ) {
        out = ( a.movementCost < b.movementCost );
    } else if ( a.absDx != b.absDx ) {
        out = ( a.absDx < b.absDx );
    } else if ( a.absDy != b.absDy ) {
        out = ( a.absDy < b.absDy );
    } else if ( a.rect.x() != b.rect.x() ) {
        out = ( a.rect.x() < b.rect.x() );
    } else {
        out = ( a.rect.y() < b.rect.y() );
    }

    return out;
}

bool findBestPlacementContained( const QRegion &allowed,
                                const QRect &desired,
                                const int w,
                                const int h,
                                QRect *outRect )
{
    bool found;
    CandidatePlacement best;
    const QSpan<const QRect> allowedRects = allowed.rects();
    const QVector<int> xs = candidateXPositions( desired, allowedRects, w );
    const QVector<int> ys = candidateYPositions( desired, allowedRects, h );

    found = false;

    for ( const int y : ys ) {
        for ( const int x : xs ) {
            const QRect r( x, y, w, h );
            if ( fullyContainsRect( allowed, r ) ) {
                const CandidatePlacement c = scoreCandidate( allowed, desired, r, 1, 1 );
                if ( !found || isBetterContained( c, best ) ) {
                    best = c;
                    found = true;
                }
            }
        }
    }

    if ( found && outRect ) {
        *outRect = best.rect;
    }

    return found;
}

bool findBestPlacementDegraded( const QRegion &allowed,
                               const QRect &desired,
                               const int w,
                               const int h,
                               const int titleBarHeight,
                               const int titleBarGripWidth,
                               QRect *outRect )
{
    bool found;
    CandidatePlacement best;
    const QSpan<const QRect> allowedRects = allowed.rects();
    const QVector<int> xs = candidateXPositions( desired, allowedRects, w );
    const QVector<int> ys = candidateYPositions( desired, allowedRects, h );

    found = false;

    for ( const int y : ys ) {
        for ( const int x : xs ) {
            const QRect r( x, y, w, h );
            const CandidatePlacement c = scoreCandidate( allowed, desired, r, titleBarHeight, titleBarGripWidth );

            if ( !found || isBetterDegraded( c, best ) ) {
                best = c;
                found = true;
            }
        }
    }

    if ( found && outRect ) {
        *outRect = best.rect;
    }

    return found;
}

}

namespace windowrectfitter {

FitResult fit( const QRect &desired,
               const QVector<QRect> &allowedRects,
               const QSize &minimumSize,
               const int titleBarHeight,
               const int titleBarGripWidth )
{
    FitResult out;
    const QRegion allowed = buildAllowedRegion( allowedRects );
    const int minW = qMax( 1, minimumSize.width() );
    const int minH = qMax( 1, minimumSize.height() );
    int desiredW;
    int desiredH;
    QRect desiredRect;
    QRect fitted;
    bool found;
    bool done;

    out.rect = QRect();
    out.outcome = Outcome::Fallback;
    out.fullyContained = false;

    done = false;

    if ( allowed.isEmpty() ) {
        out.rect = QRect( 0, 0, minW, minH );
        done = true;
    }

    desiredW = desired.width();
    desiredH = desired.height();
    fitted = QRect();
    found = false;

    if ( !done ) {
        if ( desiredW < minW ) {
            desiredW = minW;
        }
        if ( desiredH < minH ) {
            desiredH = minH;
        }

        desiredRect = QRect( desired.x(), desired.y(), desiredW, desiredH );

        fitted = desiredRect;
        found = findBestPlacementContained( allowed, desiredRect, desiredW, desiredH, &fitted );
        if ( found ) {
            out.rect = fitted;
            out.fullyContained = true;

            if ( fitted == desiredRect ) {
                out.outcome = Outcome::Unchanged;
            } else {
                out.outcome = Outcome::Moved;
            }

            done = true;
        }
    }

    if ( !done ) {
        const int maxW = desiredW;
        const int maxH = desiredH;
        qint64 bestArea;
        QSize bestSize;
        bool anySizeFound;

        bestArea = -1;
        bestSize = QSize( minW, minH );
        anySizeFound = false;

        for ( int h = maxH; h >= minH; --h ) {
            int low = minW;
            int high = maxW;
            int bestW = -1;

            while ( low <= high ) {
                const int mid = low + ( high - low ) / 2;
                QRect tmp;
                const bool ok = findBestPlacementContained( allowed, desiredRect, mid, h, &tmp );
                if ( ok ) {
                    bestW = mid;
                    low = mid + 1;
                } else {
                    high = mid - 1;
                }
            }

            if ( bestW >= minW ) {
                const qint64 area = static_cast<qint64>( bestW ) * static_cast<qint64>( h );
                if ( ( area > bestArea ) || ( area == bestArea && bestW > bestSize.width() ) ) {
                    bestArea = area;
                    bestSize = QSize( bestW, h );
                    anySizeFound = true;
                }
            }

            if ( bestArea == static_cast<qint64>( maxW ) * static_cast<qint64>( maxH ) )
                h = minH;
        }

        if ( anySizeFound ) {
            QRect bestRect;
            const bool ok = findBestPlacementContained( allowed, desiredRect, bestSize.width(), bestSize.height(), &bestRect );
            if ( ok ) {
                out.rect = bestRect;
                out.outcome = Outcome::Resized;
                out.fullyContained = true;
                done = true;
            }
        }
    }

    if ( !done ) {
        QRect degraded;
        const bool ok = findBestPlacementDegraded( allowed, desiredRect, minW, minH, titleBarHeight, titleBarGripWidth, &degraded );
        if ( ok ) {
            out.rect = degraded;
            out.outcome = Outcome::Degraded;
            out.fullyContained = fullyContainsRect( allowed, degraded );
        } else {
            out.rect = QRect( 0, 0, minW, minH );
            out.outcome = Outcome::Fallback;
            out.fullyContained = false;
        }
    }

    return out;
}

}
