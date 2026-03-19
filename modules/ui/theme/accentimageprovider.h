#ifndef MODULES_UI_THEME_ACCENTIMAGEPROVIDER_H
#define MODULES_UI_THEME_ACCENTIMAGEPROVIDER_H

#include <QQuickImageProvider>

class AccentImageProvider final : public QQuickImageProvider
{
public:
    AccentImageProvider();

    QImage requestImage( const QString &id, QSize *size, const QSize &requestedSize ) override;

private:
    QImage renderPlane( double hueDegrees, const QSize &requestedSize );
    QImage renderHueBar( const QSize &requestedSize );
};

#endif
