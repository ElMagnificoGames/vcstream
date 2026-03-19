#include "modules/app/devices/localdevicecatalogue.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMediaDevices>
#include <QScreen>
#include <QString>
#include <QVariantMap>

#include <QAudioDevice>
#include <QCameraDevice>

#if __has_include( <QWindowCapture> )
#include <QCapturableWindow>
#include <QWindowCapture>
#define VCSTREAM_HAS_QT_WINDOW_CAPTURE 1
#else
#define VCSTREAM_HAS_QT_WINDOW_CAPTURE 0
#endif

namespace {

bool mediaEnumerationDisabled()
{
    const QByteArray v = qgetenv( "VCSTREAM_DISABLE_MEDIA_ENUM" );
    return ( v == "1" || v == "true" || v == "yes" );
}

QString idToStableString( const QByteArray &id )
{
    return QString::fromLatin1( id.toHex() );
}

}

LocalDeviceCatalogue::LocalDeviceCatalogue( QObject *parent )
    : QObject( parent )
{
    m_windowCaptureStatus = QString();
    if ( !mediaEnumerationDisabled() ) {
        refresh();
    }
}

QVariantList LocalDeviceCatalogue::screens() const
{
    return m_screens;
}

QVariantList LocalDeviceCatalogue::cameras() const
{
    return m_cameras;
}

QVariantList LocalDeviceCatalogue::microphones() const
{
    return m_microphones;
}

QVariantList LocalDeviceCatalogue::audioOutputs() const
{
    return m_audioOutputs;
}

QVariantList LocalDeviceCatalogue::windows() const
{
    return m_windows;
}

QString LocalDeviceCatalogue::windowCaptureStatus() const
{
    return m_windowCaptureStatus;
}

void LocalDeviceCatalogue::setScreens( const QVariantList &screens )
{
    if ( m_screens != screens ) {
        m_screens = screens;
        Q_EMIT screensChanged();
    }
}

void LocalDeviceCatalogue::setCameras( const QVariantList &cameras )
{
    if ( m_cameras != cameras ) {
        m_cameras = cameras;
        Q_EMIT camerasChanged();
    }
}

void LocalDeviceCatalogue::setMicrophones( const QVariantList &microphones )
{
    if ( m_microphones != microphones ) {
        m_microphones = microphones;
        Q_EMIT microphonesChanged();
    }
}

void LocalDeviceCatalogue::setAudioOutputs( const QVariantList &audioOutputs )
{
    if ( m_audioOutputs != audioOutputs ) {
        m_audioOutputs = audioOutputs;
        Q_EMIT audioOutputsChanged();
    }
}

void LocalDeviceCatalogue::setWindows( const QVariantList &windows )
{
    if ( m_windows != windows ) {
        m_windows = windows;
        Q_EMIT windowsChanged();
    }
}

void LocalDeviceCatalogue::setWindowCaptureStatus( const QString &status )
{
    if ( m_windowCaptureStatus != status ) {
        m_windowCaptureStatus = status;
        Q_EMIT windowCaptureStatusChanged();
    }
}

void LocalDeviceCatalogue::refresh()
{
    if ( mediaEnumerationDisabled() ) {
        setScreens( QVariantList() );
        setCameras( QVariantList() );
        setMicrophones( QVariantList() );
        setAudioOutputs( QVariantList() );
        setWindows( QVariantList() );
        setWindowCaptureStatus( QStringLiteral( "Device enumeration is disabled." ) );
        return;
    }

    QVariantList screens;

    {
        QGuiApplication *guiApp;

        guiApp = qobject_cast<QGuiApplication *>( QCoreApplication::instance() );
        if ( guiApp != nullptr ) {
            const QList<QScreen *> screenList = QGuiApplication::screens();
            for ( QScreen *screen : screenList ) {
                if ( screen != nullptr ) {
                    QVariantMap m;
                    m.insert( QStringLiteral( "name" ), screen->name() );
                    m.insert( QStringLiteral( "geometry" ), screen->geometry() );
                    m.insert( QStringLiteral( "availableGeometry" ), screen->availableGeometry() );
                    m.insert( QStringLiteral( "devicePixelRatio" ), screen->devicePixelRatio() );
                    screens.append( m );
                }
            }
        }
    }

    setScreens( screens );

    {
        QVariantList cameras;
        const QList<QCameraDevice> cameraList = QMediaDevices::videoInputs();
        const QCameraDevice defaultCamera = QMediaDevices::defaultVideoInput();

        for ( const QCameraDevice &d : cameraList ) {
            QVariantMap m;
            m.insert( QStringLiteral( "name" ), d.description() );
            m.insert( QStringLiteral( "id" ), idToStableString( d.id() ) );
            m.insert( QStringLiteral( "isDefault" ), d == defaultCamera );
            cameras.append( m );
        }

        setCameras( cameras );
    }

    {
        QVariantList microphones;
        const QList<QAudioDevice> micList = QMediaDevices::audioInputs();
        const QAudioDevice defaultMic = QMediaDevices::defaultAudioInput();

        for ( const QAudioDevice &d : micList ) {
            QVariantMap m;
            m.insert( QStringLiteral( "name" ), d.description() );
            m.insert( QStringLiteral( "id" ), idToStableString( d.id() ) );
            m.insert( QStringLiteral( "isDefault" ), d == defaultMic );
            microphones.append( m );
        }

        setMicrophones( microphones );
    }

    {
        QVariantList outputs;
        const QList<QAudioDevice> outList = QMediaDevices::audioOutputs();
        const QAudioDevice defaultOut = QMediaDevices::defaultAudioOutput();

        for ( const QAudioDevice &d : outList ) {
            QVariantMap m;
            m.insert( QStringLiteral( "name" ), d.description() );
            m.insert( QStringLiteral( "id" ), idToStableString( d.id() ) );
            m.insert( QStringLiteral( "isDefault" ), d == defaultOut );
            outputs.append( m );
        }

        setAudioOutputs( outputs );
    }

#if VCSTREAM_HAS_QT_WINDOW_CAPTURE
    {
        QVariantList windows;
        const QList<QCapturableWindow> windowList = QWindowCapture::capturableWindows();

        for ( const QCapturableWindow &w : windowList ) {
            QVariantMap m;
            m.insert( QStringLiteral( "name" ), w.description() );
            windows.append( m );
        }

        setWindows( windows );
        setWindowCaptureStatus( QString() );
    }
#else
    setWindows( QVariantList() );
    setWindowCaptureStatus( QStringLiteral( "Window capture enumeration is not available in this Qt build." ) );
#endif
}
