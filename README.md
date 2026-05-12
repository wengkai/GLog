# GLogKit

**Version** (CMake `project`): 0.1.0

GLogKit is amateur radio log management software based on **Qt6** and **C++17**, for importing, editing, searching, statistics, and visualization of ADIF logs.

---

## Project Goals

* **ADIF compatibility**: Standard ADIF (`.adi` / `.adif`) parsing and importing.
* **Efficiency**: High-performance tabular editing and retrieval.
* **Analytical tools**: Award tracking, CSV/ADIF export, and geographic visualization.
* **Extensibility**: Foundation for rule-based plugins and a wider ecosystem.

---

## Current Capabilities

* **Importing**: Open and merge ADIF files with asynchronous processing.
* **Grid operations**: In-table editing, drag-and-drop, copy/paste, row deletion, column sorting/hiding.
* **Search**: Keyword search and regular expressions.
* **Manual entry**: Manual QSO entry with basic field validation.
* **Duplicate QSOs**: Find duplicates by configurable ADIF fields; designate a major record (including via context menu), merge others into it, or drop non-major rows ([`DuplicatesManager`](gui/DuplicatesManager.cpp)).
* **Award tracking**: Statistics for **DXCC**, **WAC**, **CQ**, and **WAS** (and related award logic in the model).
* **Award plugins**: Load and manage award plugins from the UI; sample **WAS** plugin under [`plugins/was_plugin`](plugins/was_plugin).
* **Prefix / entity data (CTY)**: Configure and refresh `cty.dat`-backed prefix mapping ([`ConfigureCtyDialog`](gui/ConfigureCtyDialog.cpp)); online fetch with fallback to the bundled database when needed.
* **Export**: ADIF or CSV.
* **Map visualization**: Geographic markers with opacity scaled by contact density (SVG world map + markers).

---

## Technical Stack

* **Language**: C++17
* **Framework**: Qt6 - Core, Gui, Widgets, Network, **SvgWidgets**, **OpenGLWidgets**, Sql, Test; on Linux, **WaylandClient** is used when available (optional).
* **Build system**: CMake (minimum 3.22)
* **Parser generation**: Flex / Bison (ADIF lexer/parser)
* **Database**: SQLite (persistence and queries)

---

## Directory Structure

| Path | Description |
| :--- | :--- |
| [`adif/`](adif/) | ADIF field types, constants, and generated maps. |
| [`parser/`](parser/) | Flex/Bison sources and driver for ADIF parsing. |
| [`model/`](model/) | Data models, repositories, statistics, duplicates logic. |
| [`gui/`](gui/) | Main window, search, map, CTY config, QSO entry, duplicates, award plugin manager. |
| [`tools/`](tools/) | Shared widgets (tables, map view), helpers, concurrency utilities. |
| [`plugins/`](plugins/) | Award plugins (e.g. WAS). |
| [`tests/`](tests/) | Unit and stress tests (`BUILD_TESTING`). |
| [`include/GLogKit/`](include/GLogKit/) | Public headers installed with the SDK. |
| [`cmake/`](cmake/) | `GLogKitConfig.cmake.in` and package wiring for `find_package(GLogKit)`. |
| [`scripts/`](scripts/) | Optional tooling (e.g. Windows coverage scripts used by CMake custom targets). |
| [`linux-deploy/`](linux-deploy/) | Linux packaging assets (e.g. desktop file, AppImage-related inputs). |
| [`assets/`](assets/) | Bundled resources (e.g. maps, `cty.dat`). |
| [`.github/workflows/`](.github/workflows/) | CI: multi-platform build, test, and release artifacts. |

---

## Build and Run

### Dependencies

* **Qt 6.7+** (required by the root `CMakeLists.txt` via `find_package(Qt6 6.7 ...)`)
* **CMake 3.22+**
* **C++17** compiler (MSVC, GCC, or Clang)
* **Flex and Bison** (to generate the parser); on Windows the CI uses **WinFlexBison** and passes `-DFLEX_INCLUDE_DIRS=...` to CMake.

### Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Multi-configuration generators (Visual Studio, Xcode) use `--config Release` (or `Debug`, etc.) on the build and test steps.

### Run tests

Single-configuration (Ninja/Makefiles):

```bash
ctest --test-dir build --output-on-failure
```

Multi-configuration (e.g. MSVC):

```bash
ctest --test-dir build -C Release --output-on-failure
```

Tests are gated by the CMake option **`BUILD_TESTING`** (default **ON**). Headless Linux CI runs Qt in **offscreen** mode (`QT_QPA_PLATFORM=offscreen`).

### Platform notes

* **Windows**: If Flex headers are not on the default include path, set **`FLEX_INCLUDE_DIRS`** to the WinFlexBison `include` directory (see [`.github/workflows/cmake-multi-platform.yml`](.github/workflows/cmake-multi-platform.yml)).
* **Contributors (Windows)**: When using MSVC, optional **`coverage`** / **`llvm-coverage`** custom targets may be available if OpenCppCoverage or LLVM `llvm-cov` / `llvm-profdata` are installed; see [`scripts/`](scripts/) and the root `CMakeLists.txt`.

---

## CI and release artifacts

GitHub Actions workflow: [`.github/workflows/cmake-multi-platform.yml`](.github/workflows/cmake-multi-platform.yml).

* **Triggers**: pushes and pull requests to `main`; tags matching `v*`.
* **Linux**: Build and test in a container with **Qt 6.11**; produces an **AppImage** (and related archive).
* **Windows**: **Qt 6.10.3**, MSVC 2022, WinFlexBison, **NSIS**; produces a **zip** and an **installer `.exe`**.
* **macOS**: **Qt 6.10.3**; produces **`.dmg`** and **`.tar.xz`**.
* **Releases**: For tags `refs/tags/v*`, when Linux artifact upload succeeds, workflow assets are attached to a GitHub Release (AppImage, zip, dmg, tar.xz, exe).

CI Qt versions are **what the pipeline validates**; locally you only need to satisfy **`find_package(Qt6 6.7 ...)`**.

---

## SDK and `find_package(GLogKit)`

The GUI and parsers are built into a shared library **`GLogKitCore`**; the application target is **`GLogKit`**. CMake installs an export set **`GLogKitTargets`** with namespace **`GLogKit::`** (e.g. link `GLogKit::GLogKitCore`).

The option **`DISTRIBUTE_SDK`** (default **ON**) controls installing public headers under `include/` and the CMake package (`GLogKitConfig.cmake`, version file, `GLogKitTargets.cmake`). After `cmake --install` with a chosen prefix, consumers can use:

```cmake
find_package(GLogKit REQUIRED)
target_link_libraries(my_target PRIVATE GLogKit::GLogKitCore)
```

Advanced **`BUNDLED_QT`** packaging paths are intended for redistributors; most developers link against a normal Qt6 SDK.

---

## Known Limitations

* **Plugins**: Only a sample WAS-style path is shipped; a richer third-party plugin catalog and stricter ABI guarantees are not in place yet.
* **Automation**: Duplicate handling is **user-driven** (pick major, merge, delete minors); there is no fully automatic policy engine for all merge edge cases.
