
## Task 0.1 — Create repository structure and project skeleton

### What

- Added a Qt 6 + C++17 application skeleton that builds into a single executable and opens a basic window.
- Introduced a CMake build with strict warnings enabled by default.
- Documented build and run instructions.

### Why

- Establish a runnable baseline so later roadmap tasks can be implemented incrementally and demonstrated.
- Use a build system that handles Qt code generation steps portably on Windows and Linux.
- Enforce a strict warning policy early to keep the codebase clean as it grows.

### Acceptance criteria

- The project configures and builds with CMake.
- Running the built binary opens a basic Qt window.
- Repository structure and build instructions are documented.

### Decisions

- UI stack: Qt Quick/QML for the initial skeleton.
- Build system: CMake.
- Warnings policy: `-Werror` (or `/WX`) enabled by default; do not proactively suppress warnings from dependencies, only add targeted exceptions if real problems surface.
- Standard mode: default to standard C++17 (`CMAKE_CXX_EXTENSIONS` is OFF); GNU extensions remain allowed when justified and documented per `docs/CODE-QT6.md`.
- Versioning: initial project version is defined in `CMakeLists.txt` as `0.0.1`.

### Technical notes

- Build entry point: `CMakeLists.txt` and `apps/vcstream/CMakeLists.txt`.
- App entry point: `apps/vcstream/main.cpp`.
- QML is embedded via Qt resources: `apps/vcstream/qml/qml.qrc` and `apps/vcstream/qml/main.qml`.
- Build/run documentation: `docs/BUILD.md`.

## Task 0.3 — Define initial module boundaries

### What

- Added an initial module-boundaries design note describing the intended major services/classes and their responsibility boundaries.
- Recorded an initial dependency direction to discourage early dependency cycles.

### Why

- The repository's long-term architecture has several interacting subsystems (capture, render, relay, HTTP media server, coordination).
- A concrete decomposition and dependency direction reduces accidental overlap and keeps later work (UI shell, transport/protocol ADRs, module DD files) coherent.

### Acceptance criteria

- Each proposed module has a one-paragraph responsibility summary.
- Obvious overlaps/conflicts are called out explicitly.
- At least one dependency direction rule is recorded.

### Decisions

- Introduced `SharedTypes` as a small, shared home for the core data model types/ids described in `docs/OVERVIEW.md`, to reduce cross-module coupling.
- Declared `Diagnostics` as sink-like (many-to-one), to reduce the risk of dependency cycles.

### Technical notes

- Module-boundaries note: `docs/MODULES.md`.

## Task 1.1 — Build the main application shell

### What

- Expanded the UI from a single placeholder window into a structured application shell with explicit placeholder areas for role controls, participant list, source list, chat, diagnostics, and a central stage/video area.
- Introduced the first concrete C++ module (`AppSupervisor`) as a UI-facing supervisor/lifecycle surface.
- Added a QtTest harness and a first unit test target for `AppSupervisor`.
- Added an explicit QML/C++ boundary guideline to keep presentation changes QML-only and keep stateful/fallible work in C++.

### Why

- The roadmap requires a coherent main window layout before role enablement, settings, and conferencing services are introduced.
- A stable shell layout reduces churn as the app grows and provides clear destinations for later UI panels.
- Establishing a QML-first presentation boundary early supports fast UI iteration without accidentally smuggling UI policy into C++.

### Acceptance criteria

- App launches cleanly.
- Main window exists with visible placeholder areas for participant list, chat, diagnostics, role controls, and source lists.
- Shutdown is clean.

### Decisions

- UI stack: adopted Qt Quick Controls 2 for the shell (`ApplicationWindow`, toolbars, split views, tabs) rather than re-implementing common controls from primitives.
- Boundary rule: QML owns presentation (layout/styling/ordering) and C++ owns state/services; QML should bind to C++ properties/models and invoke explicit commands.

### Technical notes

- UI shell: `apps/vcstream/qml/main.qml`.
- Quick Controls 2 dependency: `CMakeLists.txt`, `apps/vcstream/CMakeLists.txt`.
- Supervisor module: `modules/app/lifecycle/appsupervisor.h`, `modules/app/lifecycle/appsupervisor.cpp`, `modules/app/lifecycle/appsupervisor-dd.txt`.
- Unit tests:
  - CMake test wiring: `CMakeLists.txt`, `apps/unittests/CMakeLists.txt`, `apps/unittests/app/lifecycle/CMakeLists.txt`.
  - Test: `apps/unittests/app/lifecycle/tst_appsupervisor.cpp`.
  - Shared helpers: `apps/unittests/helpers/test_random.h`.
