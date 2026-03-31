#include "modules/app/settings/apppreferences.h"

#include <QSettings>

#include <QtGlobal>

namespace {

const QString kSettingsDisplayNameKey = QStringLiteral( "ui/profile/displayName" );
const QString kSettingsThemeModeKey = QStringLiteral( "ui/theme/mode" );
const QString kSettingsAccentKey = QStringLiteral( "ui/theme/accent" );
const QString kSettingsFontPresetKey = QStringLiteral( "ui/appearance/fontPreset" );
const QString kSettingsFontFamilyKey = QStringLiteral( "ui/appearance/fontFamily" );
const QString kSettingsFontScalePercentKey = QStringLiteral( "ui/appearance/fontScalePercent" );
const QString kSettingsDensityKey = QStringLiteral( "ui/appearance/density" );
const QString kSettingsZoomPercentKey = QStringLiteral( "ui/appearance/zoomPercent" );
const QString kSettingsCustomAccentLightnessKey = QStringLiteral( "ui/theme/customAccentLightness" );
const QString kSettingsCustomAccentChromaKey = QStringLiteral( "ui/theme/customAccentChroma" );
const QString kSettingsCustomAccentHueDegreesKey = QStringLiteral( "ui/theme/customAccentHueDegrees" );
const QString kSettingsSoftwareRenderingEnabledKey = QStringLiteral( "ui/compat/softwareRendering" );

}

AppPreferences::AppPreferences( QObject *parent )
    : QObject( parent )
{
    m_displayName = QString();
    m_themeMode = QStringLiteral( "victorian" );
    m_accent = QStringLiteral( "victorian" );
    m_fontPreset = QStringLiteral( "victorian" );
    m_fontFamily = QString();
    m_fontScalePercent = 100;
    m_density = QStringLiteral( "comfortable" );
    m_zoomPercent = 100;
    m_customAccentLightness = 0.75;
    m_customAccentChroma = 0.16;
    m_customAccentHueDegrees = 260.0;
    m_softwareRenderingEnabled = false;

    reload();
}

bool AppPreferences::softwareRenderingEnabled() const
{
    return m_softwareRenderingEnabled;
}

void AppPreferences::setSoftwareRenderingEnabled( const bool enabled )
{
    if ( m_softwareRenderingEnabled != enabled ) {
        m_softwareRenderingEnabled = enabled;
        Q_EMIT softwareRenderingEnabledChanged();
    }
}

bool AppPreferences::softwareRenderingActive() const
{
    // This reflects the current process state (not the persisted preference).
    // The software renderer is enabled via the Qt Quick backend adaptation.
    const QString v = qEnvironmentVariable( "QT_QUICK_BACKEND" );
    return v.compare( QStringLiteral( "software" ), Qt::CaseInsensitive ) == 0;
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

QString AppPreferences::fontPreset() const
{
    return m_fontPreset;
}

void AppPreferences::setFontPreset( const QString &preset )
{
    QString clamped;

    clamped = preset;
    if ( clamped != QStringLiteral( "system" )
         && clamped != QStringLiteral( "victorian" )
         && clamped != QStringLiteral( "custom" ) ) {
        clamped = QStringLiteral( "victorian" );
    }

    if ( m_fontPreset != clamped ) {
        m_fontPreset = clamped;
        Q_EMIT fontPresetChanged();
    }
}

QString AppPreferences::fontFamily() const
{
    return m_fontFamily;
}

void AppPreferences::setFontFamily( const QString &family )
{
    if ( m_fontFamily != family ) {
        m_fontFamily = family;
        Q_EMIT fontFamilyChanged();
    }
}

int AppPreferences::fontScalePercent() const
{
    return m_fontScalePercent;
}

void AppPreferences::setFontScalePercent( const int percent )
{
    int clamped;

    clamped = percent;

    if ( clamped < 75 ) {
        clamped = 75;
    }
    if ( clamped > 150 ) {
        clamped = 150;
    }

    if ( m_fontScalePercent != clamped ) {
        m_fontScalePercent = clamped;
        Q_EMIT fontScalePercentChanged();
    }
}

QString AppPreferences::density() const
{
    return m_density;
}

void AppPreferences::setDensity( const QString &density )
{
    QString clamped;

    clamped = density;
    if ( clamped != QStringLiteral( "compact" )
         && clamped != QStringLiteral( "comfortable" )
         && clamped != QStringLiteral( "spacious" ) ) {
        clamped = QStringLiteral( "comfortable" );
    }

    if ( m_density != clamped ) {
        m_density = clamped;
        Q_EMIT densityChanged();
    }
}

int AppPreferences::zoomPercent() const
{
    return m_zoomPercent;
}

void AppPreferences::setZoomPercent( const int percent )
{
    int clamped;

    clamped = percent;

    if ( clamped < 50 ) {
        clamped = 50;
    }
    if ( clamped > 200 ) {
        clamped = 200;
    }

    if ( m_zoomPercent != clamped ) {
        m_zoomPercent = clamped;
        Q_EMIT zoomPercentChanged();
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
    setThemeMode( settings.value( kSettingsThemeModeKey, m_themeMode ).toString() );
    setAccent( settings.value( kSettingsAccentKey, m_accent ).toString() );
    setFontPreset( settings.value( kSettingsFontPresetKey, m_fontPreset ).toString() );
    setFontFamily( settings.value( kSettingsFontFamilyKey, QString() ).toString() );
    setFontScalePercent( settings.value( kSettingsFontScalePercentKey, m_fontScalePercent ).toInt() );
    setDensity( settings.value( kSettingsDensityKey, QStringLiteral( "comfortable" ) ).toString() );
    setZoomPercent( settings.value( kSettingsZoomPercentKey, m_zoomPercent ).toInt() );
    setCustomAccentLightness( settings.value( kSettingsCustomAccentLightnessKey, m_customAccentLightness ).toDouble() );
    setCustomAccentChroma( settings.value( kSettingsCustomAccentChromaKey, m_customAccentChroma ).toDouble() );
    setCustomAccentHueDegrees( settings.value( kSettingsCustomAccentHueDegreesKey, m_customAccentHueDegrees ).toDouble() );

    setSoftwareRenderingEnabled( settings.value( kSettingsSoftwareRenderingEnabledKey, m_softwareRenderingEnabled ).toBool() );
}

void AppPreferences::save()
{
    QSettings settings;

    settings.setValue( kSettingsDisplayNameKey, m_displayName );
    settings.setValue( kSettingsThemeModeKey, m_themeMode );
    settings.setValue( kSettingsAccentKey, m_accent );
    settings.setValue( kSettingsFontPresetKey, m_fontPreset );
    settings.setValue( kSettingsFontFamilyKey, m_fontFamily );
    settings.setValue( kSettingsFontScalePercentKey, m_fontScalePercent );
    settings.setValue( kSettingsDensityKey, m_density );
    settings.setValue( kSettingsZoomPercentKey, m_zoomPercent );
    settings.setValue( kSettingsCustomAccentLightnessKey, m_customAccentLightness );
    settings.setValue( kSettingsCustomAccentChromaKey, m_customAccentChroma );
    settings.setValue( kSettingsCustomAccentHueDegreesKey, m_customAccentHueDegrees );

    settings.setValue( kSettingsSoftwareRenderingEnabledKey, m_softwareRenderingEnabled );
    settings.sync();
}
