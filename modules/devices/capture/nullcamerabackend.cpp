#include "modules/devices/capture/nullcamerabackend.h"

#include "modules/devices/capture/camerastreambackend.h"

#include <QColor>
#include <QImage>
#include <QTimer>
#include <QVideoFrame>

namespace {

class NullCameraStreamBackend final : public CameraStreamBackend
{
    Q_OBJECT

public:
    explicit NullCameraStreamBackend( QObject *parent )
        : CameraStreamBackend( parent )
        , m_timer( nullptr )
        , m_frameCounter( 0 )
    {
        m_timer = new QTimer( this );
        m_timer->setInterval( 66 );
        QObject::connect( m_timer, &QTimer::timeout, this, &NullCameraStreamBackend::onTick );
    }

    void start() override
    {
        m_timer->start();
    }

    void stop() override
    {
        m_timer->stop();
    }

private Q_SLOTS:
    void onTick()
    {
        QImage img;
        QColor fill;
        QVideoFrame frame;

        img = QImage( 320, 180, QImage::Format_ARGB32_Premultiplied );
        fill = QColor::fromHsl( ( m_frameCounter * 19 ) % 360, 160, 120 );
        img.fill( fill );

        frame = QVideoFrame( img );
        m_frameCounter += 1;

        Q_EMIT videoFrameChanged( frame );
    }

private:
    QTimer *m_timer;
    int m_frameCounter;
};

}

QString NullCameraBackend::descriptionForId( const QString &deviceId ) const
{
    Q_UNUSED( deviceId )
    return QStringLiteral( "Null camera" );
}

std::unique_ptr<CameraStreamBackend> NullCameraBackend::createStream( const QString &deviceId, QObject *parent ) const
{
    Q_UNUSED( deviceId )

    return std::unique_ptr<CameraStreamBackend>( new NullCameraStreamBackend( parent ) );
}

#include "nullcamerabackend.moc"
