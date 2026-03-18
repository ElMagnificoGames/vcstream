#ifndef APPS_UNITTESTS_HELPERS_TEST_RANDOM_H
#define APPS_UNITTESTS_HELPERS_TEST_RANDOM_H

#include <QByteArray>
#include <QLatin1Char>
#include <QRandomGenerator>
#include <QString>
#include <QtGlobal>

namespace testhelpers {

inline quint32 seedFromEnvironmentOrDefault( const char *envName, const quint32 defaultSeed )
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

inline QString randomAsciiString( QRandomGenerator &rng, const int minLen, const int maxLen )
{
    static const char alphabet[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "-_";

    int minLength;
    int maxLength;

    minLength = minLen;
    maxLength = maxLen;

    if ( minLength < 0 ) {
        minLength = 0;
    }

    if ( maxLength < minLength ) {
        maxLength = minLength;
    }

    const int len = rng.bounded( minLength, maxLength + 1 );
    QString out;

    out.resize( len );

    for ( int i = 0; i < len; ++i ) {
        const int index = rng.bounded( static_cast<int>( sizeof( alphabet ) - 1 ) );
        out[i] = QLatin1Char( alphabet[index] );
    }

    return out;
}

}

#endif
