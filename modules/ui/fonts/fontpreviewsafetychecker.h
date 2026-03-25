#ifndef MODULES_UI_FONTS_FONTPREVIEWSAFETYCHECKER_H
#define MODULES_UI_FONTS_FONTPREVIEWSAFETYCHECKER_H

#include <QObject>
#include <QString>

class FontPreviewSafetyChecker : public QObject
{
    Q_OBJECT

public:
    explicit FontPreviewSafetyChecker( QObject *parent = nullptr );
    ~FontPreviewSafetyChecker() override;

    virtual void startCheck( const QString &family, int pixelSize ) = 0;

Q_SIGNALS:
    void checkFinished( const QString &family, int pixelSize, bool ok );
};

#endif
