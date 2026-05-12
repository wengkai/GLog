# Award plugins

See [README.md](../README.md) (Current capabilities / Known limitations) for user-facing plugin behavior. This page documents the **C ABI**, how the application loads plugins, and the shipped WAS sample.

## Header contract

Plugins include [`include/GLogKit/award_plugin.h`](../include/GLogKit/award_plugin.h) and must **not** define `GLOGKIT_LIB` while compiling the plugin. That macro is reserved for `GLogKitCore`; the header intentionally `#error`s if `GLOGKIT_LIB` is set, preventing accidental inclusion from the core build.

Plugin translation units must not define `GLOGKIT_LIB`. The `extern "C"` entry points in `award_plugin.h` are declared with `DECL_EXPORT`, which maps to **dllexport** on Windows and default visibility on ELF, so the module **exports** the symbols the host resolves. Types such as `IGRecord` use `GLOGKIT_API`, which becomes **import** when building against the installed headers-those symbols come from `GLogKitCore`.

## Exported C symbols

The host resolves these `extern "C"` entry points by name (see [`tools/LoadedAwardPlugin.h`](../tools/LoadedAwardPlugin.h) `tryResolve`):

| Symbol | Purpose |
| -------- | --------- |
| `pluginName` | Returns a stable display name string. |
| `install` | One-time setup (e.g., download auxiliary data). |
| `uninstall` | Tear down resources created by `install`. |
| `beforeEvaluate` | Called before iterating QSOs for a report run. |
| `afterEvaluate` | Called after all `evaluate` calls; may finalize JSON/text buffers. |
| `evaluate` | Inspect one `IGRecord` via the supplied `IGRecordGetValueByField` callback. |
| `getResult` | Copy UTF-8 (or ASCII) result bytes into `result_buf`, length into `result_len` (see `IGRecord.h` contract). |
| `getLastError` | Same buffer pattern for diagnostic text. |

`IGRecord` itself is an opaque handle; field access uses [`IGRecordGetValueByField`](../include/GLogKit/IGRecord.h): **lower-case** field names, two-call sizing pattern (`result_buf == nullptr` allowed to query length), `noexcept` semantics, `IGRecord::Result` return codes (`NoError`, `Overflow`, `NotFound`, etc.).

## Host-side loading

`LoadedAwardPlugin` wraps `std::unique_ptr<QLibrary>` and resolves every symbol listed above. `valid()` requires all pointers non-null (`tools/LoadedAwardPlugin.h`).

`AwardPluginManager`:

- Keeps a `Synchronized<std::vector<LoadedAwardPlugin>>` for the table model.
- Exposes `IAwardPluginsManager::getPAwardPlugins()` returning a short-lived `IAwardPluginsProxy` that locks the vector for read (`gui/AwardPluginManager.h`).
- UI actions call `installAwardPluginAfterConfirmed`, `uninstallAwardPluginAfterConfirmed`, etc., toggling an internal `std::atomic<Status>` per plugin (`Enabled` / `Pending` / `Disabled`).

Award statistics (`AwardEntityCountReport`, `model/award_entity_count_report.cpp`) iterate snapshots of QSOs and invoke only plugins whose status is `Enabled`.

## Sample: WAS plugin

[`plugins/was_plugin`](../plugins/was_plugin) builds `WASPlugin` as a Qt `MODULE` (`plugins/was_plugin/CMakeLists.txt`):

- Links `Qt6::Core`, `Qt6::Sql`, `Qt6::Network` only-**not** `GLogKitCore`.
- Implements the C exports in `was_plugin.cpp` by forwarding to a global `WASPlugin instance`.
- `evaluate` reads `call` and `state` through the callback, optionally queries an FCC SQLite cache by callsign, and tracks worked US states.
- `install` downloads FCC ULS data and builds `fcc_amat.sqlite` under `QStandardPaths::AppDataLocation`.

When `GLOGKIT_INTERNAL_BUILD` is `ON` (default in the main project `CMakeLists.txt`), the WAS module installs into a `plugins/` destination for packaging.

## Practical notes for new plugins

1. Match the **exact** export names expected by `LoadedAwardPlugin` (no C++ name mangling on the exports).
2. Keep `evaluate` re-entrant across QSOs; use `beforeEvaluate` / `afterEvaluate` for batch state.
3. Respect buffer sizes: oversized payloads must return `IGRecord::Result::Overflow` and set `*result_len` to the required length per `IGRecord.h` documentation.
4. Prefer minimal dependencies so binary compatibility with the host's Qt runtime is easier to reason about.

## Related reading

- [architecture.md](architecture.md) - where `AwardPluginManager` sits relative to the model.
- [data-model-and-sqlite.md](data-model-and-sqlite.md) - how QSO snapshots are produced for reporting.
