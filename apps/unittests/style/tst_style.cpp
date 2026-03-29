#include <QtTest/QTest>

#include <QCoreApplication>
#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include "modules/app/defence/crashguard.h"

class tst_Style : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void source_noStreamingOperatorChains();
    void source_noVoidEarlyReturns();
    void source_singleExitInFunctionBodies();
    void source_noBreakOrContinueInLoops();
};

namespace {

enum class ParenContext
{
    Other,
    ControlHeader,
    LambdaParams,
};

enum class BraceContext
{
    Other,
    Control,
    Function,
    Lambda,
    TypeOrNamespace,
};

struct ReturnInfo
{
    qsizetype startIndex;
    qsizetype endIndex;
    int relativeBraceDepth;
};

struct BraceFrame
{
    BraceContext context;
    qsizetype openIndex;
    int openLine;
    int openBraceDepth;
    QList<ReturnInfo> returns;
};

enum class BreakableContext
{
    Other,
    Loop,
    Switch,
};

QStringList sourceRoots()
{
    const QString root = QString::fromUtf8( VCSTREAM_SOURCE_DIR );
    return QStringList{
        QDir( root ).filePath( QStringLiteral( "apps" ) ),
        QDir( root ).filePath( QStringLiteral( "modules" ) ),
    };
}

bool shouldSkipPath( const QString &absPath )
{
    return absPath.contains( QStringLiteral( "/third_party/" ) )
        || absPath.contains( QStringLiteral( "/build/" ) )
        || absPath.endsWith( QStringLiteral( "/apps/unittests/style/tst_style.cpp" ) );
}

bool isQtTestRowExceptionLine( const QString &line )
{
    return line.contains( QStringLiteral( "QTest::newRow" ) )
        || line.contains( QStringLiteral( "QTest::addRow" ) );
}

QString stripCommentsAndLiterals( const QString &text )
{
    QString out;
    out.reserve( text.size() );

    qsizetype i;
    const qsizetype n = text.size();
    bool inLineComment;
    bool inBlockComment;
    bool inString;
    bool inChar;

    inLineComment = false;
    inBlockComment = false;
    inString = false;
    inChar = false;

    for ( i = 0; i < n; ++i ) {
        const QChar ch = text.at( i );
        const QChar next = ( i + 1 < n ? text.at( i + 1 ) : QChar() );

        if ( inLineComment ) {
            if ( ch == QLatin1Char( '\n' ) ) {
                inLineComment = false;
                out.append( ch );
            } else {
                out.append( QLatin1Char( ' ' ) );
            }
            continue;
        }

        if ( inBlockComment ) {
            if ( ch == QLatin1Char( '*' ) && next == QLatin1Char( '/' ) ) {
                inBlockComment = false;
                out.append( QLatin1Char( ' ' ) );
                out.append( QLatin1Char( ' ' ) );
                ++i;
            } else {
                if ( ch == QLatin1Char( '\n' ) ) {
                    out.append( ch );
                } else {
                    out.append( QLatin1Char( ' ' ) );
                }
            }
            continue;
        }

        if ( inString ) {
            if ( ch == QLatin1Char( '\\' ) ) {
                out.append( QLatin1Char( ' ' ) );
                if ( i + 1 < n ) {
                    out.append( QLatin1Char( ' ' ) );
                    ++i;
                }
            } else if ( ch == QLatin1Char( '"' ) ) {
                inString = false;
                out.append( QLatin1Char( ' ' ) );
            } else {
                if ( ch == QLatin1Char( '\n' ) ) {
                    out.append( ch );
                } else {
                    out.append( QLatin1Char( ' ' ) );
                }
            }
            continue;
        }

        if ( inChar ) {
            if ( ch == QLatin1Char( '\\' ) ) {
                out.append( QLatin1Char( ' ' ) );
                if ( i + 1 < n ) {
                    out.append( QLatin1Char( ' ' ) );
                    ++i;
                }
            } else if ( ch == QLatin1Char( '\'' ) ) {
                inChar = false;
                out.append( QLatin1Char( ' ' ) );
            } else {
                if ( ch == QLatin1Char( '\n' ) ) {
                    out.append( ch );
                } else {
                    out.append( QLatin1Char( ' ' ) );
                }
            }
            continue;
        }

        if ( ch == QLatin1Char( '/' ) && next == QLatin1Char( '/' ) ) {
            inLineComment = true;
            out.append( QLatin1Char( ' ' ) );
            out.append( QLatin1Char( ' ' ) );
            ++i;
            continue;
        }

        if ( ch == QLatin1Char( '/' ) && next == QLatin1Char( '*' ) ) {
            inBlockComment = true;
            out.append( QLatin1Char( ' ' ) );
            out.append( QLatin1Char( ' ' ) );
            ++i;
            continue;
        }

        if ( ch == QLatin1Char( '"' ) ) {
            inString = true;
            out.append( QLatin1Char( ' ' ) );
            continue;
        }

        if ( ch == QLatin1Char( '\'' ) ) {
            inChar = true;
            out.append( QLatin1Char( ' ' ) );
            continue;
        }

        out.append( ch );
    }

    return out;
}

int lineNumberForIndex( const QString &text, const int index )
{
    int line;

    line = 1;
    const qsizetype target = static_cast<qsizetype>( index );
    const qsizetype n = text.size();
    for ( qsizetype i = 0; i < target && i < n; ++i ) {
        if ( text.at( i ) == QLatin1Char( '\n' ) ) {
            ++line;
        }
    }

    return line;
}

bool isIdentStart( const QChar ch )
{
    return ch.isLetter() || ch == QLatin1Char( '_' );
}

bool isIdentContinue( const QChar ch )
{
    return ch.isLetterOrNumber() || ch == QLatin1Char( '_' );
}

bool isControlKeyword( const QString &ident )
{
    return ident == QStringLiteral( "if" )
        || ident == QStringLiteral( "for" )
        || ident == QStringLiteral( "while" )
        || ident == QStringLiteral( "switch" )
        || ident == QStringLiteral( "catch" );
}

bool isLoopKeyword( const QString &ident )
{
    return ident == QStringLiteral( "for" )
        || ident == QStringLiteral( "while" )
        || ident == QStringLiteral( "do" );
}

BreakableContext breakableContextForKeyword( const QString &ident )
{
    if ( ident == QStringLiteral( "switch" ) ) {
        return BreakableContext::Switch;
    }

    if ( isLoopKeyword( ident ) ) {
        return BreakableContext::Loop;
    }

    return BreakableContext::Other;
}

bool isTypeOrNamespaceKeyword( const QString &ident )
{
    return ident == QStringLiteral( "namespace" )
        || ident == QStringLiteral( "class" )
        || ident == QStringLiteral( "struct" )
        || ident == QStringLiteral( "union" )
        || ident == QStringLiteral( "enum" );
}

BraceContext classifyBraceOpen( const QString &cleaned,
    const int braceIndex,
    const bool hasPendingParen,
    const ParenContext pendingParenContext,
    const QChar lastNonSpaceChar,
    const QString &lastIdentifier )
{
    Q_UNUSED( cleaned );
    Q_UNUSED( braceIndex );

    if ( hasPendingParen ) {
        if ( pendingParenContext == ParenContext::ControlHeader ) {
            return BraceContext::Control;
        }

        if ( pendingParenContext == ParenContext::LambdaParams ) {
            return BraceContext::Lambda;
        }

        return BraceContext::Function;
    }

    if ( lastNonSpaceChar == QLatin1Char( ']' ) ) {
        return BraceContext::Lambda;
    }

    if ( lastNonSpaceChar == QLatin1Char( ')' ) ) {
        return BraceContext::Other;
    }

    if ( isTypeOrNamespaceKeyword( lastIdentifier ) ) {
        return BraceContext::TypeOrNamespace;
    }

    return BraceContext::Other;
}

void checkSingleExitInCleanedText( const QString &path, const QString &cleaned, QStringList *outFailures )
{
    QList<ParenContext> parenStack;
    QList<BraceFrame> braceStack;
    QString lastIdentifier;
    QChar lastNonSpaceChar;
    ParenContext lastClosedParenContext;
    int braceDepth;
    bool hasPendingParen;
    ParenContext pendingParenContext;

    lastIdentifier = QString();
    lastNonSpaceChar = QChar();
    lastClosedParenContext = ParenContext::Other;
    braceDepth = 0;
    hasPendingParen = false;
    pendingParenContext = ParenContext::Other;

    const qsizetype n = cleaned.size();

    for ( qsizetype i = 0; i < n; ++i ) {
        const QChar ch = cleaned.at( i );

        if ( ch.isSpace() ) {
            continue;
        }

        if ( isIdentStart( ch ) ) {
            qsizetype j;
            QString ident;

            j = i;
            while ( j < n && isIdentContinue( cleaned.at( j ) ) ) {
                ++j;
            }

            ident = cleaned.mid( i, j - i );
            lastIdentifier = ident;
            lastNonSpaceChar = QChar();

            if ( ident == QStringLiteral( "return" ) ) {
                // Attribute this return to the innermost function-like brace frame.
                int functionIndex;
                const int braceCount = static_cast<int>( braceStack.size() );

                functionIndex = -1;
                for ( int k = braceCount - 1; k >= 0; --k ) {
                    if ( braceStack.at( k ).context == BraceContext::Function || braceStack.at( k ).context == BraceContext::Lambda ) {
                        functionIndex = k;
                        break;
                    }
                }

                if ( functionIndex >= 0 ) {
                    qsizetype end;
                    int relDepth;

                    end = j;
                    while ( end < n && cleaned.at( end ) != QLatin1Char( ';' ) ) {
                        ++end;
                    }

                    if ( end < n ) {
                        relDepth = ( braceDepth - braceStack.at( functionIndex ).openBraceDepth ) + 1;

                        ReturnInfo ri;
                        ri.startIndex = i;
                        ri.endIndex = end;
                        ri.relativeBraceDepth = relDepth;

                        braceStack[ functionIndex ].returns.append( ri );
                    }
                }
            }

            i = j - 1;
            continue;
        }

        if ( ch == QLatin1Char( '(' ) ) {
            ParenContext ctx;

            ctx = ParenContext::Other;
            if ( isControlKeyword( lastIdentifier ) ) {
                ctx = ParenContext::ControlHeader;
            } else {
                if ( lastNonSpaceChar == QLatin1Char( ']' ) ) {
                    ctx = ParenContext::LambdaParams;
                }
            }

            parenStack.append( ctx );
            lastNonSpaceChar = ch;
            continue;
        }

        if ( ch == QLatin1Char( ')' ) ) {
            if ( !parenStack.isEmpty() ) {
                lastClosedParenContext = parenStack.takeLast();
            } else {
                lastClosedParenContext = ParenContext::Other;
            }

            hasPendingParen = true;
            pendingParenContext = lastClosedParenContext;

            lastNonSpaceChar = ch;
            continue;
        }

        if ( ch == QLatin1Char( '{' ) ) {
            BraceFrame frame;
            frame.context = classifyBraceOpen( cleaned,
                static_cast<int>( i ),
                hasPendingParen,
                pendingParenContext,
                lastNonSpaceChar,
                lastIdentifier );
            frame.openIndex = i;
            frame.openLine = lineNumberForIndex( cleaned, static_cast<int>( i ) );
            frame.openBraceDepth = braceDepth + 1;
            frame.returns.clear();

            braceDepth += 1;
            braceStack.append( frame );

            hasPendingParen = false;
            pendingParenContext = ParenContext::Other;

            lastNonSpaceChar = ch;
            continue;
        }

        if ( ch == QLatin1Char( '}' ) ) {
            if ( braceDepth > 0 ) {
                braceDepth -= 1;
            }

            if ( !braceStack.isEmpty() ) {
                const BraceFrame frame = braceStack.takeLast();

                if ( frame.context == BraceContext::Function || frame.context == BraceContext::Lambda ) {
                    if ( frame.returns.size() > 1 ) {
                        const int firstLine = lineNumberForIndex( cleaned, static_cast<int>( frame.returns.at( 0 ).startIndex ) );
                        const int secondLine = lineNumberForIndex( cleaned, static_cast<int>( frame.returns.at( 1 ).startIndex ) );
                        outFailures->append( QStringLiteral( "%1:%2: multiple return statements in a single function-like body (also at %3)" )
                                                .arg( path )
                                                .arg( firstLine )
                                                .arg( secondLine ) );
                    } else {
                        if ( frame.returns.size() == 1 ) {
                            const ReturnInfo ri = frame.returns.first();
                            const int retLine = lineNumberForIndex( cleaned, static_cast<int>( ri.startIndex ) );

                            if ( ri.relativeBraceDepth != 1 ) {
                                outFailures->append( QStringLiteral( "%1:%2: return is not at function top-level (early return)" )
                                                        .arg( path )
                                                        .arg( retLine ) );
                            } else {
                                // Ensure the return statement is the final statement in the function-like body.
                                bool onlyWhitespace;

                                onlyWhitespace = true;
                                for ( qsizetype k = ri.endIndex + 1; k < i; ++k ) {
                                    const QChar tail = cleaned.at( k );
                                    if ( !tail.isSpace() ) {
                                        // Ignore preprocessor directives which do not represent runtime statements.
                                        if ( tail != QLatin1Char( '#' ) ) {
                                            onlyWhitespace = false;
                                            break;
                                        }
                                    }
                                }

                                if ( !onlyWhitespace ) {
                                    outFailures->append( QStringLiteral( "%1:%2: return is not the final statement in the function-like body" )
                                                            .arg( path )
                                                            .arg( retLine ) );
                                }
                            }
                        }
                    }
                }
            }

            lastNonSpaceChar = ch;
            continue;
        }

        // Track a few punctuation tokens used by the heuristics.
        if ( ch == QLatin1Char( ']' ) || ch == QLatin1Char( ';' ) || ch == QLatin1Char( ':' ) ) {
            lastNonSpaceChar = ch;

            if ( ch == QLatin1Char( ';' ) ) {
                hasPendingParen = false;
                pendingParenContext = ParenContext::Other;
            }
            continue;
        }

        lastNonSpaceChar = ch;
    }
}

void checkNoBreakOrContinueInLoops( const QString &path, const QString &cleaned, QStringList *outFailures )
{
    struct Frame {
        BreakableContext ctx;
    };

    QList<ParenContext> parenStack;
    QList<Frame> braceStack;
    QString lastIdentifier;
    QChar lastNonSpaceChar;

    bool hasPendingControl;
    QString pendingControlKeyword;
    ParenContext pendingParenContext;

    lastIdentifier = QString();
    lastNonSpaceChar = QChar();
    hasPendingControl = false;
    pendingControlKeyword = QString();
    pendingParenContext = ParenContext::Other;

    const qsizetype n = cleaned.size();

    for ( qsizetype i = 0; i < n; ++i ) {
        const QChar ch = cleaned.at( i );

        if ( ch.isSpace() ) {
            continue;
        }

        if ( isIdentStart( ch ) ) {
            qsizetype j;
            QString ident;

            j = i;
            while ( j < n && isIdentContinue( cleaned.at( j ) ) ) {
                ++j;
            }

            ident = cleaned.mid( i, j - i );
            lastIdentifier = ident;
            lastNonSpaceChar = QChar();

            if ( ident == QStringLiteral( "break" ) || ident == QStringLiteral( "continue" ) ) {
                qsizetype k;
                bool isStatement;

                // Only consider `break;` / `continue;` (ignore e.g. labels or identifiers).
                k = j;
                while ( k < n && cleaned.at( k ).isSpace() ) {
                    ++k;
                }

                isStatement = ( k < n && cleaned.at( k ) == QLatin1Char( ';' ) );

                if ( isStatement ) {
                    BreakableContext nearest;

                    nearest = BreakableContext::Other;
                    {
                        const int braceCount = static_cast<int>( braceStack.size() );
                        for ( int s = braceCount - 1; s >= 0; --s ) {
                        if ( braceStack.at( s ).ctx == BreakableContext::Loop || braceStack.at( s ).ctx == BreakableContext::Switch ) {
                            nearest = braceStack.at( s ).ctx;
                            break;
                        }
                        }
                    }

                    const int lineNo = lineNumberForIndex( cleaned, static_cast<int>( i ) );

                    if ( ident == QStringLiteral( "continue" ) ) {
                        if ( nearest == BreakableContext::Loop ) {
                            outFailures->append( QStringLiteral( "%1:%2: forbidden continue in loop" ).arg( path ).arg( lineNo ) );
                        }
                    } else {
                        // `break` is allowed when it breaks out of a switch statement.
                        if ( nearest == BreakableContext::Loop ) {
                            outFailures->append( QStringLiteral( "%1:%2: forbidden break in loop (use a flag and a single exit)" ).arg( path ).arg( lineNo ) );
                        }
                    }
                }
            }

            i = j - 1;
            continue;
        }

        if ( ch == QLatin1Char( '(' ) ) {
            ParenContext ctx;

            ctx = ParenContext::Other;
            if ( isControlKeyword( lastIdentifier ) ) {
                ctx = ParenContext::ControlHeader;
            } else {
                if ( lastNonSpaceChar == QLatin1Char( ']' ) ) {
                    ctx = ParenContext::LambdaParams;
                }
            }

            parenStack.append( ctx );
            lastNonSpaceChar = ch;
            continue;
        }

        if ( ch == QLatin1Char( ')' ) ) {
            ParenContext closed;

            closed = ParenContext::Other;
            if ( !parenStack.isEmpty() ) {
                closed = parenStack.takeLast();
            }

            hasPendingControl = ( closed == ParenContext::ControlHeader );
            pendingParenContext = closed;
            pendingControlKeyword = hasPendingControl ? lastIdentifier : QString();

            lastNonSpaceChar = ch;
            continue;
        }

        if ( ch == QLatin1Char( '{' ) ) {
            Frame frame;

            frame.ctx = BreakableContext::Other;

            if ( hasPendingControl && pendingParenContext == ParenContext::ControlHeader ) {
                frame.ctx = breakableContextForKeyword( pendingControlKeyword );
            } else {
                if ( lastIdentifier == QStringLiteral( "do" ) ) {
                    frame.ctx = BreakableContext::Loop;
                }
            }

            braceStack.append( frame );

            hasPendingControl = false;
            pendingParenContext = ParenContext::Other;
            pendingControlKeyword = QString();

            lastNonSpaceChar = ch;
            continue;
        }

        if ( ch == QLatin1Char( '}' ) ) {
            if ( !braceStack.isEmpty() ) {
                braceStack.takeLast();
            }

            lastNonSpaceChar = ch;
            continue;
        }

        if ( ch == QLatin1Char( ';' ) ) {
            hasPendingControl = false;
            pendingParenContext = ParenContext::Other;
            pendingControlKeyword = QString();
        }

        lastNonSpaceChar = ch;
    }
}

}

