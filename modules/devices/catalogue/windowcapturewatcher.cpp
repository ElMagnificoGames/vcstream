#include "modules/devices/catalogue/windowcapturewatcher.h"

#include <algorithm>

#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantMap>

#if __has_include( <QWindowCapture> )
#include <QWindowCapture>
#endif

namespace {

bool stringLessByCaseInsensitiveThenCaseSensitive( const QString &a, const QString &b )
{
    bool out;

    out = false;

    const int ci = a.compare( b, Qt::CaseInsensitive );
    if ( ci < 0 ) {
        out = true;
    } else {
        if ( ci > 0 ) {
            out = false;
        } else {
            out = ( a < b );
        }
    }

    return out;
}

#if __has_include( <QWindowCapture> )
bool windowSetEquals( const QList<QCapturableWindow> &a, const QList<QCapturableWindow> &b )
{
    bool out;

    out = false;

    if ( a.size() == b.size() ) {
        QVector<char> used;
        used.resize( b.size() );
        used.fill( 0 );

        bool ok;
        ok = true;

        for ( int i = 0; i < a.size(); ++i ) {
            bool found;
            found = false;

            for ( int j = 0; j < b.size(); ++j ) {
                if ( !found ) {
                    if ( used.at( j ) == 0 ) {
                        if ( a.at( i ) == b.at( j ) ) {
                            used[ j ] = 1;
                            found = true;
                        }
                    }
                }
            }

            if ( !found ) {
                ok = false;
            }
        }

        out = ok;
    }

    return out;
}

QString storedDescriptionForWindow( const QCapturableWindow &w, const QList<QCapturableWindow> &windows, const QStringList &descriptions )
{
    QString out;

    out = QString();

    for ( int i = 0; i < windows.size(); ++i ) {
        if ( windows.at( i ) == w ) {
            if ( i >= 0 && i < descriptions.size() ) {
                out = descriptions.at( i );
            }
        }
    }

    return out;
}

QString storedIdForWindow( const QCapturableWindow &w, const QList<QCapturableWindow> &windows, const QStringList &ids )
{
    QString out;

    out = QString();

    for ( int i = 0; i < windows.size(); ++i ) {
        if ( windows.at( i ) == w ) {
            if ( i >= 0 && i < ids.size() ) {
                out = ids.at( i );
            }
        }
    }

    return out;
}
#endif

}

struct WindowCaptureWatcher::Impl {
    QTimer *windowPollTimer;
    QTimer *windowTitleRefreshTimer;

    QStringList windowNamesVisible;
    QList<WindowInfo> windowPending;
    bool windowTitlesDirty;

#if __has_include( <QWindowCapture> )
    QList<QCapturableWindow> windowIdentityVisible;
    QStringList windowIdentityDescriptions;
    QStringList windowIdentityIds;
    quint64 nextWindowId;
#endif
};

WindowCaptureWatcher::WindowCaptureWatcher( QObject *parent )
    : QObject( parent )
    , m_impl( new Impl() )
{
    m_impl->windowPollTimer = nullptr;
    m_impl->windowTitleRefreshTimer = nullptr;
    m_impl->windowNamesVisible.clear();
    m_impl->windowPending.clear();
    m_impl->windowTitlesDirty = false;

#if __has_include( <QWindowCapture> )
    m_impl->windowIdentityVisible.clear();
    m_impl->windowIdentityDescriptions.clear();
    m_impl->windowIdentityIds.clear();
    m_impl->nextWindowId = 1;
#endif

    m_impl->windowPollTimer = new QTimer( this );
    m_impl->windowPollTimer->setInterval( 2000 );
    QObject::connect( m_impl->windowPollTimer, &QTimer::timeout, this, &WindowCaptureWatcher::onPollTick );

    m_impl->windowTitleRefreshTimer = new QTimer( this );
    m_impl->windowTitleRefreshTimer->setInterval( 30000 );
    QObject::connect( m_impl->windowTitleRefreshTimer, &QTimer::timeout, this, &WindowCaptureWatcher::onTitleRefreshTick );

    m_impl->windowPollTimer->start();
    m_impl->windowTitleRefreshTimer->start();
}

WindowCaptureWatcher::~WindowCaptureWatcher() = default;

