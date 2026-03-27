#ifndef MODULES_DEVICES_CAPTURE_NULLCAMERABACKEND_H
#define MODULES_DEVICES_CAPTURE_NULLCAMERABACKEND_H

#include "modules/devices/capture/camerabackend.h"

class NullCameraBackend final : public CameraBackend
{
public:
    NullCameraBackend() = default;
    ~NullCameraBackend() override = default;

    QString descriptionForId( const QString &deviceId ) const override;
    std::unique_ptr<CameraStreamBackend> createStream( const QString &deviceId, QObject *parent ) const override;
};

#endif