void tst_Style::source_noStreamingOperatorChains()
{
    const QStringList roots = sourceRoots();
    QStringList failures;

    for ( const QString &root : roots ) {
        QDirIterator it( root, QStringList{ QStringLiteral( "*.cpp" ), QStringLiteral( "*.h" ) }, QDir::Files, QDirIterator::Subdirectories );
        while ( it.hasNext() ) {
            const QString path = it.next();
            if ( shouldSkipPath( path ) ) {
                continue;
            }

            QFile f( path );
            if ( !f.open( QIODevice::ReadOnly ) ) {
                failures.append( QStringLiteral( "%1: failed to open" ).arg( path ) );
                continue;
            }

            const QByteArray bytes = f.readAll();
            const QString text = QString::fromUtf8( bytes );
            const QStringList lines = text.split( QLatin1Char( '\n' ) );

            for ( int i = 0; i < lines.size(); ++i ) {
                const int lineNo = i + 1;
                const QString line = lines.at( i );

                if ( isQtTestRowExceptionLine( line ) ) {
                    continue;
                }

                // Ban stream/chaining uses of operator<<. Bitshifts are allowed.
                // This is a best-effort text gate; do not add new exceptions unless
                // they are strictly unavoidable due to third-party APIs.
                if ( line.contains( QStringLiteral( "qDebug() <<" ) )
                    || line.contains( QStringLiteral( "qInfo() <<" ) )
                    || line.contains( QStringLiteral( "qWarning() <<" ) )
                    || line.contains( QStringLiteral( "qCritical() <<" ) )
                    || line.contains( QStringLiteral( "qFatal() <<" ) )
                    || line.contains( QStringLiteral( "std::cout <<" ) )
                    || line.contains( QStringLiteral( "std::cerr <<" ) )
                    || line.contains( QStringLiteral( "std::clog <<" ) ) ) {
                    failures.append( QStringLiteral( "%1:%2: forbidden streaming operator chain" ).arg( path ).arg( lineNo ) );
                    continue;
                }

                // Catch common Qt streaming/container chaining patterns without tripping on bitshifts.
                if ( line.contains( QStringLiteral( "<<" ) )
                    && ( line.contains( QLatin1Char( '"' ) )
                         || line.contains( QStringLiteral( "QStringLiteral" ) )
                         || line.contains( QStringLiteral( "QLatin1String" ) )
                         || line.contains( QStringLiteral( "QTextStream" ) )
                         || line.contains( QStringLiteral( "QDataStream" ) )
                         || line.contains( QStringLiteral( "QStringList" ) ) ) ) {
                    failures.append( QStringLiteral( "%1:%2: suspicious operator<< usage (stream/chaining)" ).arg( path ).arg( lineNo ) );
                    continue;
                }
            }
        }
    }

    if ( !failures.isEmpty() ) {
        QFAIL( qPrintable( failures.join( QLatin1Char( '\n' ) ) ) );
    }
}

