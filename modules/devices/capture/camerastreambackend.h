#ifndef MODULES_DEVICES_CAPTURE_CAMERASTREAMBACKEND_H
#define MODULES_DEVICES_CAPTURE_CAMERASTREAMBACKEND_H

#include <QObject>
#include <QString>

class QVideoFrame;

class CameraStreamBackend : public QObject
{
    Q_OBJECT

    Q_PROPERTY( QString errorText READ errorText NOTIFY errorTextChanged )

public:
    explicit CameraStreamBackend( QObject *parent = nullptr )
        : QObject( parent )
    {
    }

    ~CameraStreamBackend() override = default;

    virtual void start() = 0;
    virtual void stop() = 0;

    QString errorText() const
    {
        return m_errorText;
    }

Q_SIGNALS:
    void videoFrameChanged( const QVideoFrame &frame );
    void errorTextChanged();

protected:
    void setErrorText( const QString &text )
    {
        if ( m_errorText != text ) {
            m_errorText = text;
            Q_EMIT errorTextChanged();
        }
    }

private:
    QString m_errorText;
};

#endif