- QML/C++ boundary documentation (repo-specific rule): `docs/MODULES.md`.
- QML/C++ boundary documentation (generic guideline): `docs/CODE-QT6.md`.

## Task 1.2 — Add role enablement UI

### What

- Updated the role control copy to be user-intent phrased ("Join room" and "Host room") with hover tooltips for optional detail.
- Added `AppSupervisor` state for join/host role enablement and exposed it to QML via `Q_PROPERTY`.
- Added unit tests covering role state defaults and change-signal behaviour.
- Reframed the HTTP export capability as "Browser Export" (not OBS-specific) and shifted the UI concept away from a global toggle toward per-source export toggles (placeholder-only for now).
- Updated documentation to encode the UI copy principle and to rename "OBS bridge" terminology to "Browser Export".

### Why

- The user-facing UI should avoid networking jargon and describe intent for non-technical participants.
- Role enablement state needs a concrete C++ surface early so later networking/HTTP work has a stable wiring point.
- The HTTP export feature is useful beyond OBS; naming and UI structure should not imply it is OBS-only or global.

### Acceptance criteria

- User can toggle join/host role enablement in the UI.
- Current local role state is visible.
- UI state changes do not crash the app.

### Decisions

- UI copy: prefer user-intent wording with plain-language tooltips for power-user detail.
- Feature naming: renamed "OBS bridge" to "Browser Export" to reflect a general browser-source style local export.
- UI shape: Browser Export is treated as a per-source export (not a global role toggle).

### Technical notes

- Role state surface: `modules/app/lifecycle/appsupervisor.h`, `modules/app/lifecycle/appsupervisor.cpp`, `modules/app/lifecycle/appsupervisor-dd.txt`.
- Role UI bindings: `apps/vcstream/qml/main.qml`, `apps/vcstream/qml/LandingPage.qml`, `apps/vcstream/qml/ShellPage.qml`.
- Role tests: `apps/unittests/app/lifecycle/tst_appsupervisor.cpp`.
- Documentation updates:
  - UI copy guideline: `AGENTS.md`, `docs/ROADMAP.md`.
  - Rename to Browser Export: `docs/OVERVIEW.md`, `docs/ROADMAP.md`.

## Task 1.3 — Add settings persistence skeleton

### What

- Added a first settings persistence path using Qt `QSettings`.
- Persisted main window placement (x/y/width/height) and restored it on startup.
- Repaired restored window placement against the current multi-monitor layout so the window does not disappear off-screen, whilst never shrinking below the UI minimum size.
- Made the minimum window size explicit in QML and added a test that attempts to resize below the minimum to ensure it is actually enforced.
- Added a pure geometry fitter (`WindowRectFitter`) with unit tests covering common multi-monitor edge cases.
- Updated unit tests and QML UI smoke tests to use an isolated temporary `QSettings` location so test runs do not depend on or pollute real user settings.
- Documented the settings mechanism and storage location.

### Why

- The roadmap needs a concrete place to persist user configuration before networking/capture settings arrive.
- Persisting the main window size is a low-risk, user-visible setting that exercises the end-to-end persistence path without locking in higher-level UX decisions (such as auto-joining rooms).
- Hermetic tests reduce flakiness and prevent accidental interactions with developer machines' real settings.

### Acceptance criteria

- App remembers at least one simple setting across restarts (main window placement).
- Settings storage location is documented.
- The design leaves space for future device/network/security settings (keys are namespaced under `ui/` for now).

### Decisions

- Persistence mechanism: use Qt `QSettings` as the initial settings backend.
- Persisted setting chosen: main window placement (not role enablement state) to avoid surprising auto-join/auto-host behaviour.

### Technical notes

- Settings load/save lives in the app entrypoint:
  - `apps/vcstream/main.cpp`
- UI minimum size is set in QML:
  - `apps/vcstream/qml/main.qml`
  - `apps/vcstream/qml/ShellPage.qml`
- Test isolation for settings:
  - `apps/unittests/app/lifecycle/tst_appsupervisor.cpp`
  - `apps/unittests/qml/tst_qml_ui.cpp`
