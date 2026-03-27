#ifndef MODULES_DEVICES_CAPTURE_CAMERAPREVIEWHANDLE_H
#define MODULES_DEVICES_CAPTURE_CAMERAPREVIEWHANDLE_H

#include <QObject>
#include <QString>

#include <QPointer>

#include <QVideoSink>

class MediaCapture;

class CameraPreviewHandle final : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString deviceId READ deviceId CONSTANT )
    Q_PROPERTY( QString description READ description NOTIFY descriptionChanged )
    Q_PROPERTY( bool running READ running WRITE setRunning NOTIFY runningChanged )
    Q_PROPERTY( QString errorText READ errorText NOTIFY errorTextChanged )

public:
    explicit CameraPreviewHandle( MediaCapture &owner, const QString &deviceId, QObject *parent = nullptr );
    ~CameraPreviewHandle() override;

    QString deviceId() const;
    QString description() const;
    QVideoSink *viewSink() const;

    bool running() const;
    void setRunning( bool running );

    QString errorText() const;
    void setErrorText( const QString &text );

    Q_INVOKABLE void setViewSink( QVideoSink *sink );
    Q_INVOKABLE void close();

Q_SIGNALS:
    void descriptionChanged();
    void runningChanged();
    void errorTextChanged();

private:
    friend class MediaCapture;

    void setDescription( const QString &description );

private:
    MediaCapture &m_owner;
    const QString m_deviceId;
    QString m_description;
    QString m_errorText;
    QPointer<QVideoSink> m_viewSink;
    bool m_running;
    bool m_registered;
};

#endif
