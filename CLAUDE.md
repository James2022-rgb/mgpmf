# mgpmf

GPMF (GoPro Metadata Format) parser library. Thin C++ pimpl wrapper
around the Rust-based `gpmf_tools` C ABI (`jgpmf_capi`), so consumers
get a clean header without dragging cbindgen-generated FFI types into
their public surface.

Designed to pair with mdemux's `Mp4GpmfTrackDemuxer`: the demuxer hands
per-MP4-sample byte ranges out of the `gpmd` track, mgpmf parses each
range into a `GpmfSample` from which the caller pulls structured
telemetry (GPS9, accel, gyro, gravity, orientation quaternions).

## Language

All code comments MUST be written in English.

## Symbols

It is forbidden to use the full width forms of symbols that have
counterparts in ASCII. e.g. `()`, `:`, `,`, `0-9`.

## Coding style

- `m<short>` library naming.
- `src/<libname>/{public,private}` layout; public include path is
  reached as `#include "mgpmf/public/<file>.h"`.
- TU header (`#include "<this file>.h"`) listed first in each
  `.cpp`, then C++ system headers, then external headers, then
  public project headers, then private project headers -- each
  group separated by a blank line.
- Slabs of related includes are grouped and labeled with the
  same `// public project headers --...` style markers used in
  the existing files.
- Public headers MUST NOT include `jgpmf_capi.h`. The C FFI handle
  lives behind a pimpl `struct Impl` defined in the `.cpp`.

## Build

CMake-based C++23 static library. Typically consumed as a sibling-lib
`add_subdirectory(... mgpmf ...)` from a parent project's
`CMakeLists.txt`. The parent **MUST** add `mbase` first --
mgpmf's `CMakeLists.txt` asserts the target already exists.

Target name: `mgpmf`. Links `mbase` PUBLIC, `jgpmf_capi` PRIVATE.

`jgpmf_capi` is built from source via `FetchContent` + a
`cargo build -p gpmf_capi --release` custom command (same pattern
mnexus uses for `vidsynt`). The build host MUST have `cargo` on PATH;
configure-time `find_program(CARGO_EXECUTABLE cargo REQUIRED)` fails
fast otherwise.

When adding source files to `CMakeLists.txt`, list them in
alphabetical order within each `set(...)` block.

## Directory structure

```
src/mgpmf/
  public/   <- API surface (no jgpmf_capi.h leaks)
    gpmf_sample.h
  private/  <- implementation (touches jgpmf_capi freely)
    gpmf_sample.cpp
```

## Public API

- `Gps9` -- one GPS fix snapshot: latitude / longitude (degrees),
  altitude (m), speed_2d / speed_3d (m/s), dop (unitless), fix
  (raw GoPro fix code), days_since_2000 + seconds_since_midnight
  for UTC reconstruction.
- `Vec3f` / `Quatf` -- POD value types used by the IMU getters.
- `GpmfSample` -- RAII handle around one parsed `gpmd` MP4 sample.
  - `Parse(bytes, len) -> std::unique_ptr<GpmfSample>` (nullptr on
    parse failure).
  - `gps9() -> std::optional<Gps9>` -- one value per sample at most.
    `nullopt` when this sample carried no GPS9 fix.
  - `accl() -> std::span<Vec3f const>` -- accelerometer samples
    in m/s^2 (~200 Hz inside one MP4 sample).
  - `gyro() -> std::span<Vec3f const>` -- gyroscope, rad/s
    (~400 Hz).
  - `grav() -> std::span<Vec3f const>` -- gravity vector, unit-norm.
  - `cori() -> std::span<Quatf const>` -- camera orientation
    quaternions (w, x, y, z).
  - `iori() -> std::span<Quatf const>` -- IMU orientation
    quaternions.
  - The spans alias into the `GpmfSample`'s internal arena; their
    lifetime ends when the `GpmfSample` is destroyed.

Time alignment is the caller's job: mgpmf has no notion of MP4
PTS. The caller pulls the MP4 sample's PTS / duration from its
demuxer and distributes them across the returned spans (uniform
inside the sample is the GoPro convention).

The public types live in namespace `mgpmf`.

## Dependencies

- `mbase` -- assertions, logging. Linked PUBLIC. Provided by the
  parent; mgpmf asserts the target exists.
- `jgpmf_capi` -- the C ABI of `gpmf_tools`
  (https://github.com/James2022-rgb/gpmf_tools), built from source
  via `cargo build -p gpmf_capi --release`. mgpmf's CMakeLists
  pulls a pinned revision via FetchContent and drives the cargo
  command itself (no upstream `CMakeLists.txt` to reuse).

## License

`jgpmf_capi` follows the gpmf_tools repo's license (consult the
upstream repo). mgpmf itself adds no extra license constraint
beyond mbase and the project the consumer chooses.

## Threading

`JgpmfSample` parsing + accessor calls are safe in isolation per
sample. A single `GpmfSample` instance is not internally
synchronized; the caller serializes calls on the same instance.
Distinct instances on different threads are fine.

## Commit messages

Conventional-Commits-style with the lib as the scope, e.g.
`feat(mgpmf): ...`, `fix(mgpmf): ...`, `perf(mgpmf): ...`,
`docs(mgpmf): ...`, `refactor(mgpmf): ...`.
