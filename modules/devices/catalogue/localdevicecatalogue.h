#ifndef MODULES_DEVICES_CATALOGUE_LOCALDEVICECATALOGUE_H
#define MODULES_DEVICES_CATALOGUE_LOCALDEVICECATALOGUE_H

#include <QObject>
#include <QVariantList>

class LocalDeviceCatalogue final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QVariantList screens READ screens NOTIFY screensChanged )
    Q_PROPERTY( QVariantList cameras READ cameras NOTIFY camerasChanged )
    Q_PROPERTY( QVariantList microphones READ microphones NOTIFY microphonesChanged )
    Q_PROPERTY( QVariantList audioOutputs READ audioOutputs NOTIFY audioOutputsChanged )
    Q_PROPERTY( QVariantList windows READ windows NOTIFY windowsChanged )
    Q_PROPERTY( QString windowCaptureStatus READ windowCaptureStatus NOTIFY windowCaptureStatusChanged )

public:
    explicit LocalDeviceCatalogue( QObject *parent = nullptr );

    QVariantList screens() const;
    QVariantList cameras() const;
    QVariantList microphones() const;
    QVariantList audioOutputs() const;
    QVariantList windows() const;
    QString windowCaptureStatus() const;

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void screensChanged();
    void camerasChanged();
    void microphonesChanged();
    void audioOutputsChanged();
    void windowsChanged();
    void windowCaptureStatusChanged();

private:
    void setScreens( const QVariantList &screens );
    void setCameras( const QVariantList &cameras );
    void setMicrophones( const QVariantList &microphones );
    void setAudioOutputs( const QVariantList &audioOutputs );
    void setWindows( const QVariantList &windows );
    void setWindowCaptureStatus( const QString &status );

private:
    QVariantList m_screens;
    QVariantList m_cameras;
    QVariantList m_microphones;
    QVariantList m_audioOutputs;
    QVariantList m_windows;
    QString m_windowCaptureStatus;
};

#endif
