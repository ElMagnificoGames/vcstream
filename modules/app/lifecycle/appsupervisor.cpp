#include "modules/app/lifecycle/appsupervisor.h"

#include <QCoreApplication>
#include <QIcon>

#include "modules/app/devices/localdevicecatalogue.h"
#include "modules/app/settings/apppreferences.h"

AppSupervisor::AppSupervisor( QObject *parent )
    : QObject( parent )
{
    m_joinRoomEnabled = false;
    m_hostRoomEnabled = false;

    m_preferences = new AppPreferences( this );
    m_deviceCatalogue = new LocalDeviceCatalogue( this );
}

QString AppSupervisor::appVersion() const
{
    QString version;

    version = QCoreApplication::applicationVersion();

    return version;
}

void AppSupervisor::shutdown()
{
    AppPreferences *preferences;

    preferences = qobject_cast<AppPreferences *>( m_preferences );
    if ( preferences != nullptr ) {
        preferences->save();
    }
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

QObject *AppSupervisor::preferences() const
{
    return m_preferences;
}

QObject *AppSupervisor::deviceCatalogue() const
{
    return m_deviceCatalogue;
}

bool AppSupervisor::themeIconAvailable( const QString &name ) const
{
    bool out;

    out = false;

    if ( !name.isEmpty() ) {
        out = QIcon::hasThemeIcon( name );
    }

    return out;
}
