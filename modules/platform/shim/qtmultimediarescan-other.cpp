#include "modules/platform/shim/qtmultimediarescan.h"

#include <QString>
#include <QtGlobal>

#if !defined( Q_OS_LINUX )

namespace qtmultimediarescan {

bool requestVideoInputsRescan()
{
    return false;
}

bool requestVideoInputsRescan( QString *outDebugText )
{
    if ( outDebugText != nullptr ) {
        *outDebugText = QStringLiteral( "Camera rescan unavailable on this platform." );
    }
    return false;
}

}

#endif
