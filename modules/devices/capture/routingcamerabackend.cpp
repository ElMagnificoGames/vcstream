#include "modules/devices/capture/routingcamerabackend.h"

#include "modules/devices/capture/camerastreambackend.h"

#include <QString>

namespace {

bool isDShowId( const QString &id )
{
    return id.startsWith( QStringLiteral( "dshow:" ) );
}

}

RoutingCameraBackend::RoutingCameraBackend( std::unique_ptr<CameraBackend> qtBackend,
    std::unique_ptr<CameraBackend> dshowBackend )
    : m_qtBackend( std::move( qtBackend ) )
    , m_dshowBackend( std::move( dshowBackend ) )
{
}

QString RoutingCameraBackend::descriptionForId( const QString &deviceId ) const
{
    QString out;

    out = QString();

    if ( isDShowId( deviceId ) ) {
        if ( m_dshowBackend != nullptr ) {
            out = m_dshowBackend->descriptionForId( deviceId );
        }
    } else {
        if ( m_qtBackend != nullptr ) {
            out = m_qtBackend->descriptionForId( deviceId );
        }
    }

    return out;
}

std::unique_ptr<CameraStreamBackend> RoutingCameraBackend::createStream( const QString &deviceId, QObject *parent ) const
{
    std::unique_ptr<CameraStreamBackend> out;

    out = nullptr;

    if ( isDShowId( deviceId ) ) {
        if ( m_dshowBackend != nullptr ) {
            out = m_dshowBackend->createStream( deviceId, parent );
        }
    } else {
        if ( m_qtBackend != nullptr ) {
            out = m_qtBackend->createStream( deviceId, parent );
        }
    }

    return out;
}
