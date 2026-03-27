#include <QtTest>

#include <QCoreApplication>
#include <QVideoFrame>
#include <QVideoSink>

#include <memory>

#include "modules/devices/capture/mediacapture.h"
#include "modules/devices/capture/camerapreviewhandle.h"
#include "modules/devices/capture/nullcamerabackend.h"

class tst_MediaCapture final : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openCloseOpen_againYieldsFrames();
    void multipleHandles_sameDevice_shareStream();
};

void tst_MediaCapture::openCloseOpen_againYieldsFrames()
{
    MediaCapture capture( std::make_unique<NullCameraBackend>() );
    QVideoSink sink;

    bool gotFirst;
    bool gotSecond;

    gotFirst = false;
    gotSecond = false;

    QObject::connect( &sink, &QVideoSink::videoFrameChanged, this, [&]( const QVideoFrame &frame ) {
        Q_UNUSED( frame )
        if ( !gotFirst ) {
            gotFirst = true;
        } else {
            gotSecond = true;
        }
    } );

    QObject *h1Obj;
    h1Obj = capture.acquireCameraPreviewHandle( QStringLiteral( "cam0" ), &capture );
    QVERIFY( h1Obj != nullptr );

    auto *h1 = qobject_cast<CameraPreviewHandle *>( h1Obj );
    QVERIFY( h1 != nullptr );

    h1->setViewSink( &sink );
    h1->setRunning( true );

    QTRY_VERIFY_WITH_TIMEOUT( gotFirst, 1500 );

    h1->close();
    QCoreApplication::processEvents();

    QObject *h2Obj;
    h2Obj = capture.acquireCameraPreviewHandle( QStringLiteral( "cam0" ), &capture );
    QVERIFY( h2Obj != nullptr );

    auto *h2 = qobject_cast<CameraPreviewHandle *>( h2Obj );
    QVERIFY( h2 != nullptr );

    h2->setViewSink( &sink );
    h2->setRunning( true );

    QTRY_VERIFY_WITH_TIMEOUT( gotSecond, 1500 );

    h2->close();
}

void tst_MediaCapture::multipleHandles_sameDevice_shareStream()
{
    MediaCapture capture( std::make_unique<NullCameraBackend>() );
    QVideoSink sinkA;
    QVideoSink sinkB;

    QObject *h1Obj;
    QObject *h2Obj;

    h1Obj = capture.acquireCameraPreviewHandle( QStringLiteral( "cam0" ), &capture );
    h2Obj = capture.acquireCameraPreviewHandle( QStringLiteral( "cam0" ), &capture );
    QVERIFY( h1Obj != nullptr );
    QVERIFY( h2Obj != nullptr );

    auto *h1 = qobject_cast<CameraPreviewHandle *>( h1Obj );
    auto *h2 = qobject_cast<CameraPreviewHandle *>( h2Obj );
    QVERIFY( h1 != nullptr );
    QVERIFY( h2 != nullptr );

    h1->setViewSink( &sinkA );
    h2->setViewSink( &sinkB );

    h1->setRunning( true );
    h2->setRunning( true );

    QCOMPARE( capture.activeCameraCount(), 1 );

    h1->close();
    QCoreApplication::processEvents();

    QCOMPARE( capture.activeCameraCount(), 1 );

    h2->close();
    QTRY_VERIFY_WITH_TIMEOUT( capture.activeCameraCount() == 0, 1500 );
}

QTEST_MAIN( tst_MediaCapture )

#include "tst_mediacapture.moc"