- Window placement fitter:
  - `modules/ui/geometry/windowrectfitter.h`
  - `modules/ui/geometry/windowrectfitter.cpp`
  - `modules/ui/geometry/windowrectfitter-dd.txt`
  - `apps/unittests/ui/geometry/tst_windowrectfitter.cpp`
- Settings documentation:
  - `docs/SETTINGS.md`
  - `docs/BUILD.md`

## Task 1.3a — Add top-of-stack exception guards (defensive programming)

### What

- Added a small defensive crash-guard module that provides:
  - a last-resort catch-all wrapper for program entry points
  - a last-resort catch-all wrapper for explicitly created worker-thread entry points (policy chosen by the thread creation site)
  - a `std::terminate` handler that logs a best-effort fatal message
- Wired the guard into the main application entry point and the QML UI smoke test runner.
- Added unit tests that verify the guarded runner catches and converts exceptions into a defined non-zero exit code.
- Updated `docs/CODE-QT6.md` to mandate this guard policy in a project-agnostic way.

### Why

- This codebase uses a no-`throw` policy, but exceptions may still arise indirectly (allocation failures, third-party boundaries, unexpected Qt/standard library behaviour).
- If a failure escapes the intended boundary, a top-of-stack guard provides a last line of defence:
  - produce a useful log message
  - attempt to exit gracefully when possible
  - avoid silent termination or hard-to-debug aborts without context

### Acceptance criteria

- App and test executables have a top-level catch-all guard at their entry points.
- A `std::terminate` handler exists to log truly unhandled failures.
- The policy is documented in `docs/CODE-QT6.md` so future changes keep the guard in place.
- Unit tests demonstrate that the guard converts exceptions into a defined failure result.

### Decisions

- Worker-thread unhandled-exception response is chosen case-by-case at the thread creation site (stop thread vs request whole-app exit).
- Crash-path logging is best-effort and low-allocation (stderr + Qt critical log).

### Technical notes

- Crash guard module:
  - `modules/app/defence/crashguard.h`
  - `modules/app/defence/crashguard.cpp`
  - `modules/app/defence/crashguard-dd.txt`
- Entry point wiring:
  - `apps/vcstream/main.cpp`
  - `apps/unittests/qml/tst_qml_ui.cpp`
- Tests:
  - `apps/unittests/app/defence/tst_crashguard.cpp`
  - `apps/unittests/app/defence/CMakeLists.txt`
- Policy update:
  - `docs/CODE-QT6.md`

## Task 1.4 — Add Preferences UI (settings + local device catalogue)

### What

- Added a Preferences overlay reachable from both the landing page and the main shell toolbar (gear icon with text fallback).
- Implemented a small preferences store (`AppPreferences`) backed by Qt `QSettings`.
- Implemented a local device catalogue (`LocalDeviceCatalogue`) that can enumerate and refresh without restarting the app:
  - screens
  - cameras
  - microphones
  - audio output devices
  - capturable windows (when supported by the Qt build)
- Wired preferences persistence into orderly shutdown and ensured UI closes persist current edits.
- Switched the Preferences navigation to a left-hand category list (Discord-style) instead of top tabs.
- Added an in-app theme system (System/Light/Dark + accent colours) that applies immediately.
- Added a custom accent colour picker based on the OKLCH colour model (hue slider + chroma/lightness plane).
- Extended the QML UI smoke test to open Preferences from the landing page and from the shell, switch categories, refresh the device catalogue, and close the overlay without emitting warnings.

### Why

- The roadmap needs a stable home for user preferences before capture and networking services arrive.
- A local “what does my machine see?” catalogue reduces future debugging time for device enumeration, permissions, and platform differences.

### Acceptance criteria

- Preferences entry point exists in the app shell UI.
- At least one preference is persisted and restored (beyond window placement).
- Device catalogue can refresh without restart and does not require starting capture.
- Failure/absence cases (no devices, missing window-capture support) are represented without crashing.

### Decisions

- Persistence: store preferences under `ui/` keys in `QSettings` alongside existing UI placement settings.
- Theme implementation: do not use `QQuickStyle` for theming; instead derive an application palette from the active style/system palette and apply mode/accent adjustments at the `ApplicationWindow.palette` level.
- Custom accent colour: use OKLCH and clamp colours to the sRGB gamut by reducing chroma.
- Capturable window enumeration: when supported, report only the window description (no stable OS id is provided by the current Qt API).

