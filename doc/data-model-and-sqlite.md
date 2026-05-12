# Data model and SQLite

See [README.md](../README.md) for the product-level description of import, merge, and export. This page summarizes persistence: schema, DAO, repository, and the services that sit above them.

## Schema (authoritative source)

Table definitions live in `GRecordDao::initSchema` in [`model/recorddao.cpp`](../model/recorddao.cpp).

| Table | Purpose |
| ------- | --------- |
| `files` | One row per logical ADIF file path: `id`, `filename` (unique), `last_sync` (unix seconds). |
| `records` | One row per QSO row bound to a file: `id`, `file_id` (FK → `files`), `internal_order` (ordering within the file). |
| `record_fields` | Sparse EAV storage: `(record_id, field_key)` primary key, `field_value` text, FK → `records` with cascade delete. |

Indexes:

- `idx_records_file_order` on `(file_id, internal_order)`.
- `idx_record_fields_key` on `field_key`.

There is no separate migration framework: schema is ensured idempotently with `CREATE TABLE IF NOT EXISTS` / `CREATE INDEX IF NOT EXISTS` on init.

## Layering

| Component | File(s) | Role |
| ----------- | --------- | ------ |
| `GRecordDao` | `model/recorddao.{h,cpp}` | Synchronous SQL: init schema, file CRUD, upsert/delete records and fields, reorder sync. Throws `std::runtime_error` (or `std::invalid_argument` for invalid ids) on failure. |
| `SqliteDbExecutor` | `tools/SqliteDbExecutor.{h,cpp}` | Connection wrapper and transaction helper used by the DAO. |
| `GRecordRepository` | `model/recordrepository.{h,cpp}` | Async façade: each public method returns `QFuture` completed on a worker thread after copying inputs as needed. |
| `AdifFileService` | `model/adiffile_service.{h,cpp}` | Orchestrates open/merge/string insert: parse via `AdifParseService`, optional backup through `GRecordRepository`, then applies results to `AdifModel` on the GUI thread. Tracks `activePersistedFileId()` for the current SQLite `files.id`. |
| `AdifParseService` | `model/adifparse_service.{h,cpp}` | Stateless ADIF text → `GRecord` (+ errors); no Qt model dependency. |
| `AdifModel` | `model/adifdb.{h,cpp}` (and related) | In-memory table model; receives `AdifFileService` and optional `GRecordRepository` via setters used from the GUI. |

When you need to change how rows are stored or queried, start at `GRecordDao` and propagate outward: repository methods should remain thin wrappers that preserve threading contracts.

## Backup enablement

`GLogApplication::enableBackup` constructs a `GRecordRepository` with path `<AppDataLocation>/glog_records.sqlite`, calls `initSchemaAsync`, then attaches it to `AdifModel::setDbBackup` on success (`gui/GLogApplication.cpp`).

If schema init fails, the error is logged; the UI can still operate without backup depending on model behavior.

## Import and persistence interaction

`AdifFileService::openFile` (sync body used by async wrappers):

1. Opens a `std::ifstream` to the selected file.
2. Parses with `AdifParseService::parse(..., ParseMode::Batch)`.
3. If `AdifModel::getDbBackup()` is set, resolves a canonical path string, finds or creates `files.id`, loads existing rows when merging, and issues `upsertRecordAsync` / ordering calls as appropriate (read the implementation for merge vs replace semantics).

Parsing errors surface as `std::runtime_error` messages propagated to callers; the async entry points wrap these for UI feedback.

## Serialization

ADIF/CSV export and in-memory serialization helpers live under `model/adif_serialization.*` and related types; they reuse `GRecord` field maps rather than bypassing the DAO.

## Related reading

- [architecture.md](architecture.md) - import threading overview.
- [parser-and-adif.md](parser-and-adif.md) - how `AdifParseService` fills `GRecord` vectors.
