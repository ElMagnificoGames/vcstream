#ifndef MODULES_DEVICES_CAPTURE_QTCAMERABACKEND_H
#define MODULES_DEVICES_CAPTURE_QTCAMERABACKEND_H

#include "modules/devices/capture/camerabackend.h"

class QtCameraBackend final : public CameraBackend
{
public:
    QtCameraBackend() = default;
    ~QtCameraBackend() override = default;

    QString descriptionForId( const QString &deviceId ) const override;
    std::unique_ptr<CameraStreamBackend> createStream( const QString &deviceId, QObject *parent ) const override;
};

#endif
