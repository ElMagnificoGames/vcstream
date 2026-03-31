#include <QByteArray>
#include <QCameraDevice>
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QMediaDevices>
#include <QString>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

#include <QAudioDevice>

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "modules/platform/shim/qtstartupshim.h"
#include "modules/platform/shim/dshowcameraenum.h"

#include "modules/devices/capture/camerapreviewhandle.h"
#include "modules/devices/capture/mediacapture.h"

namespace {

void printLine( const QString &s )
{
    const QByteArray utf8 = s.toUtf8();
    std::fwrite( utf8.constData(), 1, static_cast<size_t>( utf8.size() ), stdout );
    std::fwrite( "\n", 1, 1, stdout );
}

}

int main( int argc, char **argv )
{
    bool captureFirst;
    int captureDShowIndex;

    captureFirst = false;
    captureDShowIndex = -1;
    for ( int i = 1; i < argc; ++i ) {
        if ( std::strcmp( argv[i], "--capture-first" ) == 0 ) {
            captureFirst = true;
        } else if ( std::strcmp( argv[i], "--capture-dshow-index" ) == 0 ) {
            if ( i + 1 < argc ) {
                captureDShowIndex = std::atoi( argv[i + 1 ] );
            }
        }
    }

    qtstartupshim::applyBeforeQGuiApplication();

    QCoreApplication::setOrganizationName( QStringLiteral( "ElMagnificoGames" ) );
    QCoreApplication::setApplicationName( QStringLiteral( "vcstream_cameraprobe" ) );

    QGuiApplication app( argc, argv );

    printLine( QStringLiteral( "vcstream_cameraprobe" ) );
    printLine( QStringLiteral( "Qt version: %1" ).arg( QString::fromLatin1( qVersion() ) ) );

    const QString backend = qEnvironmentVariable( "QT_MEDIA_BACKEND" );
    if ( backend.isEmpty() ) {
        printLine( QStringLiteral( "QT_MEDIA_BACKEND: (unset)" ) );
    } else {
        printLine( QStringLiteral( "QT_MEDIA_BACKEND: %1" ).arg( backend ) );
    }

    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    const QCameraDevice def = QMediaDevices::defaultVideoInput();

    printLine( QStringLiteral( "videoInputs: %1" ).arg( cameras.size() ) );

    if ( !cameras.isEmpty() ) {
        for ( int i = 0; i < cameras.size(); ++i ) {
            const QCameraDevice d = cameras.at( i );

            const QByteArray id = d.id();
            const QString idHex = QString::fromLatin1( id.toHex() );
            const bool isDefault = ( d == def );

            printLine( QStringLiteral( "[%1] %2" ).arg( i ).arg( d.description() ) );
            printLine( QStringLiteral( "    id.hex=%1" ).arg( idHex ) );
            printLine( QStringLiteral( "    id.bytes=%1" ).arg( id.size() ) );
            printLine( QStringLiteral( "    isDefault=%1" ).arg( isDefault ? QStringLiteral( "true" ) : QStringLiteral( "false" ) ) );
        }
    }

    QString dshowDebug;
    const QVector<dshowcameraenum::VideoInput> dshowInputs = dshowcameraenum::enumerateVideoInputs( &dshowDebug );
    printLine( QStringLiteral( "dshow.videoInputs: %1" ).arg( dshowInputs.size() ) );
    if ( !dshowDebug.isEmpty() ) {
        printLine( QStringLiteral( "dshow.debug: %1" ).arg( dshowDebug ) );
    }

    for ( int i = 0; i < dshowInputs.size(); ++i ) {
        const dshowcameraenum::VideoInput v = dshowInputs.at( i );
        printLine( QStringLiteral( "[%1] %2" ).arg( i ).arg( v.name ) );
        printLine( QStringLiteral( "    id=%1" ).arg( v.stableId ) );
    }

    if ( captureFirst ) {
        QString deviceId;

        deviceId = QString();
        if ( !cameras.isEmpty() ) {
            deviceId = QString::fromLatin1( cameras.first().id().toHex() );
        } else {
            if ( captureDShowIndex >= 0 && captureDShowIndex < dshowInputs.size() ) {
                deviceId = dshowInputs.at( captureDShowIndex ).stableId;
            } else {
                if ( !dshowInputs.isEmpty() ) {
                    deviceId = dshowInputs.first().stableId;
                }
            }
        }

        if ( deviceId.isEmpty() ) {
            printLine( QStringLiteral( "captureFirst: no cameras" ) );
        } else {
            printLine( QStringLiteral( "captureFirst.id=%1" ).arg( deviceId ) );

            MediaCapture capture;
            QVideoSink sink;
            int frameCount;

            frameCount = 0;

            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot( true );
            timeout.setInterval( 3000 );

            QObject::connect( &timeout, &QTimer::timeout, &loop, &QEventLoop::quit );
            QObject::connect( &sink, &QVideoSink::videoFrameChanged, &app, [&]( const QVideoFrame &frame ) {
                Q_UNUSED( frame )
                frameCount += 1;
                if ( frameCount == 1 ) {
                    loop.quit();
                }
            } );

            QObject *handleObj;
            handleObj = capture.acquireCameraPreviewHandle( deviceId, &capture );

            auto *handle = qobject_cast<CameraPreviewHandle *>( handleObj );
            if ( handle == nullptr ) {
                printLine( QStringLiteral( "captureFirst: handle creation failed" ) );
            } else {
                handle->setViewSink( &sink );
                handle->setRunning( true );

                timeout.start();
                loop.exec();

                printLine( QStringLiteral( "captureFirst.frames=%1" ).arg( frameCount ) );

                if ( !handle->errorText().isEmpty() ) {
                    printLine( QStringLiteral( "captureFirst.error=%1" ).arg( handle->errorText() ) );
                }

                handle->close();
                QCoreApplication::processEvents();
            }
        }
    }

    {
        const QList<QAudioDevice> mics = QMediaDevices::audioInputs();
        const QAudioDevice defMic = QMediaDevices::defaultAudioInput();
        printLine( QStringLiteral( "audioInputs: %1" ).arg( mics.size() ) );
        for ( int i = 0; i < mics.size(); ++i ) {
            const QAudioDevice d = mics.at( i );
            const bool isDefault = ( d == defMic );
            printLine( QStringLiteral( "[%1] %2 (default=%3)" )
                           .arg( i )
                           .arg( d.description() )
                           .arg( isDefault ? QStringLiteral( "true" ) : QStringLiteral( "false" ) ) );
        }
    }

    {
        const QList<QAudioDevice> outs = QMediaDevices::audioOutputs();
        const QAudioDevice defOut = QMediaDevices::defaultAudioOutput();
        printLine( QStringLiteral( "audioOutputs: %1" ).arg( outs.size() ) );
        for ( int i = 0; i < outs.size(); ++i ) {
            const QAudioDevice d = outs.at( i );
            const bool isDefault = ( d == defOut );
            printLine( QStringLiteral( "[%1] %2 (default=%3)" )
                           .arg( i )
                           .arg( d.description() )
                           .arg( isDefault ? QStringLiteral( "true" ) : QStringLiteral( "false" ) ) );
        }
    }

    return 0;
}
