#ifndef MODULES_UI_FONTS_FONTPREVIEWSAFETYCACHE_H
#define MODULES_UI_FONTS_FONTPREVIEWSAFETYCACHE_H

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QString>

class FontPreviewSafetyChecker;

class FontPreviewSafetyCache final : public QObject
{
    Q_OBJECT

public:
    struct Config
    {
        int timeoutMs;
        int maxStuckChecks;
    };

    enum class Status
    {
        NeverChecked = 0,
        Checking = 1,
        Safe = 2,
        Bad = 3,
        TimedOut = 4,
        Blocked = 5,
    };
    Q_ENUM( Status )

    explicit FontPreviewSafetyCache( QObject *parent = nullptr );
    FontPreviewSafetyCache( FontPreviewSafetyChecker *checker, const FontPreviewSafetyCache::Config &config, QObject *parent = nullptr );
    ~FontPreviewSafetyCache() override;

    Q_INVOKABLE FontPreviewSafetyCache::Status statusForFamily( const QString &family, int pixelSize ) const;
    Q_INVOKABLE void requestCheck( const QString &family, int pixelSize );

Q_SIGNALS:
    void statusChanged( const QString &family, int pixelSize, FontPreviewSafetyCache::Status status );

private Q_SLOTS:
    void onCheckFinished( const QString &family, int pixelSize, bool ok );
    void onCheckTimedOut( const QString &family, int pixelSize );

private:
    struct Key {
        QString family;
        int pixelSize;
    };

    friend uint qHash( const FontPreviewSafetyCache::Key &k, uint seed );
    friend bool operator==( const FontPreviewSafetyCache::Key &a, const FontPreviewSafetyCache::Key &b );

    static Key makeKey( const QString &family, int pixelSize );

    void startCheck( const QString &family, int pixelSize );
    static FontPreviewSafetyCache::Config defaultConfig();

    mutable QMutex m_mutex;
    QHash<Key, Status> m_status;
    QHash<Key, bool> m_countedStuck;

    int m_stuckChecks;

    FontPreviewSafetyCache::Config m_config;
    FontPreviewSafetyChecker *m_checker;
    bool m_ownChecker;
};

inline bool operator==( const FontPreviewSafetyCache::Key &a, const FontPreviewSafetyCache::Key &b )
{
    return a.pixelSize == b.pixelSize && a.family == b.family;
}

inline uint qHash( const FontPreviewSafetyCache::Key &k, uint seed = 0 )
{
    return qHash( k.family, seed ) ^ ( qHash( k.pixelSize, seed ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 ) );
}

#endif
