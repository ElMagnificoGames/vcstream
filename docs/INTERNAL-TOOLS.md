# Internal Tools

This repository includes a small set of developer diagnostics that are not intended to ship to end users.

## Build option

Internal tools are not built by default.

Enable them via:

```sh
cmake -S . -B build -DVCSTREAM_BUILD_INTERNAL_TOOLS=ON
cmake --build build
```

## vcstream_fontprobe

`vcstream_fontprobe` is a font rendering diagnostics tool used to investigate font stack issues (stalls, corrupted glyph output, fallback behaviour).

- Target: `vcstream_fontprobe`
- Source: `apps/fontprobe/`

Notes:
- The default output directory is a temp directory under `vcstream-fontprobe`.
- The tool attempts Qt Quick scenarios by default; in environments without a working Qt Quick backend (for example headless CI), Qt Quick scenarios may be skipped.

Example:

```sh
./build/bin/vcstream_fontprobe --family "Noto Sans"
```

## vcstream_cameraprobe

`vcstream_cameraprobe` is a camera diagnostics tool used to investigate camera enumeration and preview capture issues.

- Target: `vcstream_cameraprobe`
- Source: `apps/cameraprobe/`

It prints:

- Qt Multimedia camera enumeration
- Windows DirectShow camera enumeration (when available)

It can also attempt to start capture for the first available camera to confirm whether frames are arriving.

Examples:

```sh
./build/bin/vcstream_cameraprobe
./build/bin/vcstream_cameraprobe --capture-first
```
