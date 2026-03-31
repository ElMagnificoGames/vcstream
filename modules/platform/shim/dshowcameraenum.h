#pragma once

#include <QString>

#include <QVector>

namespace dshowcameraenum {

struct VideoInput {
    QString stableId;
    QString name;
    QString monikerDisplayName;
};

// Enumerates DirectShow video capture devices.
//
// On Windows this returns the current snapshot of `CLSID_VideoInputDeviceCategory`.
// On other platforms it returns an empty list.
QVector<VideoInput> enumerateVideoInputs( QString *outDebugText );

// Returns a stable ID in the form `dshow:<hex>` where `<hex>` is the UTF-8 hex
// encoding of a DirectShow moniker display name.
QString stableIdFromMonikerDisplayName( const QString &monikerDisplayName );

// Decodes a `dshow:<hex>` stable ID back into its moniker display name.
bool stableIdToMonikerDisplayName( const QString &stableId, QString *outMonikerDisplayName );

bool stableIdLooksLikeDShow( const QString &stableId );

}