void WindowCaptureWatcher::refreshNow()
{
    const bool hasGuiApp = ( qobject_cast<QGuiApplication *>( QCoreApplication::instance() ) != nullptr );
    if ( hasGuiApp ) {
#if __has_include( <QWindowCapture> )
        const QList<QCapturableWindow> windowList = QWindowCapture::capturableWindows();

    const bool setChanged = !windowSetEquals( windowList, m_impl->windowIdentityVisible );
    bool titlesChanged;

    titlesChanged = false;

    if ( setChanged ) {
        // Keep window ids stable within a process run.
        QStringList newIds;
        newIds.reserve( windowList.size() );

        for ( const QCapturableWindow &w : windowList ) {
            QString existingId = storedIdForWindow( w, m_impl->windowIdentityVisible, m_impl->windowIdentityIds );
            if ( existingId.isEmpty() ) {
                existingId = QStringLiteral( "w%1" ).arg( m_impl->nextWindowId );
                m_impl->nextWindowId += 1;
            }
            newIds.append( existingId );
        }

        m_impl->windowIdentityVisible = windowList;
        m_impl->windowIdentityIds = newIds;

        m_impl->windowIdentityDescriptions.clear();
        for ( const QCapturableWindow &w : windowList ) {
            m_impl->windowIdentityDescriptions.append( w.description() );
        }

        QList<WindowInfo> infos;
        infos.reserve( windowList.size() );
        for ( int i = 0; i < windowList.size(); ++i ) {
            WindowInfo info;
            info.name = windowList.at( i ).description();
            if ( i >= 0 && i < m_impl->windowIdentityIds.size() ) {
                info.id = m_impl->windowIdentityIds.at( i );
            }
            infos.append( info );
        }

        std::sort( infos.begin(), infos.end(),
            []( const WindowInfo &a, const WindowInfo &b ) { return stringLessByCaseInsensitiveThenCaseSensitive( a.name, b.name ); } );

        QVariantList windows;
        QStringList names;
        for ( const WindowInfo &info : infos ) {
            QVariantMap m;
            m.insert( QStringLiteral( "name" ), info.name );
            m.insert( QStringLiteral( "id" ), info.id );
            windows.append( m );
            names.append( info.name );
        }

        Q_EMIT windowSnapshotAvailable( windows, QString() );

        m_impl->windowNamesVisible = names;
        m_impl->windowPending.clear();
        m_impl->windowTitlesDirty = false;
    } else {
        // Same set of windows; detect title churn and defer the visible update.
        for ( const QCapturableWindow &w : windowList ) {
            const QString before = storedDescriptionForWindow( w, m_impl->windowIdentityVisible, m_impl->windowIdentityDescriptions );
            const QString after = w.description();
            if ( before != after ) {
                titlesChanged = true;
            }
        }

        if ( titlesChanged ) {
            QList<WindowInfo> pending;
            pending.reserve( windowList.size() );
            for ( const QCapturableWindow &w : windowList ) {
                WindowInfo info;
                info.name = w.description();
                info.id = storedIdForWindow( w, m_impl->windowIdentityVisible, m_impl->windowIdentityIds );
                pending.append( info );
            }

            std::sort( pending.begin(), pending.end(),
                []( const WindowInfo &a, const WindowInfo &b ) { return stringLessByCaseInsensitiveThenCaseSensitive( a.name, b.name ); } );

            m_impl->windowPending = pending;
            m_impl->windowTitlesDirty = true;
        }
    }
#endif
    }
}

void WindowCaptureWatcher::refreshTitlesIfDirty()
{
    const bool hasGuiApp = ( qobject_cast<QGuiApplication *>( QCoreApplication::instance() ) != nullptr );
    if ( hasGuiApp ) {
#if __has_include( <QWindowCapture> )
        if ( m_impl->windowTitlesDirty ) {
        if ( !m_impl->windowPending.isEmpty() ) {
            QVariantList windows;
            QStringList names;
            for ( const WindowInfo &info : m_impl->windowPending ) {
                QVariantMap m;
                m.insert( QStringLiteral( "name" ), info.name );
                m.insert( QStringLiteral( "id" ), info.id );
                windows.append( m );
                names.append( info.name );
            }

            Q_EMIT windowSnapshotAvailable( windows, QString() );

            m_impl->windowNamesVisible = names;
            m_impl->windowPending.clear();
        }

        // Refresh the stored descriptions snapshot so we do not repeatedly
        // report the same title churn.
        {
            const QList<QCapturableWindow> windowList = QWindowCapture::capturableWindows();
            if ( windowSetEquals( windowList, m_impl->windowIdentityVisible ) ) {
                QStringList currentAligned;
                QStringList aligned;

                for ( const QCapturableWindow &w : windowList ) {
                    currentAligned.append( w.description() );
                }

                for ( const QCapturableWindow &w : m_impl->windowIdentityVisible ) {
                    aligned.append( storedDescriptionForWindow( w, windowList, currentAligned ) );
                }

                if ( aligned.size() == m_impl->windowIdentityVisible.size() ) {
                    m_impl->windowIdentityDescriptions = aligned;
                }
            } else {
                // The set changed while we were waiting; let the fast poll handle it.
            }
        }

        m_impl->windowTitlesDirty = false;
        }
#endif
    }
}

void WindowCaptureWatcher::onPollTick()
{
    refreshNow();
}

void WindowCaptureWatcher::onTitleRefreshTick()
{
    refreshTitlesIfDirty();
}
