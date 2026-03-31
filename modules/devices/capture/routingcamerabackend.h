#ifndef MODULES_DEVICES_CAPTURE_ROUTINGCAMERABACKEND_H
#define MODULES_DEVICES_CAPTURE_ROUTINGCAMERABACKEND_H

#include "modules/devices/capture/camerabackend.h"

#include <memory>

class RoutingCameraBackend final : public CameraBackend
{
public:
    explicit RoutingCameraBackend( std::unique_ptr<CameraBackend> qtBackend,
        std::unique_ptr<CameraBackend> dshowBackend );
    ~RoutingCameraBackend() override = default;

    QString descriptionForId( const QString &deviceId ) const override;
    std::unique_ptr<CameraStreamBackend> createStream( const QString &deviceId, QObject *parent ) const override;

private:
    std::unique_ptr<CameraBackend> m_qtBackend;
    std::unique_ptr<CameraBackend> m_dshowBackend;
};

#endif
