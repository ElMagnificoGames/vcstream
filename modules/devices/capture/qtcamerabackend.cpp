#include "modules/devices/capture/qtcamerabackend.h"

#include "modules/devices/capture/camerastreambackend.h"

#include <QByteArray>
#include <QCamera>
#include <QCameraDevice>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QVideoFrame>
#include <QVideoSink>

namespace {

QByteArray deviceIdStringToBytes( const QString &deviceId )
{
    QByteArray out;

    out = QByteArray();
    if ( !deviceId.isEmpty() ) {
        out = QByteArray::fromHex( deviceId.toLatin1() );
    }

    return out;
}

class QtCameraStreamBackend final : public CameraStreamBackend
{
public:
    explicit QtCameraStreamBackend( const QCameraDevice &device, QObject *parent )
        : CameraStreamBackend( parent )
        , m_camera( nullptr )
        , m_session( nullptr )
        , m_sink( nullptr )
    {
        m_camera = new QCamera( device, this );
        m_session = new QMediaCaptureSession( this );
        m_sink = new QVideoSink( this );

        m_session->setCamera( m_camera );
        m_session->setVideoSink( m_sink );

        QObject::connect( m_sink, &QVideoSink::videoFrameChanged, this, &QtCameraStreamBackend::videoFrameChanged );
    }

    void start() override
    {
        m_camera->start();
    }

    void stop() override
    {
        m_camera->stop();
    }

private:
    QCamera *m_camera;
    QMediaCaptureSession *m_session;
    QVideoSink *m_sink;
};

bool findCameraDevice( const QString &deviceId, QCameraDevice *out )
{
    bool found;
    QCameraDevice result;

    found = false;
    result = QCameraDevice();

    const QByteArray idBytes = deviceIdStringToBytes( deviceId );
    const QList<QCameraDevice> devices = QMediaDevices::videoInputs();

    for ( const QCameraDevice &d : devices ) {
        if ( !found ) {
            if ( d.id() == idBytes ) {
                result = d;
                found = true;
            }
        }
    }

    if ( out != nullptr ) {
        *out = result;
    }

    return found;
}

}

QString QtCameraBackend::descriptionForId( const QString &deviceId ) const
{
    QString out;
    QCameraDevice device;
    const bool found = findCameraDevice( deviceId, &device );

    out = QString();

    if ( found && !device.isNull() ) {
        out = device.description();
    }

    return out;
}

std::unique_ptr<CameraStreamBackend> QtCameraBackend::createStream( const QString &deviceId, QObject *parent ) const
{
    std::unique_ptr<CameraStreamBackend> out;
    QCameraDevice device;
    const bool found = findCameraDevice( deviceId, &device );

    out = nullptr;

    if ( found && !device.isNull() ) {
        out = std::make_unique<QtCameraStreamBackend>( device, parent );
    }

    return out;
}
