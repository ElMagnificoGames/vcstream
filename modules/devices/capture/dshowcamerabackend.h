#ifndef MODULES_DEVICES_CAPTURE_DSHOWCAMERABACKEND_H
#define MODULES_DEVICES_CAPTURE_DSHOWCAMERABACKEND_H

#include "modules/devices/capture/camerabackend.h"

class DShowCameraBackend final : public CameraBackend
{
public:
    DShowCameraBackend() = default;
    ~DShowCameraBackend() override = default;

    QString descriptionForId( const QString &deviceId ) const override;
    std::unique_ptr<CameraStreamBackend> createStream( const QString &deviceId, QObject *parent ) const override;
};

#endif
