#include "modules/devices/catalogue/devicememorycache.h"

#include <QtTest/QtTest>

class tst_DeviceMemoryCache final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void missingDevices_areAppendedWithAvailableFalse();
    void cache_isBoundedAndEvictsOldMissingFirst();
};

static DeviceMemoryCache::DeviceInfo makeInfo( const QString &id, const QString &name, const bool isDefault )
{
    DeviceMemoryCache::DeviceInfo out;

    out.id = id;
    out.name = name;
    out.isDefault = isDefault;

    return out;
}

void tst_DeviceMemoryCache::missingDevices_areAppendedWithAvailableFalse()
{
    DeviceMemoryCache cache( 20 );

    QList<DeviceMemoryCache::DeviceInfo> snapshot;
    snapshot.append( makeInfo( QStringLiteral( "a" ), QStringLiteral( "Cam A" ), true ) );
    snapshot.append( makeInfo( QStringLiteral( "b" ), QStringLiteral( "Cam B" ), false ) );

    cache.updateFromSnapshot( snapshot );

    // Now only A remains.
    snapshot.clear();
    snapshot.append( makeInfo( QStringLiteral( "a" ), QStringLiteral( "Cam A" ), true ) );
    cache.updateFromSnapshot( snapshot );

    QVariantList out;
    QSet<QString> ids;
    ids.insert( QStringLiteral( "a" ) );
    cache.appendMissingToVariantList( &out, ids );

    QCOMPARE( out.size(), 1 );
    const QVariantMap m = out.at( 0 ).toMap();
    QCOMPARE( m.value( QStringLiteral( "id" ) ).toString(), QStringLiteral( "b" ) );
    QCOMPARE( m.value( QStringLiteral( "name" ) ).toString(), QStringLiteral( "Cam B" ) );
    QCOMPARE( m.value( QStringLiteral( "available" ) ).toBool(), false );
    QCOMPARE( m.value( QStringLiteral( "isDefault" ) ).toBool(), false );
}

void tst_DeviceMemoryCache::cache_isBoundedAndEvictsOldMissingFirst()
{
    DeviceMemoryCache cache( 3 );

    QList<DeviceMemoryCache::DeviceInfo> snapshot;
    snapshot.append( makeInfo( QStringLiteral( "a" ), QStringLiteral( "Cam A" ), false ) );
    snapshot.append( makeInfo( QStringLiteral( "b" ), QStringLiteral( "Cam B" ), false ) );
    snapshot.append( makeInfo( QStringLiteral( "c" ), QStringLiteral( "Cam C" ), false ) );
    cache.updateFromSnapshot( snapshot );

    // Drop A and B, keep C.
    snapshot.clear();
    snapshot.append( makeInfo( QStringLiteral( "c" ), QStringLiteral( "Cam C" ), false ) );
    cache.updateFromSnapshot( snapshot );

    // Introduce D (forces eviction because capacity is 3).
    snapshot.clear();
    snapshot.append( makeInfo( QStringLiteral( "c" ), QStringLiteral( "Cam C" ), false ) );
    snapshot.append( makeInfo( QStringLiteral( "d" ), QStringLiteral( "Cam D" ), false ) );
    cache.updateFromSnapshot( snapshot );

    // We should have at most one missing device retained (either A or B), because C and D are available.
    QVariantList missing;
    QSet<QString> ids;
    ids.insert( QStringLiteral( "c" ) );
    ids.insert( QStringLiteral( "d" ) );
    cache.appendMissingToVariantList( &missing, ids );

    QVERIFY( missing.size() <= 1 );
    if ( missing.size() == 1 ) {
        const QString missingId = missing.at( 0 ).toMap().value( QStringLiteral( "id" ) ).toString();
        QVERIFY( missingId == QStringLiteral( "a" ) || missingId == QStringLiteral( "b" ) );
    }
}

QTEST_MAIN( tst_DeviceMemoryCache )

#include "tst_devicememorycache.moc"
