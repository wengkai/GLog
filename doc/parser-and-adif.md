# Parser and ADIF pipeline

See [README.md](../README.md) (Technical stack) for Flex/Bison and Qt versions. This page describes how ADIF text becomes in-memory `GRecord` rows and how generated artifacts are produced.

## Overview

| Stage | Location | Responsibility |
| ------- | ---------- | ------------------ |
| Lex + yacc | `parser/glog.l`, `parser/glog.y` | Tokenize ADIF and build records as vectors of `(field_name, value)` pairs. |
| Driver | `parser/driver_impl.hpp`, `parser/driver.hpp` | Bridge lexer/parser to storage (`AddRec`) and error collection. |
| Parse facade | `model/adifparse_service.{h,cpp}` | Choose batch vs interactive lexer, run `GLOG_PARSER::Parser`, convert raw rows to `GRecord`. |
| Typed fields | `adif/` | Strongly typed ADIF values, validation, and enum maps used by the model and serialization. |

The umbrella include `parser/glogparser.h` pulls generated headers (`parser.h` from Bison), both lexer variants, and driver headers so callers (typically through `adifparse_service.cpp`) see one include.

## CMake-generated sources

[`parser/CMakeLists.txt`](../parser/CMakeLists.txt):

1. **Bison** - `glog.y` → `${CMAKE_BINARY_DIR}/parser.cpp` and `parser.h` (`BISON_TARGET(MyParser ...)`).
2. **Flex (batch)** - `glog.l` → `driver_lex.cpp` with `--batch` (and on Windows `--wincompat --nounistd`).
3. **Flex (interactive)** - same `glog.l` → `driver_lex_interactive.cpp` with `--interactive --prefix=xx --yyclass=xxFlexLexer` to avoid symbol collisions with the batch lexer.

`ADD_FLEX_BISON_DEPENDENCY` ties lexer regeneration to the parser.

**When you need a clean reconfigure:** editing `glog.y` or `glog.l` triggers regeneration on the next CMake build. If generated files look stale, delete the build tree or run CMake again so Bison/Flex targets rerun.

**Windows:** if `FlexLexer.h` is not found, set `FLEX_INCLUDE_DIRS` to the directory containing it (WinFlexBison layout); the parser `CMakeLists.txt` attempts a Chocolatey default and emits a warning if still missing.

## Driver modes

- **`LexerBatch` + `DriverUnsynchronized`** - used by `AdifParseService::parse(..., ParseMode::Batch)` for full-file reads (`adiffile_service.cpp`).
- **`LexerInteractive` + `DriverUnsynchronized`** - used for `ParseMode::Interactive` (streaming or interactive scenarios).

`ParserDriverImpl` (in `driver_impl.hpp`) templatizes storage wrapping:

- `DriverUnsynchronized` uses `Unsynchronized` for `data` and `errors`.
- `DriverSynchronized` uses `Synchronized` (available for other call sites that need locked access).

`ParserDriver::AddError` counts lexer/parser diagnostics and can abort after `MAX_ERROR_COUNT` (see `parser/driver.hpp`).

## From raw pairs to `GRecord`

`AdifParseService::parse`:

1. Instantiates the appropriate lexer inside `DriverUnsynchronized`.
2. Runs `GLOG_PARSER::Parser` with the driver; `parser.parse() == 0` means success.
3. On success, swaps `errors` from the driver and calls `fromRawData`, which walks each row, uses `GRecord::addOrSetPairAndNormalizeKey`, and collects header field names in an `unordered_set`.

Invalid or empty rows are skipped; see `model/adifparse_service.cpp`.

## `adif/` types and generated constants

The `adif` static library compiles C++ sources under `adif/` and exposes headers from `adif/constants/` (see `adif/CMakeLists.txt` `target_include_directories`).

Many `*_map.h` headers under `adif/constants/` are produced by Python scripts:

- [`adif/constants/run_pipeline.py`](../adif/constants/run_pipeline.py) discovers `parse_*.py` modules and runs their `generate_<name>_map` functions, then refreshes the aggregate header logic defined in that file.
- [`adif/constants/gen_field_map.py`](../adif/constants/gen_field_map.py) maps ADIF field definitions to C++ helper types (scrapes the ADIF specification HTML via `adif_config.py`).

**Workflow when changing enum maps:**

1. Edit or add the relevant `parse_*.py` (or adjust `adif_config.py` / soup selectors if the upstream HTML changed).
2. From `adif/constants/`, run `python run_pipeline.py` (use the same Python environment where dependencies for `adif_config` are installed).
3. Rebuild C++ targets so compilers pick up updated headers.

Commit regenerated headers together with script changes so CI and other developers stay in sync.

## Related reading

- [architecture.md](architecture.md) - where `AdifFileService` sits in the UI stack.
- [data-model-and-sqlite.md](data-model-and-sqlite.md) - persistence after parse.
