#include "modules/platform/shim/dshowcameraenum.h"

#include <QByteArray>

#include <QtGlobal>

#if defined( Q_OS_WIN )

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <dshow.h>

namespace {

class ComInit final
{
public:
    ComInit()
        : m_hr( E_FAIL )
        , m_needsUninit( false )
    {
        m_hr = CoInitializeEx( nullptr, COINIT_APARTMENTTHREADED );
        if ( m_hr == S_OK || m_hr == S_FALSE ) {
            m_needsUninit = true;
        }
    }

    ~ComInit()
    {
        if ( m_needsUninit ) {
            CoUninitialize();
        }
    }

    HRESULT hr() const
    {
        return m_hr;
    }

private:
    HRESULT m_hr;
    bool m_needsUninit;
};

template <typename T>
class ComPtr final
{
public:
    ComPtr()
        : m_ptr( nullptr )
    {
    }

    ~ComPtr()
    {
        reset();
    }

    ComPtr( const ComPtr & ) = delete;
    ComPtr &operator=( const ComPtr & ) = delete;

    T *get() const
    {
        return m_ptr;
    }

    T **out()
    {
        reset();
        return &m_ptr;
    }

    void reset()
    {
        if ( m_ptr != nullptr ) {
            m_ptr->Release();
            m_ptr = nullptr;
        }
    }

    T *operator->() const
    {
        return m_ptr;
    }

private:
    T *m_ptr;
};

QString toHexUtf8( const QString &s )
{
    const QByteArray utf8 = s.toUtf8();
    return QString::fromLatin1( utf8.toHex() );
}

QString fromHexUtf8( const QString &hex, bool *ok )
{
    QString out;
    bool localOk;

    out = QString();
    localOk = false;

    if ( !hex.isEmpty() ) {
        const QByteArray bytes = QByteArray::fromHex( hex.toLatin1() );
        if ( !bytes.isEmpty() ) {
            out = QString::fromUtf8( bytes );
            localOk = true;
        }
    }

    if ( ok != nullptr ) {
        *ok = localOk;
    }

    return out;
}

QString readPropertyBagString( IPropertyBag &bag, const wchar_t *key )
{
    QString out;
    VARIANT v;

    out = QString();
    VariantInit( &v );

    const HRESULT hr = bag.Read( key, &v, nullptr );
    if ( SUCCEEDED( hr ) ) {
        if ( v.vt == VT_BSTR && v.bstrVal != nullptr ) {
            out = QString::fromWCharArray( v.bstrVal );
        }
    }

    VariantClear( &v );
    return out;
}

bool monikerDisplayName( IMoniker &m, QString *out )
{
    bool ok;

    ok = false;
    if ( out != nullptr ) {
        *out = QString();
    }

    ComPtr<IBindCtx> ctx;
    const HRESULT hrCtx = CreateBindCtx( 0, ctx.out() );
    if ( SUCCEEDED( hrCtx ) && ctx.get() != nullptr ) {
        LPOLESTR name = nullptr;
        const HRESULT hrName = m.GetDisplayName( ctx.get(), nullptr, &name );
        if ( SUCCEEDED( hrName ) && name != nullptr ) {
            if ( out != nullptr ) {
                *out = QString::fromWCharArray( name );
            }
            ok = true;
        }
        if ( name != nullptr ) {
            CoTaskMemFree( name );
        }
    }

    return ok;
}

}

namespace dshowcameraenum {

QVector<VideoInput> enumerateVideoInputs( QString *outDebugText )
{
    QVector<VideoInput> out;
    QString debugText;

    out = QVector<VideoInput>();
    debugText = QString();

    const ComInit com;
    const HRESULT initHr = com.hr();
    if ( FAILED( initHr ) && initHr != RPC_E_CHANGED_MODE ) {
        debugText = QStringLiteral( "DirectShow camera enumeration failed: COM init failed." );
    } else {
        ComPtr<ICreateDevEnum> devEnum;
        HRESULT hr = CoCreateInstance( CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( devEnum.out() ) );
        if ( FAILED( hr ) || devEnum.get() == nullptr ) {
            debugText = QStringLiteral( "DirectShow camera enumeration failed: no system device enumerator." );
        } else {
            ComPtr<IEnumMoniker> enumMoniker;
            hr = devEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, enumMoniker.out(), 0 );
            if ( hr != S_OK || enumMoniker.get() == nullptr ) {
                // S_FALSE means "no devices".
                debugText = QString();
            } else {
                bool done;

                done = false;
                while ( !done ) {
                    ComPtr<IMoniker> moniker;
                    ULONG fetched;

                    fetched = 0;
                    hr = enumMoniker->Next( 1, moniker.out(), &fetched );
                    if ( hr == S_OK && fetched == 1 && moniker.get() != nullptr ) {
                        ComPtr<IPropertyBag> bag;
                        const HRESULT hrBag = moniker->BindToStorage( nullptr, nullptr, IID_PPV_ARGS( bag.out() ) );

                        QString name;
                        if ( SUCCEEDED( hrBag ) && bag.get() != nullptr ) {
                            name = readPropertyBagString( *bag.get(), L"FriendlyName" );
                        } else {
                            name = QString();
                        }

                        QString displayName;
                        const bool haveDisplayName = monikerDisplayName( *moniker.get(), &displayName );

                        if ( haveDisplayName && !displayName.isEmpty() ) {
                            VideoInput v;
                            v.monikerDisplayName = displayName;
                            v.stableId = stableIdFromMonikerDisplayName( displayName );
                            v.name = name.isEmpty() ? displayName : name;
                            out.append( v );
                        }
                    } else {
                        done = true;
                    }
                }
            }
        }
    }

    if ( outDebugText != nullptr ) {
        *outDebugText = debugText;
    }

    return out;
}

QString stableIdFromMonikerDisplayName( const QString &monikerDisplayName )
{
    QString out;

    out = QString();

    if ( !monikerDisplayName.isEmpty() ) {
        out = QStringLiteral( "dshow:%1" ).arg( toHexUtf8( monikerDisplayName ) );
    }

    return out;
}

bool stableIdToMonikerDisplayName( const QString &stableId, QString *outMonikerDisplayName )
{
    bool ok;
    QString out;

    ok = false;
    out = QString();

    if ( stableIdLooksLikeDShow( stableId ) ) {
        const QString hex = stableId.mid( QStringLiteral( "dshow:" ).size() );
        bool decoded;
        out = fromHexUtf8( hex, &decoded );
        ok = decoded;
    }

    if ( outMonikerDisplayName != nullptr ) {
        *outMonikerDisplayName = out;
    }

    return ok;
}

bool stableIdLooksLikeDShow( const QString &stableId )
{
    return stableId.startsWith( QStringLiteral( "dshow:" ) );
}

}

#endif
