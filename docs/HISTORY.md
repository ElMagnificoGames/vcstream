
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
- Added additional Appearance preferences:
  - font family override (system-installed fonts)
  - font size scaling (75% to 150%)
  - UI density (compact/comfortable/spacious)
  - zoom scaling (50% to 200%)
- Extended the QML UI smoke test to open Preferences from the landing page and from the shell, switch categories, refresh the device catalogue, and close the overlay without emitting warnings.

### Why

- The roadmap needs a stable home for user preferences before capture and networking services arrive.
- A local “what does my machine see?” catalogue reduces future debugging time for device enumeration, permissions, and platform differences.

### Acceptance criteria

- Preferences entry point exists in the app shell UI.
- At least one preference is persisted and restored (beyond window placement).
- Device catalogue can refresh without restart and does not require starting capture.
- Failure/absence cases (no devices, missing window-capture support) are represented without crashing.
- Appearance preferences apply immediately and do not introduce layout glitches or QML warnings.

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
  - `AppSupervisor` also provides a font family enumeration helper for QML.
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

### Temporary note (font picker hang + corrupted preview)

During interactive testing, a repeatable UI stall (multi-second hang that recovers) was observed in the font picker while dragging the scrollbar thumb. A corrupted preview render was also observed for some fonts.

What was done (temporary diagnostics)
- A temporary, opt-in, fine-grained timing trace was implemented to capture GUI-thread stalls during font picker scroll-thumb drags.
  - Enable: `VCSTREAM_TRACE_FONT_SCROLL=1`
  - Output: `/tmp/vcstream-fontscroll-<pid>.log`
  - Implementation (removed again after diagnosis):
    - Logger: `apps/vcstream/perftrace.h`, `apps/vcstream/perftrace.cpp`
    - Wiring: `apps/vcstream/main.cpp` (exposed `perfTrace` to QML)
    - QML trace: `apps/vcstream/qml/VcFontPicker.qml` (16ms sampling while the thumb was pressed)
- The tracer was iterated to correctly compute visible indices/families during scrolling (ListView `indexAt()` content coordinates).

What was shown (evidence)
- The hang is a real GUI-thread stall: log samples show large deltas (roughly ~1s to ~6s) while `barPressed=true`.
- List model metrics remained stable during stalls (count/content height/viewport height did not thrash), arguing against a model rebuild as the primary cause.
- After the tracer was corrected, stalls correlated strongly with a specific visible region of the list that contained Bitcount “Ink” font families (for example “Bitcount Grid Double Ink”).
- Screenshots showed corrupted preview rendering for at least “Bitcount Grid Double Ink”.
- The user confirmed other applications on the same system also struggle with “Bitcount Grid Double Ink”, suggesting the issue may be in the font itself and/or shared font rendering stack rather than VCStream-only.

Additional technical investigation (evidence)
- Font inspection on the affected system showed:
  - “Bitcount Grid Double Ink” is a colour font (`color=true`) with COLR/CPAL tables and COLR version 1 (COLRv1), and is also a variable font (fvar axes present).
- Other colour fonts tested by the user (for example Bungee Color, Cairo Play, Noto Color Emoji, Aref Ruqaa Ink, Blaka Ink) did not reproduce the hang or corruption in VCStream.

What is still speculation (not proven)
- The exact cause of the stall is not proven. A plausible hypothesis is that previewing sample text in certain fonts triggers expensive or buggy rendering/shaping paths that can block the GUI thread.

What remains to be done (future work)
- Design a robustness strategy that does not globally blacklist fonts based on one machine.
  - Prefer adaptive behaviour: avoid blocking the GUI thread while previewing fonts (for example async/cached previews, throttling preview updates during thumb drag and search, and/or falling back per-font after detecting repeated stalls).
- Reproduce and validate on multiple systems/Qt versions.

Cleanup
- The temporary tracing code described above has been removed after collecting logs, to avoid leaving diagnostic plumbing in production builds.

## Task 1.4b — Add font preview render health cache

### What

- Added a UI-facing font render health cache that can check a font family and mark it as safe or unsafe for preview rendering on the current machine.
- Wired the cache into the Preferences font picker so per-row previews only render with a candidate font after it passes the preview check.
- Extended the Preferences “sample” preview box to suppress rendering when the selected font is known-bad for preview, whilst still allowing the font to be selected and persisted.

### Why

