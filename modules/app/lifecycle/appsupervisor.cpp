#include "modules/app/lifecycle/appsupervisor.h"

#include <QCoreApplication>

AppSupervisor::AppSupervisor( QObject *parent )
    : QObject( parent )
{
    m_joinRoomEnabled = false;
    m_hostRoomEnabled = false;
}

QString AppSupervisor::appVersion() const
{
    QString version;

    version = QCoreApplication::applicationVersion();

    return version;
}

void AppSupervisor::shutdown()
{
}

bool AppSupervisor::joinRoomEnabled() const
{
    return m_joinRoomEnabled;
}

void AppSupervisor::setJoinRoomEnabled( const bool enabled )
{
    if ( m_joinRoomEnabled != enabled ) {
        m_joinRoomEnabled = enabled;
        Q_EMIT joinRoomEnabledChanged();
    }
}

bool AppSupervisor::hostRoomEnabled() const
{
    return m_hostRoomEnabled;
}

void AppSupervisor::setHostRoomEnabled( const bool enabled )
{
    if ( m_hostRoomEnabled != enabled ) {
        m_hostRoomEnabled = enabled;
        Q_EMIT hostRoomEnabledChanged();
    }
}
