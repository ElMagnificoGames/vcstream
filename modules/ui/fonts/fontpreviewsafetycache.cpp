#include "modules/ui/fonts/fontpreviewsafetycache.h"

#include "modules/ui/fonts/fontpreviewsafetychecker.h"
#include "modules/ui/fonts/fontpreviewsafetycheckerdefault.h"

#include <QMutexLocker>
#include <QTimer>

namespace {

#define VCSTREAM_FONT_PREVIEW_SAFETY_DEFAULT_MAX_STUCK_CHECKS 8
#define VCSTREAM_FONT_PREVIEW_SAFETY_DEFAULT_TIMEOUT_MS 15000

}

FontPreviewSafetyCache::Config FontPreviewSafetyCache::defaultConfig()
{
    FontPreviewSafetyCache::Config c;
    c.timeoutMs = VCSTREAM_FONT_PREVIEW_SAFETY_DEFAULT_TIMEOUT_MS;
    c.maxStuckChecks = VCSTREAM_FONT_PREVIEW_SAFETY_DEFAULT_MAX_STUCK_CHECKS;
    return c;
}

FontPreviewSafetyCache::FontPreviewSafetyCache( QObject *parent )
    : FontPreviewSafetyCache( nullptr, defaultConfig(), parent )
{
}

FontPreviewSafetyCache::FontPreviewSafetyCache( FontPreviewSafetyChecker *checker, const FontPreviewSafetyCache::Config &config, QObject *parent )
    : QObject( parent )
{
    m_stuckChecks = 0;
    m_config = config;
    m_checker = checker;
    m_ownChecker = false;

    if ( m_config.timeoutMs <= 0 ) {
        m_config.timeoutMs = VCSTREAM_FONT_PREVIEW_SAFETY_DEFAULT_TIMEOUT_MS;
    }
    if ( m_config.maxStuckChecks <= 0 ) {
        m_config.maxStuckChecks = VCSTREAM_FONT_PREVIEW_SAFETY_DEFAULT_MAX_STUCK_CHECKS;
    }

    if ( m_checker == nullptr ) {
        m_checker = new FontPreviewSafetyCheckerDefault( this );
        m_ownChecker = true;
    }

    QObject::connect( m_checker, &FontPreviewSafetyChecker::checkFinished, this, &FontPreviewSafetyCache::onCheckFinished, Qt::QueuedConnection );
}

FontPreviewSafetyCache::~FontPreviewSafetyCache()
{
    m_countedStuck.clear();

    if ( m_ownChecker && m_checker != nullptr ) {
        // Owned checkers are parented to this, so they will be deleted by QObject.
        m_checker = nullptr;
    }
}

FontPreviewSafetyCache::Key FontPreviewSafetyCache::makeKey( const QString &family, const int pixelSize )
{
    Key k;
    k.family = family;
    k.pixelSize = pixelSize;
    return k;
}

FontPreviewSafetyCache::Status FontPreviewSafetyCache::statusForFamily( const QString &family, const int pixelSize ) const
{
    QMutexLocker lock( &m_mutex );
    const Key k = makeKey( family, pixelSize );
    return m_status.value( k, Status::NeverChecked );
}

void FontPreviewSafetyCache::startCheck( const QString &family, const int pixelSize )
{
    if ( m_checker != nullptr ) {
        m_checker->startCheck( family, pixelSize );
    }

    QTimer::singleShot( m_config.timeoutMs, this, [this, family, pixelSize]() {
        this->onCheckTimedOut( family, pixelSize );
    } );
}

void FontPreviewSafetyCache::requestCheck( const QString &family, const int pixelSize )
{
    bool shouldStart;
    Status emitStatus;
    bool shouldEmit;

    shouldStart = false;
    emitStatus = Status::NeverChecked;
    shouldEmit = false;

    {
        const QMutexLocker lock( &m_mutex );
        const Key k = makeKey( family, pixelSize );
        const Status current = m_status.value( k, Status::NeverChecked );

        if ( current == Status::Safe || current == Status::Bad || current == Status::Checking || current == Status::TimedOut ) {
            return;
        }

        if ( m_stuckChecks >= m_config.maxStuckChecks ) {
            if ( current != Status::Blocked ) {
                m_status.insert( k, Status::Blocked );
                emitStatus = Status::Blocked;
                shouldEmit = true;
            }
        } else {
            m_status.insert( k, Status::Checking );
            shouldStart = true;
            emitStatus = Status::Checking;
            shouldEmit = true;
        }
    }

    if ( shouldEmit ) {
        Q_EMIT statusChanged( family, pixelSize, emitStatus );
    }

    if ( shouldStart ) {
        startCheck( family, pixelSize );
    }
}

void FontPreviewSafetyCache::onCheckTimedOut( const QString &family, const int pixelSize )
{
    bool shouldEmit;

    shouldEmit = false;

    {
        const QMutexLocker lock( &m_mutex );
        const Key k = makeKey( family, pixelSize );
        const Status current = m_status.value( k, Status::NeverChecked );
        if ( current != Status::Checking ) {
            return;
        }

        m_status.insert( k, Status::TimedOut );

        if ( !m_countedStuck.value( k, false ) ) {
            m_countedStuck.insert( k, true );
            ++m_stuckChecks;
        }

        shouldEmit = true;
    }

    if ( shouldEmit ) {
        Q_EMIT statusChanged( family, pixelSize, Status::TimedOut );
    }
}

void FontPreviewSafetyCache::onCheckFinished( const QString &family, const int pixelSize, const bool ok )
{
    const Status next = ok ? Status::Safe : Status::Bad;
    bool shouldEmit;
    bool countedStuck;

    shouldEmit = false;
    countedStuck = false;

    {
        const QMutexLocker lock( &m_mutex );
        const Key k = makeKey( family, pixelSize );
        const Status current = m_status.value( k, Status::NeverChecked );

        if ( current != Status::Checking && current != Status::TimedOut ) {
            return;
        }

        m_status.insert( k, next );
        shouldEmit = true;

        countedStuck = m_countedStuck.value( k, false );
        if ( countedStuck && m_stuckChecks > 0 ) {
            --m_stuckChecks;
        }

        m_countedStuck.remove( k );
    }

    if ( shouldEmit ) {
        Q_EMIT statusChanged( family, pixelSize, next );
    }
}