- Some fonts can render corrupted output (or trigger large stalls) in the underlying font stack without emitting Qt warnings or errors.
- The font picker previously attempted to render sample text for many different font families during scroll, which can repeatedly hit the problematic path.
- The UI needs a robustness strategy that is behaviour-based on the current machine and does not hardcode a global blacklist.

### Acceptance criteria

- The font picker can avoid repeatedly attempting per-row previews for fonts that the preview check classifies as corrupted.
- The user can still select any font family (the cache is advisory and must not block selection).
- UI changes introduce no QML warnings (QML warning hygiene gate remains green).

### Decisions

- Preflight is based on output sanity heuristics (small glyph render into a `QImage`) rather than timing thresholds, to avoid false positives due to transient CPU/scheduling variability.
- The cache is advisory-only and is used by preview code; it does not gate persisted preference values.
- Preflight work is run on a dedicated worker thread and results are cached per `(family, pixelSize)`.

### Technical notes

- New module:
  - `modules/ui/fonts/fontpreviewsafetycache.h`
  - `modules/ui/fonts/fontpreviewsafetycache.cpp`
  - `modules/ui/fonts/fontpreviewsafetycache-dd.txt`
- Supervisor wiring:
  - `AppSupervisor` now owns and exposes `fontPreviewSafetyCache`: `modules/app/lifecycle/appsupervisor.h`, `modules/app/lifecycle/appsupervisor.cpp`, `modules/app/lifecycle/appsupervisor-dd.txt`
- Preferences UI integration:
  - `apps/vcstream/qml/VcFontPicker.qml`
  - `apps/vcstream/qml/PreferencesOverlay.qml`
- Build wiring:
  - `modules/ui/fonts/CMakeLists.txt`
  - `modules/CMakeLists.txt`
  - `modules/app/lifecycle/CMakeLists.txt`
- QML type registration (for enum visibility/debugging):
  - `apps/vcstream/main.cpp`

## Task 1.4c — Make font preview checks resilient and silhouette-based

### What

- Reworked the font preview health cache to use an outline silhouette comparison rather than simple pixel bounding heuristics.
- Expanded the cache state model so the UI can distinguish:
  - never checked
  - checking in progress
  - safe to preview
  - preview output appears incorrect
  - check timed out (inconclusive)
  - check blocked because too many previous checks timed out
- Updated the font picker preview UI to show user-facing status messages for these states and to only render per-row previews when the cache says the font is safe.

### Why

- Some fonts can produce corrupted raster output even for very small strings.
- Simple sanity heuristics can misclassify corrupted output as "safe" at small preview sizes.
- When the underlying font stack hangs, Qt does not provide a safe way to abort the operation within the same process.
  The application must protect its UI without forcing unsafe thread termination.

### Acceptance criteria

- The font picker does not render per-row previews for fonts until the preview check succeeds.
- If a check does not complete, the UI remains responsive and reports "Preview unavailable" rather than stalling.
- QML warning gate remains green.

### Decisions

- The font preview check compares two renders of the same string:
  - raster output via `QPainter::drawText`
  - silhouette output via `QRawFont::pathForGlyph`
  It then computes a mask similarity score (IoU) to classify safe vs incorrect.
- Timeouts are treated as inconclusive (`TimedOut`), not as "bad font".
- Checks are run on per-check worker threads. Threads that time out are treated as potentially stuck and are not forcibly terminated.
- To cap worst-case resource growth when the font stack hangs, the cache blocks starting new checks once a fixed number of timeouts has been reached.
  Blocked entries may be retried later when checks are requested again.

### Technical notes

- Cache implementation updates:
  - `modules/ui/fonts/fontpreviewsafetycache.h`
  - `modules/ui/fonts/fontpreviewsafetycache.cpp`
  - `modules/ui/fonts/fontpreviewsafetycache-dd.txt`
  - `modules/ui/fonts/fontpreviewsafetychecker.h`
  - `modules/ui/fonts/fontpreviewsafetychecker.cpp`
  - `VCSTREAM_FONT_PREVIEW_SAFETY_DEFAULT_MAX_STUCK_CHECKS` is a single source-level constant.
- UI updates:
  - `apps/vcstream/qml/VcFontPicker.qml`
  - `apps/vcstream/qml/PreferencesOverlay.qml`

- Unit tests:
  - Added deterministic tests for the cache state machine using an injected fake checker:
    - `apps/unittests/ui/fonts/tst_fontpreviewsafetycache.cpp`

## Roadmap note — Defer paper-grain overlay and reduced-motion preference

### What

- Updated the roadmap to explicitly mark the paper-grain overlay and reduced-motion preference as deferred follow-ups.

