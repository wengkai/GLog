# GLogKit developer documentation

This directory contains **maintainer- and contributor-oriented** material: architecture, parse pipeline, persistence, award plugins, and contribution workflow.

For end-user goals, feature lists, install/run, CI summaries, and `find_package(GLogKit)`, see the repository [README.md](../README.md).

## How to read these docs

| If you are… | Start here |
| ------------- | ------------ |
| New to the codebase | [architecture.md](architecture.md), then [data-model-and-sqlite.md](data-model-and-sqlite.md) |
| Changing ADIF parsing or lexer/parser | [parser-and-adif.md](parser-and-adif.md) |
| Working on SQLite backup, files, or repositories | [data-model-and-sqlite.md](data-model-and-sqlite.md) |
| Building or shipping an award plugin | [plugins.md](plugins.md) |
| Opening a PR or fixing CI | [contributing.md](contributing.md) |

## Index

- [architecture.md](architecture.md) - Layers, main targets, and module dependencies.
- [parser-and-adif.md](parser-and-adif.md) - Flex/Bison generation, drivers, `AdifParseService`, and Python-generated constants.
- [data-model-and-sqlite.md](data-model-and-sqlite.md) - `GRecordDao`, `GRecordRepository`, file/import services, schema overview.
- [plugins.md](plugins.md) - Award plugin C ABI, `QLibrary` resolution, sample WAS plugin.
- [contributing.md](contributing.md) - Tests, formatting, GitHub Actions, common build issues.
