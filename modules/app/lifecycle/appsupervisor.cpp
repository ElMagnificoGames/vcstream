#include "modules/app/lifecycle/appsupervisor.h"

#include <QCoreApplication>

AppSupervisor::AppSupervisor( QObject *parent )
    : QObject( parent )
{
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
