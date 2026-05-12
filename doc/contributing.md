# Contributing

See [README.md](../README.md) for dependency versions, basic configure/build/test commands, and CI artifact overview. This page focuses on workflow expectations, hygiene, and troubleshooting.

## Branching and CI

GitHub Actions workflow: [`.github/workflows/cmake-multi-platform.yml`](../.github/workflows/cmake-multi-platform.yml).

- **Triggers:** pushes and pull requests to `main`; tags matching `*` (including release tags `v*`).
- **Linux:** container `ghcr.io/llxx2013/qt-builder:6.11.0`, Ninja Release build, `QT_QPA_PLATFORM=offscreen ctest`.
- **Windows:** Chocolatey `winflexbison3` and `nsis`, Qt **6.10.3** MSVC 2022 x64, CMake with `-DFLEX_INCLUDE_DIRS="C:\ProgramData\chocolatey\lib\winflexbison3\tools"`, `ctest -C Release`.
- **macOS:** Homebrew Flex/Bison, Qt **6.10.3**, checkout uses `submodules: recursive` on that job.

Environment variable `ADIF_VOLUME_STRESS` is set to `"1"` in CI to enable heavier tests when supported.

**Expectation:** keep `main` green; run the closest local matrix entry (Release + tests) before opening a PR.

## Building and testing locally

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Multi-config generators (Visual Studio, Xcode) need `-C Release` (or your chosen config) on both `cmake --build` and `ctest`.

`BUILD_TESTING` (CMake option, default **ON**) gates test targets. Disable only when you intentionally skip tests: `-DBUILD_TESTING=OFF`.

### Linux headless runs

Match CI when running without a display:

```bash
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure
```

## Code style

- **Formatting:** `.clang-format` at the repository root-run `clang-format` on touched files before review.
- **Tests:** `tests/.clang-tidy` applies to test sources; keep new tests consistent with existing patterns (`QtTest`, `GLogConcurrent`, etc.).

## Parser generator pitfalls (Windows)

If MSVC fails with missing `FlexLexer.h`, pass the WinFlexBison include directory to CMake:

```text
-DFLEX_INCLUDE_DIRS="C:\ProgramData\chocolatey\lib\winflexbison3\tools"
```

The parser `CMakeLists.txt` attempts to auto-detect this path but may still warn `FLEX_INCLUDE_DIRS:NOTFOUND` if tools live elsewhere-set the variable explicitly.

## macOS note

The workflow installs Flex via Homebrew and prepends its `bin` to `PATH` so CMake finds a modern Flex. If you build locally without that, point CMake to your Flex/Bison similarly.

## Optional coverage targets

On Windows with MSVC, optional `coverage` / `llvm-coverage` custom targets may appear when OpenCppCoverage or LLVM coverage tools are detected-see root `CMakeLists.txt` and `scripts/` (also summarized in the root README).

## Documentation

Developer-focused Markdown lives under [`doc/`](README.md). User-facing summaries stay in the root `README.md`; avoid duplicating long sections-link between them.

## Related reading

- [architecture.md](architecture.md) - module map for navigating unfamiliar code.
- [parser-and-adif.md](parser-and-adif.md) - regenerating lexer/parser outputs.