### Why

- The current application feature set is still early and UI-focused; these items are polish rather than core scaffolding.
- Paper grain is decorative and easy to get wrong (contrast/readability and scaling artefacts).
- The UI currently contains little explicit authored motion, so a reduced-motion preference would not provide meaningful immediate benefit.

### Decisions

- Paper grain remains a possible future enhancement, but is not a near-term priority; revisit once there are more content-dense screens and we can validate readability.
- Reduced-motion preference remains a possible future enhancement; revisit once we add non-trivial animations/transitions so the preference has immediate effect.

### Technical notes

- Roadmap update: `docs/ROADMAP.md`

## Task 1.4d — Bundle Victorian fonts and add typeface preset

### What

- Bundled a Victorian font pair (body + headings) and registered it at startup so the app's Victorian style has consistent typography across machines.
- Added a new Preferences "Typeface" preset so users can pick between Victorian, system default, and a custom installed font.
- Made Victorian the default for new installs for theme mode, accent, and typeface preset.

### Why

- The project style direction relies on authored typography; relying only on system fonts undermines the intended visual identity.
- A preset avoids forcing users to understand font-family names whilst keeping the existing power-user custom font picker.

### Acceptance criteria

- Victorian typography renders even on machines without any special fonts installed.
- Users can switch between Victorian, system, and custom font choices without QML warnings or layout glitches.

### Decisions

- Typeface preset is independent of theme mode; selecting Victorian mode does not implicitly change the typeface preset.
- Custom font selection is represented as a preset (`custom`) plus an optional font family string; the empty string remains "system default".
- Bundled fonts chosen from the Google Fonts catalogue under SIL OFL 1.1:
  - body: Libre Caslon Text (variable font)
  - headings: Libre Caslon Display

### Technical notes

- Bundled fonts and licences:
  - `third_party/fonts/librecaslontext/`
  - `third_party/fonts/librecaslondisplay/`
- Font resource wiring:
  - `modules/ui/fonts/bundledfonts.qrc`
  - `modules/ui/fonts/bundledfonts.h`
  - `modules/ui/fonts/bundledfonts.cpp`
  - `modules/ui/fonts/CMakeLists.txt`
- App wiring:
  - `modules/app/lifecycle/appsupervisor.h`
  - `modules/app/lifecycle/appsupervisor.cpp`
- Preferences persistence:
  - `modules/app/settings/apppreferences.h`
  - `modules/app/settings/apppreferences.cpp`
  - `modules/app/settings/apppreferences-dd.txt`
  - `docs/SETTINGS.md`
- QML UI updates:
  - `apps/vcstream/qml/main.qml`
  - `apps/vcstream/qml/PreferencesOverlay.qml`
  - `apps/vcstream/qml/LandingPage.qml`
  - `apps/vcstream/qml/ShellPage.qml`
- QML smoke tests updated:
  - `apps/unittests/qml/tst_qml_ui.cpp`

## Repository note — Adopt AGPL-3.0-only licence

### What

- Added the GNU Affero General Public License v3 text and a repository notice declaring this project as `AGPL-3.0-only`.

### Why

- The project goal is strong copyleft, including hosted network use-cases for future relay/rendezvous components.
- The project explicitly avoids automatic "or later" licensing.

### Technical notes

- Licence text: `LICENSE`
- Repository notice: `NOTICE`

## Repository note — Forbid streaming/chaining `operator<<`

### What

- Updated coding conventions to forbid overloaded `operator<<` chains used for streaming/chaining (Qt log streaming, `QTextStream`, iostreams, `QDataStream`, and Qt container chaining idioms).
- Added a unit-test style gate that scans the source tree and fails if forbidden `operator<<` streaming patterns are introduced.
- Converted existing offending code to explicit printf-style logging and explicit append/formatting patterns.

### Why

- Readability is the project's top priority; stream/chaining overloads tend to hide formatting/encoding decisions and encourage long, hard-to-scan expressions.
- Automated enforcement prevents future regressions by making violations immediately visible in CI.

### Technical notes

- Convention update: `docs/CODE-QT6.md`
- Agent-facing reminder: `AGENTS.md`
- Style gate test:
  - `apps/unittests/style/tst_style.cpp`
  - `apps/unittests/style/CMakeLists.txt`

## Task 1.5 — Add landing-page popups (Scheduling + Support)

### What

