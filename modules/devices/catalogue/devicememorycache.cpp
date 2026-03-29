#include "modules/devices/catalogue/devicememorycache.h"

#include <algorithm>

#include <QList>
#include <QSet>
#include <QVariantMap>

DeviceMemoryCache::DeviceMemoryCache( const int capacity )
    : m_capacity( 20 )
    , m_serial( 0 )
    , m_knownById()
{
    setCapacity( capacity );
}

void DeviceMemoryCache::setCapacity( const int capacity )
{
    int next;

    next = capacity;
    if ( next < 0 ) {
        next = 0;
    }

    m_capacity = next;

    evictIfNeeded();
}

int DeviceMemoryCache::capacity() const
{
    return m_capacity;
}

void DeviceMemoryCache::clear()
{
    m_knownById.clear();
    m_serial = 0;
}

void DeviceMemoryCache::updateFromSnapshot( const QList<DeviceInfo> &snapshot )
{
    // Mark everything missing, then re-mark seen entries as available.
    for ( auto it = m_knownById.begin(); it != m_knownById.end(); ++it ) {
        it->available = false;
        it->isDefault = false;
    }

    for ( const DeviceInfo &d : snapshot ) {
        if ( !d.id.isEmpty() ) {
            if ( m_knownById.contains( d.id ) ) {
                KnownDevice kd;

                kd = m_knownById.value( d.id );
                kd.available = true;
                kd.name = d.name;
                kd.isDefault = d.isDefault;
                kd.lastSeenSerial = ( m_serial + 1 );

                m_serial += 1;
                m_knownById.insert( d.id, kd );
            } else {
                KnownDevice kd;

                kd.id = d.id;
                kd.name = d.name;
                kd.isDefault = d.isDefault;
                kd.available = true;
                kd.lastSeenSerial = ( m_serial + 1 );

                m_serial += 1;
                m_knownById.insert( d.id, kd );
            }
        }
    }

    evictIfNeeded();
}

void DeviceMemoryCache::appendMissingToVariantList( QVariantList *inOutList, const QSet<QString> &snapshotIds ) const
{
    QList<KnownDevice> missing;

    if ( inOutList != nullptr ) {
        for ( const KnownDevice &kd : m_knownById ) {
            if ( !snapshotIds.contains( kd.id ) ) {
                missing.append( kd );
            }
        }

        std::sort( missing.begin(), missing.end(), []( const KnownDevice &a, const KnownDevice &b ) {
            bool out;

            out = false;

            const int nameCmp = a.name.compare( b.name, Qt::CaseInsensitive );
            if ( nameCmp < 0 ) {
                out = true;
            } else {
                if ( nameCmp > 0 ) {
                    out = false;
                } else {
                    out = ( a.id < b.id );
                }
            }

            return out;
        } );

        for ( const KnownDevice &kd : missing ) {
            QVariantMap m;

            m.insert( QStringLiteral( "name" ), kd.name );
            m.insert( QStringLiteral( "id" ), kd.id );
            m.insert( QStringLiteral( "isDefault" ), false );
            m.insert( QStringLiteral( "available" ), false );
            inOutList->append( m );
        }
    }
}

void DeviceMemoryCache::evictIfNeeded()
{
    if ( m_capacity <= 0 ) {
        m_knownById.clear();
    } else {
        while ( m_knownById.size() > m_capacity ) {
            QString victimId;
            bool victimAvailable;
            quint64 victimSerial;
            bool hasVictim;

            victimId = QString();
            victimAvailable = true;
            victimSerial = 0;
            hasVictim = false;

            for ( auto it = m_knownById.constBegin(); it != m_knownById.constEnd(); ++it ) {
                const KnownDevice kd = it.value();

                if ( !hasVictim ) {
                    victimId = kd.id;
                    victimAvailable = kd.available;
                    victimSerial = kd.lastSeenSerial;
                    hasVictim = true;
                } else {
                    bool prefer;

                    // Prefer evicting missing devices first.
                    prefer = false;
                    if ( victimAvailable && !kd.available ) {
                        prefer = true;
                    } else {
                        if ( victimAvailable == kd.available ) {
                            prefer = ( kd.lastSeenSerial < victimSerial );
                        }
                    }

                    if ( prefer ) {
                        victimId = kd.id;
                        victimAvailable = kd.available;
                        victimSerial = kd.lastSeenSerial;
                    }
                }
            }

            if ( hasVictim ) {
                m_knownById.remove( victimId );
            } else {
                m_knownById.clear();
            }
        }
    }
}
