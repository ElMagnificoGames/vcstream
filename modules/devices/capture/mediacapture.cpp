#include "modules/devices/capture/mediacapture.h"

#include "modules/devices/capture/camerabackend.h"
#include "modules/devices/capture/camerapreviewhandle.h"
#include "modules/devices/capture/camerastreambackend.h"
#include "modules/devices/capture/qtcamerabackend.h"
#include "modules/devices/capture/routingcamerabackend.h"
#include "modules/devices/capture/dshowcamerabackend.h"

#include <QList>
#include <QPointer>
#include <QVideoFrame>
#include <QVideoSink>

#include <limits>

class MediaCapture::CameraStream final : public QObject
{
    Q_OBJECT

public:
    explicit CameraStream( const QString &deviceId,
        const QString &description,
        CameraStreamBackend &backend,
        QObject *parent )
        : QObject( parent )
        , m_deviceId( deviceId )
        , m_description( description )
        , m_backend( &backend )
        , m_lastFrame()
        , m_errorText()
        , m_framesReceived( 0 )
        , m_runningRefCount( 0 )
        , m_active( false )
        , m_viewSinks()
        , m_handles()
    {
        QObject::connect( m_backend, &CameraStreamBackend::videoFrameChanged, this, &CameraStream::onVideoFrameChanged );
        QObject::connect( m_backend, &CameraStreamBackend::errorTextChanged, this, &CameraStream::onBackendErrorTextChanged );

        m_errorText = m_backend->errorText();
    }

    QString deviceId() const
    {
        return m_deviceId;
    }

    QString description() const
    {
        return m_description;
    }

    void addViewSink( QVideoSink *sink )
    {
        if ( sink != nullptr ) {
            if ( !m_viewSinks.contains( sink ) ) {
                m_viewSinks.append( sink );

                if ( m_lastFrame.isValid() ) {
                    sink->setVideoFrame( m_lastFrame );
                }
            }
        }
    }

    void removeViewSink( QVideoSink *sink )
    {
        if ( sink != nullptr ) {
            m_viewSinks.removeAll( sink );
        }
    }

    void setActive( const bool active )
    {
        if ( m_active != active ) {
            m_active = active;

            if ( m_active ) {
                m_backend->start();
            } else {
                m_backend->stop();
                m_lastFrame = QVideoFrame();
                m_framesReceived = 0;

                for ( const QPointer<QVideoSink> &sink : m_viewSinks ) {
                    if ( sink != nullptr ) {
                        sink->setVideoFrame( QVideoFrame() );
                    }
                }

                for ( const QPointer<CameraPreviewHandle> &h : m_handles ) {
                    if ( h != nullptr ) {
                        h->setHasFrame( false );
                        h->setFramesReceived( m_framesReceived );
                    }
                }
            }
        }
    }

    QString errorText() const
    {
        return m_errorText;
    }

    void addHandle( CameraPreviewHandle &handle )
    {
        if ( !m_handles.contains( &handle ) ) {
            m_handles.append( &handle );
        }

        handle.setErrorText( m_errorText );
        handle.setHasFrame( m_lastFrame.isValid() );
        handle.setFramesReceived( m_framesReceived );
    }

    void removeHandle( CameraPreviewHandle &handle )
    {
        m_handles.removeAll( &handle );
    }

    int runningRefCount() const
    {
        return m_runningRefCount;
    }

    void incrementRunningRefCount( const int delta )
    {
        int next;

        next = m_runningRefCount;
        next += delta;
        if ( next < 0 ) {
            next = 0;
        }

        m_runningRefCount = next;
    }

    bool isUnused() const
    {
        return m_viewSinks.isEmpty() && ( m_runningRefCount <= 0 );
    }

private Q_SLOTS:
    void onVideoFrameChanged( const QVideoFrame &frame )
    {
        m_lastFrame = frame;
        m_framesReceived += 1;

        QList<QPointer<CameraPreviewHandle>> remainingHandles;
        remainingHandles.reserve( m_handles.size() );

        for ( const QPointer<CameraPreviewHandle> &h : m_handles ) {
            if ( h != nullptr ) {
                h->setHasFrame( true );
                h->setFramesReceived( m_framesReceived );
                remainingHandles.append( h );
            }
        }

        m_handles = remainingHandles;

        QList<QPointer<QVideoSink>> remaining;
        remaining.reserve( m_viewSinks.size() );

        for ( const QPointer<QVideoSink> &sink : m_viewSinks ) {
            if ( sink != nullptr ) {
                sink->setVideoFrame( frame );
                remaining.append( sink );
            }
        }

        m_viewSinks = remaining;
    }

    void onBackendErrorTextChanged()
    {
        const QString next = m_backend->errorText();
        if ( m_errorText != next ) {
            m_errorText = next;

            QList<QPointer<CameraPreviewHandle>> remaining;
            remaining.reserve( m_handles.size() );

            for ( const QPointer<CameraPreviewHandle> &h : m_handles ) {
                if ( h != nullptr ) {
                    h->setErrorText( m_errorText );
                    remaining.append( h );
                }
            }

            m_handles = remaining;

            if ( !m_errorText.isEmpty() ) {
                for ( const QPointer<QVideoSink> &sink : m_viewSinks ) {
                    if ( sink != nullptr ) {
                        sink->setVideoFrame( QVideoFrame() );
                    }
                }
            }
        }
    }

private:
    const QString m_deviceId;
    const QString m_description;
    CameraStreamBackend *m_backend;

