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

Notes:
- Compiler warnings are treated as errors by default. The warning policy is centralised in the root `CMakeLists.txt` via the `vcstream_warnings` interface target, and most in-repo targets link it.

## Run

The binary is written to `build/bin/`.

```sh
./build/bin/vcstream
```

## Build types and sanitisers

On single-config generators (Ninja/Makefiles), VCStream defaults `CMAKE_BUILD_TYPE` to `Debug` when it is not provided.

- Debug builds (default) enable sanitiser instrumentation by default (`VCSTREAM_ENABLE_SANITIZERS=ON`).
- Release builds do not enable sanitisers.

Examples:

```sh
# Debug (default)
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure

# Release
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
ctest --test-dir build-release --output-on-failure
```

To explicitly disable sanitisers (for example if your toolchain does not support them):

```sh
cmake -S . -B build -DVCSTREAM_ENABLE_SANITIZERS=OFF
```

## Internal tools

This repository includes some developer-only diagnostics that are not built by default.

- Enable: `-DVCSTREAM_BUILD_INTERNAL_TOOLS=ON`
- Documentation: `docs/INTERNAL-TOOLS.md`

## Settings

Settings persistence notes (including storage location) are documented in `docs/SETTINGS.md`.
