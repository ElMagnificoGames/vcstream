#include "modules/devices/capture/dshowcamerabackend.h"

#include <QtGlobal>

#if !defined( Q_OS_WIN )

QString DShowCameraBackend::descriptionForId( const QString &deviceId ) const
{
    Q_UNUSED( deviceId )
    return QString();
}

std::unique_ptr<CameraStreamBackend> DShowCameraBackend::createStream( const QString &deviceId, QObject *parent ) const
{
    Q_UNUSED( deviceId )
    Q_UNUSED( parent )
    return nullptr;
}

#endif
