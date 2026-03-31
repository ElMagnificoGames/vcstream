#include "modules/devices/capture/dshowcamerabackend.h"

#include "modules/platform/shim/dshowcameraenum.h"

#include "modules/devices/capture/camerastreambackend.h"

#include <QByteArray>
#include <QImage>
#include <QMetaObject>
#include <QTimer>
#include <QVideoFrame>

#include <atomic>
#include <cstring>

#include <QtGlobal>

#if defined( Q_OS_WIN )

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <dshow.h>
#include <dvdmedia.h>

// qedit.h is not reliably available; define the small subset we need.
// Based on the legacy DirectShow Sample Grabber interfaces.

// {C1F400A0-3F08-11D3-9F0B-006008039E37}
static const CLSID VCSTREAM_CLSID_SampleGrabber =
    { 0xC1F400A0, 0x3F08, 0x11D3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

// {C1F400A4-3F08-11D3-9F0B-006008039E37}
static const CLSID VCSTREAM_CLSID_NullRenderer =
    { 0xC1F400A4, 0x3F08, 0x11D3, { 0x9F, 0x0B, 0x00, 0x60, 0x08, 0x03, 0x9E, 0x37 } };

// {6B652FFF-11FE-4fce-92AD-0266B5D7C78F}
static const IID VCSTREAM_IID_ISampleGrabber =
    { 0x6B652FFF, 0x11FE, 0x4FCE, { 0x92, 0xAD, 0x02, 0x66, 0xB5, 0xD7, 0xC7, 0x8F } };

// {0579154A-2B53-4994-B0D0-E773148EFF85}
static const IID VCSTREAM_IID_ISampleGrabberCB =
    { 0x0579154A, 0x2B53, 0x4994, { 0xB0, 0xD0, 0xE7, 0x73, 0x14, 0x8E, 0xFF, 0x85 } };

struct ISampleGrabberCB : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SampleCB( double sampleTime, IMediaSample *sample ) = 0;
    virtual HRESULT STDMETHODCALLTYPE BufferCB( double sampleTime, BYTE *buffer, long bufferLen ) = 0;
};

struct ISampleGrabber : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE SetOneShot( BOOL oneShot ) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetMediaType( const AM_MEDIA_TYPE *type ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType( AM_MEDIA_TYPE *type ) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetBufferSamples( BOOL bufferSamples ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer( long *bufferSize, long *buffer ) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCurrentSample( IMediaSample **sample ) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetCallback( ISampleGrabberCB *callback, long callbackType ) = 0;
};

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

    void reset()
    {
        if ( m_ptr != nullptr ) {
            m_ptr->Release();
            m_ptr = nullptr;
        }
    }

    T **out()
    {
        reset();
        return &m_ptr;
    }

    T *operator->() const
    {
        return m_ptr;
    }

private:
    T *m_ptr;
};

void freeMediaType( AM_MEDIA_TYPE &mt )
{
    if ( mt.cbFormat != 0 && mt.pbFormat != nullptr ) {
        CoTaskMemFree( mt.pbFormat );
        mt.cbFormat = 0;
        mt.pbFormat = nullptr;
    }

    if ( mt.pUnk != nullptr ) {
        mt.pUnk->Release();
        mt.pUnk = nullptr;
    }
}

void freeMediaTypePtr( AM_MEDIA_TYPE *mt )
{
    if ( mt != nullptr ) {
        freeMediaType( *mt );
        CoTaskMemFree( mt );
    }
}

struct ConnectedVideoFormat {
    int width;
    int height;
    int stride;
    int bitsPerPixel;
    bool bottomUp;
    GUID subtype;
};

int clampByte( const int v )
{
    int out;

    out = v;
    if ( out < 0 ) {
        out = 0;
    }
    if ( out > 255 ) {
        out = 255;
    }

    return out;
}

void yuvToRgb( const int y, const int u, const int v, unsigned char *outBgra )
{
    const int c = y - 16;
    const int d = u - 128;
    const int e = v - 128;

    const int c2 = ( c < 0 ? 0 : c );

    const int r = ( 298 * c2 + 409 * e + 128 ) >> 8;
    const int g = ( 298 * c2 - 100 * d - 208 * e + 128 ) >> 8;
    const int b = ( 298 * c2 + 516 * d + 128 ) >> 8;

    outBgra[0] = static_cast<unsigned char>( clampByte( b ) );
    outBgra[1] = static_cast<unsigned char>( clampByte( g ) );
    outBgra[2] = static_cast<unsigned char>( clampByte( r ) );
    outBgra[3] = 0xFF;
}

