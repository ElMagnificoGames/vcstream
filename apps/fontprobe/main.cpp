#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QProcess>
#include <QRawFont>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <cstdio>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>

#include "modules/app/defence/crashguard.h"

namespace {

void writeUtf8( FILE *out, const QString &s )
{
    if ( out == nullptr ) {
        return;
    }

    const QByteArray utf8 = s.toUtf8();
    ( void )fwrite( utf8.constData(), 1u, static_cast<size_t>( utf8.size() ), out );
}

void writeLiteral( FILE *out, const char *s )
{
    if ( out == nullptr || s == nullptr ) {
        return;
    }

    ( void )fputs( s, out );
}

QString sha256Hex( const QByteArray &bytes )
{
    return QString::fromLatin1( QCryptographicHash::hash( bytes, QCryptographicHash::Sha256 ).toHex() );
}

QString sha256HexImage( const QImage &img )
{
    if ( img.isNull() ) {
        return QStringLiteral( "(null)" );
    }

    QImage tmp = img;
    if ( tmp.format() != QImage::Format_ARGB32_Premultiplied ) {
        tmp = tmp.convertToFormat( QImage::Format_ARGB32_Premultiplied );
    }

    const int bpl = tmp.bytesPerLine();
    const int h = tmp.height();
    if ( bpl <= 0 || h <= 0 || tmp.bits() == nullptr ) {
        return QStringLiteral( "(empty)" );
    }

    return sha256Hex( QByteArray::fromRawData( reinterpret_cast<const char *>( tmp.constBits() ), bpl * h ) );
}

QString safeFileSlug( const QString &s )
{
    QString out;
    out.reserve( s.size() );

    for ( const QChar ch : s ) {
        if ( ch.isLetterOrNumber() ) {
            out.append( ch.toLower() );
        } else if ( ch == QLatin1Char( '-' ) || ch == QLatin1Char( '_' ) ) {
            out.append( ch );
        } else if ( ch.isSpace() ) {
            out.append( QLatin1Char( '_' ) );
        }
    }

    if ( out.isEmpty() ) {
        return QStringLiteral( "font" );
    }

    return out.left( 80 );
}

QString colrVersionString( const QByteArray &colr )
{
    if ( colr.size() < 2 ) {
        return QStringLiteral( "unknown" );
    }

    const quint8 hi = static_cast<quint8>( colr.at( 0 ) );
    const quint8 lo = static_cast<quint8>( colr.at( 1 ) );
    const quint16 v = static_cast<quint16>( ( hi << 8 ) | lo );
    return QString::number( v );
}

QString cpalVersionString( const QByteArray &cpal )
{
    if ( cpal.size() < 2 ) {
        return QStringLiteral( "unknown" );
    }

    const quint8 hi = static_cast<quint8>( cpal.at( 0 ) );
    const quint8 lo = static_cast<quint8>( cpal.at( 1 ) );
    const quint16 v = static_cast<quint16>( ( hi << 8 ) | lo );
    return QString::number( v );
}

void printFontFacts( FILE *out, const QFont &font )
{
    const QByteArray familyUtf8 = font.family().toUtf8();
    ( void )fprintf( out, "Font request family: '%s'\n", familyUtf8.constData() );
    ( void )fprintf( out, "Font request pixelSize: %d\n", font.pixelSize() );

    const QFontInfo info( font );
    const QByteArray resolvedFamilyUtf8 = info.family().toUtf8();
    ( void )fprintf( out, "Font resolved family: '%s'\n", resolvedFamilyUtf8.constData() );
    ( void )fprintf( out, "Font exactMatch: %s\n", ( info.exactMatch() ? "true" : "false" ) );

    const QRawFont raw = QRawFont::fromFont( font );
    ( void )fprintf( out, "QRawFont valid: %s\n", ( raw.isValid() ? "true" : "false" ) );

    if ( raw.isValid() ) {
        const QByteArray colr = raw.fontTable( "COLR" );
        const QByteArray cpal = raw.fontTable( "CPAL" );
        const QByteArray fvar = raw.fontTable( "fvar" );

        const QString colrVersion = colr.isEmpty() ? QStringLiteral( "n/a" ) : colrVersionString( colr );
        const QString cpalVersion = cpal.isEmpty() ? QStringLiteral( "n/a" ) : cpalVersionString( cpal );
        const QByteArray colrVersionUtf8 = colrVersion.toUtf8();
        const QByteArray cpalVersionUtf8 = cpalVersion.toUtf8();

        ( void )fprintf( out,
            "Table COLR present: %s version: %s\n",
            ( colr.isEmpty() ? "false" : "true" ),
            colrVersionUtf8.constData() );
        ( void )fprintf( out,
            "Table CPAL present: %s version: %s\n",
            ( cpal.isEmpty() ? "false" : "true" ),
            cpalVersionUtf8.constData() );
        ( void )fprintf( out, "Table fvar present: %s\n", ( fvar.isEmpty() ? "false" : "true" ) );
    }
}

struct RunConfig {
    QString family;
    QString text;
    int width;
    int height;
    int pixelSize;
    QString outDir;
    QString scenario;
    int warmupMs;
    bool stress;
    int stressSteps;
    int stressMaxFamilies;
    bool stressScan;
    bool emitSilhouette;
    bool emitOverlay;
    bool emitDiff;
};

bool ensureDir( const QString &path, QString *error )
{
    QDir d( path );
    if ( d.exists() ) {
        return true;
    }

    if ( QDir().mkpath( path ) ) {
        return true;
    }

    if ( error ) {
        *error = QStringLiteral( "Failed to create output directory: %1" ).arg( path );
    }
    return false;
}

QString outputPathFor( const RunConfig &cfg, const QString &tag )
{
    const QString familySlug = safeFileSlug( cfg.family );
    const QString ts = QDateTime::currentDateTimeUtc().toString( QStringLiteral( "yyyyMMdd-hhmmss-zzz" ) );
    static quint64 counter = 0;
    ++counter;
    const qint64 pid = QCoreApplication::applicationPid();
    return QDir( cfg.outDir ).filePath( QStringLiteral( "%1-%2-%3-p%4-%5.png" ).arg( ts, familySlug, tag ).arg( pid ).arg( counter ) );
}

QString outputPathForIndex( const RunConfig &cfg, const QString &tag, const int index, const QString &family )
{
    const QString familySlug = safeFileSlug( family );
    const QString ts = QDateTime::currentDateTimeUtc().toString( QStringLiteral( "yyyyMMdd-hhmmss-zzz" ) );
    static quint64 counter = 0;
    ++counter;
    const qint64 pid = QCoreApplication::applicationPid();
    return QDir( cfg.outDir ).filePath( QStringLiteral( "%1-%2-%3-%4-p%5-%6.png" ).arg( ts, tag ).arg( index ).arg( familySlug ).arg( pid ).arg( counter ) );
}

bool isNonWhite( const QRgb px )
{
    if ( qAlpha( px ) == 0 ) {
        return false;
    }
    return !( qRed( px ) == 255 && qGreen( px ) == 255 && qBlue( px ) == 255 );
}

QImage renderSilhouette( FILE *out, const RunConfig &cfg, const QFont &font )
{
    QImage img( cfg.width, cfg.height, QImage::Format_ARGB32_Premultiplied );
    img.fill( qRgba( 255, 255, 255, 255 ) );

    const QRawFont raw = QRawFont::fromFont( font );
    if ( !raw.isValid() ) {
        writeLiteral( out, "Silhouette: QRawFont invalid\n" );
        return QImage();
    }

    const QVector<quint32> glyphs = raw.glyphIndexesForString( cfg.text );
    if ( glyphs.isEmpty() ) {
        writeLiteral( out, "Silhouette: no glyphs for string\n" );
        return QImage();
    }

    const QVector<QPointF> adv = raw.advancesForGlyphIndexes( glyphs );

    const int margin = 12;
    const qreal baselineY = margin + raw.ascent();
    QPointF pen( margin, baselineY );

    QPainterPath path;
    int emptyPaths = 0;

    const int n = glyphs.size();
    for ( int i = 0; i < n; ++i ) {
        const QPainterPath gp = raw.pathForGlyph( glyphs.at( i ) );
        if ( gp.isEmpty() ) {
            ++emptyPaths;
        } else {
            QTransform t;
            t.translate( pen.x(), pen.y() );
            path.addPath( t.map( gp ) );
        }

        if ( i < adv.size() ) {
            pen += adv.at( i );
        }
    }

    ( void )fprintf( out, "Silhouette: glyphs=%d emptyPaths=%d\n", glyphs.size(), emptyPaths );

    QPainter p( &img );
    p.setRenderHint( QPainter::Antialiasing, false );
    p.setPen( Qt::NoPen );
    p.setBrush( QColor( 0, 0, 0 ) );
    p.drawPath( path );
    p.end();

    return img;
}

double computeMaskIoU( const QImage &a, const QImage &b )
{
    if ( a.isNull() || b.isNull() ) {
        return 0.0;
    }
    if ( a.size() != b.size() ) {
        return 0.0;
    }

    const QImage aa = ( a.format() == QImage::Format_ARGB32_Premultiplied ) ? a : a.convertToFormat( QImage::Format_ARGB32_Premultiplied );
    const QImage bb = ( b.format() == QImage::Format_ARGB32_Premultiplied ) ? b : b.convertToFormat( QImage::Format_ARGB32_Premultiplied );

    quint64 inter = 0;
    quint64 uni = 0;

    const int w = aa.width();
    const int h = aa.height();
    for ( int y = 0; y < h; ++y ) {
        const QRgb *ar = reinterpret_cast<const QRgb *>( aa.constScanLine( y ) );
        const QRgb *br = reinterpret_cast<const QRgb *>( bb.constScanLine( y ) );
        for ( int x = 0; x < w; ++x ) {
            const bool ai = isNonWhite( ar[ x ] );
            const bool bi = isNonWhite( br[ x ] );
            if ( ai && bi ) {
                ++inter;
            }
            if ( ai || bi ) {
                ++uni;
            }
        }
    }

    if ( uni == 0 ) {
        return 0.0;
    }

    return static_cast<double>( inter ) / static_cast<double>( uni );
}

QImage renderOverlay( const QImage &base, const QImage &sil )
{
    if ( base.isNull() || sil.isNull() || base.size() != sil.size() ) {
        return QImage();
    }

    QImage out = base.convertToFormat( QImage::Format_ARGB32_Premultiplied );
    QImage s = sil.convertToFormat( QImage::Format_ARGB32_Premultiplied );

    QPainter p( &out );
    p.setCompositionMode( QPainter::CompositionMode_SourceOver );
    p.setOpacity( 0.35 );
    p.drawImage( 0, 0, s );
    p.end();

    return out;
}

QImage renderDiffMask( const QImage &raster, const QImage &sil )
{
    if ( raster.isNull() || sil.isNull() || raster.size() != sil.size() ) {
        return QImage();
    }

    const QImage rr = raster.convertToFormat( QImage::Format_ARGB32_Premultiplied );
    const QImage ss = sil.convertToFormat( QImage::Format_ARGB32_Premultiplied );
    QImage out( rr.size(), QImage::Format_ARGB32_Premultiplied );
    out.fill( qRgba( 255, 255, 255, 255 ) );

    const int w = out.width();
    const int h = out.height();
    for ( int y = 0; y < h; ++y ) {
        const QRgb *rrow = reinterpret_cast<const QRgb *>( rr.constScanLine( y ) );
        const QRgb *srow = reinterpret_cast<const QRgb *>( ss.constScanLine( y ) );
        QRgb *orow = reinterpret_cast<QRgb *>( out.scanLine( y ) );
        for ( int x = 0; x < w; ++x ) {
            const bool ri = isNonWhite( rrow[ x ] );
            const bool si = isNonWhite( srow[ x ] );

            if ( ri && si ) {
                orow[ x ] = qRgba( 0, 0, 0, 255 );
            } else if ( ri && !si ) {
                // Raster ink not explained by silhouette.
                orow[ x ] = qRgba( 220, 30, 30, 255 );
            } else if ( !ri && si ) {
                // Silhouette ink missing from raster.
                orow[ x ] = qRgba( 30, 90, 220, 255 );
            } else {
                orow[ x ] = qRgba( 255, 255, 255, 255 );
            }
        }
    }

    return out;
}

int runQPainterScenario( const RunConfig &cfg )
{
    writeLiteral( stdout, "Scenario: qpaint\n" );

    QImage img( cfg.width, cfg.height, QImage::Format_ARGB32_Premultiplied );
    img.fill( qRgba( 255, 255, 255, 255 ) );

    QPainter p( &img );
    p.setRenderHint( QPainter::Antialiasing, true );
    p.setRenderHint( QPainter::TextAntialiasing, true );

    QFont f;
    f.setFamily( cfg.family );
    f.setPixelSize( cfg.pixelSize );

    printFontFacts( stdout, f );

    p.setFont( f );
    p.setPen( QColor( 20, 20, 20 ) );

    const QRect r( 12, 12, cfg.width - 24, cfg.height - 24 );
    p.drawText( r, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, cfg.text );
    p.end();

    const QString path = outputPathFor( cfg, QStringLiteral( "qpaint" ) );
    if ( !img.save( path ) ) {
        const QByteArray pathUtf8 = path.toUtf8();
        ( void )fprintf( stdout, "Save failed: '%s'\n", pathUtf8.constData() );
        return 2;
    }

    {
        const QByteArray pathUtf8 = path.toUtf8();
        const QByteArray shaUtf8 = sha256HexImage( img ).toUtf8();
        ( void )fprintf( stdout, "Output: '%s'\n", pathUtf8.constData() );
        ( void )fprintf( stdout, "Image sha256: %s\n", shaUtf8.constData() );
    }

    if ( cfg.emitSilhouette || cfg.emitOverlay || cfg.emitDiff ) {
        const QImage sil = renderSilhouette( stdout, cfg, f );
        if ( !sil.isNull() ) {
            const QString sp = outputPathFor( cfg, QStringLiteral( "qpaint-silhouette" ) );
            sil.save( sp );
            {
                const QByteArray spUtf8 = sp.toUtf8();
                const QByteArray silShaUtf8 = sha256HexImage( sil ).toUtf8();
                ( void )fprintf( stdout, "Silhouette output: '%s'\n", spUtf8.constData() );
                ( void )fprintf( stdout, "Silhouette sha256: %s\n", silShaUtf8.constData() );
            }

            const double iou = computeMaskIoU( img, sil );
            {
                const QByteArray iouUtf8 = QString::number( iou, 'f', 4 ).toUtf8();
                ( void )fprintf( stdout, "Mask IoU: %s\n", iouUtf8.constData() );
            }

            if ( cfg.emitOverlay ) {
                const QImage overlay = renderOverlay( img, sil );
                if ( !overlay.isNull() ) {
                    const QString op = outputPathFor( cfg, QStringLiteral( "qpaint-overlay" ) );
                    overlay.save( op );
                    {
                        const QByteArray opUtf8 = op.toUtf8();
                        ( void )fprintf( stdout, "Overlay output: '%s'\n", opUtf8.constData() );
                    }
                }
            }

            if ( cfg.emitDiff ) {
                const QImage diff = renderDiffMask( img, sil );
                if ( !diff.isNull() ) {
                    const QString dp = outputPathFor( cfg, QStringLiteral( "qpaint-diff" ) );
                    diff.save( dp );
                    {
                        const QByteArray dpUtf8 = dp.toUtf8();
                        ( void )fprintf( stdout, "Diff output: '%s'\n", dpUtf8.constData() );
                    }
                }
            }
        }
    }

    return 0;
}

QString quickQmlForScenario( const RunConfig &cfg, const QString &renderType )
{
    const QString qml = QStringLiteral(
        "import QtQuick 2.15\n"
        "Item {\n"
        "  width: %1; height: %2\n"
        "  Rectangle { anchors.fill: parent; color: '#ffffff' }\n"
        "  Text {\n"
        "    id: t\n"
        "    objectName: 'probeText'\n"
        "    x: 12; y: 12\n"
        "    width: parent.width - 24\n"
        "    wrapMode: Text.WordWrap\n"
        "    text: %3\n"
        "    color: '#141414'\n"
        "    font.family: %4\n"
        "    font.pixelSize: %5\n"
        "    renderType: %6\n"
        "  }\n"
        "}\n" )
        .arg( cfg.width )
        .arg( cfg.height )
        .arg( QString( cfg.text ).replace( QLatin1Char( '\\' ), QStringLiteral( "\\\\" ) )
                                .replace( QLatin1Char( '\'' ), QStringLiteral( "\\'" ) )
                                .prepend( QLatin1Char( '\'' ) )
                                .append( QLatin1Char( '\'' ) ) )
        .arg( QString( cfg.family ).replace( QLatin1Char( '\\' ), QStringLiteral( "\\\\" ) )
                                  .replace( QLatin1Char( '\'' ), QStringLiteral( "\\'" ) )
                                  .prepend( QLatin1Char( '\'' ) )
                                  .append( QLatin1Char( '\'' ) ) )
        .arg( cfg.pixelSize )
        .arg( renderType );

    return qml;
}

QString quickQmlForScrollStress( const RunConfig &cfg, const QString &renderType )
{
    Q_UNUSED( cfg );

    // families is injected from C++ after object creation.
    // Keep the delegate similar to VcFontPicker: label in system font + sample line in the candidate font.
    const QString qml = QStringLiteral( R"QML(
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
  id: root
  width: 640; height: 360
  property var families: []
  property string sampleText: 'The quick brown fox jumps over the lazy dog. 0123456789'
  property int rowH: 52

  Rectangle { anchors.fill: parent; color: '#ffffff' }

  ListView {
    id: lv
    objectName: 'probeListView'
    anchors.fill: parent
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    spacing: 4
    model: root.families

    delegate: Item {
      width: ListView.view.width
      height: root.rowH

      Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 1

        Text {
          text: modelData
          color: '#141414'
          elide: Text.ElideRight
          width: parent.width
          font.family: Qt.application.font.family
          font.pixelSize: 14
          renderType: %1
        }

        Text {
          text: root.sampleText
          color: '#555555'
          elide: Text.ElideRight
          width: parent.width
          maximumLineCount: 1
          font.family: modelData
          font.pixelSize: 18
          renderType: %1
        }
      }
    }
  }
}
)QML" );

    return qml.arg( renderType );
}

