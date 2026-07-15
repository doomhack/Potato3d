# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Potato3d is a fixed-point software 3D renderer designed for very slow processors — primarily the Game Boy
Advance (ARM7TDMI, no FPU, no hardware 3D). It sorts/culls geometry with a precomputed BSP tree, renders
front-to-back with zero overdraw (no Z-buffer in the normal path), and avoids division in the hot path via a
reciprocal lookup table. Math is fixed-point (`P3D::fp`, backed by `FP16`) everywhere except one-time
setup/tooling code; switching to `double` is a single `#define` in `ConfigUser.h`.

The repo is one Qt-based toolchain (asset baking + desktop testing) feeding a GBA game/engine build
(devkitARM). There is no single build for "the project" — pick the sub-project relevant to the task.

## Repository layout

- **Root (`3dmaths/`, `RenderDevice.h`, `RenderTriangle.h`, `RenderTarget.h`, `RenderCommon.h`, `TextureCache.h`,
  `Pixel.h`, `PixelShaderDefault.h`, `PixelShaderGBA8.h`, `bspmodel.h`/`.cpp`, `BspModelDefs.h`, `Config*.h`) —
  the actual renderer library. This is the shared core; nothing here is project-specific. It's compiled
  directly into whichever example/tool includes it (there is no separate lib build step).
- **`P3DObj2Bsp/`** — Qt command-line tool. Converts a `.obj`/`.mtl` model into the engine's custom binary BSP
  format (`bspbuilder.*` builds the tree, `bspmodelexport.*` serializes it). Outputs both a raw `.bsp` file and
  a `.cpp` file with the same bytes as a `const unsigned char modeldata[]` array for embedding directly into a
  GBA ROM (see `SaveBytesAsCFile` in `main.cpp`). Source `.obj` paths are hardcoded at the top of `main.cpp` —
  edit that path rather than adding CLI argument parsing.
- **`P3DPVS/`** — Qt command-line tool. Computes Potentially Visible Set (PVS) data for a BSP model
  (multi-threaded raycast-based visibility between leaf nodes), producing the vis-data blob consumed by
  `VisData::CheckPvs` in `bspmodel.h`. Also hardcodes its input model path in `source/main.cpp`.
- **`Potato3dExample2/`** — the current GBA example/game. Has both a `.pro` (Qt, for desktop iteration/editing
  with `compile_commands.json` via qtcreator/clangd) and a devkitARM `Makefile` (the real GBA build). This is
  the project to build/run when testing renderer or gameplay changes on real GBA target.
- **`P3DBenchmark/`** — GBA benchmark harness (devkitARM Makefile) used to measure renderer performance
  changes; results tracked in `Performance.xlsx`. Use this when a change claims a performance improvement.
- **`Potato3dExample`** — older Qt desktop example using a legacy `object3d.h` API that no longer exists at the
  repo root. Treat as unmaintained/reference-only; don't extend it.
- **`Qt Debug Helper/`** — a Qt Creator debugger pretty-printer (`personaltypes.py`) for inspecting the fixed
  point / vector types while debugging in Qt Creator.

## Build

### Qt sub-projects (P3DObj2Bsp, P3DPVS, and the desktop build of Potato3dExample2/P3DBenchmark)

These are qmake `.pro` projects requiring Qt 6 (C++20) and are normally built/run from Qt Creator. From the
command line:

```
qmake <ProjectName>.pro
make
```

`P3DObj2Bsp` and `P3DPVS` are `cmdline`/no-GUI Qt apps — run the resulting binary directly. Note both hardcode
input model file paths near the top of their `main.cpp`; change the path there when pointing at a different
model.

### GBA build (Potato3dExample2, P3DBenchmark)

Requires devkitARM (devkitPro) with `DEVKITARM` set in the environment:

```
export DEVKITARM=/path/to/devkitARM
cd Potato3dExample2   # or P3DBenchmark
make
```

Produces `.elf`/`.gba`/`.sav`. Run the `.gba` in an emulator (mGBA, VBA) or on hardware.

Files named `*.iwram.cpp`/`*.iwram.s` (or the `*.redir.iwram.cpp` wrappers that just `#include` a root-level
`.cpp`) are placed in IWRAM (fast on-chip RAM) by the devkitARM GBA build rules for speed — this is why the
per-project `source/` directories contain thin "redirect" files instead of compiling the root `.cpp` files
directly. When adding a new root-level `.cpp` that needs to be IWRAM-resident, add a matching
`source/<name>.redir.iwram.cpp` in the GBA project rather than referencing the root file straight from the
Makefile.