bool tryParseConnectedFormat( ISampleGrabber &grabber, ConnectedVideoFormat *out )
{
    bool ok;
    AM_MEDIA_TYPE mt;

    ok = false;
    if ( out != nullptr ) {
        out->width = 0;
        out->height = 0;
        out->stride = 0;
        out->bitsPerPixel = 0;
        out->bottomUp = false;
        std::memset( &out->subtype, 0, sizeof( out->subtype ) );
    }

    std::memset( &mt, 0, sizeof( mt ) );

    const HRESULT hr = grabber.GetConnectedMediaType( &mt );
    if ( SUCCEEDED( hr ) ) {
        if ( out != nullptr ) {
            out->subtype = mt.subtype;
        }
        const bool isVideoInfo = ( mt.formattype == FORMAT_VideoInfo )
            && ( mt.pbFormat != nullptr )
            && ( mt.cbFormat >= static_cast<ULONG>( sizeof( VIDEOINFOHEADER ) ) );
        const bool isVideoInfo2 = ( mt.formattype == FORMAT_VideoInfo2 )
            && ( mt.pbFormat != nullptr )
            && ( mt.cbFormat >= static_cast<ULONG>( sizeof( VIDEOINFOHEADER2 ) ) );

        if ( isVideoInfo || isVideoInfo2 ) {
            const BITMAPINFOHEADER *bmi;
            if ( isVideoInfo ) {
                const auto *vih = reinterpret_cast<const VIDEOINFOHEADER *>( mt.pbFormat );
                bmi = &vih->bmiHeader;
            } else {
                const auto *vih2 = reinterpret_cast<const VIDEOINFOHEADER2 *>( mt.pbFormat );
                bmi = &vih2->bmiHeader;
            }

            if ( bmi != nullptr ) {
                const int w = static_cast<int>( bmi->biWidth );
                const int rawH = static_cast<int>( bmi->biHeight );
                const int h = ( rawH < 0 ? -rawH : rawH );
                const int bpp = static_cast<int>( bmi->biBitCount );

                const int stride = ( ( w * bpp + 31 ) / 32 ) * 4;
                bool bottomUp;

                // `biHeight` sign is meaningful for classic RGB DIB formats.
                // For packed YUV capture formats (for example YUY2/UYVY), many DirectShow
                // sources report a positive height even though the sample memory is already
                // top-down. Treat those as top-down to avoid an incorrect vertical flip.
                bottomUp = false;
                if ( mt.subtype == MEDIASUBTYPE_RGB32 || mt.subtype == MEDIASUBTYPE_RGB24 ) {
                    bottomUp = ( rawH > 0 );
                }

                if ( w > 0 && h > 0 && stride > 0 ) {
                    if ( out != nullptr ) {
                        out->width = w;
                        out->height = h;
                        out->stride = stride;
                        out->bitsPerPixel = bpp;
                        out->bottomUp = bottomUp;
                    }
                    ok = true;
                }
            }
        }
    }

    freeMediaType( mt );
    return ok;
}

class DShowCameraStreamBackend;

class SampleGrabberCallback final : public ISampleGrabberCB
{
public:
    explicit SampleGrabberCallback( DShowCameraStreamBackend *owner )
        : m_ref( 1 )
        , m_owner( owner )
    {
    }

    void detachOwner()
    {
        m_owner.store( nullptr, std::memory_order_release );
    }

    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppv ) override
    {
        HRESULT out;

        out = E_NOINTERFACE;

        if ( ppv != nullptr ) {
            *ppv = nullptr;
        }

        if ( ppv != nullptr ) {
            if ( riid == IID_IUnknown || riid == VCSTREAM_IID_ISampleGrabberCB ) {
                *ppv = static_cast<ISampleGrabberCB *>( this );
                AddRef();
                out = S_OK;
            }
        }

        return out;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return static_cast<ULONG>( m_ref.fetch_add( 1, std::memory_order_relaxed ) + 1 );
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG out;

        out = static_cast<ULONG>( m_ref.fetch_sub( 1, std::memory_order_acq_rel ) - 1 );
        if ( out == 0 ) {
            delete this;
        }

        return out;
    }

    HRESULT STDMETHODCALLTYPE SampleCB( double sampleTime, IMediaSample *sample ) override;

    HRESULT STDMETHODCALLTYPE BufferCB( double sampleTime, BYTE *buffer, long bufferLen ) override
    {
        Q_UNUSED( sampleTime )
        Q_UNUSED( buffer )
        Q_UNUSED( bufferLen )
        return E_NOTIMPL;
    }