int runQtQuickScenario( const RunConfig &cfg, const QString &renderType, const QString &tag )
{
    {
        const QByteArray tagUtf8 = tag.toUtf8();
        ( void )fprintf( stdout, "Scenario: %s\n", tagUtf8.constData() );
    }

    // Also print font facts from the QFont side so we can compare fallback behaviour.
    {
        QFont f;
        f.setFamily( cfg.family );
        f.setPixelSize( cfg.pixelSize );
        printFontFacts( stdout, f );
    }

    QQmlEngine engine;
    QQmlComponent component( &engine );
    const QString qml = quickQmlForScenario( cfg, renderType );
    component.setData( qml.toUtf8(), QUrl( QStringLiteral( "inmemory:/FontProbe.qml" ) ) );

    // QQmlComponent may compile asynchronously.
    if ( component.status() == QQmlComponent::Loading ) {
        const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + 2000;
        while ( component.status() == QQmlComponent::Loading && QDateTime::currentMSecsSinceEpoch() < deadline ) {
            QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
        }
    }

    if ( component.status() != QQmlComponent::Ready ) {
        const QByteArray errStrUtf8 = component.errorString().toUtf8();
        ( void )fprintf( stdout, "QML component not ready. status=%d\n", static_cast<int>( component.status() ) );
        ( void )fprintf( stdout, "%s\n", errStrUtf8.constData() );
        const QList<QQmlError> errors = component.errors();
        for ( const QQmlError &e : errors ) {
            const QByteArray eUtf8 = e.toString().toUtf8();
            ( void )fprintf( stdout, "QML error: %s\n", eUtf8.constData() );
        }

        writeLiteral( stdout, "QML source:\n" );
        writeUtf8( stdout, qml );
        writeLiteral( stdout, "\n" );
        return 3;
    }

    QObject *rootObj = component.create();
    QQuickItem *rootItem = qobject_cast<QQuickItem *>( rootObj );
    if ( rootItem == nullptr ) {
        writeLiteral( stdout, "Root object is not a QQuickItem.\n" );
        return 4;
    }

    QQuickView view( &engine, nullptr );
    view.setResizeMode( QQuickView::SizeRootObjectToView );
    view.setColor( Qt::transparent );
    view.setWidth( cfg.width );
    view.setHeight( cfg.height );
    view.setFlags( Qt::Tool | Qt::FramelessWindowHint );
    view.setContent( QUrl( QStringLiteral( "inmemory:/FontProbe.qml" ) ), &component, rootItem );

    view.show();

    // Give the scene graph a moment to initialise and render at least one frame.
    const qint64 start = QDateTime::currentMSecsSinceEpoch();
    while ( QDateTime::currentMSecsSinceEpoch() - start < cfg.warmupMs ) {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
    }

    const QImage img = view.grabWindow();

    const QString path = outputPathFor( cfg, tag );
    if ( img.isNull() ) {
        const QByteArray pathUtf8 = path.toUtf8();
        writeLiteral( stdout, "grabWindow returned null image.\n" );
        ( void )fprintf( stdout, "Output (not written): '%s'\n", pathUtf8.constData() );
        return 5;
    }

    if ( !img.save( path ) ) {
        const QByteArray pathUtf8 = path.toUtf8();
        ( void )fprintf( stdout, "Save failed: '%s'\n", pathUtf8.constData() );
        return 6;
    }

    {
        const QByteArray pathUtf8 = path.toUtf8();
        const QByteArray shaUtf8 = sha256HexImage( img ).toUtf8();
        ( void )fprintf( stdout, "Output: '%s'\n", pathUtf8.constData() );
        ( void )fprintf( stdout, "Image sha256: %s\n", shaUtf8.constData() );
    }
    return 0;
}

