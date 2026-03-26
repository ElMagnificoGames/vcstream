#include "modules/ui/fonts/fontpreviewsafetycheckerdefault.h"

#include <QFont>
#include <QFontInfo>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRawFont>
#include <QThread>
#include <QTransform>

namespace {

#define VCSTREAM_FONT_PREVIEW_SAFETY_CHECK_SUPERSAMPLE 4

static bool isNonWhite( const QRgb px )
{
    if ( qAlpha( px ) == 0 ) {
        return false;
    }
    return !( qRed( px ) == 255 && qGreen( px ) == 255 && qBlue( px ) == 255 );
}

struct MaskStats {
    quint64 rasterInk;
    quint64 refInk;
    quint64 outsideInk;
    quint64 missingInk;
};

static MaskStats computeMaskStats( const QImage &raster, const QImage &ref )
{
    MaskStats st;
    st.rasterInk = 0;
    st.refInk = 0;
    st.outsideInk = 0;
    st.missingInk = 0;

    if ( raster.isNull() || ref.isNull() ) {
        return st;
    }
    if ( raster.size() != ref.size() ) {
        return st;
    }

    const QImage rr = ( raster.format() == QImage::Format_ARGB32_Premultiplied )
        ? raster
        : raster.convertToFormat( QImage::Format_ARGB32_Premultiplied );
    const QImage bb = ( ref.format() == QImage::Format_ARGB32_Premultiplied )
        ? ref
        : ref.convertToFormat( QImage::Format_ARGB32_Premultiplied );

    const int w = rr.width();
    const int h = rr.height();
    for ( int y = 0; y < h; ++y ) {
        const QRgb *rrow = reinterpret_cast<const QRgb *>( rr.constScanLine( y ) );
        const QRgb *brow = reinterpret_cast<const QRgb *>( bb.constScanLine( y ) );
        for ( int x = 0; x < w; ++x ) {
            const bool ri = isNonWhite( rrow[ x ] );
            const bool bi = isNonWhite( brow[ x ] );
            if ( ri ) {
                ++st.rasterInk;
            }
            if ( bi ) {
                ++st.refInk;
            }
            if ( ri && !bi ) {
                ++st.outsideInk;
            }
            if ( !ri && bi ) {
                ++st.missingInk;
            }
        }
    }

    return st;
}

static double computeMaskIoU( const QImage &a, const QImage &b )
{
    const MaskStats st = computeMaskStats( a, b );
    const quint64 inter = ( st.rasterInk >= st.outsideInk ) ? ( st.rasterInk - st.outsideInk ) : 0;
    const quint64 uni = st.rasterInk + st.missingInk;
    if ( uni == 0 ) {
        return 0.0;
    }
    return static_cast<double>( inter ) / static_cast<double>( uni );
}

static bool blitAlphaMaskAsBlack( QImage &dst, const QImage &alpha, const QPoint topLeft )
{
    if ( dst.isNull() || alpha.isNull() ) {
        return false;
    }

    const QImage aa = ( alpha.format() == QImage::Format_Alpha8 ) ? alpha : alpha.convertToFormat( QImage::Format_Alpha8 );
    if ( aa.isNull() ) {
        return false;
    }

    const int w = aa.width();
    const int h = aa.height();

    for ( int y = 0; y < h; ++y ) {
        const int dy = topLeft.y() + y;
        if ( dy < 0 || dy >= dst.height() ) {
            continue;
        }

        const uchar *arow = aa.constScanLine( y );
        QRgb *drow = reinterpret_cast<QRgb *>( dst.scanLine( dy ) );
        for ( int x = 0; x < w; ++x ) {
            const int dx = topLeft.x() + x;
            if ( dx < 0 || dx >= dst.width() ) {
                continue;
            }

            if ( arow[ x ] > 0 ) {
                drow[ dx ] = qRgba( 0, 0, 0, 255 );
            }
        }
    }

    return true;
}

static QImage renderReferenceMask( const QString &text, const QFont &font, const QSize &size, const int margin )
{
    QImage img( size, QImage::Format_ARGB32_Premultiplied );
    img.fill( qRgba( 255, 255, 255, 255 ) );

    const QRawFont raw = QRawFont::fromFont( font );
    if ( !raw.isValid() ) {
        return QImage();
    }

    const QVector<quint32> glyphs = raw.glyphIndexesForString( text );
    if ( glyphs.isEmpty() ) {
        return QImage();
    }

    const QVector<QPointF> adv = raw.advancesForGlyphIndexes( glyphs );
    const qreal baselineY = static_cast<qreal>( margin ) + raw.ascent();
    QPointF pen( margin, baselineY );

    QPainterPath outlinePath;
    int outlineGlyphs = 0;
    int alphaGlyphs = 0;

    const qsizetype n = glyphs.size();
    for ( qsizetype i = 0; i < n; ++i ) {
        const quint32 g = glyphs.at( i );
        bool drew = false;

        const QPainterPath gp = raw.pathForGlyph( g );
        if ( !gp.isEmpty() ) {
            QTransform t;
            t.translate( pen.x(), pen.y() );
            outlinePath.addPath( t.map( gp ) );
            ++outlineGlyphs;
            drew = true;
        }

        if ( !drew ) {
            const QImage a = raw.alphaMapForGlyph( g, QRawFont::PixelAntialiasing );
            if ( !a.isNull() && !a.size().isEmpty() ) {
                const QRectF br = raw.boundingRect( g );
                const QPoint topLeft( qRound( pen.x() + br.x() ), qRound( pen.y() + br.y() ) );
                if ( blitAlphaMaskAsBlack( img, a, topLeft ) ) {
                    ++alphaGlyphs;
                    drew = true;
                }
            }
        }

        if ( i < adv.size() ) {
            pen += adv.at( i );
        }
    }

    if ( !outlinePath.isEmpty() ) {
        QPainter p( &img );
        p.setRenderHint( QPainter::Antialiasing, true );
        p.setPen( Qt::NoPen );
        p.setBrush( QColor( 0, 0, 0 ) );
        p.drawPath( outlinePath );
        p.end();
    }

    if ( outlineGlyphs <= 0 && alphaGlyphs <= 0 ) {
        return QImage();
    }

    return img;
}

static QImage renderRasterMask( const QString &text, const QFont &font, const QSize &size, const int margin )
{
    QImage img( size, QImage::Format_ARGB32_Premultiplied );
    img.fill( qRgba( 255, 255, 255, 255 ) );

    QPainter p( &img );
    p.setRenderHint( QPainter::TextAntialiasing, true );
    p.setRenderHint( QPainter::Antialiasing, true );
    p.setPen( QColor( 0, 0, 0 ) );
    p.setFont( font );

    const QFontMetrics fm( font );
    const int baselineY = margin + fm.ascent();
    p.drawText( margin, baselineY, text );
    p.end();

    return img;
}

static bool checkPreviewLooksSane( const QString &family, const int pixelSize )
{
    if ( family.trimmed().isEmpty() ) {
        return true;
    }

    const int px = ( pixelSize > 0 ) ? pixelSize : 16;
    const int ss = VCSTREAM_FONT_PREVIEW_SAFETY_CHECK_SUPERSAMPLE;

    QFont f;
    f.setFamily( family );
    f.setPixelSize( px * ss );

    // If Qt resolves to a different family, treat it as safe for preview.
    {
        const QFontInfo info( f );
        if ( info.family().compare( family, Qt::CaseInsensitive ) != 0 ) {
            return true;
        }
    }

    const QString text = QStringLiteral( "Ag" );
    const int w = 220;
    const int h = 80;
    const int margin = 12;
    const QSize scaledSize( w * ss, h * ss );

    // Detect per-glyph fallback (font merging).
    const QImage rasterMergedScaled = renderRasterMask( text, f, scaledSize, margin * ss );

    QFont noMergeFont = f;
    noMergeFont.setStyleStrategy( QFont::NoFontMerging );
    const QImage rasterNoMergeScaled = renderRasterMask( text, noMergeFont, scaledSize, margin * ss );

    if ( rasterMergedScaled.isNull() || rasterNoMergeScaled.isNull() ) {
        return false;
    }

    const QImage rasterMerged = rasterMergedScaled.scaled( w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    const QImage rasterNoMerge = rasterNoMergeScaled.scaled( w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );

    {
        const MaskStats stMerge = computeMaskStats( rasterMerged, rasterNoMerge );
        if ( stMerge.rasterInk > 0 && stMerge.refInk > 0 ) {
            const double iouMerge = computeMaskIoU( rasterMerged, rasterNoMerge );
            if ( iouMerge < 0.25 ) {
                return true;
            }
        }
    }

    const QImage refScaled = renderReferenceMask( text, f, scaledSize, margin * ss );

    if ( refScaled.isNull() ) {
        const MaskStats st = computeMaskStats( rasterMerged, rasterMerged );
        return st.rasterInk > 0;
    }

    const QImage ref = refScaled.scaled( w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
    const MaskStats st = computeMaskStats( rasterMerged, ref );
    if ( st.rasterInk == 0 || st.refInk == 0 ) {
        return false;
    }

    const double outsideFrac = static_cast<double>( st.outsideInk ) / static_cast<double>( st.rasterInk );
    const double missingFrac = static_cast<double>( st.missingInk ) / static_cast<double>( st.refInk );

    if ( outsideFrac > 0.55 ) {
        return false;
    }
    if ( missingFrac > 0.90 ) {
        return false;
    }

    return true;
}

class FontPreviewSafetyWorker final : public QObject
{
    Q_OBJECT

public:
    explicit FontPreviewSafetyWorker( QObject *parent = nullptr )
        : QObject( parent )
    {
    }

public Q_SLOTS:
    void checkRequested( const QString &family, const int pixelSize )
    {
        const bool ok = checkPreviewLooksSane( family, pixelSize );
        Q_EMIT checkFinished( family, pixelSize, ok );
    }

Q_SIGNALS:
    void checkFinished( const QString &family, int pixelSize, bool ok );
};

}

FontPreviewSafetyCheckerDefault::FontPreviewSafetyCheckerDefault( QObject *parent )
    : FontPreviewSafetyChecker( parent )
{
}

FontPreviewSafetyCheckerDefault::~FontPreviewSafetyCheckerDefault()
{
    for ( auto it = m_threads.begin(); it != m_threads.end(); ++it ) {
        QThread *t = it.value();
        if ( t == nullptr ) {
            continue;
        }

        t->quit();
        t->wait( 50 );
        if ( !t->isRunning() ) {
            delete t;
        }
    }

    m_threads.clear();
}

void FontPreviewSafetyCheckerDefault::startCheck( const QString &family, const int pixelSize )
{
    const Key key = qMakePair( family, pixelSize );

    QThread *t = new QThread();
    t->setObjectName( QStringLiteral( "FontPreviewSafetyCheckThread" ) );

    FontPreviewSafetyWorker *w = new FontPreviewSafetyWorker();
    w->moveToThread( t );

    QObject::connect( w, &FontPreviewSafetyWorker::checkFinished, this, [this, key, t]( const QString &f, const int px, const bool ok ) {
        Q_EMIT checkFinished( f, px, ok );

        m_threads.remove( key );

        t->quit();
        t->wait( 2000 );
        delete t;
    }, Qt::QueuedConnection );

    QObject::connect( t, &QThread::finished, w, &QObject::deleteLater );

    m_threads.insert( key, t );
    t->start();

    QMetaObject::invokeMethod( w,
        "checkRequested",
        Qt::QueuedConnection,
        Q_ARG( QString, family ),
        Q_ARG( int, pixelSize ) );
}

#include "fontpreviewsafetycheckerdefault.moc"
