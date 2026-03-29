#include "modules/platform/shim/qtstartupshim.h"

#include <QtGlobal>

#if !defined( Q_OS_WIN )

namespace qtstartupshim {

void applyBeforeQGuiApplication()
{
}

}

#endif