int runQtQuickScrollStress( const RunConfig &cfg, const QString &renderType, const QString &tag )
{
    {
        const QByteArray tagUtf8 = tag.toUtf8();
        ( void )fprintf( stdout, "Scenario: %s\n", tagUtf8.constData() );
    }

    QFont f;
    f.setFamily( cfg.family );
    f.setPixelSize( cfg.pixelSize );
    printFontFacts( stdout, f );

    // Build a families list; ensure the requested family is included.
    QStringList families = QFontDatabase::families();
    families.removeDuplicates();
    families.sort( Qt::CaseInsensitive );

    if ( !families.contains( cfg.family ) ) {
        families.prepend( cfg.family );
    }

    if ( cfg.stressMaxFamilies > 0 && families.size() > cfg.stressMaxFamilies ) {
        // Keep the requested family near the middle so a sweep is likely to bring it into view.
        const int keep = cfg.stressMaxFamilies;
        const int mid = qMin( families.size() - 1, qMax( 0, families.indexOf( cfg.family ) ) );
        const int start = qMax( 0, mid - keep / 2 );
        families = families.mid( start, keep );
        if ( !families.contains( cfg.family ) ) {
            families.prepend( cfg.family );
        }
    }

    ( void )fprintf( stdout, "Families in model: %d\n", families.size() );
    ( void )fprintf( stdout, "Requested family index: %d\n", families.indexOf( cfg.family ) );
    if ( !families.isEmpty() ) {
        const QByteArray firstUtf8 = families.first().toUtf8();
        const QByteArray lastUtf8 = families.last().toUtf8();
        ( void )fprintf( stdout, "First family: '%s'\n", firstUtf8.constData() );
        ( void )fprintf( stdout, "Last family: '%s'\n", lastUtf8.constData() );
    }

    QQmlEngine engine;
    QQmlComponent component( &engine );
    const QString qml = quickQmlForScrollStress( cfg, renderType );
    component.setData( qml.toUtf8(), QUrl( QStringLiteral( "inmemory:/FontProbeScroll.qml" ) ) );

    if ( component.status() == QQmlComponent::Loading ) {
        const qint64 deadline = QDateTime::currentMSecsSinceEpoch() + 2000;
        while ( component.status() == QQmlComponent::Loading && QDateTime::currentMSecsSinceEpoch() < deadline ) {
            QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
        }
    }

    if ( component.status() != QQmlComponent::Ready ) {
        const QByteArray errStrUtf8 = component.errorString().toUtf8();
        ( void )fprintf( stdout, "QML component not ready. status=%d\n", static_cast<int>( component.status() ) );
        ( void )fprintf( stdout, "%s\n", errStrUtf8.constData() );
        return 3;
    }

    QObject *rootObj = component.create();
    QQuickItem *rootItem = qobject_cast<QQuickItem *>( rootObj );
    if ( rootItem == nullptr ) {
        writeLiteral( stdout, "Root object is not a QQuickItem.\n" );
        return 4;
    }

    rootItem->setProperty( "width", cfg.width );
    rootItem->setProperty( "height", cfg.height );
    rootItem->setProperty( "families", families );
    rootItem->setProperty( "sampleText", cfg.text );

    QQuickView view( &engine, nullptr );
    view.setResizeMode( QQuickView::SizeRootObjectToView );
    view.setColor( Qt::transparent );
    view.setWidth( cfg.width );
    view.setHeight( cfg.height );
    view.setFlags( Qt::Tool | Qt::FramelessWindowHint );
    view.setContent( QUrl( QStringLiteral( "inmemory:/FontProbeScroll.qml" ) ), &component, rootItem );
    view.show();

    const qint64 start = QDateTime::currentMSecsSinceEpoch();
    while ( QDateTime::currentMSecsSinceEpoch() - start < cfg.warmupMs ) {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
    }

    QObject *lvObj = rootObj->findChild<QObject *>( QStringLiteral( "probeListView" ) );
    if ( lvObj == nullptr ) {
        writeLiteral( stdout, "ListView not found.\n" );
        return 8;
    }

    const int rowH = rootItem->property( "rowH" ).toInt();

    qint64 worstMs = 0;
    QString worstFamily;
    int worstStep = -1;

    const double contentHeight = lvObj->property( "contentHeight" ).toDouble();
    const double viewportHeight = lvObj->property( "height" ).toDouble();
    const double maxY = qMax( 0.0, contentHeight - viewportHeight );

    if ( cfg.stressScan ) {
        const int stride = rowH + 4;
        const int scanCount = families.size();

        ( void )fprintf( stdout, "Scan mode: true count=%d stridePx=%d\n", scanCount, stride );
        ( void )fflush( stdout );

        for ( int i = 0; i < scanCount; ++i ) {
            const double y = qMin( maxY, qMax( 0.0, static_cast<double>( i * stride ) ) );
            const QString family = families.at( i );
            const QByteArray familyUtf8 = family.toUtf8();

            ( void )fprintf( stdout, "Index %d family='%s' set contentY=%.3f ...\n", i, familyUtf8.constData(), y );
            ( void )fflush( stdout );

            QElapsedTimer timer;
            timer.start();

            lvObj->setProperty( "contentY", y );
            QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
            writeLiteral( stdout, "  after events; grabbing...\n" );
            ( void )fflush( stdout );

            const QImage img = view.grabWindow();

            const qint64 ms = timer.elapsed();
            ( void )fprintf( stdout, "  elapsedMs=%lld\n", static_cast<long long>( ms ) );
            ( void )fflush( stdout );

            if ( !img.isNull() && ms >= 1000 ) {
                const QString slowPath = outputPathForIndex( cfg, tag + QStringLiteral( "-slow" ), i, family );
                img.save( slowPath );
                const QByteArray slowPathUtf8 = slowPath.toUtf8();
                const QByteArray shaUtf8 = sha256HexImage( img ).toUtf8();
                ( void )fprintf( stdout, "  slow snapshot: '%s' sha256=%s\n", slowPathUtf8.constData(), shaUtf8.constData() );
                ( void )fflush( stdout );
            }

            if ( ms > worstMs ) {
                worstMs = ms;
                worstFamily = family;
                worstStep = i;
            }
        }
    } else {
        const int steps = qMax( 3, cfg.stressSteps );

        ( void )fprintf( stdout, "Scan mode: false steps=%d\n", steps );
        ( void )fflush( stdout );

        for ( int i = 0; i < steps; ++i ) {
            const double t = ( steps > 1 ) ? ( static_cast<double>( i ) / static_cast<double>( steps - 1 ) ) : 0.0;
            const double y = t * maxY;

            // Approximate which row is centred.
            const int approxIndex = ( rowH > 0 ) ? static_cast<int>( ( y + viewportHeight * 0.5 ) / static_cast<double>( rowH + 4 ) ) : -1;
            QString approxFamily;
            if ( approxIndex >= 0 && approxIndex < families.size() ) {
                approxFamily = families.at( approxIndex );
            }
            const QByteArray approxFamilyUtf8 = approxFamily.toUtf8();

            ( void )fprintf( stdout,
                "Step %d/%d contentY=%.3f approxFamily='%s' ...\n",
                i,
                ( steps - 1 ),
                y,
                approxFamilyUtf8.constData() );
            ( void )fflush( stdout );

            QElapsedTimer timer;
            timer.start();

            lvObj->setProperty( "contentY", y );
            QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
            writeLiteral( stdout, "  after events; grabbing...\n" );
            ( void )fflush( stdout );

            const QImage img = view.grabWindow();
            Q_UNUSED( img );

            const qint64 ms = timer.elapsed();
            ( void )fprintf( stdout, "  elapsedMs=%lld\n", static_cast<long long>( ms ) );
            ( void )fflush( stdout );

            if ( ms > worstMs ) {
                worstMs = ms;
                worstFamily = approxFamily;
                worstStep = i;
            }
        }
    }

    {
        const QByteArray worstFamilyUtf8 = worstFamily.toUtf8();
        ( void )fprintf( stdout,
            "Worst elapsedMs: %lld at step %d approxFamily='%s'\n",
            static_cast<long long>( worstMs ),
            worstStep,
            worstFamilyUtf8.constData() );
    }

    const QImage finalImg = view.grabWindow();
    const QString path = outputPathFor( cfg, tag );
    if ( !finalImg.isNull() ) {
        finalImg.save( path );
        const QByteArray pathUtf8 = path.toUtf8();
        const QByteArray shaUtf8 = sha256HexImage( finalImg ).toUtf8();
        ( void )fprintf( stdout, "Output: '%s'\n", pathUtf8.constData() );
        ( void )fprintf( stdout, "Image sha256: %s\n", shaUtf8.constData() );
    }

    return 0;
}

