#ifndef MODULES_UI_PLACEMENT_WINDOWPLACEMENT_H
#define MODULES_UI_PLACEMENT_WINDOWPLACEMENT_H

#include <QWindow>

namespace windowplacement {

void restoreAndRepairMainWindowPlacement( QWindow *window );
void saveMainWindowPlacement( QWindow *window );

}

#endif
