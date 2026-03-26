#include "modules/app/platform/qtshims.h"

#include <QDir>
#include <QString>

#include <QtGlobal>

namespace qtshims {

void applyBeforeQGuiApplication()
{
#if defined( Q_OS_WIN )
    if ( !qEnvironmentVariableIsSet( "QT_QPA_FONTDIR" ) ) {
        QString windir;
        QString fontDir;

        windir = qEnvironmentVariable( "WINDIR" );
        if ( windir.isEmpty() ) {
            windir = QStringLiteral( "C:\\Windows" );
        }

        fontDir = QDir( windir ).filePath( QStringLiteral( "Fonts" ) );
        if ( QDir( fontDir ).exists() ) {
            qputenv( "QT_QPA_FONTDIR", fontDir.toUtf8() );
        }
    }
#endif
}

}
