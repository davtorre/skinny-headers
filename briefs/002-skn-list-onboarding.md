# Brief 002: onboard skn_list.c as a proper skinny-headers module

## Context
There's an untracked prototype at the repo root: `skn_list.c`. Read it in
full before starting — it's short. The PM (this session) reviewed it and
concluded the *idea* is good and worth keeping, but the file doesn't
follow any of this repo's conventions yet. See `CLAUDE.md` at the repo
root for the house style — read it fully, this brief only calls out the
specific deltas.

**What the prototype does, conceptually:** a lightweight, *non-owning*
tagged linked list. Each node stores a type tag + a `void *` pointing to
an external variable (not a copy) — `int`/`float`/`double`/`char*`
(string). You build the row of nodes once, then in a loop mutate the
*referenced* variables and reprint the row — no rebuilding the list per
iteration. Good for incremental/streaming tabular output (think: a live
log row whose fields are pointers into your working variables).

This is a distinct concept from `skinny-reader/skn_dat.h`'s
`DatList`/`DatListNode` (those hold parsed, owned list *values* inside a
`.dat` document) — no overlap, no rename collision, already checked.

**Naming decision (already made, don't relitigate):** public API prefix
is `lstv_` (for "list viewer" — the intended use is repeatedly *viewing*
current values of a live row, not general list manipulation). New module
directory: `skinny-list/`. Header file: `skn_list.h` (file name doesn't
need to match the prefix — precedent: `skn_log.h` uses prefix `slog_`,
`skn_bar.h` uses prefix `sbar_`, both in `skinny-log/`).

## Task

1. **Convert to header-only stb-style.** New file
   `skinny-list/skn_list.h`, guarded the same way every other module is:
   `#ifndef SKN_LIST_H` / `#define SKN_LIST_H` ... `#endif`, with the
   implementation behind `#ifdef SKN_LIST_IMPLEMENTATION` /
   `#define SKN_LIST_IMPLEMENTATION` before including, exactly like
   `skn_log.h` / `skn_dat.h`. Top-of-file comment block in the same style
   as those two (MODES-less here, but same MODULE / USAGE / EXAMPLE
   shape as `skn_log.h`'s header comment).

2. **Rename for convention:**
   - `skn_list` struct → `SknListNode` (PascalCase, matches `SknLog`,
     `DatDocument`, etc.). Keep it as a single node struct — the
     existing usage pattern is a raw `SknListNode *head` pointer, no
     separate container struct; don't invent one, that's out of scope.
   - The `enum { SKN_INT, SKN_FLOAT, SKN_DOUBLE, SKN_CHAR }` type tag:
     give it a name, e.g. `typedef enum { LSTV_INT, LSTV_FLOAT,
     LSTV_DOUBLE, LSTV_CHAR } SknListType;` (checked: `SKN_INT` etc.
     don't collide with anything else in the repo today, but they're
     also not namespaced to this module — fix that while renaming).
   - Public functions get the `lstv_` prefix:
     `create_node`→`lstv_create_node`, `push_front`→`lstv_push_front`,
     `free_list`→`lstv_free_list`, `print_list`→`lstv_print_list`.
   - `print_data` is only ever called internally (by `print_list`) in
     the original file — make it a `static` internal helper following
     the `name__helper()` convention (see `skn_log.h`'s `log__...`
     helpers, prefix internal helpers with the *short* module word, not
     the public prefix): `list__print_data`.

3. **Fix the malloc-failure bug.** In the original, `create_node`
   correctly returns `NULL` on allocation failure, but every call site
   (`push_front(&row, create_node(...))`) dereferences the result
   without checking — NULL deref on OOM. Fix this properly: either have
   `lstv_push_front` reject a NULL node safely (no-op + return an error
   code), or change the demo/call pattern so allocation failure is
   checked before pushing. Pick whichever fits the existing function
   signatures best; document the choice in the function's doc comment.

4. **English comments only.** The original has Italian comments (`"solo
   indirizzo della variabile"`, `"liberi solo il nodo, NON i dati"`,
   `"trattato come stringa"`, `"costruiamo la Skinny List"`). Translate
   all of them, and make sure every public function gets a doc comment
   in the same style as `skn_log.h`/`skn_dat.h`'s public API block
   (purpose, ownership semantics — call out explicitly that nodes do
   **not** own the data they point to, `lstv_free_list` only frees
   nodes).

5. **Example program.** `skinny-list/example_list.c` — port the
   prototype's `main()` (the "build row once, mutate + reprint in a
   loop, write to a file" pattern) to the new API. Keep it demonstrating
   the core value: incremental output without rebuilding the list.

6. **Docs.** `skinny-list/docs/list/skn_list.md` — same shape as the
   other modules' docs: a Mermaid classDiagram (data model: `SknListNode`
   fields, `SknListType`) and a flowchart (function dependencies),
   blue/grey/green convention (public/internal/data types) as established
   in `docs/log/skn_log.md` and `docs/dat/skn_dat.md`.

7. **CLAUDE.md.** Add a new module entry under a `skinny-list/` heading,
   matching the shape of the existing `skinny-log/` entry (short
   description, structs, public API list).

## Constraints
- No external dependencies — stdlib only, same as the prototype already
  is.
- Don't touch `skinny-reader/skn_dat.h`'s `DatList`/`DatListNode` — confirmed
  no overlap, this is a separate module.
- Build and run `example_list.c`; check the fix in point 3 actually
  triggers correctly under simulated allocation failure if that's
  practical, otherwise reason through it and note in your report why
  you're confident it's correct.
- Match existing code style: comment density, brace style, naming
  conventions as used across `skn_log.h`/`skn_dat.h`/`skn_csv.h`.

## Deliverable
A commit (or diff) covering:
- `skinny-list/skn_list.h` (new)
- `skinny-list/example_list.c` (new)
- `skinny-list/docs/list/skn_list.md` (new)
- `CLAUDE.md` (new module section)
- removal of the root-level `skn_list.c` prototype (superseded)

Report back to the project manager when done, or sooner if you hit a
blocker or a design question not settled above (e.g. if `lstv_push_front`'s
NULL-handling signature turns out to need a call-site change you're
unsure about). The PM logs every exchange, in both directions, in
`agent_comms.csv` at the repo root — say clearly which brief you're
reporting against (`002-skn-list-onboarding.md`).