- Added two optional landing-page actions: a scheduling popup and a support popup.
- Implemented each popup as its own modal overlay with outside-click dismissal, an explicit close button, and no layout participation in the underlying page.
- Wired the scheduling popup to open the external schedule coordination tool in the platform-default browser.
- Fixed the scheduling popup's internal layout sizing so its contents are measured and rendered correctly.
- Reworked the support popup from placeholder copy into a fuller, scrollable support page that explains why channel growth matters, ranks useful free support actions from most important to less important, and ends with a quieter note about gifting subscriptions.
- Expanded QML layout/smoke test coverage so modal overlays are exercised and the overlap checks treat overlays as separate interaction layers.
- Made the Scheduling/Support/Preferences action cluster consistent across the landing screen and the in-room shell, and hosted the popups at the app root so either page can open them.
- Adjusted typography so modal titles use the body face, and the shell masthead uses the display face at a larger size.

### Why

- The roadmap called for two small optional landing actions that stay out of the main start flow while still being easy to discover.
- Scheduling help is useful before a user has committed to joining or hosting a room, so it belongs on the landing screen rather than in deeper settings.
- The support popup needed to do more than advertise links: it needed to explain why supporting the channel matters in plain language, avoid streamer jargon, and make free support feel concrete and worthwhile.
- The support copy is longer than a compact tooltip-style popup, so the UI needed a scrollable body and stronger typographic hierarchy to keep it readable on smaller windows.

### Acceptance criteria

- The landing screen now exposes two separate optional buttons: scheduling and support.
- Each button opens its own overlay popup without causing landing-page layout reflow or hover oscillation.
- External links are opened via the platform-default browser.
- The optional actions are available from both the landing screen and the in-room shell.
- QML UI smoke tests explicitly exercise opening and closing both popups and continue to fail on any warnings.

### Decisions

- Kept the new actions in the landing page's existing top-right utility area, alongside Preferences, so they remain clearly optional and visually secondary to the main Join/Host actions.
- Marked the landing popup-launch buttons and browser-launch buttons with `testSkipActivate: true` so the automatic interaction sweep can still hover them without opening overlays or external browsers opportunistically in CI.
- Used a compact scheduling popup with one clear external action, but made the support popup scrollable because the copy needs room for ranked items and explanatory text.
- Kept the support content structured for skimming (short intro, ranked free-support items, and an optional paid-support note) while staying within a single landing-only popup.

### Technical notes

- Landing-page popup implementation and copy:
  - `apps/vcstream/qml/LandingPage.qml`
- Reusable action cluster + shared overlays:
  - `apps/vcstream/qml/VcUtilityActions.qml`
  - `apps/vcstream/qml/SchedulingOverlay.qml`
  - `apps/vcstream/qml/SupportOverlay.qml`
  - `apps/vcstream/qml/main.qml`
- QML warning-gate and popup interaction coverage:
  - `apps/unittests/qml/tst_qml_ui.cpp`
- Version bump for this feature task:
  - `CMakeLists.txt`

## Task 1.5a — Stabilise Qt runtime defaults (Windows + QML warning gate)

### What

- Added an app-owned Qt Quick Controls configuration (`qtquickcontrols2.conf`) that defaults the control style to `Basic` so the app's wrapper controls can customise `contentItem`/`background` without Qt emitting warnings.
- Introduced a small platform shim module that applies startup-time Qt defaults that must run before `QGuiApplication` is constructed.
- On Windows, the shim sets `QT_QPA_FONTDIR` to `%WINDIR%/Fonts` when not already specified, preventing Qt's FreeType font database backend from warning about missing SDK font directories.
- Wired the shim into all GUI entry points in this repository that construct `QGuiApplication` (`vcstream`, the QML smoke-test runner, and `vcstream_fontprobe`).
- Centralised the compiler warning policy into a shared CMake interface target so all in-repo targets build with consistent strict warnings.

### Why

- QML warnings are a hard test gate in this repository; Qt Quick Controls native styles on Windows can emit warnings when the app customises control internals.
- The font subsystem warning indicates Qt is probing an SDK directory that does not exist in typical deployments; leaving this unresolved causes the warning gate to fail and makes Windows behaviour noisier than Linux.
- Centralising these pre-`QGuiApplication` adjustments avoids platform-specific clutter in otherwise cross-platform entrypoint code and ensures consistent behaviour across executables.

### Acceptance criteria

