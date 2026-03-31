#include "modules/ui/video/vcvideopaintitem.h"

#include <algorithm>

#include <QPainter>
#include <QRectF>
#include <QSize>
#include <QTimer>

#include <QVideoFrame>
#include <QVideoSink>

namespace {

QRectF aspectFitRect( const QSize &src, const QRectF &dst )
{
    QRectF out;

    out = QRectF();

    if ( !src.isEmpty() && !dst.isEmpty() ) {
        const qreal srcW = static_cast<qreal>( src.width() );
        const qreal srcH = static_cast<qreal>( src.height() );
        const qreal dstW = dst.width();
        const qreal dstH = dst.height();

        const qreal scale = std::min( dstW / srcW, dstH / srcH );
        const qreal w = srcW * scale;
        const qreal h = srcH * scale;
        const qreal x = dst.x() + ( dstW - w ) / 2.0;
        const qreal y = dst.y() + ( dstH - h ) / 2.0;
        out = QRectF( x, y, w, h );
    }

    return out;
}

}

VcVideoPaintItem::VcVideoPaintItem( QQuickItem *parent )
    : QQuickPaintedItem( parent )
    , m_sink( nullptr )
    , m_throttleTimer( nullptr )
    , m_clock()
    , m_pendingFrame()
    , m_pendingValid( false )
    , m_pendingSerial( 0 )
    , m_paintedSerial( 0 )
    , m_lastImage()
    , m_hasFrame( false )
    , m_framesReceived( 0 )
    , m_errorText()
    , m_updateScheduled( false )
    , m_lastPaintMs( 0 )
    , m_targetIntervalMs( 33 )
{
    // QQuickPaintedItem defaults are tuned for correctness; prefer speed.
    setAntialiasing( false );
    setMipmap( false );
    setOpaquePainting( false );

    m_clock.start();
    m_lastPaintMs = -m_targetIntervalMs;

    m_sink = new QVideoSink( this );
    QObject::connect( m_sink, &QVideoSink::videoFrameChanged, this, &VcVideoPaintItem::onVideoFrameChanged );

    m_throttleTimer = new QTimer( this );
    m_throttleTimer->setSingleShot( true );
    QObject::connect( m_throttleTimer, &QTimer::timeout, this, &VcVideoPaintItem::onThrottleTimeout );
}

VcVideoPaintItem::~VcVideoPaintItem()
{
}

QVideoSink *VcVideoPaintItem::videoSink() const
{
    return m_sink;
}

bool VcVideoPaintItem::hasFrame() const
{
    return m_hasFrame;
}

int VcVideoPaintItem::framesReceived() const
{
    return m_framesReceived;
}

QString VcVideoPaintItem::errorText() const
{
    return m_errorText;
}

void VcVideoPaintItem::paint( QPainter *painter )
{
    m_updateScheduled = false;
    m_lastPaintMs = m_clock.elapsed();

    const bool canPaint = ( painter != nullptr ) && m_pendingValid;

    if ( canPaint ) {
        if ( m_paintedSerial != m_pendingSerial ) {
            const QImage img = m_pendingFrame.toImage();
            if ( img.isNull() ) {
                setHasFrame( false );
                setErrorText( QStringLiteral( "Preview is not available in software rendering mode for this camera format." ) );
                m_lastImage = QImage();
            } else {
                setHasFrame( true );
                setErrorText( QString() );
                m_lastImage = img;
            }

            m_paintedSerial = m_pendingSerial;
        }

        if ( !m_lastImage.isNull() ) {
            const QRectF dst = boundingRect();
            const QRectF target = aspectFitRect( m_lastImage.size(), dst );
            if ( !target.isEmpty() ) {
                painter->drawImage( target, m_lastImage );
            }
        }
    }
}

void VcVideoPaintItem::onVideoFrameChanged( const QVideoFrame &frame )
{
    if ( frame.isValid() ) {
        m_pendingFrame = frame;
        m_pendingValid = true;
        m_pendingSerial += 1;

        m_framesReceived += 1;
        Q_EMIT framesReceivedChanged();
    } else {
        m_pendingFrame = QVideoFrame();
        m_pendingValid = false;
        m_pendingSerial += 1;
        m_lastImage = QImage();
        setHasFrame( false );
        setErrorText( QString() );
    }

    requestRepaint();
}

void VcVideoPaintItem::onThrottleTimeout()
{
    update();
}

void VcVideoPaintItem::setHasFrame( const bool hasFrame )
{
    if ( m_hasFrame != hasFrame ) {
        m_hasFrame = hasFrame;
        Q_EMIT hasFrameChanged();
    }
}

void VcVideoPaintItem::setErrorText( const QString &text )
{
    if ( m_errorText != text ) {
        m_errorText = text;
        Q_EMIT errorTextChanged();
    }
}

void VcVideoPaintItem::requestRepaint()
{
    if ( !m_updateScheduled ) {
        const qint64 now = m_clock.elapsed();
        const qint64 elapsedSincePaint = now - m_lastPaintMs;
        const qint64 dueIn = static_cast<qint64>( m_targetIntervalMs ) - elapsedSincePaint;

        m_updateScheduled = true;

        if ( dueIn <= 0 ) {
            update();
        } else {
            if ( m_throttleTimer != nullptr ) {
                m_throttleTimer->start( static_cast<int>( dueIn ) );
            } else {
                update();
            }
        }
    }
}
