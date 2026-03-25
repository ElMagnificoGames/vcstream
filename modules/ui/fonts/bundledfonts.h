#ifndef MODULES_UI_FONTS_BUNDLEDFONTS_H
#define MODULES_UI_FONTS_BUNDLEDFONTS_H

#include <QString>

namespace bundledfonts {

struct Families
{
    QString victorianBody;
    QString victorianHeading;
};

// Registers bundled fonts with Qt's font database.
// Safe to call multiple times; registration is performed once per process.
void ensureRegistered();

// Returns the resolved font family names discovered after registration.
// Empty strings mean the bundled font was not registered successfully.
Families families();

}

#endif