private:
    std::atomic<long> m_ref;
    std::atomic<DShowCameraStreamBackend *> m_owner;
};

class DShowCameraStreamBackend final : public CameraStreamBackend
{
public:
    explicit DShowCameraStreamBackend( const QString &deviceId, const QString &monikerDisplayName, QObject *parent )
        : CameraStreamBackend( parent )
        , m_deviceId( deviceId )
        , m_monikerDisplayName( monikerDisplayName )
        , m_com()
        , m_initOk( false )
        , m_format()
        , m_graph()
        , m_builder()
        , m_control()
        , m_sourceFilter()
        , m_grabberFilter()
        , m_grabber()
        , m_nullRenderer()
        , m_callback( nullptr )
        , m_frameCount( 0 )
        , m_noFrameTimer( this )
    {
        m_format.width = 0;
        m_format.height = 0;
        m_format.stride = 0;
        m_format.bitsPerPixel = 0;
        m_format.bottomUp = false;
        std::memset( &m_format.subtype, 0, sizeof( m_format.subtype ) );

        m_noFrameTimer.setSingleShot( true );
        m_noFrameTimer.setInterval( 700 );
        QObject::connect( &m_noFrameTimer, &QTimer::timeout, this, [this]() {
            if ( m_frameCount.load( std::memory_order_acquire ) <= 0 ) {
                if ( errorText().isEmpty() ) {
                    setErrorText( QStringLiteral( "No video is coming from this camera right now." ) );
                }
            }
        } );

        initialise();
    }

    ~DShowCameraStreamBackend() override
    {
        stop();

        if ( m_callback != nullptr ) {
            m_callback->detachOwner();
            m_callback->Release();
            m_callback = nullptr;
        }
    }

    bool isInitialised() const
    {
        return m_initOk;
    }

    void start() override
    {
        if ( m_initOk && m_control.get() != nullptr ) {
            m_frameCount.store( 0, std::memory_order_release );
            m_noFrameTimer.start();

            const HRESULT hr = m_control->Run();
            if ( FAILED( hr ) ) {
                setErrorText( QStringLiteral( "This camera could not be started." ) );
            }
        }
    }

    void stop() override
    {
        if ( m_control.get() != nullptr ) {
            m_noFrameTimer.stop();
            m_control->Stop();
        }
    }