There is no automated test suite in this repo; validation is done by building for GBA (and/or the Qt desktop
config) and running in an emulator, plus the `P3DBenchmark` harness for perf-sensitive changes.

## Configuration

Renderer-wide constants live in two files at the repo root, both included via `Config.h`:

- **`ConfigUser.h`** — user-tunable settings: pixel format (`pixelType`/`pixel`, currently RGB332 8bpp),
  texture size, light/fog levels, fixed-point vs float (`USE_FLOAT`), meters-to-world-units scale, etc.
- **`ConfigInternal.h`** — derived constants computed from `ConfigUser.h` values (shifts/masks); don't hand-edit
  these, change the source value in `ConfigUser.h` instead.

`RENDER_STATS` and `THREAD_LOCAL` are auto-disabled/no-op on `__arm__` (the GBA target) since GBA has no
threads and stats tracking costs cycles that matter there. When touching renderer hot paths, keep in mind code
gated by `#ifdef RENDER_STATS` or `no_inline`/`PROFILING` is intentionally structured for GBA codegen, not just
style.

## Architecture notes

- **Rendering flow**: `RenderDevice` (root `RenderDevice.h`) owns the matrix stack, projection, and viewport,
  and is the public API a game calls (`SetPerspective`, `PushMatrix`/`Translate`/`RotateX/Y/Z`, `DrawTriangle`,
  `BeginFrame`/`EndFrame`). It transforms vertices and hands off per-triangle rasterization to
  `P3D::Internal::RenderTriangle<render_flags, TPixelShader>` (`RenderTriangle.h`), which is templated on a
  `RenderFlags` bitmask (Z-test/write, alpha test, perspective-correct mapping mode, back/front-face culling,
  fog, vertex light) and a pixel shader type — so different render configurations are distinct
  compile-time-specialized classes, chosen at runtime via `RenderDevice::SetRenderFlags<flags, Shader>()`.
- **Pixel shaders**: `PixelShaderDefault.h` is the generic shader; `PixelShaderGBA8.h` is a GBA-specific
  variant. `Pixel<RBits,GBits,BBits,TStorageType>` (`Pixel.h`) is a compile-time bitfield-packed color type
  (`RGB888`/`RGB565`/`RGB555` instantiations provided); the active format is chosen by `pixelType` in
  `ConfigUser.h`.
- **World geometry**: models are precompiled BSP trees, not runtime-built. `P3DObj2Bsp` bakes a `.obj` into the
  binary layout described in `BspModelDefs.h` (`BspModelHeader`, `BspModelNode`, `BspModelTriangle`, etc. — all
  offset-based, meant to be `reinterpret_cast` directly over a flat byte blob, e.g. `modeldata[]` linked into
  ROM). `bspmodel.h`/`.cpp` (`P3D::BspModel`) reads that blob at runtime: `Sort()` walks the tree front-to-back
  (or back-to-front for translucency) from a view position, applying frustum AABB rejection and optional PVS
  and backface culling, and emits an ordered triangle list for the renderer to draw with zero overdraw.
- **PVS**: `P3DPVS` precomputes, per BSP leaf, which other leaves are potentially visible (`VisDataHeader` /
  per-node visibility bitsets in `BspModelDefs.h`). `BspModel::SetVisData()` attaches this at runtime, and
  `Sort()` consults `VisData::CheckPvs(src, dst)` to skip invisible subtrees. This is the actively-developed
  area of the codebase right now (see recent commit history) — PVS correctness bugs tend to show up as visual
  dropouts (missing geometry) or hangs in the PVS bake itself given its recursive/multi-threaded visibility
  search.
- **Fixed point math**: `3dmaths/fp.h` defines `FP16`; `3dmaths/recip.cpp`/`recip.h` provide the reciprocal
  table that replaces division in `V3`/`V4`/`M4`/perspective-divide code. `3dmaths/f3dmath.h` is the umbrella
  include for `v2.h`/`v3.h`/`v4.h`/`m4.h`/`plane.h`/`aabb.h`. When editing math code, remember it must compile
  and behave correctly under both the fixed-point (`FP16`) and `USE_FLOAT` (`double`) configurations.
- **Texture cache**: `TextureCacheBase`/`TextureCacheDefault` (`TextureCache.h`) is a pluggable interface for
  managing which textures are resident (relevant on GBA where texture memory is scarce);
  `texturecachegba.h`/`.cpp` in the example/benchmark projects provide a GBA-specific implementation swapped in
  via `RenderDevice::SetTextureCache()`.
