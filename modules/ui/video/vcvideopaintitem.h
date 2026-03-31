#ifndef MODULES_UI_VIDEO_VCVIDEOPAINTITEM_H
#define MODULES_UI_VIDEO_VCVIDEOPAINTITEM_H

#include <QElapsedTimer>
#include <QImage>
#include <QQuickPaintedItem>
#include <QString>

#include <QVideoFrame>
#include <QVideoSink>

class QPainter;
class QTimer;

// A software-renderer-friendly video preview item.
//
// In Qt Quick software scenegraph mode, Qt Multimedia's VideoOutput can fail to
// render frames even though QVideoSink receives them. This item provides a
// fallback that draws the latest frame via QPainter.
class VcVideoPaintItem : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY( QVideoSink *videoSink READ videoSink CONSTANT )
    Q_PROPERTY( bool hasFrame READ hasFrame NOTIFY hasFrameChanged )
    Q_PROPERTY( int framesReceived READ framesReceived NOTIFY framesReceivedChanged )
    Q_PROPERTY( QString errorText READ errorText NOTIFY errorTextChanged )

public:
    explicit VcVideoPaintItem( QQuickItem *parent = nullptr );
    ~VcVideoPaintItem() override;

    QVideoSink *videoSink() const;

    bool hasFrame() const;
    int framesReceived() const;
    QString errorText() const;

    void paint( QPainter *painter ) override;

Q_SIGNALS:
    void hasFrameChanged();
    void framesReceivedChanged();
    void errorTextChanged();

private Q_SLOTS:
    void onVideoFrameChanged( const QVideoFrame &frame );
    void onThrottleTimeout();

private:
    void setHasFrame( bool hasFrame );
    void setErrorText( const QString &text );
    void requestRepaint();

private:
    QVideoSink *m_sink;
    QTimer *m_throttleTimer;
    QElapsedTimer m_clock;

    QVideoFrame m_pendingFrame;
    bool m_pendingValid;
    int m_pendingSerial;
    int m_paintedSerial;

    QImage m_lastImage;
    bool m_hasFrame;
    int m_framesReceived;
    QString m_errorText;

    bool m_updateScheduled;
    qint64 m_lastPaintMs;
    int m_targetIntervalMs;
};

#endif