    void onSampleBuffer( BYTE *buffer, const long bufferLen )
    {
        bool ok;
        bool supported;
        QString dropReason;

        ok = true;
        supported = true;
        dropReason = QString();

        if ( buffer == nullptr ) {
            ok = false;
        }

        if ( ok && bufferLen <= 0 ) {
            ok = false;
        }

        if ( ok && ( m_format.width <= 0 || m_format.height <= 0 || m_format.stride <= 0 ) ) {
            ok = false;
        }

        const int w = m_format.width;
        const int h = m_format.height;
        const int inStride = m_format.stride;
        const int bpp = m_format.bitsPerPixel;

        if ( ok ) {
            const bool isRgb32 = ( bpp == 32 ) && ( m_format.subtype == MEDIASUBTYPE_RGB32 );
            const bool isRgb24 = ( bpp == 24 ) && ( m_format.subtype == MEDIASUBTYPE_RGB24 );
            const bool isYuy2 = ( bpp == 16 ) && ( m_format.subtype == MEDIASUBTYPE_YUY2 );
            const bool isUyvy = ( bpp == 16 ) && ( m_format.subtype == MEDIASUBTYPE_UYVY );

            if ( !isRgb32 && !isRgb24 && !isYuy2 && !isUyvy ) {
                supported = false;
                dropReason = QStringLiteral( "This camera is sending video in a format VCStream can't display yet." );
            }
        }

        if ( ok ) {
            const int minBytes = inStride * h;
            if ( bufferLen < minBytes ) {
                ok = false;
            }
        }

        QImage img;
        if ( ok && supported ) {
            img = QImage( w, h, QImage::Format_ARGB32 );
            if ( img.isNull() ) {
                ok = false;
            }
        }

        if ( ok && supported ) {
            m_frameCount.fetch_add( 1, std::memory_order_acq_rel );

            if ( !errorText().isEmpty() ) {
                QMetaObject::invokeMethod( this, [this]() {
                    setErrorText( QString() );
                }, Qt::QueuedConnection );
            }

            for ( int y = 0; y < h; ++y ) {
                const int srcY = ( m_format.bottomUp ? ( h - 1 - y ) : y );
                const unsigned char *src = reinterpret_cast<const unsigned char *>( buffer + ( srcY * inStride ) );
                unsigned char *dst = img.scanLine( y );

                if ( m_format.subtype == MEDIASUBTYPE_RGB32 ) {
                    // Input is typically BGRX. Copy and force opaque alpha.
                    std::memcpy( dst, src, static_cast<size_t>( w ) * 4U );
                    for ( int x = 0; x < w; ++x ) {
                        dst[ x * 4 + 3 ] = 0xFF;
                    }
                } else if ( m_format.subtype == MEDIASUBTYPE_RGB24 ) {
                    // Input is BGR24. Expand to BGRA32.
                    for ( int x = 0; x < w; ++x ) {
                        const int inOff = x * 3;
                        const int outOff = x * 4;
                        dst[ outOff + 0 ] = src[ inOff + 0 ];
                        dst[ outOff + 1 ] = src[ inOff + 1 ];
                        dst[ outOff + 2 ] = src[ inOff + 2 ];
                        dst[ outOff + 3 ] = 0xFF;
                    }
                } else if ( m_format.subtype == MEDIASUBTYPE_YUY2 ) {
                    // Y0 U Y1 V
                    for ( int x = 0; x < w; x += 2 ) {
                        const int inOff = x * 2;
                        const int outOff0 = x * 4;
                        const int outOff1 = ( x + 1 ) * 4;

                        const int y0 = src[ inOff + 0 ];
                        const int u = src[ inOff + 1 ];
                        const int y1 = src[ inOff + 2 ];
                        const int v = src[ inOff + 3 ];

                        yuvToRgb( y0, u, v, dst + outOff0 );
                        if ( x + 1 < w ) {
                            yuvToRgb( y1, u, v, dst + outOff1 );
                        }
                    }
                } else {
                    // U Y0 V Y1 (UYVY)
                    for ( int x = 0; x < w; x += 2 ) {
                        const int inOff = x * 2;
                        const int outOff0 = x * 4;
                        const int outOff1 = ( x + 1 ) * 4;

                        const int u = src[ inOff + 0 ];
                        const int y0 = src[ inOff + 1 ];
                        const int v = src[ inOff + 2 ];
                        const int y1 = src[ inOff + 3 ];

                        yuvToRgb( y0, u, v, dst + outOff0 );
                        if ( x + 1 < w ) {
                            yuvToRgb( y1, u, v, dst + outOff1 );
                        }
                    }
                }
            }

            const QVideoFrame frame( img );
            QMetaObject::invokeMethod( this, [this, frame]() {
                Q_EMIT videoFrameChanged( frame );
            }, Qt::QueuedConnection );
        } else {
            if ( ok && !supported && !dropReason.isEmpty() ) {
                if ( errorText().isEmpty() ) {
                    const QString text = dropReason;
                    QMetaObject::invokeMethod( this, [this, text]() {
                        setErrorText( text );
                    }, Qt::QueuedConnection );
                }
            }
        }
    }

private:
    void initialise()
    {
        bool ok;
        HRESULT hr;

        ok = true;
        hr = E_FAIL;

        const HRESULT initHr = m_com.hr();
        if ( FAILED( initHr ) && initHr != RPC_E_CHANGED_MODE ) {
            ok = false;
        }

        ComPtr<IBindCtx> ctx;
        if ( ok ) {
            hr = CreateBindCtx( 0, ctx.out() );
            if ( FAILED( hr ) || ctx.get() == nullptr ) {
                ok = false;
            }
        }

        ComPtr<IMoniker> moniker;
        if ( ok ) {
            ULONG eaten;
            eaten = 0;

            const wchar_t *displayName = reinterpret_cast<const wchar_t *>( m_monikerDisplayName.utf16() );
            hr = MkParseDisplayName( ctx.get(), displayName, &eaten, moniker.out() );
            if ( FAILED( hr ) || moniker.get() == nullptr ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = moniker->BindToObject( nullptr, nullptr, IID_PPV_ARGS( m_sourceFilter.out() ) );
            if ( FAILED( hr ) || m_sourceFilter.get() == nullptr ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = CoCreateInstance( CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( m_graph.out() ) );
            if ( FAILED( hr ) || m_graph.get() == nullptr ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = CoCreateInstance( CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( m_builder.out() ) );
            if ( FAILED( hr ) || m_builder.get() == nullptr ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = m_builder->SetFiltergraph( m_graph.get() );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = m_graph->AddFilter( m_sourceFilter.get(), L"Video Capture" );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            // Some virtual cameras do not produce frames until a format is selected.
            ComPtr<IAMStreamConfig> cfg;
            hr = m_builder->FindInterface( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_sourceFilter.get(), IID_PPV_ARGS( cfg.out() ) );

            if ( SUCCEEDED( hr ) && cfg.get() != nullptr ) {
                int count;
                int capSize;
                int chosenIndex;
                GUID chosenSubtype;

                count = 0;
                capSize = 0;
                chosenIndex = -1;
                std::memset( &chosenSubtype, 0, sizeof( chosenSubtype ) );

                hr = cfg->GetNumberOfCapabilities( &count, &capSize );
                if ( SUCCEEDED( hr ) && count > 0 && capSize > 0 ) {
                    // Prefer formats we can currently convert.
                    // OBS Virtual Camera commonly exposes NV12/I420/YUY2; pick YUY2 when available.
                    for ( int i = 0; i < count; ++i ) {
                        QByteArray caps;
                        caps.resize( capSize );

                        AM_MEDIA_TYPE *pmt;
                        pmt = nullptr;

                        const HRESULT hrCaps = cfg->GetStreamCaps( i, &pmt, reinterpret_cast<BYTE *>( caps.data() ) );
                        if ( SUCCEEDED( hrCaps ) && pmt != nullptr ) {
                            if ( chosenIndex < 0 ) {
                                chosenIndex = i;
                                chosenSubtype = pmt->subtype;
                            }

                            if ( chosenIndex >= 0 ) {
                                if ( pmt->subtype == MEDIASUBTYPE_YUY2 ) {
                                    chosenIndex = i;
                                    chosenSubtype = pmt->subtype;
                                } else if ( pmt->subtype == MEDIASUBTYPE_UYVY && chosenSubtype != MEDIASUBTYPE_YUY2 ) {
                                    chosenIndex = i;
                                    chosenSubtype = pmt->subtype;
                                }
                            }
                        }

                        freeMediaTypePtr( pmt );
                    }

                    if ( chosenIndex >= 0 ) {
                        QByteArray caps;
                        caps.resize( capSize );

                        AM_MEDIA_TYPE *pmt;
                        pmt = nullptr;

                        const HRESULT hrCaps = cfg->GetStreamCaps( chosenIndex, &pmt, reinterpret_cast<BYTE *>( caps.data() ) );
                        if ( SUCCEEDED( hrCaps ) && pmt != nullptr ) {
                            cfg->SetFormat( pmt );
                        }

                        freeMediaTypePtr( pmt );
                    }
                }
            }
        }

        if ( ok ) {
            hr = CoCreateInstance( VCSTREAM_CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( m_grabberFilter.out() ) );
            if ( FAILED( hr ) || m_grabberFilter.get() == nullptr ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = m_grabberFilter->QueryInterface( VCSTREAM_IID_ISampleGrabber, reinterpret_cast<void **>( m_grabber.out() ) );
            if ( FAILED( hr ) || m_grabber.get() == nullptr ) {
                ok = false;
            }
        }

        if ( ok ) {
            AM_MEDIA_TYPE mt;
            std::memset( &mt, 0, sizeof( mt ) );
            mt.majortype = MEDIATYPE_Video;
            mt.subtype = GUID_NULL;
            mt.formattype = GUID_NULL;
            hr = m_grabber->SetMediaType( &mt );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = m_grabber->SetOneShot( FALSE );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            // Some DirectShow sources only deliver frames to the callback reliably
            // when buffering is enabled.
            hr = m_grabber->SetBufferSamples( TRUE );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            m_callback = new SampleGrabberCallback( this );
            hr = m_grabber->SetCallback( m_callback, 0 );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = m_graph->AddFilter( m_grabberFilter.get(), L"Sample Grabber" );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = CoCreateInstance( VCSTREAM_CLSID_NullRenderer, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( m_nullRenderer.out() ) );
            if ( FAILED( hr ) || m_nullRenderer.get() == nullptr ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = m_graph->AddFilter( m_nullRenderer.get(), L"Null Renderer" );
            if ( FAILED( hr ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            hr = m_builder->RenderStream( &PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, m_sourceFilter.get(), m_grabberFilter.get(), m_nullRenderer.get() );
            if ( FAILED( hr ) ) {
                hr = m_builder->RenderStream( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, m_sourceFilter.get(), m_grabberFilter.get(), m_nullRenderer.get() );
                if ( FAILED( hr ) ) {
                    ok = false;
                }
            }
        }

        if ( ok ) {
            if ( !tryParseConnectedFormat( *m_grabber.get(), &m_format ) ) {
                ok = false;
            }
        }

        if ( ok ) {
            const int bpp = m_format.bitsPerPixel;
            const bool isRgb32 = ( bpp == 32 ) && ( m_format.subtype == MEDIASUBTYPE_RGB32 );
            const bool isRgb24 = ( bpp == 24 ) && ( m_format.subtype == MEDIASUBTYPE_RGB24 );
            const bool isYuy2 = ( bpp == 16 ) && ( m_format.subtype == MEDIASUBTYPE_YUY2 );
            const bool isUyvy = ( bpp == 16 ) && ( m_format.subtype == MEDIASUBTYPE_UYVY );

            if ( !isRgb32 && !isRgb24 && !isYuy2 && !isUyvy ) {
                setErrorText( QStringLiteral( "This camera is sending video in a format VCStream can't display yet." ) );
            }
        }

        if ( ok ) {
            hr = m_graph->QueryInterface( IID_PPV_ARGS( m_control.out() ) );
            if ( FAILED( hr ) || m_control.get() == nullptr ) {
                ok = false;
            }
        }

        m_initOk = ok;
    }

private:
    const QString m_deviceId;
    const QString m_monikerDisplayName;

    ComInit m_com;
    bool m_initOk;
    ConnectedVideoFormat m_format;

    ComPtr<IGraphBuilder> m_graph;
    ComPtr<ICaptureGraphBuilder2> m_builder;
    ComPtr<IMediaControl> m_control;

    ComPtr<IBaseFilter> m_sourceFilter;
    ComPtr<IBaseFilter> m_grabberFilter;
    ComPtr<ISampleGrabber> m_grabber;
    ComPtr<IBaseFilter> m_nullRenderer;

    SampleGrabberCallback *m_callback;

    std::atomic<int> m_frameCount;
    QTimer m_noFrameTimer;
};

HRESULT STDMETHODCALLTYPE SampleGrabberCallback::SampleCB( double sampleTime, IMediaSample *sample )
{
    Q_UNUSED( sampleTime )

    HRESULT out;
    DShowCameraStreamBackend *owner;

    out = S_OK;
    owner = m_owner.load( std::memory_order_acquire );

    if ( owner != nullptr && sample != nullptr ) {
        BYTE *buffer;
        const long len = sample->GetActualDataLength();

        buffer = nullptr;
        const HRESULT hrPtr = sample->GetPointer( &buffer );
        if ( SUCCEEDED( hrPtr ) && buffer != nullptr && len > 0 ) {
            owner->onSampleBuffer( buffer, len );
        }
    }

    return out;
}

// BufferCB is unused; we prefer SampleCB.

}

QString DShowCameraBackend::descriptionForId( const QString &deviceId ) const
{
    QString out;

    out = QString();

    if ( dshowcameraenum::stableIdLooksLikeDShow( deviceId ) ) {
        QString debug;
        const QVector<dshowcameraenum::VideoInput> list = dshowcameraenum::enumerateVideoInputs( &debug );
        for ( const dshowcameraenum::VideoInput &v : list ) {
            if ( out.isEmpty() ) {
                if ( v.stableId == deviceId ) {
                    out = v.name;
                }
            }
        }
    }

    return out;
}

std::unique_ptr<CameraStreamBackend> DShowCameraBackend::createStream( const QString &deviceId, QObject *parent ) const
{
    std::unique_ptr<CameraStreamBackend> out;
    QString displayName;
    bool ok;

    out = nullptr;
    displayName = QString();
    ok = dshowcameraenum::stableIdToMonikerDisplayName( deviceId, &displayName );

    if ( ok && !displayName.isEmpty() ) {
        std::unique_ptr<DShowCameraStreamBackend> stream;
        stream = std::make_unique<DShowCameraStreamBackend>( deviceId, displayName, parent );
        if ( stream->isInitialised() ) {
            out = std::move( stream );
        }
    }

    return out;
}

#endif
