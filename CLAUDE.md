# skinny-headers — project notes for Claude

## Repo
GitHub: https://github.com/davidetorre92/skinny-headers

## Style conventions
- Single header-only `.h` files, stb-style (`#define SKN_<NAME>_IMPLEMENTATION`)
- File naming: `skn_<name>.h`
- No external dependencies — only standard C headers
- Internal (static) functions use double-underscore prefix: `name__helper()`
- Each module lives in its own directory with its header, example source, and example data

## Modules

### skinny-headers/ — .dat file parser (`skn_dat.h`)
- Format: `key value` per line, comments with `#`
- Value types: `int`, `float` (suffix `f`), `double`, `string` (quoted), homogeneous lists `[...]`
- Type inference: bare integer = int, decimal = double, decimal + `f` = float
- Structs: `DatDocument` (linked list of `DatRow`) → `DatRow` (key + `DatValue`) → `DatValue` (tagged union) → `DatList` + `DatListNode`
- Public API: `dat_load()`, `dat_parse_string()`, `dat_get()`, `dat_free()`
- Docs: `docs/dat/skn_dat.md` (Mermaid class diagram + function dependency flowchart)

### skinny-csv/ — CSV parser (`skn_csv.h`)
- Format: RFC-4180 CSV with quoted fields and `""` escaping; UTF-8 BOM stripped
- Type inference per column: `int`, `float` (suffix `f`), `double`, `string`
- Column types take the most general type found across all rows
- Supports explicit schemas via `CsvSchema[]` for mixed-inference files
- Structs: `CsvDocument` → `CsvColData` (typed union arrays) → `DatArray` (view returned by csv_get)
- Public API: `csv_load()`, `csv_load_schema()`, `csv_parse_string()`, `csv_parse_string_schema()`, `csv_get()`, `csv_get_schema()`, `csv_print_schema()`, `csv_free()`

### skinny-log/ — logger and progress bars

#### `skn_log.h` — logger
- Three modes: `SKN_LOG_PLAIN` (message only), `SKN_LOG_TIMED` (`[YYYY-MM-DD HH:mm:ss]` prefix), `SKN_LOG_LEVEL` (timestamp + INFO/WARNING/ERROR label)
- Stream can be `stdout`, `stderr`, or any open `FILE*`
- Timer: `slog_start_timer` / `slog_end_timer` using `clock_gettime(CLOCK_MONOTONIC)` (POSIX only)
- `slog_end_timer` accepts a message string; `%t` is replaced with elapsed time, unit auto-selected (µs → ms → s → min → h → days); `%%` outputs a literal `%`
- Public API: `slog_init()`, `slog_free()`, `slog_print()`, `slog_start_timer()`, `slog_end_timer()`, `slog_timestamp()`

#### `skn_bar.h` — progress bars
- `SknBar`: single in-place bar rendered with `\r`; format `[▰▰▰...▱▱▱] 62%  62/100`
- `SknBarSet`: N bars managed with ANSI cursor codes; supports same modes and timer as `skn_log.h`
- `SknLogMode` / `SknLogLevel` defined here when `skn_log.h` is not already included; safe to use both headers together
- All output while a bar set is active must go through `sbars_log()` / `sbars_end_timer()` — direct writes to the same stream garble the display
- Intended pattern: bar set on `stdout`, logger on a file `FILE*` — fully independent, no coordination needed
- `SknBar` public API: `sbar_init()`, `sbar_free()`, `sbar_set_chars()`, `sbar_update()`, `sbar_step()`, `sbar_finish()`
- `SknBarSet` public API: `sbars_init()`, `sbars_free()`, `sbars_set_mode()`, `sbars_config()`, `sbars_set_chars()`, `sbars_start()`, `sbars_update()`, `sbars_step()`, `sbars_start_timer()`, `sbars_log()`, `sbars_end_timer()`, `sbars_finish()`

### skinny-list/ — non-owning tagged linked list ("list viewer") (`skn_list.h`)
- A lightweight, non-owning linked list of tagged nodes: each node holds a type tag and a `void*` pointing at an external, caller-owned variable (not copied). Build a row of nodes once, mutate the referenced variables in a loop, and reprint without rebuilding the list — for incremental/streaming tabular output.
- Nodes never own their data: `lstv_free_list` only frees the nodes, never the pointed-to variables.
- Supported types: `LSTV_INT`, `LSTV_FLOAT`, `LSTV_DOUBLE`, `LSTV_CHAR` (string)
- Struct: `SknListNode` (type tag + `void*` data + `next`); used as a raw `SknListNode *head` pointer, no separate container struct
- `lstv_create_node` returns `NULL` on allocation failure; `lstv_push_front` treats a `NULL` node as a safe no-op (returns `-1`, list unchanged)
- Distinct from `skinny-reader/skn_dat.h`'s `DatList`/`DatListNode`, which hold parsed, owned list *values* inside a `.dat` document — no overlap
- Public API: `lstv_create_node()`, `lstv_push_front()`, `lstv_free_list()`, `lstv_print_list()`
- Docs: `docs/list/skn_list.md` (Mermaid class diagram + function dependency flowchart)

## Docs convention
- Each library gets a `docs/<name>/skn_<name>.md`
- Mermaid diagrams: blue = public functions, grey = internal functions, green = data types
- Two diagrams per library: data model (classDiagram) + function dependencies (flowchart)
