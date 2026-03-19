#include "modules/app/settings/apppreferences.h"

#include <QSettings>

namespace {

const QString kSettingsDisplayNameKey = QStringLiteral( "ui/profile/displayName" );
const QString kSettingsThemeModeKey = QStringLiteral( "ui/theme/mode" );
const QString kSettingsAccentKey = QStringLiteral( "ui/theme/accent" );
const QString kSettingsCustomAccentLightnessKey = QStringLiteral( "ui/theme/customAccentLightness" );
const QString kSettingsCustomAccentChromaKey = QStringLiteral( "ui/theme/customAccentChroma" );
const QString kSettingsCustomAccentHueDegreesKey = QStringLiteral( "ui/theme/customAccentHueDegrees" );

}

AppPreferences::AppPreferences( QObject *parent )
    : QObject( parent )
{
    m_displayName = QString();
    m_themeMode = QStringLiteral( "system" );
    m_accent = QStringLiteral( "system" );
    m_customAccentLightness = 0.75;
    m_customAccentChroma = 0.16;
    m_customAccentHueDegrees = 260.0;

    reload();
}

QString AppPreferences::displayName() const
{
    return m_displayName;
}

void AppPreferences::setDisplayName( const QString &name )
{
    if ( m_displayName != name ) {
        m_displayName = name;
        Q_EMIT displayNameChanged();
    }
}

QString AppPreferences::themeMode() const
{
    return m_themeMode;
}

void AppPreferences::setThemeMode( const QString &mode )
{
    if ( m_themeMode != mode ) {
        m_themeMode = mode;
        Q_EMIT themeModeChanged();
    }
}

QString AppPreferences::accent() const
{
    return m_accent;
}

void AppPreferences::setAccent( const QString &accent )
{
    if ( m_accent != accent ) {
        m_accent = accent;
        Q_EMIT accentChanged();
    }
}

double AppPreferences::customAccentLightness() const
{
    return m_customAccentLightness;
}

void AppPreferences::setCustomAccentLightness( const double lightness )
{
    double clamped;

    clamped = lightness;

    if ( clamped < 0.0 ) {
        clamped = 0.0;
    }
    if ( clamped > 1.0 ) {
        clamped = 1.0;
    }

    if ( m_customAccentLightness != clamped ) {
        m_customAccentLightness = clamped;
        Q_EMIT customAccentLightnessChanged();
    }
}

double AppPreferences::customAccentChroma() const
{
    return m_customAccentChroma;
}

void AppPreferences::setCustomAccentChroma( const double chroma )
{
    double clamped;

    clamped = chroma;

    if ( clamped < 0.0 ) {
        clamped = 0.0;
    }
    if ( clamped > 1.0 ) {
        clamped = 1.0;
    }

    if ( m_customAccentChroma != clamped ) {
        m_customAccentChroma = clamped;
        Q_EMIT customAccentChromaChanged();
    }
}

double AppPreferences::customAccentHueDegrees() const
{
    return m_customAccentHueDegrees;
}

void AppPreferences::setCustomAccentHueDegrees( const double hueDegrees )
{
    double out;

    out = hueDegrees;

    while ( out < 0.0 ) {
        out += 360.0;
    }
    while ( out >= 360.0 ) {
        out -= 360.0;
    }

    if ( m_customAccentHueDegrees != out ) {
        m_customAccentHueDegrees = out;
        Q_EMIT customAccentHueDegreesChanged();
    }
}

void AppPreferences::reload()
{
    QSettings settings;

    setDisplayName( settings.value( kSettingsDisplayNameKey, QString() ).toString() );
    setThemeMode( settings.value( kSettingsThemeModeKey, QStringLiteral( "system" ) ).toString() );
    setAccent( settings.value( kSettingsAccentKey, QStringLiteral( "system" ) ).toString() );
    setCustomAccentLightness( settings.value( kSettingsCustomAccentLightnessKey, m_customAccentLightness ).toDouble() );
    setCustomAccentChroma( settings.value( kSettingsCustomAccentChromaKey, m_customAccentChroma ).toDouble() );
    setCustomAccentHueDegrees( settings.value( kSettingsCustomAccentHueDegreesKey, m_customAccentHueDegrees ).toDouble() );
}

void AppPreferences::save()
{
    QSettings settings;

    settings.setValue( kSettingsDisplayNameKey, m_displayName );
    settings.setValue( kSettingsThemeModeKey, m_themeMode );
    settings.setValue( kSettingsAccentKey, m_accent );
    settings.setValue( kSettingsCustomAccentLightnessKey, m_customAccentLightness );
    settings.setValue( kSettingsCustomAccentChromaKey, m_customAccentChroma );
    settings.setValue( kSettingsCustomAccentHueDegreesKey, m_customAccentHueDegrees );
    settings.sync();
}