void tst_Style::source_noVoidEarlyReturns()
{
    const QStringList roots = sourceRoots();
    QStringList failures;

    for ( const QString &root : roots ) {
        QDirIterator it( root, QStringList{ QStringLiteral( "*.cpp" ), QStringLiteral( "*.h" ) }, QDir::Files, QDirIterator::Subdirectories );
        while ( it.hasNext() ) {
            const QString path = it.next();
            if ( shouldSkipPath( path ) ) {
                continue;
            }

            // Style gates apply to production code only.
            if ( path.contains( QStringLiteral( "/apps/unittests/" ) ) ) {
                continue;
            }

            QFile f( path );
            if ( !f.open( QIODevice::ReadOnly ) ) {
                failures.append( QStringLiteral( "%1: failed to open" ).arg( path ) );
                continue;
            }

            const QByteArray bytes = f.readAll();
            const QString text = QString::fromUtf8( bytes );
            const QStringList lines = text.split( QLatin1Char( '\n' ) );

            for ( int i = 0; i < lines.size(); ++i ) {
                const int lineNo = i + 1;
                const QString line = lines.at( i );

                // House rule (CODE-QT6.md): void functions must not use early return.
                // This is a best-effort text gate.
                if ( line.contains( QRegularExpression( QStringLiteral( "\\breturn\\s*;" ) ) ) ) {
                    failures.append( QStringLiteral( "%1:%2: forbidden early return in void function (return;)" ).arg( path ).arg( lineNo ) );
                }
            }
        }
    }

    if ( !failures.isEmpty() ) {
        QFAIL( qPrintable( failures.join( QLatin1Char( '\n' ) ) ) );
    }
}

