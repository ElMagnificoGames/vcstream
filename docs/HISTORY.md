
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