int runWorkerOnce( const RunConfig &cfg )
{
    if ( cfg.scenario == QStringLiteral( "qpaint" ) ) {
        return runQPainterScenario( cfg );
    }

    if ( cfg.scenario == QStringLiteral( "quick-qt" ) ) {
        return runQtQuickScenario( cfg, QStringLiteral( "Text.QtRendering" ), QStringLiteral( "quick-qt" ) );
    }

    if ( cfg.scenario == QStringLiteral( "quick-native" ) ) {
        return runQtQuickScenario( cfg, QStringLiteral( "Text.NativeRendering" ), QStringLiteral( "quick-native" ) );
    }

    if ( cfg.scenario == QStringLiteral( "quick-scroll-qt" ) ) {
        return runQtQuickScrollStress( cfg, QStringLiteral( "Text.QtRendering" ), QStringLiteral( "quick-scroll-qt" ) );
    }

    if ( cfg.scenario == QStringLiteral( "quick-scroll-native" ) ) {
        return runQtQuickScrollStress( cfg, QStringLiteral( "Text.NativeRendering" ), QStringLiteral( "quick-scroll-native" ) );
    }

    {
        const QByteArray scenarioUtf8 = cfg.scenario.toUtf8();
        ( void )fprintf( stderr, "Unknown scenario: '%s'\n", scenarioUtf8.constData() );
    }
    return 7;
}

