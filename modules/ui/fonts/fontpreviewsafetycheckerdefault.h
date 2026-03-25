#ifndef MODULES_UI_FONTS_FONTPREVIEWSAFETYCHECKERDEFAULT_H
#define MODULES_UI_FONTS_FONTPREVIEWSAFETYCHECKERDEFAULT_H

#include "modules/ui/fonts/fontpreviewsafetychecker.h"

#include <QHash>
#include <QPair>

class QThread;

class FontPreviewSafetyCheckerDefault final : public FontPreviewSafetyChecker
{
    Q_OBJECT

public:
    explicit FontPreviewSafetyCheckerDefault( QObject *parent = nullptr );
    ~FontPreviewSafetyCheckerDefault() override;

    void startCheck( const QString &family, int pixelSize ) override;

private:
    using Key = QPair<QString, int>;
    QHash<Key, QThread *> m_threads;
};

#endif
