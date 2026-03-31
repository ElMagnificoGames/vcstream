#include "modules/devices/catalogue/localdevicecatalogue.h"

#include "modules/devices/catalogue/fakedevicenamegenerator.h"
#include "modules/platform/shim/qtmultimediarescan.h"
#include "modules/platform/shim/dshowcameraenum.h"

#include <algorithm>

#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMediaDevices>
#include <QScreen>
#include <QString>
#include <QTimer>
#include <QVariantMap>

#include <QSet>

#include <QAudioDevice>
#include <QCameraDevice>

#include <QRect>

#include "modules/devices/catalogue/windowcapturewatcher.h"

namespace {

bool mediaEnumerationFaked()
{
    // -1 means unknown. Cache the env var result for the process lifetime.
    // The user-facing contract is "read at startup".
    static int m_mediaEnumerationDisabled = -1;

    if ( m_mediaEnumerationDisabled < 0 ) {
        const QByteArray v = qgetenv( "VCSTREAM_DEBUG_FAKE_MEDIA_ENUM" );
        const bool enabled = ( v == "1" || v == "true" || v == "yes" );
        m_mediaEnumerationDisabled = enabled ? 1 : 0;
    }

    return ( m_mediaEnumerationDisabled != 0 );
}

QString idToStableString( const QByteArray &id )
{
    return QString::fromLatin1( id.toHex() );
}

QString screenToStableId( QScreen *screen )
{
    QString out;

    out = QString();

    if ( screen != nullptr ) {
        const quintptr addr = reinterpret_cast<quintptr>( screen );
        out = QStringLiteral( "s%1" ).arg( QString::number( static_cast<qulonglong>( addr ), 16 ) );
    }

    return out;
}

bool isVideoInputsRescanPermanentlyUnsupported( const QString &debugText )
{
    bool out;

    out = false;

    if ( !debugText.isEmpty() ) {
        // These strings are authored in our shim and represent permanent conditions
        // that will not change at runtime.
        out = debugText.contains( QStringLiteral( "unavailable on this platform" ) )
            || debugText.contains( QStringLiteral( "private headers not available" ) );
    }

    return out;
}

bool deviceInfoLessByNameThenId( const DeviceMemoryCache::DeviceInfo &a, const DeviceMemoryCache::DeviceInfo &b )
{
    bool out;

    out = false;

    const int nameCmp = a.name.compare( b.name, Qt::CaseInsensitive );
    if ( nameCmp < 0 ) {
        out = true;
    } else {
        if ( nameCmp > 0 ) {
            out = false;
        } else {
            out = ( a.id < b.id );
        }
    }

    return out;
}

}

struct LocalDeviceCatalogue::FakeState {
    FakeDeviceNameGenerator nameGen;
    QStringList screenNames;
    QStringList cameraNames;
    QStringList microphoneNames;
    QStringList audioOutputNames;
    QStringList windowNames;
};