int runOrchestrator( const RunConfig &cfg, const QString &exePath, const int timeoutMs )
{
    const QByteArray platformUtf8 = QGuiApplication::platformName().toUtf8();
    const QByteArray familyUtf8 = cfg.family.toUtf8();
    const QByteArray outDirUtf8 = cfg.outDir.toUtf8();
    ( void )fprintf( stdout, "vcstream_fontprobe\n" );
    ( void )fprintf( stdout, "Qt version: %s\n", QT_VERSION_STR );
    ( void )fprintf( stdout, "Platform: %s\n", platformUtf8.constData() );
    ( void )fprintf( stdout, "Family: '%s'\n", familyUtf8.constData() );
    ( void )fprintf( stdout, "Pixel size: %d\n", cfg.pixelSize );
    ( void )fprintf( stdout, "Image size: %dx%d\n", cfg.width, cfg.height );
    ( void )fprintf( stdout, "Output dir: '%s'\n", outDirUtf8.constData() );
    ( void )fprintf( stdout, "Timeout (per scenario): %d ms\n", timeoutMs );

    QStringList scenarios = {
        QStringLiteral( "qpaint" ),
        QStringLiteral( "quick-qt" ),
        QStringLiteral( "quick-native" ),
    };

    if ( cfg.stress ) {
        scenarios.append( QStringLiteral( "quick-scroll-qt" ) );
        scenarios.append( QStringLiteral( "quick-scroll-native" ) );
    }

    int worst = 0;

    int skipped = 0;

    for ( const QString &scenario : scenarios ) {
        {
            const QByteArray scenarioUtf8 = scenario.toUtf8();
            ( void )fprintf( stdout, "\n--- Running: %s ---\n", scenarioUtf8.constData() );
            ( void )fflush( stdout );
        }

        QProcess p;
        QStringList args;
        args.append( QStringLiteral( "--worker" ) );
        args.append( QStringLiteral( "--scenario" ) );
        args.append( scenario );
        args.append( QStringLiteral( "--family" ) );
        args.append( cfg.family );
        args.append( QStringLiteral( "--text" ) );
        args.append( cfg.text );
        args.append( QStringLiteral( "--width" ) );
        args.append( QString::number( cfg.width ) );
        args.append( QStringLiteral( "--height" ) );
        args.append( QString::number( cfg.height ) );
        args.append( QStringLiteral( "--pixel-size" ) );
        args.append( QString::number( cfg.pixelSize ) );
        args.append( QStringLiteral( "--warmup-ms" ) );
        args.append( QString::number( cfg.warmupMs ) );
        args.append( QStringLiteral( "--stress" ) );
        args.append( cfg.stress ? QStringLiteral( "1" ) : QStringLiteral( "0" ) );
        args.append( QStringLiteral( "--stress-steps" ) );
        args.append( QString::number( cfg.stressSteps ) );
        args.append( QStringLiteral( "--stress-max-families" ) );
        args.append( QString::number( cfg.stressMaxFamilies ) );
        args.append( QStringLiteral( "--emit-silhouette" ) );
        args.append( cfg.emitSilhouette ? QStringLiteral( "1" ) : QStringLiteral( "0" ) );
        args.append( QStringLiteral( "--emit-overlay" ) );
        args.append( cfg.emitOverlay ? QStringLiteral( "1" ) : QStringLiteral( "0" ) );
        args.append( QStringLiteral( "--emit-diff" ) );
        args.append( cfg.emitDiff ? QStringLiteral( "1" ) : QStringLiteral( "0" ) );
        args.append( QStringLiteral( "--out-dir" ) );
        args.append( cfg.outDir );

        p.setProgram( exePath );
        p.setArguments( args );
        p.setProcessChannelMode( QProcess::MergedChannels );
        p.start();

        if ( !p.waitForStarted( 2000 ) ) {
            writeLiteral( stdout, "Failed to start worker.\n" );
            worst = qMax( worst, 100 );
            continue;
        }

        if ( !p.waitForFinished( timeoutMs ) ) {
            writeLiteral( stdout, "Timeout; killing worker.\n" );
            p.kill();
            p.waitForFinished( 2000 );
            worst = qMax( worst, 124 );
            continue;
        }

        const QByteArray log = p.readAll();
        ( void )fwrite( log.constData(), 1u, static_cast<size_t>( log.size() ), stdout );

        const int code = p.exitCode();
        if ( p.exitStatus() != QProcess::NormalExit ) {
            writeLiteral( stdout, "Worker crashed.\n" );
            worst = qMax( worst, 125 );
        } else if ( code != 0 ) {
            const bool isQuickScenario = scenario.startsWith( QStringLiteral( "quick-" ) );
            const bool treatAsSkipped = isQuickScenario && ( code == 3 || code == 4 || code == 5 );
            if ( treatAsSkipped ) {
                ( void )fprintf( stdout,
                    "Worker exitCode: %d (skipped; Qt Quick may be unavailable in this environment)\n",
                    code );
                ++skipped;
            } else {
                ( void )fprintf( stdout, "Worker exitCode: %d\n", code );
                worst = qMax( worst, code );
            }
        }
    }

    if ( skipped > 0 ) {
        ( void )fprintf( stdout, "\nNote: %d Qt Quick scenario(s) were skipped.\n", skipped );
    }

    return worst;
}

}

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "vcstream_fontprobe main", [argc, argv]() -> int {
        int localArgc = argc;
        char **localArgv = argv;

        QCoreApplication::setOrganizationName( QStringLiteral( "ElMagnificoGames" ) );
        QCoreApplication::setApplicationName( QStringLiteral( "vcstream_fontprobe" ) );

        QGuiApplication app( localArgc, localArgv );
        QCommandLineParser parser;

        parser.setApplicationDescription( QStringLiteral( "vcstream font rendering probe" ) );
        parser.addHelpOption();

        const QCommandLineOption workerOpt( QStringLiteral( "worker" ), QStringLiteral( "Run a single scenario and exit." ) );
        const QCommandLineOption scenarioOpt( QStringLiteral( "scenario" ), QStringLiteral( "Scenario name (worker mode)." ), QStringLiteral( "name" ) );
        const QCommandLineOption familyOpt( QStringLiteral( "family" ), QStringLiteral( "Font family name to test." ), QStringLiteral( "name" ) );
        const QCommandLineOption textOpt( QStringLiteral( "text" ), QStringLiteral( "Text to render." ), QStringLiteral( "text" ),
            QStringLiteral( "The quick brown fox jumps over the lazy dog. 0123456789" ) );
        const QCommandLineOption widthOpt( QStringLiteral( "width" ), QStringLiteral( "Output image width." ), QStringLiteral( "px" ), QStringLiteral( "640" ) );
        const QCommandLineOption heightOpt( QStringLiteral( "height" ), QStringLiteral( "Output image height." ), QStringLiteral( "px" ), QStringLiteral( "160" ) );
        const QCommandLineOption pixelSizeOpt( QStringLiteral( "pixel-size" ), QStringLiteral( "Font pixel size." ), QStringLiteral( "px" ), QStringLiteral( "24" ) );
        const QCommandLineOption warmupOpt( QStringLiteral( "warmup-ms" ), QStringLiteral( "Qt Quick warmup duration." ), QStringLiteral( "ms" ), QStringLiteral( "400" ) );
        const QCommandLineOption outDirOpt( QStringLiteral( "out-dir" ), QStringLiteral( "Output directory." ), QStringLiteral( "dir" ),
            QString() );
        const QCommandLineOption timeoutOpt( QStringLiteral( "timeout-ms" ), QStringLiteral( "Timeout per scenario (orchestrator)." ), QStringLiteral( "ms" ),
            QStringLiteral( "6000" ) );

        const QCommandLineOption stressOpt( QStringLiteral( "stress" ), QStringLiteral( "Enable scroll stress scenarios." ), QStringLiteral( "0|1" ), QStringLiteral( "0" ) );
        const QCommandLineOption stressStepsOpt( QStringLiteral( "stress-steps" ), QStringLiteral( "Scroll steps for stress scenarios." ), QStringLiteral( "n" ), QStringLiteral( "24" ) );
        const QCommandLineOption stressMaxFamiliesOpt( QStringLiteral( "stress-max-families" ), QStringLiteral( "Max families in stress model (0 = all)." ), QStringLiteral( "n" ), QStringLiteral( "220" ) );
        const QCommandLineOption stressScanOpt( QStringLiteral( "stress-scan" ), QStringLiteral( "Scan every family (stress scenario)." ), QStringLiteral( "0|1" ), QStringLiteral( "0" ) );

        const QCommandLineOption emitSilhouetteOpt( QStringLiteral( "emit-silhouette" ), QStringLiteral( "Emit silhouette render (qpaint scenario)." ), QStringLiteral( "0|1" ), QStringLiteral( "0" ) );
        const QCommandLineOption emitOverlayOpt( QStringLiteral( "emit-overlay" ), QStringLiteral( "Emit overlay of silhouette over raster (qpaint scenario)." ), QStringLiteral( "0|1" ), QStringLiteral( "0" ) );
        const QCommandLineOption emitDiffOpt( QStringLiteral( "emit-diff" ), QStringLiteral( "Emit diff mask between raster and silhouette (qpaint scenario)." ), QStringLiteral( "0|1" ), QStringLiteral( "0" ) );

        parser.addOption( workerOpt );
        parser.addOption( scenarioOpt );
        parser.addOption( familyOpt );
        parser.addOption( textOpt );
        parser.addOption( widthOpt );
        parser.addOption( heightOpt );
        parser.addOption( pixelSizeOpt );
        parser.addOption( warmupOpt );
        parser.addOption( outDirOpt );
        parser.addOption( timeoutOpt );
        parser.addOption( stressOpt );
        parser.addOption( stressStepsOpt );
        parser.addOption( stressMaxFamiliesOpt );
        parser.addOption( stressScanOpt );
        parser.addOption( emitSilhouetteOpt );
        parser.addOption( emitOverlayOpt );
        parser.addOption( emitDiffOpt );

        parser.process( app );

        RunConfig cfg;
        cfg.family = parser.value( familyOpt );
        cfg.text = parser.value( textOpt );
        cfg.width = parser.value( widthOpt ).toInt();
        cfg.height = parser.value( heightOpt ).toInt();
        cfg.pixelSize = parser.value( pixelSizeOpt ).toInt();
        cfg.outDir = parser.value( outDirOpt );
        cfg.scenario = parser.value( scenarioOpt );
        cfg.warmupMs = parser.value( warmupOpt ).toInt();
        cfg.stress = ( parser.value( stressOpt ) == QStringLiteral( "1" )
                       || parser.value( stressOpt ) == QStringLiteral( "true" )
                       || parser.value( stressOpt ) == QStringLiteral( "yes" ) );
        cfg.stressSteps = parser.value( stressStepsOpt ).toInt();
        cfg.stressMaxFamilies = parser.value( stressMaxFamiliesOpt ).toInt();
        cfg.stressScan = ( parser.value( stressScanOpt ) == QStringLiteral( "1" )
                           || parser.value( stressScanOpt ) == QStringLiteral( "true" )
                           || parser.value( stressScanOpt ) == QStringLiteral( "yes" ) );

        cfg.emitSilhouette = ( parser.value( emitSilhouetteOpt ) == QStringLiteral( "1" )
                               || parser.value( emitSilhouetteOpt ) == QStringLiteral( "true" )
                               || parser.value( emitSilhouetteOpt ) == QStringLiteral( "yes" ) );
        cfg.emitOverlay = ( parser.value( emitOverlayOpt ) == QStringLiteral( "1" )
                            || parser.value( emitOverlayOpt ) == QStringLiteral( "true" )
                            || parser.value( emitOverlayOpt ) == QStringLiteral( "yes" ) );
        cfg.emitDiff = ( parser.value( emitDiffOpt ) == QStringLiteral( "1" )
                         || parser.value( emitDiffOpt ) == QStringLiteral( "true" )
                         || parser.value( emitDiffOpt ) == QStringLiteral( "yes" ) );

        if ( cfg.width <= 0 ) cfg.width = 640;
        if ( cfg.height <= 0 ) cfg.height = 160;
        if ( cfg.pixelSize <= 0 ) cfg.pixelSize = 24;
        if ( cfg.warmupMs < 0 ) cfg.warmupMs = 0;
        if ( cfg.stressSteps <= 0 ) cfg.stressSteps = 24;
        if ( cfg.stressMaxFamilies < 0 ) cfg.stressMaxFamilies = 0;

        if ( cfg.outDir.isEmpty() ) {
            const QString tmp = QStandardPaths::writableLocation( QStandardPaths::TempLocation );
            if ( !tmp.isEmpty() ) {
                cfg.outDir = QDir( tmp ).filePath( QStringLiteral( "vcstream-fontprobe" ) );
            } else {
                cfg.outDir = QDir( QDir::tempPath() ).filePath( QStringLiteral( "vcstream-fontprobe" ) );
            }
        }

        QString dirError;
        if ( !ensureDir( cfg.outDir, &dirError ) ) {
            const QByteArray dirErrorUtf8 = dirError.toUtf8();
            ( void )fprintf( stderr, "%s\n", dirErrorUtf8.constData() );
            return 2;
        }

        if ( cfg.family.isEmpty() ) {
            writeLiteral( stderr, "Missing required --family argument.\n" );
            return 2;
        }

        if ( parser.isSet( workerOpt ) ) {
            if ( cfg.scenario.isEmpty() ) {
                writeLiteral( stderr, "Missing required --scenario in --worker mode.\n" );
                return 2;
            }
            return runWorkerOnce( cfg );
        }

        const int timeoutMs = parser.value( timeoutOpt ).toInt();
        const QString exePath = QCoreApplication::applicationFilePath();
        return runOrchestrator( cfg, exePath, qMax( 1000, timeoutMs ) );
    } );

    return exitCode;
}
