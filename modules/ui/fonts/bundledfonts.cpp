#include "modules/ui/fonts/bundledfonts.h"

#include <QByteArray>
#include <QFontDatabase>
#include <QDebug>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QStringList>

static void ensureBundledFontsResourceInitialised()
{
    // This .qrc is compiled into a static library. Ensure the resource is
    // initialised even if the linker would otherwise drop the init symbol.
    Q_INIT_RESOURCE( bundledfonts );
}

namespace {

bundledfonts::Families g_families;

QString firstFamilyForId( const int id )
{
    QString out;

    if ( id < 0 ) {
        return QString();
    }

    const QStringList list = QFontDatabase::applicationFontFamilies( id );
    if ( list.isEmpty() ) {
        return QString();
    }

    out = list.first();
    return out;
}

void registerOnce()
{
    static bool registered;

    if ( registered ) {
        return;
    }

    // QFontDatabase requires a GUI application instance.
    if ( qobject_cast<QGuiApplication *>( QCoreApplication::instance() ) == nullptr ) {
        return;
    }

    registered = true;

    // Body font: legible serif.
    const QString bodyPath = QStringLiteral( ":/fonts/librecaslontext/LibreCaslonText-wght.ttf" );
    const int bodyId = QFontDatabase::addApplicationFont( bodyPath );
    if ( bodyId < 0 ) {
        const QByteArray bodyPathUtf8 = bodyPath.toUtf8();
        qWarning( "Bundled font registration failed: %s", bodyPathUtf8.constData() );
    }
    g_families.victorianBody = firstFamilyForId( bodyId );
    if ( bodyId >= 0 && g_families.victorianBody.isEmpty() ) {
        const QByteArray bodyPathUtf8 = bodyPath.toUtf8();
        qWarning( "Bundled font registered but returned no families: %s", bodyPathUtf8.constData() );
    }

    // Heading font: display serif.
    const QString headingPath = QStringLiteral( ":/fonts/librecaslondisplay/LibreCaslonDisplay-Regular.ttf" );
    const int headingId = QFontDatabase::addApplicationFont( headingPath );
    if ( headingId < 0 ) {
        const QByteArray headingPathUtf8 = headingPath.toUtf8();
        qWarning( "Bundled font registration failed: %s", headingPathUtf8.constData() );
    }
    g_families.victorianHeading = firstFamilyForId( headingId );
    if ( headingId >= 0 && g_families.victorianHeading.isEmpty() ) {
        const QByteArray headingPathUtf8 = headingPath.toUtf8();
        qWarning( "Bundled font registered but returned no families: %s", headingPathUtf8.constData() );
    }

    // If italic is present, register it so rich text/emphasis can resolve.
    const QString italicPath = QStringLiteral( ":/fonts/librecaslontext/LibreCaslonText-Italic-wght.ttf" );
    const int italicId = QFontDatabase::addApplicationFont( italicPath );
    if ( italicId < 0 ) {
        const QByteArray italicPathUtf8 = italicPath.toUtf8();
        qWarning( "Bundled font registration failed: %s", italicPathUtf8.constData() );
    }
}

}

void bundledfonts::ensureRegistered()
{
    ensureBundledFontsResourceInitialised();
    registerOnce();
}

bundledfonts::Families bundledfonts::families()
{
    ensureBundledFontsResourceInitialised();
    registerOnce();
    return g_families;
}
