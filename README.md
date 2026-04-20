# GLogKit

GLogKit is an amateur radio log management software based on **Qt6** and **C++17**, designed for importing, editing, searching, statistics, and visualization of ADIF logs.

---

## Project Goals

* **ADIF Compatibility**: Fully compatible with standard ADIF (`.adi` / `.adif`) data parsing and importing.
* **Efficiency**: Provide a high-performance tabular log editing and retrieval experience.
* **Analytical Tools**: Support award tracking, CSV/ADIF export, and geographic visualization.
* **Extensibility**: Establish an interface foundation for future rule-based plugins and ecosystem expansion.

---

## Current Capabilities

* **Importing**: Supports opening/merging ADIF files via asynchronous processing.
* **Grid Operations**: In-table editing, drag-and-drop, copy/paste, row deletion, and column sorting/hiding.
* **Search**: Keyword-based search and Regex (Regular Expression) support.
* **Manual Entry**: Manual QSO entry with basic field validation.
* **Award Tracking**: Statistics for **DXCC**, **WAC**, **CQ**, and **WAS**.
* **Export**: Export logs to ADIF or CSV formats.
* **Map Visualization**: Geographic point mapping with intensity adjusted by contact density.

---

## Technical Stack

* **Language**: C++17
* **Framework**: Qt6 (Core, Gui, Widgets, Network, Svg, OpenGL, Sql, Test)
* **Build System**: CMake
* **Parser**: Flex / Bison (for high-speed ADIF parsing)
* **Database**: SQLite (for data persistence and future scalability)

---

## Directory Structure

| Directory | Description |
| :--- | :--- |
| `adif/` | ADIF data types and field constants definitions. |
| `parser/` | Lexical and syntax parsers for ADIF files. |
| `model/` | Data models, record structures, and statistical logic. |
| `gui/` | Main interface, search, maps, settings, and entry dialogs. |
| `tools/` | Reusable table/map widgets and concurrency utilities. |
| `plugins/` | Award tracking plugins (e.g., WAS sample). |
| `tests/` | Unit testing and stress testing suites. |

---

## Build and Run

### Dependencies

* **Qt 6.7+** (Recommended)
* **CMake 3.22+**
* **C++17 Compiler** (MSVC, GCC, or Clang)

### Build Instructions

```bash
cmake -S . -B build
cmake --build build --config Release
```

### Running Tests

```bash
ctest --test-dir build --output-on-failure
```

---

## Known Limitations

* **Conflict Resolution**: A full "merging/de-duplication" workflow is not yet implemented.
* **Advanced Filtering**: Multi-condition combined filtering is still in the planning phase.
* **Visualizer**: The "Great Circle" path lines on the map are not yet functional.
* **Plugin Management**: Award rule configurations and plugin management capabilities are still undergoing iteration.
