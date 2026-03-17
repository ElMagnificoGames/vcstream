
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
