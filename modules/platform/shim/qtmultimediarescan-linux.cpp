#include "modules/platform/shim/qtmultimediarescan.h"

#include <QCoreApplication>
#include <QMetaObject>
#include <QString>

#include <QtGlobal>

#if defined( Q_OS_LINUX )

#if __has_include( <QtMultimedia/private/qplatformmediaintegration_p.h> ) \
    && __has_include( <QtMultimedia/private/qplatformvideodevices_p.h> )
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qplatformvideodevices_p.h>
#define VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE 1
#define VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE_ABS 0
#elif __has_include( "/usr/include/qt6/QtMultimedia/6.10.2/QtMultimedia/private/qplatformmediaintegration_p.h" ) \
    && __has_include( "/usr/include/qt6/QtMultimedia/6.10.2/QtMultimedia/private/qplatformvideodevices_p.h" )
#include "/usr/include/qt6/QtMultimedia/6.10.2/QtMultimedia/private/qplatformmediaintegration_p.h"
#include "/usr/include/qt6/QtMultimedia/6.10.2/QtMultimedia/private/qplatformvideodevices_p.h"
#define VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE 1
#define VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE_ABS 1
#else
#define VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE 0
#define VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE_ABS 0
#endif

namespace qtmultimediarescan {

bool requestVideoInputsRescan()
{
    return requestVideoInputsRescan( nullptr );
}

bool requestVideoInputsRescan( QString *outDebugText )
{
    bool out;

    out = false;

    if ( QCoreApplication::instance() == nullptr ) {
        if ( outDebugText != nullptr ) {
            *outDebugText = QStringLiteral( "Camera rescan unavailable: no application instance." );
        }
    } else {
#if !VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE
        if ( outDebugText != nullptr ) {
            *outDebugText = QStringLiteral( "Camera rescan unavailable: Qt Multimedia private headers not available at build time." );
        }
#else
        QPlatformMediaIntegration *integration;
        QPlatformVideoDevices *videoDevices;
        const QMetaObject *mo;
        int checkIndex;
        bool invoked;

        integration = QPlatformMediaIntegration::instance();
        if ( integration != nullptr ) {
            videoDevices = integration->videoDevices();
            if ( videoDevices != nullptr ) {
                mo = videoDevices->metaObject();
                checkIndex = ( mo != nullptr ? mo->indexOfMethod( "checkCameras()" ) : -1 );

                invoked = false;
                if ( checkIndex >= 0 ) {
                    // Workaround: the FFmpeg/V4L2 backend only re-checks camera formats when the
                    // implementation's `checkCameras()` slot is invoked (it otherwise watches `/dev`).
                    invoked = QMetaObject::invokeMethod( videoDevices, "checkCameras", Qt::DirectConnection );
                }

                out = invoked;

                if ( outDebugText != nullptr ) {
                    const QString backendName = QString::fromLatin1( mo != nullptr ? mo->className() : "(unknown)" );
                    *outDebugText = QStringLiteral( "Camera rescan backend=%1; checkCameras=%2; invoked=%3" )
                                         .arg( backendName )
                                         .arg( checkIndex >= 0 ? QStringLiteral( "yes" ) : QStringLiteral( "no" ) )
                                         .arg( invoked ? QStringLiteral( "yes" ) : QStringLiteral( "no" ) );
                    if ( VCSTREAM_HAS_QT_MULTIMEDIA_PRIVATE_ABS != 0 ) {
                        outDebugText->append( QStringLiteral( " (abs include)" ) );
                    }
                }
            } else {
                if ( outDebugText != nullptr ) {
                    *outDebugText = QStringLiteral( "Camera rescan unavailable: no video devices backend." );
                }
            }
        } else {
            if ( outDebugText != nullptr ) {
                *outDebugText = QStringLiteral( "Camera rescan unavailable: Qt Multimedia integration not initialised." );
            }
        }
#endif
    }

    return out;
}

}

#endif
