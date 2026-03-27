#include "modules/app/lifecycle/appsupervisor.h"

#include <algorithm>

#include <QCoreApplication>
#include <QFontDatabase>
#include <QIcon>

#include "modules/devices/catalogue/localdevicecatalogue.h"
#include "modules/app/settings/apppreferences.h"
#include "modules/devices/capture/mediacapture.h"
#include "modules/ui/fonts/bundledfonts.h"
#include "modules/ui/fonts/fontpreviewsafetycache.h"

AppSupervisor::AppSupervisor( QObject *parent )
    : QObject( parent )
{
    bundledfonts::ensureRegistered();
    const bundledfonts::Families f = bundledfonts::families();
    m_victorianBodyFontFamily = f.victorianBody;
    m_victorianHeadingFontFamily = f.victorianHeading;

    m_joinRoomEnabled = false;
    m_hostRoomEnabled = false;

    m_preferences = new AppPreferences( this );
    m_deviceCatalogue = new LocalDeviceCatalogue( this );
    m_mediaCapture = new MediaCapture( this );
    m_fontPreviewSafetyCache = new FontPreviewSafetyCache( this );
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

QObject *AppSupervisor::mediaCapture() const
{
    return m_mediaCapture;
}

QObject *AppSupervisor::fontPreviewSafetyCache() const
{
    return m_fontPreviewSafetyCache;
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

QStringList AppSupervisor::fontFamilies() const
{
    QFontDatabase db;
    QStringList out = db.families();

    std::sort( out.begin(), out.end(), []( const QString &a, const QString &b ) {
        return a.compare( b, Qt::CaseInsensitive ) < 0;
    } );

    out.removeDuplicates();
    return out;
}

QString AppSupervisor::systemFontFamily() const
{
    return QFontDatabase::systemFont( QFontDatabase::GeneralFont ).family();
}

QString AppSupervisor::victorianBodyFontFamily() const
{
    return m_victorianBodyFontFamily;
}

QString AppSupervisor::victorianHeadingFontFamily() const
{
    return m_victorianHeadingFontFamily;
}
