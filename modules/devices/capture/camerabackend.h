#ifndef MODULES_DEVICES_CAPTURE_CAMERABACKEND_H
#define MODULES_DEVICES_CAPTURE_CAMERABACKEND_H

#include <QString>

#include <memory>

class CameraStreamBackend;
class QObject;

class CameraBackend
{
public:
    CameraBackend() = default;
    virtual ~CameraBackend() = default;

    CameraBackend( const CameraBackend & ) = delete;
    CameraBackend &operator=( const CameraBackend & ) = delete;

    virtual QString descriptionForId( const QString &deviceId ) const = 0;
    virtual std::unique_ptr<CameraStreamBackend> createStream( const QString &deviceId, QObject *parent ) const = 0;
};

#endif