void tst_Style::source_singleExitInFunctionBodies()
{
    const QStringList roots = sourceRoots();
    QStringList failures;

    for ( const QString &root : roots ) {
        QDirIterator it( root, QStringList{ QStringLiteral( "*.cpp" ), QStringLiteral( "*.h" ) }, QDir::Files, QDirIterator::Subdirectories );
        while ( it.hasNext() ) {
            const QString path = it.next();
            if ( shouldSkipPath( path ) ) {
                continue;
            }

            // Style gates apply to production code only.
            if ( path.contains( QStringLiteral( "/apps/unittests/" ) ) ) {
                continue;
            }

            QFile f( path );
            if ( !f.open( QIODevice::ReadOnly ) ) {
                failures.append( QStringLiteral( "%1: failed to open" ).arg( path ) );
                continue;
            }

            const QByteArray bytes = f.readAll();
            const QString text = QString::fromUtf8( bytes );
            const QString cleaned = stripCommentsAndLiterals( text );

            checkSingleExitInCleanedText( path, cleaned, &failures );
        }
    }

    if ( !failures.isEmpty() ) {
        QFAIL( qPrintable( failures.join( QLatin1Char( '\n' ) ) ) );
    }
}

void tst_Style::source_noBreakOrContinueInLoops()
{
    const QStringList roots = sourceRoots();
    QStringList failures;

    for ( const QString &root : roots ) {
        QDirIterator it( root, QStringList{ QStringLiteral( "*.cpp" ), QStringLiteral( "*.h" ) }, QDir::Files, QDirIterator::Subdirectories );
        while ( it.hasNext() ) {
            const QString path = it.next();
            if ( shouldSkipPath( path ) ) {
                continue;
            }

            // Style gates apply to production code only.
            if ( path.contains( QStringLiteral( "/apps/unittests/" ) ) ) {
                continue;
            }

            QFile f( path );
            if ( !f.open( QIODevice::ReadOnly ) ) {
                failures.append( QStringLiteral( "%1: failed to open" ).arg( path ) );
                continue;
            }

            const QByteArray bytes = f.readAll();
            const QString text = QString::fromUtf8( bytes );
            const QString cleaned = stripCommentsAndLiterals( text );

            checkNoBreakOrContinueInLoops( path, cleaned, &failures );
        }
    }

    if ( !failures.isEmpty() ) {
        QFAIL( qPrintable( failures.join( QLatin1Char( '\n' ) ) ) );
    }
}

int main( int argc, char **argv )
{
    int exitCode;

    crashguard::installTerminateHandler();

    exitCode = crashguard::runGuardedMain( "tst_style main", [argc, argv]() -> int {
        int localArgc;
        char **localArgv;

        localArgc = argc;
        localArgv = argv;

        QCoreApplication app( localArgc, localArgv );
        tst_Style tc;
        const int result = QTest::qExec( &tc, localArgc, localArgv );
        return result;
    } );

    return exitCode;
}

#include "tst_style.moc"
