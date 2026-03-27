#ifndef MODULES_DEVICES_CAPTURE_MEDIACAPTURE_H
#define MODULES_DEVICES_CAPTURE_MEDIACAPTURE_H

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>

#include <memory>

#include "modules/devices/capture/camerabackend.h"

class QVideoFrame;
class QVideoSink;

class CameraPreviewHandle;

class MediaCapture final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( int activeCameraCount READ activeCameraCount NOTIFY activeCameraCountChanged )

public:
    explicit MediaCapture( QObject *parent = nullptr );
    explicit MediaCapture( std::unique_ptr<CameraBackend> cameraBackend, QObject *parent = nullptr );

    Q_INVOKABLE QObject *acquireCameraPreviewHandle( const QString &deviceId, QObject *owner );

    int activeCameraCount() const;

Q_SIGNALS:
    void activeCameraCountChanged();

private:
    class CameraStream;

    friend class CameraPreviewHandle;

    void registerViewSink( CameraPreviewHandle &handle );
    void unregisterViewSink( CameraPreviewHandle &handle );
    void setHandleRunning( CameraPreviewHandle &handle, bool running );
    QString cameraDescriptionForId( const QString &deviceId ) const;

    CameraStream *ensureStreamForDeviceId( const QString &deviceId );

private:
    QHash<QString, CameraStream *> m_cameraStreams;
    std::unique_ptr<CameraBackend> m_cameraBackend;
};

#endif
