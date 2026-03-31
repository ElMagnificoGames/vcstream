#include "modules/platform/shim/dshowcameraenum.h"

#include <QtGlobal>

#if !defined( Q_OS_WIN )

namespace dshowcameraenum {

QVector<VideoInput> enumerateVideoInputs( QString *outDebugText )
{
    QVector<VideoInput> out;

    out = QVector<VideoInput>();

    if ( outDebugText != nullptr ) {
        *outDebugText = QStringLiteral( "DirectShow camera enumeration unavailable on this platform." );
    }

    return out;
}

QString stableIdFromMonikerDisplayName( const QString &monikerDisplayName )
{
    Q_UNUSED( monikerDisplayName )
    return QString();
}

bool stableIdToMonikerDisplayName( const QString &stableId, QString *outMonikerDisplayName )
{
    Q_UNUSED( stableId )
    if ( outMonikerDisplayName != nullptr ) {
        *outMonikerDisplayName = QString();
    }
    return false;
}

bool stableIdLooksLikeDShow( const QString &stableId )
{
    Q_UNUSED( stableId )
    return false;
}

}

#endif
