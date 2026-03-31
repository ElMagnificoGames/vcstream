#include "modules/devices/capture/camerapreviewhandle.h"

#include "modules/devices/capture/mediacapture.h"

CameraPreviewHandle::CameraPreviewHandle( MediaCapture &owner, const QString &deviceId, QObject *parent )
    : QObject( parent )
    , m_owner( owner )
    , m_deviceId( deviceId )
    , m_backendName( deviceId.startsWith( QStringLiteral( "dshow:" ) ) ? QStringLiteral( "DirectShow" ) : QStringLiteral( "Qt Multimedia" ) )
    , m_description()
    , m_errorText()
    , m_viewSink( nullptr )
    , m_running( false )
    , m_registered( false )
    , m_hasFrame( false )
    , m_framesReceived( 0 )
{
    m_description = QString();
    m_errorText = QString();
}

CameraPreviewHandle::~CameraPreviewHandle()
{
    setRunning( false );
    if ( m_registered ) {
        m_owner.unregisterViewSink( *this );
        m_registered = false;
    }
}

QString CameraPreviewHandle::deviceId() const
{
    return m_deviceId;
}

QString CameraPreviewHandle::backendName() const
{
    return m_backendName;
}

QString CameraPreviewHandle::description() const
{
    return m_description;
}

QVideoSink *CameraPreviewHandle::viewSink() const
{
    return m_viewSink;
}

bool CameraPreviewHandle::running() const
{
    return m_running;
}

void CameraPreviewHandle::setRunning( const bool running )
{
    if ( m_running != running ) {
        m_running = running;
        m_owner.setHandleRunning( *this, m_running );
        Q_EMIT runningChanged();
    }
}

QString CameraPreviewHandle::errorText() const
{
    return m_errorText;
}

void CameraPreviewHandle::setErrorText( const QString &text )
{
    if ( m_errorText != text ) {
        m_errorText = text;
        Q_EMIT errorTextChanged();
    }
}

bool CameraPreviewHandle::hasFrame() const
{
    return m_hasFrame;
}

int CameraPreviewHandle::framesReceived() const
{
    return m_framesReceived;
}

void CameraPreviewHandle::setViewSink( QVideoSink *sink )
{
    if ( m_viewSink != sink ) {
        if ( m_registered ) {
            m_owner.unregisterViewSink( *this );
            m_registered = false;
        }

        m_viewSink = sink;
        if ( m_viewSink != nullptr ) {
            m_owner.registerViewSink( *this );
        }
    }
}

void CameraPreviewHandle::close()
{
    setRunning( false );
    setViewSink( nullptr );
    deleteLater();
}

void CameraPreviewHandle::setDescription( const QString &description )
{
    if ( m_description != description ) {
        m_description = description;
        Q_EMIT descriptionChanged();
    }
}

void CameraPreviewHandle::setHasFrame( const bool hasFrame )
{
    if ( m_hasFrame != hasFrame ) {
        m_hasFrame = hasFrame;
        Q_EMIT hasFrameChanged();
    }
}

void CameraPreviewHandle::setFramesReceived( const int frames )
{
    if ( m_framesReceived != frames ) {
        m_framesReceived = frames;
        Q_EMIT framesReceivedChanged();
    }
}
