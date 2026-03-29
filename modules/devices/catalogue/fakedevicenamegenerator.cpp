#include "modules/devices/catalogue/fakedevicenamegenerator.h"

#include <QByteArray>

namespace {

constexpr quint32 kDefaultSeed = 0xC0FFEEu;

}

FakeDeviceNameGenerator::FakeDeviceNameGenerator()
    : FakeDeviceNameGenerator( seedFromEnvironmentOrDefault( "VCSTREAM_DEBUG_TEST_SEED", kDefaultSeed ) )
{
}

FakeDeviceNameGenerator::FakeDeviceNameGenerator( const quint32 seed )
    : m_rng( seed )
{
}

quint32 FakeDeviceNameGenerator::seedFromEnvironmentOrDefault( const char *envName, const quint32 defaultSeed )
{
    quint32 seed;
    const QByteArray seedBytes = qgetenv( envName );

    seed = defaultSeed;

    if ( !seedBytes.isEmpty() ) {
        bool ok;
        const QString seedText = QString::fromLatin1( seedBytes );
        const quint32 parsedSeed = seedText.toUInt( &ok, 0 );

        if ( ok ) {
            seed = parsedSeed;
        }
    }

    return seed;
}

const QStringList &FakeDeviceNameGenerator::fixedNames()
{
    static const QStringList k = {
        QStringLiteral( "alpha" ),
        QStringLiteral( "bravo" ),
        QStringLiteral( "charlie" ),
        QStringLiteral( "delta" ),
        QStringLiteral( "echo" ),
        QStringLiteral( "foxtrot" ),
        QStringLiteral( "golf" ),
        QStringLiteral( "hotel" ),
        QStringLiteral( "india" ),
        QStringLiteral( "juliett" ),
        QStringLiteral( "kilo" ),
        QStringLiteral( "lima" ),
        QStringLiteral( "mike" ),
        QStringLiteral( "november" ),
        QStringLiteral( "oscar" ),
        QStringLiteral( "papa" ),
        QStringLiteral( "quebec" ),
        QStringLiteral( "romeo" ),
        QStringLiteral( "sierra" ),
        QStringLiteral( "tango" ),
        QStringLiteral( "uniform" ),
        QStringLiteral( "victor" ),
        QStringLiteral( "whiskey" ),
        QStringLiteral( "x-ray" ),
        QStringLiteral( "yankee" ),
        QStringLiteral( "zulu" ),
    };

    return k;
}

QStringList FakeDeviceNameGenerator::generateSubset()
{
    QStringList out;
    const QStringList &names = fixedNames();

    // Choose a subset via a bitmask.
    quint32 mask;
    mask = m_rng.generate();

    for ( int i = 0; i < names.size(); ++i ) {
        if ( ( mask & 1u ) != 0u ) {
            out.append( names.at( i ) );
        }
        mask >>= 1;
    }

    if ( out.isEmpty() ) {
        const int idx = m_rng.bounded( static_cast<int>( names.size() ) );
        out.append( names.at( idx ) );
    }

    return out;
}

bool FakeDeviceNameGenerator::maybeUpdateNames( QStringList *names, const int keepPercent )
{
    bool changed;

    changed = false;

    if ( names == nullptr ) {
        changed = false;
    } else {
        int keep;
        keep = keepPercent;

        if ( keep < 0 ) {
            keep = 0;
        }

        if ( keep > 100 ) {
            keep = 100;
        }

        if ( names->isEmpty() ) {
            *names = generateSubset();
            changed = true;
        } else {
            // keepPercent% keep, (100-keepPercent)% replace.
            if ( m_rng.bounded( 100 ) >= keep ) {
                *names = generateSubset();
                changed = true;
            }
        }
    }

    return changed;
}
