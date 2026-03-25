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
#include <QTextStream>
#include <QtGlobal>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>

#include "modules/app/defence/crashguard.h"

namespace {

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

void printFontFacts( QTextStream &out, const QFont &font )
{
    out << "Font request family: '" << font.family() << "'\n";
    out << "Font request pixelSize: " << font.pixelSize() << "\n";

    const QFontInfo info( font );
    out << "Font resolved family: '" << info.family() << "'\n";
    out << "Font exactMatch: " << ( info.exactMatch() ? "true" : "false" ) << "\n";

    const QRawFont raw = QRawFont::fromFont( font );
    out << "QRawFont valid: " << ( raw.isValid() ? "true" : "false" ) << "\n";

    if ( raw.isValid() ) {
        const QByteArray colr = raw.fontTable( "COLR" );
        const QByteArray cpal = raw.fontTable( "CPAL" );
        const QByteArray fvar = raw.fontTable( "fvar" );

        out << "Table COLR present: " << ( colr.isEmpty() ? "false" : "true" )
            << " version: " << ( colr.isEmpty() ? QStringLiteral( "n/a" ) : colrVersionString( colr ) ) << "\n";
        out << "Table CPAL present: " << ( cpal.isEmpty() ? "false" : "true" )
            << " version: " << ( cpal.isEmpty() ? QStringLiteral( "n/a" ) : cpalVersionString( cpal ) ) << "\n";
        out << "Table fvar present: " << ( fvar.isEmpty() ? "false" : "true" ) << "\n";
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

QImage renderSilhouette( QTextStream &out, const RunConfig &cfg, const QFont &font )
{
    QImage img( cfg.width, cfg.height, QImage::Format_ARGB32_Premultiplied );
    img.fill( qRgba( 255, 255, 255, 255 ) );

    const QRawFont raw = QRawFont::fromFont( font );
    if ( !raw.isValid() ) {
        out << "Silhouette: QRawFont invalid\n";
        return QImage();
    }

    const QVector<quint32> glyphs = raw.glyphIndexesForString( cfg.text );
    if ( glyphs.isEmpty() ) {
        out << "Silhouette: no glyphs for string\n";
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

    out << "Silhouette: glyphs=" << glyphs.size() << " emptyPaths=" << emptyPaths << "\n";

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
    QTextStream out( stdout );
    out << "Scenario: qpaint\n";

    QImage img( cfg.width, cfg.height, QImage::Format_ARGB32_Premultiplied );
    img.fill( qRgba( 255, 255, 255, 255 ) );

    QPainter p( &img );
    p.setRenderHint( QPainter::Antialiasing, true );
    p.setRenderHint( QPainter::TextAntialiasing, true );

    QFont f;
    f.setFamily( cfg.family );
    f.setPixelSize( cfg.pixelSize );

    printFontFacts( out, f );

    p.setFont( f );
    p.setPen( QColor( 20, 20, 20 ) );

    const QRect r( 12, 12, cfg.width - 24, cfg.height - 24 );
    p.drawText( r, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, cfg.text );
    p.end();

    const QString path = outputPathFor( cfg, QStringLiteral( "qpaint" ) );
    if ( !img.save( path ) ) {
        out << "Save failed: '" << path << "'\n";
        return 2;
    }

    out << "Output: '" << path << "'\n";
    out << "Image sha256: " << sha256HexImage( img ) << "\n";

    if ( cfg.emitSilhouette || cfg.emitOverlay || cfg.emitDiff ) {
        const QImage sil = renderSilhouette( out, cfg, f );
        if ( !sil.isNull() ) {
            const QString sp = outputPathFor( cfg, QStringLiteral( "qpaint-silhouette" ) );
            sil.save( sp );
            out << "Silhouette output: '" << sp << "'\n";
            out << "Silhouette sha256: " << sha256HexImage( sil ) << "\n";

            const double iou = computeMaskIoU( img, sil );
            out << "Mask IoU: " << QString::number( iou, 'f', 4 ) << "\n";

            if ( cfg.emitOverlay ) {
                const QImage overlay = renderOverlay( img, sil );
                if ( !overlay.isNull() ) {
                    const QString op = outputPathFor( cfg, QStringLiteral( "qpaint-overlay" ) );
                    overlay.save( op );
                    out << "Overlay output: '" << op << "'\n";
                }
            }

            if ( cfg.emitDiff ) {
                const QImage diff = renderDiffMask( img, sil );
                if ( !diff.isNull() ) {
                    const QString dp = outputPathFor( cfg, QStringLiteral( "qpaint-diff" ) );
                    diff.save( dp );
                    out << "Diff output: '" << dp << "'\n";
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
    QTextStream out( stdout );
    out << "Scenario: " << tag << "\n";

    // Also print font facts from the QFont side so we can compare fallback behaviour.
    {
        QFont f;
        f.setFamily( cfg.family );
        f.setPixelSize( cfg.pixelSize );
        printFontFacts( out, f );
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
        out << "QML component not ready. status=" << component.status() << "\n";
        out << component.errorString() << "\n";
        const QList<QQmlError> errors = component.errors();
        for ( const QQmlError &e : errors ) {
            out << "QML error: " << e.toString() << "\n";
        }

        out << "QML source:\n";
        out << qml << "\n";
        return 3;
    }

    QObject *rootObj = component.create();
    QQuickItem *rootItem = qobject_cast<QQuickItem *>( rootObj );
    if ( rootItem == nullptr ) {
        out << "Root object is not a QQuickItem.\n";
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
        out << "grabWindow returned null image.\n";
        out << "Output (not written): '" << path << "'\n";
        return 5;
    }

    if ( !img.save( path ) ) {
        out << "Save failed: '" << path << "'\n";
        return 6;
    }

    out << "Output: '" << path << "'\n";
    out << "Image sha256: " << sha256HexImage( img ) << "\n";
    return 0;
}

int runQtQuickScrollStress( const RunConfig &cfg, const QString &renderType, const QString &tag )
{
    QTextStream out( stdout );
    out << "Scenario: " << tag << "\n";

    QFont f;
    f.setFamily( cfg.family );
    f.setPixelSize( cfg.pixelSize );
    printFontFacts( out, f );

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

    out << "Families in model: " << families.size() << "\n";
    out << "Requested family index: " << families.indexOf( cfg.family ) << "\n";
    if ( families.size() > 0 ) {
        out << "First family: '" << families.first() << "'\n";
        out << "Last family: '" << families.last() << "'\n";
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
        out << "QML component not ready. status=" << component.status() << "\n";
        out << component.errorString() << "\n";
        return 3;
    }

    QObject *rootObj = component.create();
    QQuickItem *rootItem = qobject_cast<QQuickItem *>( rootObj );
    if ( rootItem == nullptr ) {
        out << "Root object is not a QQuickItem.\n";
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
        out << "ListView not found.\n";
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

        out << "Scan mode: true count=" << scanCount << " stridePx=" << stride << "\n";
        out.flush();

        for ( int i = 0; i < scanCount; ++i ) {
            const double y = qMin( maxY, qMax( 0.0, static_cast<double>( i * stride ) ) );
            const QString family = families.at( i );

            out << "Index " << i << " family='" << family << "' set contentY=" << y << " ...\n";
            out.flush();

            QElapsedTimer timer;
            timer.start();

            lvObj->setProperty( "contentY", y );
            QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
            out << "  after events; grabbing...\n";
            out.flush();

            const QImage img = view.grabWindow();

            const qint64 ms = timer.elapsed();
            out << "  elapsedMs=" << ms << "\n";
            out.flush();

            if ( !img.isNull() && ms >= 1000 ) {
                const QString slowPath = outputPathForIndex( cfg, tag + QStringLiteral( "-slow" ), i, family );
                img.save( slowPath );
                out << "  slow snapshot: '" << slowPath << "' sha256=" << sha256HexImage( img ) << "\n";
                out.flush();
            }

            if ( ms > worstMs ) {
                worstMs = ms;
                worstFamily = family;
                worstStep = i;
            }
        }
    } else {
        const int steps = qMax( 3, cfg.stressSteps );

        out << "Scan mode: false steps=" << steps << "\n";
        out.flush();

        for ( int i = 0; i < steps; ++i ) {
            const double t = ( steps > 1 ) ? ( static_cast<double>( i ) / static_cast<double>( steps - 1 ) ) : 0.0;
            const double y = t * maxY;

            // Approximate which row is centred.
            const int approxIndex = ( rowH > 0 ) ? static_cast<int>( ( y + viewportHeight * 0.5 ) / static_cast<double>( rowH + 4 ) ) : -1;
            QString approxFamily;
            if ( approxIndex >= 0 && approxIndex < families.size() ) {
                approxFamily = families.at( approxIndex );
            }

            out << "Step " << i << "/" << ( steps - 1 ) << " contentY=" << y
                << " approxFamily='" << approxFamily << "' ...\n";
            out.flush();

            QElapsedTimer timer;
            timer.start();

            lvObj->setProperty( "contentY", y );
            QCoreApplication::processEvents( QEventLoop::AllEvents, 50 );
            out << "  after events; grabbing...\n";
            out.flush();

            const QImage img = view.grabWindow();
            Q_UNUSED( img );

            const qint64 ms = timer.elapsed();
            out << "  elapsedMs=" << ms << "\n";
            out.flush();

            if ( ms > worstMs ) {
                worstMs = ms;
                worstFamily = approxFamily;
                worstStep = i;
            }
        }
    }

    out << "Worst elapsedMs: " << worstMs << " at step " << worstStep << " approxFamily='" << worstFamily << "'\n";

    const QImage finalImg = view.grabWindow();
    const QString path = outputPathFor( cfg, tag );
    if ( !finalImg.isNull() ) {
        finalImg.save( path );
        out << "Output: '" << path << "'\n";
        out << "Image sha256: " << sha256HexImage( finalImg ) << "\n";
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

    QTextStream out( stderr );
    out << "Unknown scenario: '" << cfg.scenario << "'\n";
    return 7;
}

int runOrchestrator( const RunConfig &cfg, const QString &exePath, const int timeoutMs )
{
    QTextStream out( stdout );
    out << "vcstream_fontprobe\n";
    out << "Qt version: " << QT_VERSION_STR << "\n";
    out << "Platform: " << QGuiApplication::platformName() << "\n";
    out << "Family: '" << cfg.family << "'\n";
    out << "Pixel size: " << cfg.pixelSize << "\n";
    out << "Image size: " << cfg.width << "x" << cfg.height << "\n";
    out << "Output dir: '" << cfg.outDir << "'\n";
    out << "Timeout (per scenario): " << timeoutMs << " ms\n";

    QStringList scenarios = {
        QStringLiteral( "qpaint" ),
        QStringLiteral( "quick-qt" ),
        QStringLiteral( "quick-native" ),
    };

    if ( cfg.stress ) {
        scenarios << QStringLiteral( "quick-scroll-qt" )
                  << QStringLiteral( "quick-scroll-native" );
    }

    int worst = 0;

    int skipped = 0;

    for ( const QString &scenario : scenarios ) {
        out << "\n--- Running: " << scenario << " ---\n";
        out.flush();

        QProcess p;
        QStringList args;
        args << QStringLiteral( "--worker" )
             << QStringLiteral( "--scenario" ) << scenario
             << QStringLiteral( "--family" ) << cfg.family
             << QStringLiteral( "--text" ) << cfg.text
             << QStringLiteral( "--width" ) << QString::number( cfg.width )
             << QStringLiteral( "--height" ) << QString::number( cfg.height )
             << QStringLiteral( "--pixel-size" ) << QString::number( cfg.pixelSize )
             << QStringLiteral( "--warmup-ms" ) << QString::number( cfg.warmupMs )
             << QStringLiteral( "--stress" ) << ( cfg.stress ? QStringLiteral( "1" ) : QStringLiteral( "0" ) )
             << QStringLiteral( "--stress-steps" ) << QString::number( cfg.stressSteps )
             << QStringLiteral( "--stress-max-families" ) << QString::number( cfg.stressMaxFamilies )
             << QStringLiteral( "--emit-silhouette" ) << ( cfg.emitSilhouette ? QStringLiteral( "1" ) : QStringLiteral( "0" ) )
             << QStringLiteral( "--emit-overlay" ) << ( cfg.emitOverlay ? QStringLiteral( "1" ) : QStringLiteral( "0" ) )
             << QStringLiteral( "--emit-diff" ) << ( cfg.emitDiff ? QStringLiteral( "1" ) : QStringLiteral( "0" ) )
             << QStringLiteral( "--out-dir" ) << cfg.outDir;

        p.setProgram( exePath );
        p.setArguments( args );
        p.setProcessChannelMode( QProcess::MergedChannels );
        p.start();

        if ( !p.waitForStarted( 2000 ) ) {
            out << "Failed to start worker.\n";
            worst = qMax( worst, 100 );
            continue;
        }

        if ( !p.waitForFinished( timeoutMs ) ) {
            out << "Timeout; killing worker.\n";
            p.kill();
            p.waitForFinished( 2000 );
            worst = qMax( worst, 124 );
            continue;
        }

        const QByteArray log = p.readAll();
        out << QString::fromUtf8( log );

        const int code = p.exitCode();
        if ( p.exitStatus() != QProcess::NormalExit ) {
            out << "Worker crashed.\n";
            worst = qMax( worst, 125 );
        } else if ( code != 0 ) {
            const bool isQuickScenario = scenario.startsWith( QStringLiteral( "quick-" ) );
            const bool treatAsSkipped = isQuickScenario && ( code == 3 || code == 4 || code == 5 );
            if ( treatAsSkipped ) {
                out << "Worker exitCode: " << code << " (skipped; Qt Quick may be unavailable in this environment)\n";
                ++skipped;
            } else {
                out << "Worker exitCode: " << code << "\n";
                worst = qMax( worst, code );
            }
        }
    }

    if ( skipped > 0 ) {
        out << "\nNote: " << skipped << " Qt Quick scenario(s) were skipped.\n";
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
            QTextStream err( stderr );
            err << dirError << "\n";
            return 2;
        }

        if ( cfg.family.isEmpty() ) {
            QTextStream err( stderr );
            err << "Missing required --family argument.\n";
            return 2;
        }

        if ( parser.isSet( workerOpt ) ) {
            if ( cfg.scenario.isEmpty() ) {
                QTextStream err( stderr );
                err << "Missing required --scenario in --worker mode.\n";
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
