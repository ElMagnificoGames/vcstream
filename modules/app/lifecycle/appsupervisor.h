#ifndef MODULES_APP_LIFECYCLE_APPSUPERVISOR_H
#define MODULES_APP_LIFECYCLE_APPSUPERVISOR_H

#include <QObject>
#include <QString>
#include <QStringList>

class AppSupervisor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString appVersion READ appVersion CONSTANT )
    Q_PROPERTY( bool joinRoomEnabled READ joinRoomEnabled WRITE setJoinRoomEnabled NOTIFY joinRoomEnabledChanged )
    Q_PROPERTY( bool hostRoomEnabled READ hostRoomEnabled WRITE setHostRoomEnabled NOTIFY hostRoomEnabledChanged )
    Q_PROPERTY( QObject *preferences READ preferences CONSTANT )
    Q_PROPERTY( QObject *deviceCatalogue READ deviceCatalogue CONSTANT )
    Q_PROPERTY( QObject *mediaCapture READ mediaCapture CONSTANT )
    Q_PROPERTY( QObject *fontPreviewSafetyCache READ fontPreviewSafetyCache CONSTANT )
    Q_PROPERTY( QString systemFontFamily READ systemFontFamily CONSTANT )
    Q_PROPERTY( QString victorianBodyFontFamily READ victorianBodyFontFamily CONSTANT )
    Q_PROPERTY( QString victorianHeadingFontFamily READ victorianHeadingFontFamily CONSTANT )

public:
    explicit AppSupervisor( QObject *parent = nullptr );

    QString appVersion() const;

    bool joinRoomEnabled() const;
    void setJoinRoomEnabled( bool enabled );

    bool hostRoomEnabled() const;
    void setHostRoomEnabled( bool enabled );

    QObject *preferences() const;
    QObject *deviceCatalogue() const;
    QObject *mediaCapture() const;
    QObject *fontPreviewSafetyCache() const;
    QString systemFontFamily() const;

    QString victorianBodyFontFamily() const;
    QString victorianHeadingFontFamily() const;

    Q_INVOKABLE bool themeIconAvailable( const QString &name ) const;
    Q_INVOKABLE QStringList fontFamilies() const;

Q_SIGNALS:
    void joinRoomEnabledChanged();
    void hostRoomEnabledChanged();

public Q_SLOTS:
    void shutdown();

private:
    bool m_joinRoomEnabled;
    bool m_hostRoomEnabled;

    QObject *m_preferences;
    QObject *m_deviceCatalogue;
    QObject *m_mediaCapture;
    QObject *m_fontPreviewSafetyCache;

    QString m_victorianBodyFontFamily;
    QString m_victorianHeadingFontFamily;
};

#endif
