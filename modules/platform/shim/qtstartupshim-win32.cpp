#include "modules/platform/shim/qtstartupshim.h"

#include <QDir>
#include <QString>

#include <QtGlobal>

#if defined( Q_OS_WIN )

namespace qtstartupshim {

void applyBeforeQGuiApplication()
{
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
}

}

#endif
