#ifndef MODULES_DEVICES_CAPTURE_CAMERASTREAMBACKEND_H
#define MODULES_DEVICES_CAPTURE_CAMERASTREAMBACKEND_H

#include <QObject>

class QVideoFrame;

class CameraStreamBackend : public QObject
{
    Q_OBJECT

public:
    explicit CameraStreamBackend( QObject *parent = nullptr )
        : QObject( parent )
    {
    }

    ~CameraStreamBackend() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;

Q_SIGNALS:
    void videoFrameChanged( const QVideoFrame &frame );
};

#endif