    QVideoFrame m_lastFrame;
    QString m_errorText;
    int m_framesReceived;
    int m_runningRefCount;
    bool m_active;
    QList<QPointer<QVideoSink>> m_viewSinks;
    QList<QPointer<CameraPreviewHandle>> m_handles;
};

MediaCapture::MediaCapture( QObject *parent )
#if defined( Q_OS_WIN )
    : MediaCapture(
          std::make_unique<RoutingCameraBackend>(
              std::make_unique<QtCameraBackend>(),
              std::make_unique<DShowCameraBackend>() ),
          parent )
#else
    : MediaCapture( std::make_unique<QtCameraBackend>(), parent )
#endif
{
}

MediaCapture::MediaCapture( std::unique_ptr<CameraBackend> cameraBackend, QObject *parent )
    : QObject( parent )
    , m_cameraStreams()
    , m_cameraBackend( std::move( cameraBackend ) )
{
    if ( m_cameraBackend == nullptr ) {
        m_cameraBackend = std::make_unique<QtCameraBackend>();
    }
}

QObject *MediaCapture::acquireCameraPreviewHandle( const QString &deviceId, QObject *owner )
{
    QObject *out;
    QObject *resolvedOwner;

    out = nullptr;
    resolvedOwner = owner;

    if ( resolvedOwner == nullptr ) {
        resolvedOwner = this;
    }

    if ( !deviceId.isEmpty() ) {
        CameraPreviewHandle *handle;

        handle = new CameraPreviewHandle( *this, deviceId, resolvedOwner );
        handle->setDescription( cameraDescriptionForId( deviceId ) );
        out = handle;
    }

    return out;
}

int MediaCapture::activeCameraCount() const
{
    int out;
    const qsizetype s = m_cameraStreams.size();

    out = 0;

    if ( s > std::numeric_limits<int>::max() ) {
        out = std::numeric_limits<int>::max();
    } else {
        out = static_cast<int>( s );
    }

    return out;
}

void MediaCapture::registerViewSink( CameraPreviewHandle &handle )
{
    if ( handle.viewSink() != nullptr ) {
        CameraStream *stream;

        stream = ensureStreamForDeviceId( handle.deviceId() );
        if ( stream != nullptr ) {
            stream->addHandle( handle );
            stream->addViewSink( handle.viewSink() );
            handle.m_registered = true;
        } else {
            handle.setErrorText( QStringLiteral( "Camera device is not available." ) );
        }
    }
}

void MediaCapture::unregisterViewSink( CameraPreviewHandle &handle )
{
    CameraStream *stream;

    stream = m_cameraStreams.value( handle.deviceId(), nullptr );
    if ( stream != nullptr ) {
        if ( handle.viewSink() != nullptr ) {
            stream->removeViewSink( handle.viewSink() );
        }
        stream->removeHandle( handle );
        handle.m_registered = false;

        if ( stream->isUnused() ) {
            m_cameraStreams.remove( handle.deviceId() );
            stream->setActive( false );
            stream->deleteLater();
            Q_EMIT activeCameraCountChanged();
        }
    }
}

void MediaCapture::setHandleRunning( CameraPreviewHandle &handle, const bool running )
{
    CameraStream *stream;

    stream = nullptr;

    if ( running ) {
        stream = ensureStreamForDeviceId( handle.deviceId() );
    } else {
        stream = m_cameraStreams.value( handle.deviceId(), nullptr );
    }

    if ( stream != nullptr ) {
        stream->incrementRunningRefCount( running ? 1 : -1 );
        stream->setActive( stream->runningRefCount() > 0 );

        if ( stream->isUnused() ) {
            m_cameraStreams.remove( handle.deviceId() );
            stream->setActive( false );
            stream->deleteLater();
            Q_EMIT activeCameraCountChanged();
        }
    } else {
        if ( running ) {
            handle.setErrorText( QStringLiteral( "Camera device is not available." ) );
        }
    }
}

QString MediaCapture::cameraDescriptionForId( const QString &deviceId ) const
{
    QString out;

    out = QString();

    if ( !deviceId.isEmpty() ) {
        out = m_cameraBackend->descriptionForId( deviceId );
    }

    return out;
}

MediaCapture::CameraStream *MediaCapture::ensureStreamForDeviceId( const QString &deviceId )
{
    CameraStream *out;

    out = nullptr;

    if ( !deviceId.isEmpty() ) {
        if ( m_cameraStreams.contains( deviceId ) ) {
            out = m_cameraStreams.value( deviceId );
        } else {
            std::unique_ptr<CameraStreamBackend> backend = m_cameraBackend->createStream( deviceId, nullptr );
            if ( backend != nullptr ) {
                CameraStreamBackend *backendRaw;
                backendRaw = backend.release();
                backendRaw->setParent( this );

                const QString description = m_cameraBackend->descriptionForId( deviceId );
                CameraStream *stream;

                stream = new CameraStream( deviceId, description, *backendRaw, this );
                backendRaw->setParent( stream );

                m_cameraStreams.insert( deviceId, stream );
                Q_EMIT activeCameraCountChanged();

                out = stream;
            }
        }
    }

    return out;
}

#include "mediacapture.moc"
