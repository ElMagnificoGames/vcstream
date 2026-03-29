#ifndef MODULES_DEVICES_CATALOGUE_WINDOWCAPTUREWATCHER_H
#define MODULES_DEVICES_CATALOGUE_WINDOWCAPTUREWATCHER_H

#include <memory>

#include <QObject>
#include <QString>
#include <QVariantList>

class WindowCaptureWatcher final : public QObject
{
    Q_OBJECT

public:
    explicit WindowCaptureWatcher( QObject *parent = nullptr );
    ~WindowCaptureWatcher() override;

    void refreshNow();
    void refreshTitlesIfDirty();

Q_SIGNALS:
    void windowSnapshotAvailable( const QVariantList &windows, const QString &windowCaptureStatus );

private:
    void onPollTick();
    void onTitleRefreshTick();

private:
    struct WindowInfo {
        QString id;
        QString name;
    };

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif
