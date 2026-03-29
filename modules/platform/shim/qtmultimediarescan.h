#pragma once

class QString;

namespace qtmultimediarescan {

// Requests a best-effort rescan of Qt Multimedia video input devices.
// Returns true only when a backend-specific rescan was requested.
bool requestVideoInputsRescan();

// Same as `requestVideoInputsRescan()`, but also returns a small debug note that
// describes whether a rescan was possible.
bool requestVideoInputsRescan( QString *outDebugText );

}
