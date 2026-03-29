#ifndef MODULES_DEVICES_CATALOGUE_FAKEDEVICENAMEGENERATOR_H
#define MODULES_DEVICES_CATALOGUE_FAKEDEVICENAMEGENERATOR_H

#include <QStringList>

#include <QRandomGenerator>

class FakeDeviceNameGenerator final
{
public:
    FakeDeviceNameGenerator();
    explicit FakeDeviceNameGenerator( quint32 seed );

    // Updates `names` in-place.
    // - keepPercent% of the time it leaves the list unchanged.
    // - the rest of the time it replaces the list with a new subset of the fixed list.
    // Returns true when the list changed.
    bool maybeUpdateNames( QStringList *names, int keepPercent );

    // Generates a non-empty subset of the fixed name list.
    QStringList generateSubset();

    static const QStringList &fixedNames();

private:
    static quint32 seedFromEnvironmentOrDefault( const char *envName, quint32 defaultSeed );

private:
    QRandomGenerator m_rng;
};

#endif
