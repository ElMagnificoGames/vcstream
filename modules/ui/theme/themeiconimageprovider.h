#ifndef MODULES_UI_THEME_THEMEICONIMAGEPROVIDER_H
#define MODULES_UI_THEME_THEMEICONIMAGEPROVIDER_H

#include <QQuickImageProvider>

class ThemeIconImageProvider final : public QQuickImageProvider
{
public:
    ThemeIconImageProvider();

    QImage requestImage( const QString &id, QSize *size, const QSize &requestedSize ) override;
};

#endif
