# Settings

This application uses Qt `QSettings` for user settings.

Current persisted settings (skeleton):

- Main window placement:
  - `ui/mainWindow/x`
  - `ui/mainWindow/y`
  - `ui/mainWindow/width`
  - `ui/mainWindow/height`

- Preferences:
  - `ui/profile/displayName`
  - `ui/theme/mode`
  - `ui/theme/accent`
  - `ui/theme/customAccentLightness`
  - `ui/theme/customAccentChroma`
  - `ui/theme/customAccentHueDegrees`

If the window position is not present (or is ignored), the app chooses a default placement centred within the current screen's `availableGeometry()`.

## Storage location

The underlying storage location is determined by Qt and the platform.

- Linux: stored under the user config directory (typically `~/.config/`) in an INI-style file named from the organisation and application names.
- Windows: stored in the registry under `HKEY_CURRENT_USER\Software\<organisation>\<application>`.

In this repository, the main application sets:

- organisation name: `ElMagnificoGames`
- application name: `vcstream`

If you change these, the on-disk settings location will also change.