LocalDeviceCatalogue::LocalDeviceCatalogue( QObject *parent )
    : QObject( parent )
    , m_mediaDevices( nullptr )
    , m_refreshTimer( nullptr )
    , m_videoInputsRescanTimer( nullptr )
    , m_videoInputsRescanSupported( false )
    , m_videoInputsRescanDebugText()
    , m_windowWatcher( nullptr )
    , m_cameraMemory( 20 )
    , m_microphoneMemory( 20 )
    , m_audioOutputMemory( 20 )
    , m_fake()
{
    const bool fake = mediaEnumerationFaked();
    m_windowCaptureStatus = QString();
    m_cameraDiscoveryStatus = QString();

    m_refreshTimer = new QTimer( this );

    // In fake mode we want a steady cadence of updates to stress the UI.
    m_refreshTimer->setSingleShot( !fake );
    m_refreshTimer->setInterval( fake ? 10 : 0 );

    QObject::connect( m_refreshTimer, &QTimer::timeout, this, &LocalDeviceCatalogue::refresh );

    QGuiApplication *guiApp;

    guiApp = qobject_cast<QGuiApplication *>( QCoreApplication::instance() );
    if ( guiApp != nullptr ) {
        if ( !fake ) {
            m_mediaDevices = new QMediaDevices( this );

            QObject::connect( m_mediaDevices, &QMediaDevices::videoInputsChanged, this, &LocalDeviceCatalogue::scheduleRefresh );
            QObject::connect( m_mediaDevices, &QMediaDevices::audioInputsChanged, this, &LocalDeviceCatalogue::scheduleRefresh );
            QObject::connect( m_mediaDevices, &QMediaDevices::audioOutputsChanged, this, &LocalDeviceCatalogue::scheduleRefresh );

            // Screen hotplug / geometry changes are routed through the GUI application.
            QObject::connect( guiApp, &QGuiApplication::screenAdded, this, &LocalDeviceCatalogue::scheduleRefresh );
            QObject::connect( guiApp, &QGuiApplication::screenRemoved, this, &LocalDeviceCatalogue::scheduleRefresh );

            // Some Linux virtual cameras (for example OBS + v4l2loopback) do not
            // trigger Qt's `/dev`-watcher when they start producing formats.
            // Proactively ask the backend to re-check video inputs on a bounded
            // cadence so `videoInputsChanged` can fire when the camera becomes
            // enumerable.
#if defined( Q_OS_LINUX )
            m_videoInputsRescanTimer = new QTimer( this );
            m_videoInputsRescanTimer->setInterval( 2000 );
            QObject::connect( m_videoInputsRescanTimer, &QTimer::timeout, this, &LocalDeviceCatalogue::onVideoInputsRescanTick );

            m_videoInputsRescanSupported = qtmultimediarescan::requestVideoInputsRescan( &m_videoInputsRescanDebugText );
            if ( m_videoInputsRescanSupported && m_videoInputsRescanTimer != nullptr ) {
                m_videoInputsRescanTimer->start();
            } else {
                if ( m_videoInputsRescanTimer != nullptr && !isVideoInputsRescanPermanentlyUnsupported( m_videoInputsRescanDebugText ) ) {
                    // Retry in case the Qt Multimedia backend is not initialised yet.
                    m_videoInputsRescanTimer->start();
                }
            }

            if ( !m_videoInputsRescanSupported && isVideoInputsRescanPermanentlyUnsupported( m_videoInputsRescanDebugText ) ) {
                setCameraDiscoveryStatus( m_videoInputsRescanDebugText );
            }
#endif

            ensureWindowWatcherIfNeeded();
        }
    }

    scheduleRefresh();
}

LocalDeviceCatalogue::~LocalDeviceCatalogue() = default;

void LocalDeviceCatalogue::scheduleRefresh()
{
    if ( m_refreshTimer != nullptr ) {
        if ( !m_refreshTimer->isActive() ) {
            m_refreshTimer->start();
        }
    }
}

