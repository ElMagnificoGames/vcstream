#ifndef MODULES_DEVICES_CATALOGUE_DEVICEMEMORYCACHE_H
#define MODULES_DEVICES_CATALOGUE_DEVICEMEMORYCACHE_H

#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QVariantList>

class DeviceMemoryCache final
{
public:
    struct DeviceInfo {
        QString id;
        QString name;
        bool isDefault;
    };

public:
    explicit DeviceMemoryCache( const int capacity = 20 );

    void setCapacity( const int capacity );
    int capacity() const;

    void clear();

    void updateFromSnapshot( const QList<DeviceInfo> &snapshot );
    void appendMissingToVariantList( QVariantList *inOutList, const QSet<QString> &snapshotIds ) const;

private:
    struct KnownDevice {
        QString id;
        QString name;
        bool isDefault;
        bool available;
        quint64 lastSeenSerial;
    };

    void evictIfNeeded();

private:
    int m_capacity;
    quint64 m_serial;
    QHash<QString, KnownDevice> m_knownById;
};

#endif