### Technical notes

- Preferences UI:
  - Overlay: `apps/vcstream/qml/PreferencesOverlay.qml`
  - Entry points: `apps/vcstream/qml/LandingPage.qml`, `apps/vcstream/qml/ShellPage.qml`
  - Overlay hosting and reopen intent: `apps/vcstream/qml/main.qml`
  - Resource wiring: `apps/vcstream/qml/qml.qrc`
- Preferences persistence:
  - Store: `modules/app/settings/apppreferences.h`, `modules/app/settings/apppreferences.cpp`, `modules/app/settings/apppreferences-dd.txt`
  - Theme plumbing: `apps/vcstream/qml/main.qml`
- Device catalogue:
  - Service: `modules/app/devices/localdevicecatalogue.h`, `modules/app/devices/localdevicecatalogue.cpp`, `modules/app/devices/localdevicecatalogue-dd.txt`
  - Enumeration sources: `QGuiApplication::screens()`, `QMediaDevices`, and (when available) `QWindowCapture`.
- Wiring to QML:
  - `AppSupervisor` now owns and exposes `preferences` and `deviceCatalogue` objects:
    - `modules/app/lifecycle/appsupervisor.h`, `modules/app/lifecycle/appsupervisor.cpp`, `modules/app/lifecycle/appsupervisor-dd.txt`
  - `AppSupervisor` also provides theme-icon availability checks for QML.
- Tests:
  - QML warning gate extended: `apps/unittests/qml/tst_qml_ui.cpp`
  - `tst_appsupervisor` now isolates `QSettings` to a temporary directory: `apps/unittests/app/lifecycle/tst_appsupervisor.cpp`
- Build wiring:
  - Added Qt Multimedia dependency: `CMakeLists.txt`, `apps/vcstream/CMakeLists.txt`, `apps/unittests/qml/CMakeLists.txt`, `apps/unittests/app/lifecycle/CMakeLists.txt`
- Theme utilities:
  - OKLCH conversion: `modules/ui/colour/oklchcolour.h`, `modules/ui/colour/oklchcolour.cpp`, `modules/ui/colour/oklchutil.h`, `modules/ui/colour/oklchutil.cpp`
  - Picker images: `modules/ui/theme/accentimageprovider.h`, `modules/ui/theme/accentimageprovider.cpp`
- Settings documentation updated:
  - `docs/SETTINGS.md`

### Temporary note (ongoing UI issues)

The user reports the following issues in a real interactive run (not reproduced by `tst_qml_ui` under `QT_QPA_PLATFORM=offscreen`). These need follow-up:

- Preferences overlay category switching/rendering:
  - Symptoms: Devices category shows an empty right-hand pane (no header, no frames); a Refresh button appears on the wrong category.
  - Repro: open Preferences, click the left “Devices” category.
  - Suspects: QML layout/visibility logic in `apps/vcstream/qml/PreferencesOverlay.qml` and/or state synchronisation between `apps/vcstream/qml/main.qml` and `apps/vcstream/qml/PreferencesOverlay.qml`.

- Accent theme propagation:
  - Symptoms: the accent swatch changes, but other controls that normally use the system accent keep using the system accent instead of the chosen accent.
  - Key requirement: do not manually apply accent colour per-widget; accent must propagate through palette/style as intended.
  - Suspects: palette inheritance through `StackView` pages rooted in plain `Item` trees; some controls may not be inheriting `ApplicationWindow.palette` in the live style/plugin.

- Light theme inconsistencies:
  - Symptoms: some surfaces remain dark and some text remains light when Light mode is selected.
  - Suspects: incomplete palette role overrides and/or palette inheritance not reaching all controls.

Notes for the next agent:
- `tst_qml_ui` currently passes but does not guarantee style/plugin parity with the user's desktop (for example KDE/Breeze).
- Consider adding a minimal on-screen debug readout (behind a temporary flag) of:
  - `PreferencesOverlay.currentCategoryIndex`
  - `appSupervisor.preferences.themeMode` / `accent`
  - `ApplicationWindow.palette.highlight`
  to confirm whether the state changes are not occurring vs not being applied by the style.
