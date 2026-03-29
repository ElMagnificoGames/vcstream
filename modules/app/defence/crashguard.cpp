#include "modules/app/defence/crashguard.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <QCoreApplication>
#include <QtGlobal>

namespace crashguard {
namespace {

void writeToStderr( const char *text ) noexcept
{
    if ( text ) {
        std::fputs( text, stderr );
        std::fflush( stderr );
    }
}

void appendf( char *buffer, const std::size_t capacity, std::size_t *pos, const char *fmt, ... ) noexcept
{
    va_list args;
    int written;
    std::size_t remaining;
    std::size_t toAdvance;
    bool canAppend;

    canAppend = ( buffer != nullptr && pos != nullptr && fmt != nullptr && capacity > 0 );

    if ( canAppend ) {
        if ( *pos >= capacity ) {
            buffer[ capacity - 1 ] = '\0';
        } else {
            remaining = capacity - *pos;

            va_start( args, fmt );
            written = std::vsnprintf( buffer + *pos, remaining, fmt, args );
            va_end( args );

            if ( written < 0 ) {
                buffer[ capacity - 1 ] = '\0';
            } else {
                if ( static_cast<std::size_t>( written ) >= remaining ) {
                    toAdvance = remaining - 1;
                } else {
                    toAdvance = static_cast<std::size_t>( written );
                }

                *pos += toAdvance;
            }
        }
    }
}

void terminateHandler() noexcept
{
    char message[ 1024 ];
    std::size_t pos;

    pos = 0;
    message[ 0 ] = '\0';

    appendf( message, sizeof( message ), &pos, "FATAL: An unexpected error occurred and the application must close.\n" );
    appendf( message, sizeof( message ), &pos, "Technical details: std::terminate() called (unhandled exception or noexcept violation)\n" );

    writeToStderr( message );
    qCritical( "%s", message );

    std::abort();
}

}

void installTerminateHandler() noexcept
{
    std::set_terminate( terminateHandler );
}

void requestAppExit( const int exitCode ) noexcept
{
    QCoreApplication *app = QCoreApplication::instance();

    if ( app != nullptr ) {
        QCoreApplication::exit( exitCode );
    }
}

void logUnhandledException( const char *context, const char *reason, const char *technicalDetails ) noexcept
{
    char message[ 2048 ];
    std::size_t pos;

    const char *safeContext = context ? context : "(null context)";
    const char *safeReason = reason ? reason : "(no reason)";
    const char *safeTechnical = technicalDetails ? technicalDetails : "(no technical details)";

    pos = 0;
    message[ 0 ] = '\0';

    appendf( message, sizeof( message ), &pos, "FATAL: An unexpected error occurred and the application must close.\n" );
    appendf( message, sizeof( message ), &pos, "Reason: %s\n", safeReason );
    appendf( message, sizeof( message ), &pos, "Context: %s\n", safeContext );
    appendf( message, sizeof( message ), &pos, "Technical details: %s\n", safeTechnical );
    appendf( message, sizeof( message ), &pos, "If this keeps happening, please report it with the details above.\n" );

    writeToStderr( message );
    qCritical( "%s", message );
}

}