- On Windows, `vcstream` and `tst_qml_ui` start without Qt Quick Controls warnings about unsupported customisation.
- On Windows, startup does not emit `QFontDatabase: Cannot find font directory .../lib/fonts` warnings under offscreen/headless runs.
- The QML smoke tests continue to fail on real warnings (the fix removes underlying causes; it does not suppress warnings).

### Decisions

- Style default: set Qt Quick Controls 2 to `Basic` via `qtquickcontrols2.conf` rather than using environment variables or per-machine configuration.
- Platform shims: introduced `modules/app/platform/qtshims.*` as a narrowly scoped place for platform-specific Qt startup defaults, with an explicit warning against growth into a general helper bucket.
- Warnings policy: apply `-Wconversion` (and `-Werror`) consistently across non-MSVC builds so narrowing/sign-conversion issues do not slip through silently on Linux.

### Technical notes

- Qt Quick Controls configuration:
  - `apps/vcstream/qml/qtquickcontrols2.conf`
  - `apps/vcstream/qml/qml.qrc`
- Platform shim module:
  - `modules/app/platform/qtshims.h`
  - `modules/app/platform/qtshims.cpp`
  - `modules/app/platform/qtshims-dd.txt`
- Shim call sites:
  - `apps/vcstream/main.cpp`
  - `apps/unittests/qml/tst_qml_ui.cpp`
  - `apps/fontprobe/main.cpp`

## Task 2.1 — Write a transport and protocol ADR

### What

- Added a first architecture decision record (ADR) describing the transport and protocol direction for VCStream.

### Why

- The transport choice constrains the protocol envelope, media multiplexing, encryption assumptions, and whether NAT traversal/hole punching remains viable without a later rewrite.
- Writing this down early reduces accidental drift into TCP-only assumptions.

### Acceptance criteria

- An ADR exists that makes the transport direction explicit.
- The ADR records alternatives considered and key consequences/risks.
- The ADR explicitly calls out follow-up decisions that are intentionally deferred (trust UX, rendezvous scope details, control reliability details).

### Decisions

- Transport family: UDP secured with DTLS for both control and media.
- Control serialisation: CBOR.
- Message framing: one UDP datagram carries exactly one CBOR message.
- Control channel direction: add a small reliability layer (message ids + acks + retries) for control messages that must arrive.

### Technical notes

- ADR location:
  - `docs/adr/0001-transport-and-protocol.md`

## Maintenance — Fix `-Wconversion` build breaks; ensure accent changes repaint ComboBox indicator

### What

- Fixed several build failures revealed by enabling `-Wconversion -Werror` across the repository.
  - `oklchcolour::toSrgbFitted()` now passes explicit `float` channel values to `QColor::fromRgbF()`.
  - Font preview reference rendering now uses `qsizetype` for glyph vector iteration to avoid narrowing conversions.
  - QML UI test brace counting now uses `qsizetype` to match `QString::count()`.
- Fixed a Windows-visible UI bug where the dropdown "V" indicator on `VcComboBox` could keep the previous accent colour (notably Victorian oxblood) after changing the accent.
  - The indicator is a `Canvas`, which does not repaint automatically when its inputs change.
  - Added an explicit repaint trigger when the effective stroke colour changes.
- Added a QML smoke test that changes the accent and asserts the ComboBox indicator repaints (using a `paintSerial` counter on the Canvas).

### Why

- With strict conversion warnings enabled, previously-benign narrowing/sign conversions now fail the build; these need to be corrected rather than weakening the warning policy.
- Accent switching should visually propagate to all themed elements immediately; stale Victorian colour accents are confusing and undermine the theme system.

### Acceptance criteria

- The project builds cleanly under `-Wconversion -Werror`.
- Changing the accent updates the `VcComboBox` indicator colour without needing a resize or other incidental repaint.
- QML UI smoke tests include coverage for this repaint behaviour and emit no warnings.

### Decisions

- Treat the ComboBox indicator repaint as an explicit responsibility of `VcComboBox` (request paint on colour change) rather than relying on incidental `paletteChanged` or theme object replacement.
- Use a simple `paintSerial` property on the indicator `Canvas` to make repaint behaviour testable and deterministic.

### Technical notes

- Version bump for this maintenance change:
  - `CMakeLists.txt`
- ComboBox indicator repaint fix:
  - `apps/vcstream/qml/VcComboBox.qml`
- Repaint regression test:
  - `apps/unittests/qml/tst_qml_ui.cpp`
- `-Wconversion` build fixes:
  - `modules/ui/colour/oklchcolour.cpp`
  - `modules/ui/fonts/fontpreviewsafetycheckerdefault.cpp`
