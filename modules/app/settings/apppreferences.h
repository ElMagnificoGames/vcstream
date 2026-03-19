#ifndef MODULES_APP_SETTINGS_APPPREFERENCES_H
#define MODULES_APP_SETTINGS_APPPREFERENCES_H

#include <QObject>
#include <QString>

class AppPreferences final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged )
    Q_PROPERTY( QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged )
    Q_PROPERTY( QString accent READ accent WRITE setAccent NOTIFY accentChanged )
    Q_PROPERTY( double customAccentLightness READ customAccentLightness WRITE setCustomAccentLightness NOTIFY customAccentLightnessChanged )
    Q_PROPERTY( double customAccentChroma READ customAccentChroma WRITE setCustomAccentChroma NOTIFY customAccentChromaChanged )
    Q_PROPERTY( double customAccentHueDegrees READ customAccentHueDegrees WRITE setCustomAccentHueDegrees NOTIFY customAccentHueDegreesChanged )

public:
    explicit AppPreferences( QObject *parent = nullptr );

    QString displayName() const;
    void setDisplayName( const QString &name );

    QString themeMode() const;
    void setThemeMode( const QString &mode );

    QString accent() const;
    void setAccent( const QString &accent );

    double customAccentLightness() const;
    void setCustomAccentLightness( double lightness );

    double customAccentChroma() const;
    void setCustomAccentChroma( double chroma );

    double customAccentHueDegrees() const;
    void setCustomAccentHueDegrees( double hueDegrees );

Q_SIGNALS:
    void displayNameChanged();

    void themeModeChanged();
    void accentChanged();
    void customAccentLightnessChanged();
    void customAccentChromaChanged();
    void customAccentHueDegreesChanged();

public Q_SLOTS:
    void reload();
    void save();

private:
    QString m_displayName;
    QString m_themeMode;
    QString m_accent;
    double m_customAccentLightness;
    double m_customAccentChroma;
    double m_customAccentHueDegrees;
};

#endif
