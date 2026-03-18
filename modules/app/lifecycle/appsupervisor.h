#ifndef MODULES_APP_LIFECYCLE_APPSUPERVISOR_H
#define MODULES_APP_LIFECYCLE_APPSUPERVISOR_H

#include <QObject>
#include <QString>

class AppSupervisor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString appVersion READ appVersion CONSTANT )
    Q_PROPERTY( bool joinRoomEnabled READ joinRoomEnabled WRITE setJoinRoomEnabled NOTIFY joinRoomEnabledChanged )
    Q_PROPERTY( bool hostRoomEnabled READ hostRoomEnabled WRITE setHostRoomEnabled NOTIFY hostRoomEnabledChanged )

public:
    explicit AppSupervisor( QObject *parent = nullptr );

    QString appVersion() const;

    bool joinRoomEnabled() const;
    void setJoinRoomEnabled( bool enabled );

    bool hostRoomEnabled() const;
    void setHostRoomEnabled( bool enabled );

Q_SIGNALS:
    void joinRoomEnabledChanged();
    void hostRoomEnabledChanged();

public Q_SLOTS:
    void shutdown();

private:
    bool m_joinRoomEnabled;
    bool m_hostRoomEnabled;
};

#endif
