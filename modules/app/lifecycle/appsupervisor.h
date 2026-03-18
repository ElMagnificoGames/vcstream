#ifndef MODULES_APP_LIFECYCLE_APPSUPERVISOR_H
#define MODULES_APP_LIFECYCLE_APPSUPERVISOR_H

#include <QObject>
#include <QString>

class AppSupervisor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString appVersion READ appVersion CONSTANT )

public:
    explicit AppSupervisor( QObject *parent = nullptr );

    QString appVersion() const;

public Q_SLOTS:
    void shutdown();
};

#endif
