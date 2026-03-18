#ifndef MODULES_APP_DEFENCE_CRASHGUARD_H
#define MODULES_APP_DEFENCE_CRASHGUARD_H

#include <exception>
#include <new>

namespace crashguard {

enum class UnhandledExceptionPolicy
{
    StopThread,
    RequestAppExit
};

void installTerminateHandler() noexcept;

void requestAppExit( int exitCode ) noexcept;

void logUnhandledException( const char *context, const char *reason, const char *technicalDetails ) noexcept;

template <typename Fn>
int runGuardedMain( const char *context, Fn &&fn ) noexcept
{
    int exitCode;

    exitCode = 1;

    try {
        exitCode = fn();
    } catch ( const std::bad_alloc & ) {
        logUnhandledException( context, "Out of memory.", "std::bad_alloc" );
    } catch ( const std::exception &e ) {
        logUnhandledException( context, "An internal error occurred.", e.what() );
    } catch ( ... ) {
        logUnhandledException( context, "An internal error occurred.", "unknown exception" );
    }

    return exitCode;
}

template <typename Fn>
void runGuardedThread( const char *threadName, const UnhandledExceptionPolicy policy, Fn &&fn ) noexcept
{
    try {
        fn();
    } catch ( const std::bad_alloc & ) {
        logUnhandledException( threadName, "Out of memory.", "std::bad_alloc" );
        if ( policy == UnhandledExceptionPolicy::RequestAppExit ) {
            requestAppExit( 1 );
        }
    } catch ( const std::exception &e ) {
        logUnhandledException( threadName, "An internal error occurred.", e.what() );
        if ( policy == UnhandledExceptionPolicy::RequestAppExit ) {
            requestAppExit( 1 );
        }
    } catch ( ... ) {
        logUnhandledException( threadName, "An internal error occurred.", "unknown exception" );
        if ( policy == UnhandledExceptionPolicy::RequestAppExit ) {
            requestAppExit( 1 );
        }
    }
}

}

#endif
