# Build

This repository uses CMake.

## Prerequisites

- CMake
- A C++17 compiler
- Qt 6 development packages (Qt Quick)
- Qt 6 development packages (Qt Quick Controls 2)

## Configure

From the repository root:

```sh
cmake -S . -B build
```

If CMake cannot find Qt 6, set one of the standard CMake hints:

- `CMAKE_PREFIX_PATH` (common when using a Qt SDK install)
- or `Qt6_DIR` (points at the Qt 6 CMake package directory)

Example:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.y/<toolchain>
```

## Build

```sh
cmake --build build
```

## Run

The binary is written to `build/bin/`.

```sh
./build/bin/vcstream
```

## Internal tools

This repository includes some developer-only diagnostics that are not built by default.

- Enable: `-DVCSTREAM_BUILD_INTERNAL_TOOLS=ON`
- Documentation: `docs/INTERNAL-TOOLS.md`

## Settings

Settings persistence notes (including storage location) are documented in `docs/SETTINGS.md`.
