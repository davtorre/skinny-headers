# Brief 001: public timestamp function + configurable delimiters in skinny-log

## Context
Module: `skinny-log/` (`skn_log.h`). See `CLAUDE.md` at repo root for house
style conventions before touching anything.

Today `skn_log.h` formats timestamps only internally, via the static
helper `log__print_timestamp(FILE *stream)` (around line 109), which
hardcodes the brackets:
```c
fprintf(stream, "[%s] ", buf);
```
It's called from `slog_print` (SKN_LOG_TIMED / SKN_LOG_LEVEL modes) and
`slog_end_timer`. There is no way for a caller to get the timestamp
string itself, and no way to change the `[`/`]` delimiters.

Relevant files:
- `skinny-log/skn_log.h` — the header (types, public API block, impl block)
- `skinny-log/example_log.c` — the demo program that exercises the logger
  (calls `slog_init` 4 times: PLAIN, TIMED, LEVEL, LEVEL-on-stderr)
- `skinny-log/docs/log/skn_log.md` — Mermaid class diagram + function
  flowchart for this module

## Task

1. **Expose a public function that returns the timestamp string.**
   Add a field to `SknLog` to own a small internal buffer (e.g.
   `char ts_buf[64];`), and a public function:
   ```c
   const char *slog_timestamp(SknLog *log);
   ```
   It formats the current time as `YYYY-MM-DD HH:mm:ss`, wraps it in the
   logger's configured delimiters (see below), writes the result into
   `log->ts_buf`, and returns a pointer to it. Not required to be
   thread-safe or reentrant — this library is single-threaded/stb-style
   throughout, stay consistent with that.

   Refactor the existing internal timestamp printing
   (`log__print_timestamp`, used by `slog_print` and `slog_end_timer`) to
   go through `slog_timestamp()` instead of duplicating the formatting
   logic — no behavior change for existing modes, just de-duplication.

2. **Configurable delimiters, set at init.**
   Add an open/close delimiter pair to `SknLog` (e.g. `char *ts_open;
   char *ts_close;`), heap-owned copies (mirror how `slog_set_name`
   copies strings), freed in `slog_free`. Default is `"["` and `"]"`.

   Extend `slog_init` to accept them:
   ```c
   SknLog *slog_init(SknLogMode mode, FILE *stream,
                      const char *ts_open, const char *ts_close);
   ```
   Passing `NULL` for either falls back to that one's default (`"["` /
   `"]"`) — so `slog_init(SKN_LOG_LEVEL, stdout, NULL, NULL)` reproduces
   today's exact output. This is a deliberate breaking signature change;
   update every call site (see below) rather than adding a parallel
   `_ex` function.

3. **Update every call site of `slog_init`** to the new 4-arg signature:
   - `skinny-log/example_log.c` (4 call sites)
   - `skinny-log/docs/log/skn_log.md` (usage snippet)
   - the usage example in the `skn_log.h` top-of-file comment block
   - `CLAUDE.md`'s Public API line for `skn_log.h` — add `slog_timestamp()`
     to the list

4. **Have the example program use the new public function.** In
   `example_log.c`, add a case that (a) passes a custom delimiter pair to
   `slog_init` (something visibly different from `[`/`]`, e.g. `"("`/`")"`
   or `"<"`/`">"`), and (b) calls `slog_timestamp()` directly to print a
   report/summary line — e.g. a final "report generated at %s" line built
   with the returned string, rather than only relying on `slog_print`'s
   automatic prefix. This should demonstrate both new features together.

5. **Update docs.** `docs/log/skn_log.md` has two Mermaid diagrams: a
   classDiagram (data model) and a flowchart (function dependencies).
   Add `slog_timestamp()` as a public (blue) function node, wire it into
   the flowchart as what `slog_print`/`slog_end_timer` now call, and add
   the new `SknLog` fields (`ts_buf`, `ts_open`, `ts_close`) to the class
   diagram. Follow the existing color convention (blue = public, grey =
   internal, green = data types) already used in that file.

## Constraints
- Match existing code style exactly: comment density, `snake_case`
  function names, `__` internal-helper naming (e.g. any new static
  helper follows `log__...`), header block comment conventions at the
  top of `skn_log.h`.
- No external dependencies — stdlib/POSIX only, same as today.
- Don't touch `skn_bar.h` — it has its own duplicated timestamp logic
  (`bar__print_timestamp`) but that's out of scope for this brief.
- Keep `slog_free` correctly freeing the two new heap fields (no leaks) —
  check with the project's normal build/run of `example_log.c` (there's
  no test suite here beyond the example programs; running them and
  eyeballing/valgrind-checking output is the bar).
- Build and run `example_log.c` yourself before calling this done; paste
  a snippet of its output in your deliverable report so the timestamp +
  custom delimiter are visibly correct.

## Deliverable
A commit (or diff, if you don't have push access) covering:
- `skinny-log/skn_log.h`
- `skinny-log/example_log.c`
- `skinny-log/docs/log/skn_log.md`
- `CLAUDE.md`

Report back to the project manager when done, or sooner if you hit a
blocker or an ambiguity in this brief (e.g. if the exact `SknLog` field
layout conflicts with something else in flight). Send that message
however you're able to reach the PM in this environment (chat/handoff/
whatever channel you were dispatched through) — the PM logs every
exchange, in both directions, in `agent_comms.csv` at the repo root, so
just say clearly which brief you're reporting against (`001-log-timestamp-delimiter.md`).
