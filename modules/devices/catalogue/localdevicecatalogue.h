#ifndef MODULES_DEVICES_CATALOGUE_LOCALDEVICECATALOGUE_H
#define MODULES_DEVICES_CATALOGUE_LOCALDEVICECATALOGUE_H

#include <QObject>
#include <QString>
#include <QVariantList>

#include <memory>

#include "modules/devices/catalogue/devicememorycache.h"

class QMediaDevices;
class QTimer;
class WindowCaptureWatcher;

class LocalDeviceCatalogue final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QVariantList screens READ screens NOTIFY screensChanged )
    Q_PROPERTY( QVariantList cameras READ cameras NOTIFY camerasChanged )
    Q_PROPERTY( QVariantList microphones READ microphones NOTIFY microphonesChanged )
    Q_PROPERTY( QVariantList audioOutputs READ audioOutputs NOTIFY audioOutputsChanged )
    Q_PROPERTY( QVariantList windows READ windows NOTIFY windowsChanged )
    Q_PROPERTY( QString windowCaptureStatus READ windowCaptureStatus NOTIFY windowCaptureStatusChanged )
    Q_PROPERTY( QString cameraDiscoveryStatus READ cameraDiscoveryStatus NOTIFY cameraDiscoveryStatusChanged )

public:
    explicit LocalDeviceCatalogue( QObject *parent = nullptr );
    ~LocalDeviceCatalogue() override;

    QVariantList screens() const;
    QVariantList cameras() const;
    QVariantList microphones() const;
    QVariantList audioOutputs() const;
    QVariantList windows() const;
    QString windowCaptureStatus() const;
    QString cameraDiscoveryStatus() const;

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void screensChanged();
    void camerasChanged();
    void microphonesChanged();
    void audioOutputsChanged();
    void windowsChanged();
    void windowCaptureStatusChanged();
    void cameraDiscoveryStatusChanged();

private:
    struct FakeState;

    void scheduleRefresh();
    void onVideoInputsRescanTick();

    void ensureWindowWatcherIfNeeded();

    void setScreens( const QVariantList &screens );
    void setCameras( const QVariantList &cameras );
    void setMicrophones( const QVariantList &microphones );
    void setAudioOutputs( const QVariantList &audioOutputs );
    void setWindows( const QVariantList &windows );
    void setWindowCaptureStatus( const QString &status );
    void setCameraDiscoveryStatus( const QString &status );

private:
    QMediaDevices *m_mediaDevices;
    QTimer *m_refreshTimer;
    QTimer *m_videoInputsRescanTimer;
    bool m_videoInputsRescanSupported;
    QString m_videoInputsRescanDebugText;

    WindowCaptureWatcher *m_windowWatcher;

    DeviceMemoryCache m_cameraMemory;
    DeviceMemoryCache m_microphoneMemory;
    DeviceMemoryCache m_audioOutputMemory;

    QVariantList m_screens;
    QVariantList m_cameras;
    QVariantList m_microphones;
    QVariantList m_audioOutputs;
    QVariantList m_windows;
    QString m_windowCaptureStatus;
    QString m_cameraDiscoveryStatus;

    // Fake-mode state (debug-only): allocated only when enabled.
    std::unique_ptr<FakeState> m_fake;
};

#endif