void LocalDeviceCatalogue::ensureWindowWatcherIfNeeded()
{
    if ( m_windowWatcher == nullptr ) {
        m_windowWatcher = new WindowCaptureWatcher( this );

        QObject::connect( m_windowWatcher, &WindowCaptureWatcher::windowSnapshotAvailable, this,
            [this]( const QVariantList &windows, const QString &windowCaptureStatus ) {
                if ( !mediaEnumerationFaked() ) {
                    setWindows( windows );
                    setWindowCaptureStatus( windowCaptureStatus );
                }
            } );
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

QString LocalDeviceCatalogue::cameraDiscoveryStatus() const
{
    return m_cameraDiscoveryStatus;
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

void LocalDeviceCatalogue::setCameraDiscoveryStatus( const QString &status )
{
    if ( m_cameraDiscoveryStatus != status ) {
        m_cameraDiscoveryStatus = status;
        Q_EMIT cameraDiscoveryStatusChanged();
    }
}

void LocalDeviceCatalogue::refresh()
{
    const bool hasGuiApp = ( qobject_cast<QGuiApplication *>( QCoreApplication::instance() ) != nullptr );
    const bool fake = mediaEnumerationFaked();

    // Qt Multimedia device enumeration is UI/platform backed and is not safe to
    // run under a pure QCoreApplication (for example, unit tests).
    if ( !hasGuiApp ) {
        m_cameraMemory.clear();
        m_microphoneMemory.clear();
        m_audioOutputMemory.clear();

        setScreens( QVariantList() );
        setCameras( QVariantList() );
        setMicrophones( QVariantList() );
        setAudioOutputs( QVariantList() );
        setWindows( QVariantList() );

        setWindowCaptureStatus( QStringLiteral( "Device enumeration requires a GUI application." ) );
        setCameraDiscoveryStatus( QString() );
    } else if ( fake ) {
        // Fake enumeration: designed to stress the UI while keeping the same
        // QML-facing schema as the real catalogue.

        m_cameraMemory.clear();
        m_microphoneMemory.clear();
        m_audioOutputMemory.clear();

        if ( !m_fake ) {
            m_fake.reset( new FakeState() );
        }

        m_fake->nameGen.maybeUpdateNames( &m_fake->screenNames, 90 );
        m_fake->nameGen.maybeUpdateNames( &m_fake->cameraNames, 90 );
        m_fake->nameGen.maybeUpdateNames( &m_fake->microphoneNames, 90 );
        m_fake->nameGen.maybeUpdateNames( &m_fake->audioOutputNames, 90 );
        m_fake->nameGen.maybeUpdateNames( &m_fake->windowNames, 90 );

        {
            QVariantList screens;

            for ( int i = 0; i < m_fake->screenNames.size(); ++i ) {
                const QString name = m_fake->screenNames.at( i );

                QVariantMap m;
                m.insert( QStringLiteral( "id" ), QStringLiteral( "fake-screen:%1" ).arg( name ) );
                m.insert( QStringLiteral( "name" ), name );
                m.insert( QStringLiteral( "geometry" ), QRect( i * 1920, 0, 1920, 1080 ) );
                m.insert( QStringLiteral( "availableGeometry" ), QRect( i * 1920, 0, 1920, 1040 ) );
                m.insert( QStringLiteral( "devicePixelRatio" ), 1.0 );
                screens.append( m );
            }

            setScreens( screens );
        }

        {
            QVariantList cameras;

            for ( int i = 0; i < m_fake->cameraNames.size(); ++i ) {
                const QString name = m_fake->cameraNames.at( i );

                QVariantMap m;
                m.insert( QStringLiteral( "name" ), name );
                m.insert( QStringLiteral( "id" ), QStringLiteral( "fake-camera:%1" ).arg( name ) );
                m.insert( QStringLiteral( "isDefault" ), ( i == 0 ) );
                m.insert( QStringLiteral( "available" ), true );
                cameras.append( m );
            }

            setCameras( cameras );
        }

        {
            QVariantList microphones;

            for ( int i = 0; i < m_fake->microphoneNames.size(); ++i ) {
                const QString name = m_fake->microphoneNames.at( i );

                QVariantMap m;
                m.insert( QStringLiteral( "name" ), name );
                m.insert( QStringLiteral( "id" ), QStringLiteral( "fake-microphone:%1" ).arg( name ) );
                m.insert( QStringLiteral( "isDefault" ), ( i == 0 ) );
                m.insert( QStringLiteral( "available" ), true );
                microphones.append( m );
            }

            setMicrophones( microphones );
        }

        {
            QVariantList outputs;

            for ( int i = 0; i < m_fake->audioOutputNames.size(); ++i ) {
                const QString name = m_fake->audioOutputNames.at( i );

                QVariantMap m;
                m.insert( QStringLiteral( "name" ), name );
                m.insert( QStringLiteral( "id" ), QStringLiteral( "fake-audio-output:%1" ).arg( name ) );
                m.insert( QStringLiteral( "isDefault" ), ( i == 0 ) );
                m.insert( QStringLiteral( "available" ), true );
                outputs.append( m );
            }

            setAudioOutputs( outputs );
        }

        {
            QVariantList windows;

            for ( const QString &name : m_fake->windowNames ) {
                QVariantMap m;
                m.insert( QStringLiteral( "name" ), name );
                m.insert( QStringLiteral( "id" ), QStringLiteral( "fake-window:%1" ).arg( name ) );
                windows.append( m );
            }

            setWindows( windows );
        }

        setWindowCaptureStatus( QString() );
        setCameraDiscoveryStatus( QString() );
    } else {
        QVariantList screens;

        {
            QGuiApplication *guiApp;

            guiApp = qobject_cast<QGuiApplication *>( QCoreApplication::instance() );
            if ( guiApp != nullptr ) {
                const QList<QScreen *> screenList = QGuiApplication::screens();
                for ( QScreen *screen : screenList ) {
                    if ( screen != nullptr ) {
                        QVariantMap m;
                        m.insert( QStringLiteral( "id" ), screenToStableId( screen ) );
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

            QList<DeviceMemoryCache::DeviceInfo> snapshot;
            QSet<QString> snapshotIds;
            QSet<QString> qtNameKeys;

            for ( const QCameraDevice &d : cameraList ) {
                const QString id = idToStableString( d.id() );

                DeviceMemoryCache::DeviceInfo info;
                info.id = id;
                info.name = d.description();
                info.isDefault = ( d == defaultCamera );
                snapshot.append( info );
                snapshotIds.insert( id );

                if ( !info.name.isEmpty() ) {
                    qtNameKeys.insert( info.name.toCaseFolded() );
                }
            }

            // On Windows, some virtual cameras are exposed through DirectShow but do not
            // appear in Qt Multimedia's `QMediaDevices::videoInputs()`.
            {
                QString dshowDebug;
                const QVector<dshowcameraenum::VideoInput> dshowInputs = dshowcameraenum::enumerateVideoInputs( &dshowDebug );

                for ( const dshowcameraenum::VideoInput &v : dshowInputs ) {
                    if ( !v.stableId.isEmpty() && !v.name.isEmpty() ) {
                        const QString nameKey = v.name.toCaseFolded();
                        if ( !qtNameKeys.contains( nameKey ) ) {
                            DeviceMemoryCache::DeviceInfo info;
                            info.id = v.stableId;
                            info.name = v.name;
                            info.isDefault = false;
                            snapshot.append( info );
                            snapshotIds.insert( info.id );
                        }
                    }
                }
            }

            std::sort( snapshot.begin(), snapshot.end(), deviceInfoLessByNameThenId );

            for ( const DeviceMemoryCache::DeviceInfo &info : snapshot ) {
                QVariantMap m;
                m.insert( QStringLiteral( "name" ), info.name );
                m.insert( QStringLiteral( "id" ), info.id );
                m.insert( QStringLiteral( "isDefault" ), info.isDefault );
                m.insert( QStringLiteral( "available" ), true );
                cameras.append( m );
            }

            m_cameraMemory.updateFromSnapshot( snapshot );
            m_cameraMemory.appendMissingToVariantList( &cameras, snapshotIds );

            setCameras( cameras );
        }

        // The rescan shim is a Linux-specific workaround; on other platforms its
        // "unavailable" status is expected and not user-actionable.
#if defined( Q_OS_LINUX )
        // Treat cameraDiscoveryStatus as a capability/diagnostic indicator rather
        // than an empty-state message. When the rescan mechanism is permanently
        // unavailable, surface it even if cameras exist.
        if ( !m_videoInputsRescanSupported && isVideoInputsRescanPermanentlyUnsupported( m_videoInputsRescanDebugText ) ) {
            setCameraDiscoveryStatus( m_videoInputsRescanDebugText );
        } else {
            setCameraDiscoveryStatus( QString() );
        }
#else
        setCameraDiscoveryStatus( QString() );
#endif

        {
            QVariantList microphones;
            const QList<QAudioDevice> micList = QMediaDevices::audioInputs();
            const QAudioDevice defaultMic = QMediaDevices::defaultAudioInput();

            QList<DeviceMemoryCache::DeviceInfo> snapshot;
            QSet<QString> snapshotIds;

            for ( const QAudioDevice &d : micList ) {
                const QString id = idToStableString( d.id() );

                DeviceMemoryCache::DeviceInfo info;
                info.id = id;
                info.name = d.description();
                info.isDefault = ( d == defaultMic );
                snapshot.append( info );
                snapshotIds.insert( id );
            }

            std::sort( snapshot.begin(), snapshot.end(), deviceInfoLessByNameThenId );

            for ( const DeviceMemoryCache::DeviceInfo &info : snapshot ) {
                QVariantMap m;
                m.insert( QStringLiteral( "name" ), info.name );
                m.insert( QStringLiteral( "id" ), info.id );
                m.insert( QStringLiteral( "isDefault" ), info.isDefault );
                m.insert( QStringLiteral( "available" ), true );
                microphones.append( m );
            }

            m_microphoneMemory.updateFromSnapshot( snapshot );
            m_microphoneMemory.appendMissingToVariantList( &microphones, snapshotIds );

            setMicrophones( microphones );
        }

        {
            QVariantList outputs;
            const QList<QAudioDevice> outList = QMediaDevices::audioOutputs();
            const QAudioDevice defaultOut = QMediaDevices::defaultAudioOutput();

            QList<DeviceMemoryCache::DeviceInfo> snapshot;
            QSet<QString> snapshotIds;

            for ( const QAudioDevice &d : outList ) {
                const QString id = idToStableString( d.id() );

                DeviceMemoryCache::DeviceInfo info;
                info.id = id;
                info.name = d.description();
                info.isDefault = ( d == defaultOut );
                snapshot.append( info );
                snapshotIds.insert( id );
            }

            std::sort( snapshot.begin(), snapshot.end(), deviceInfoLessByNameThenId );

            for ( const DeviceMemoryCache::DeviceInfo &info : snapshot ) {
                QVariantMap m;
                m.insert( QStringLiteral( "name" ), info.name );
                m.insert( QStringLiteral( "id" ), info.id );
                m.insert( QStringLiteral( "isDefault" ), info.isDefault );
                m.insert( QStringLiteral( "available" ), true );
                outputs.append( m );
            }

            m_audioOutputMemory.updateFromSnapshot( snapshot );
            m_audioOutputMemory.appendMissingToVariantList( &outputs, snapshotIds );

            setAudioOutputs( outputs );
        }

        ensureWindowWatcherIfNeeded();

#if __has_include( <QWindowCapture> )
        if ( m_windowWatcher != nullptr ) {
            m_windowWatcher->refreshNow();
        }
#else
        setWindows( QVariantList() );
        setWindowCaptureStatus( QStringLiteral( "Window capture enumeration is not available in this Qt build." ) );
#endif
    }
}

void LocalDeviceCatalogue::onVideoInputsRescanTick()
{
    if ( m_videoInputsRescanTimer != nullptr ) {
        if ( mediaEnumerationFaked() ) {
            m_videoInputsRescanTimer->stop();
        } else {
#if !defined( Q_OS_LINUX )
            // The rescan shim is a Linux-specific workaround.
            m_videoInputsRescanTimer->stop();
            setCameraDiscoveryStatus( QString() );
#else
            // This is best-effort. When the backend detects a change, Qt will emit
            // `QMediaDevices::videoInputsChanged`, and our signal wiring will refresh
            // the QML-facing catalogue.
            const bool invoked = qtmultimediarescan::requestVideoInputsRescan( &m_videoInputsRescanDebugText );

            if ( invoked ) {
                m_videoInputsRescanSupported = true;
                setCameraDiscoveryStatus( QString() );
            } else {
                m_videoInputsRescanSupported = false;
                 
                if ( isVideoInputsRescanPermanentlyUnsupported( m_videoInputsRescanDebugText ) ) {
                    m_videoInputsRescanTimer->stop();
                    setCameraDiscoveryStatus( m_videoInputsRescanDebugText );
                }
            }
#endif
        }
    }
}
