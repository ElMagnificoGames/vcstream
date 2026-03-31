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
  - `ui/appearance/fontPreset`
  - `ui/appearance/fontFamily`
  - `ui/appearance/fontScalePercent`
  - `ui/appearance/density`
  - `ui/appearance/zoomPercent`
  - `ui/theme/mode`
  - `ui/theme/accent`
  - `ui/theme/customAccentLightness`
  - `ui/theme/customAccentChroma`
  - `ui/theme/customAccentHueDegrees`
  - `ui/compat/softwareRendering`

`ui/compat/softwareRendering` is applied at process startup (restart required). On Windows it forces Qt Quick to use the software scene graph adaptation by setting `QT_QUICK_BACKEND=software` when the variable is not already provided by the user.

Note: On Windows with Qt 6.11, Qt Multimedia's `VideoOutput` can fail to render frames when `QT_QUICK_BACKEND=software` is active (the sink still receives frames, but the UI stays blank). In that mode VCStream routes camera previews through a software fallback (`VcVideoPaintItem`) that paints the latest frame via `QPainter` (throttled).

If the window position is not present (or is ignored), the app chooses a default placement centred within the current screen's `availableGeometry()`.

## Storage location

The underlying storage location is determined by Qt and the platform.

- Linux: stored under the user config directory (typically `~/.config/`) in an INI-style file named from the organisation and application names.
- Windows: stored in the registry under `HKEY_CURRENT_USER\Software\<organisation>\<application>`.

In this repository, the main application sets:

- organisation name: `ElMagnificoGames`
- application name: `vcstream`

If you change these, the on-disk settings location will also change.
