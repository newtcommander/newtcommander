# Data Model: Long Path and Unicode File Name Support

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)
**Evidence**: [code-analysis.md](code-analysis.md)

This feature changes no business data; its "data model" is the
representation of file names and paths flowing through the program.

## Core representation decision (from research)

- **Internal narrow encoding: UTF-8.** All `char*` file names and
  paths inside the program carry UTF-8 instead of the active ANSI
  code page (ACP). Struct shapes stay narrow (`char*`), which keeps
  the migration incremental and the plugin struct layout familiar.
- **Boundary encoding: UTF-16.** Every OS call in the new I/O layer
  converts UTF-8 → UTF-16 and uses `W` APIs; paths ≥ ~240 chars (or
  all absolute paths, uniformly) are normalized to extended-length
  (`\\?\`, `\\?\UNC\`) form at that boundary only. `\\?\` never
  appears in the UI or in persisted data.

## Entities

### 1. `SalPathBuf` — dynamic path string (new)

Replaces fixed `char x[MAX_PATH]` buffers on migrated code paths.

| Aspect | Definition |
|--------|-----------|
| Content | UTF-8, NUL-terminated, no `\\?\` prefix (display form) |
| Capacity | Dynamic (heap; small-string optimization optional). Sanity cap `SAL_MAX_PATH_UTF8` = 3 × 32 767 + 1 bytes |
| Invariants | Valid UTF-8; backslash-separated; drive (`C:\…`) or UNC (`\\server\share\…`) absolute form for absolute paths |
| Operations | append component, add/strip backslash, strip last component, to-wide (`ToW()` → UTF-16 with `\\?\` normalization), from-wide |

Existing `Sal*` path helpers (`SalPathAppend`, `SalGetFullName`, …,
`src/salamdr3.cpp`) get length-unbounded UTF-8 semantics; their
`MAX_PATH` bounds checks change to `SAL_MAX_PATH_UTF8`/dynamic.

### 2. `CFileData` — panel/plugin item (modified, ABI-breaking)

Current: `char* Name` (ACP), 9-bit `NameLen` bitfield hard-capped at
`MAX_PATH - 5` (`src/plugins/shared/spl_com.h:205,218`).

| Field | Before | After |
|-------|--------|-------|
| `Name` | `char*` ACP | `char*` UTF-8 (exact bytes of the OS name transcoded 1:1 from UTF-16, no normalization) |
| `NameLen` | bitfield `:9` | full `unsigned` (UTF-8 byte length; max 3×255 = 765 for one component) |
| `Ext` | `char*` into `Name` | unchanged mechanics, UTF-8 |
| `DosName` | `char*` (8.3) | unchanged (8.3 names are ASCII) |
| *(new, internal to core)* | — | optional cached UTF-16 name / collation key for sort & draw (not exposed to plugins) |

Layout change ⇒ plugin interface version bump (see contracts/).

**Invariant — name fidelity**: `Name` is a lossless transcoding of
the UTF-16 name returned by directory enumeration. The program never
normalizes, case-folds, or best-fits it. NFC and NFD spellings are
distinct values (FR-006, FR-007).

### 3. Panel path state (modified)

`CFilesWindow::Path[MAX_PATH]`, `ZIPPath[MAX_PATH]`, `TargetPath`
(`src/fileswnd.h:478,489,60`) become `SalPathBuf` (or grow to the new
cap where a fixed buffer is structurally required).

### 4. Name-equivalence relation (new concept)

Used ONLY for matching and collation, never for identity or storage:

| Context | Rule |
|---------|------|
| Identity (operations, overwrite detection) | byte equality of stored name — OS semantics (FR-007) |
| Quick search, Find, wildcard masks, path input | compare `NFC(fold(input))` vs `NFC(fold(name))` — canonical-equivalence + case-insensitive match (FR-008) |
| Sort | wide-string locale collation; canonically equivalent names collate adjacently, tie-broken by binary order for determinism (FR-009) |

### 5. Equivalent-pair notice (new, transient UI state)

Trigger: a create/copy/move/rename first produces two entries in one
directory whose names are canonically equivalent but byte-different.
Payload: directory, both names (with composition form indicated).
Behavior: one-time informational dialog per operation (FR-007); never
blocks, never alters the operation result.

### 6. Plugin interface capability (modified)

| Attribute | Values |
|-----------|--------|
| Interface version | `legacy` (current released version) / `v-next` (this feature) |
| Name encoding | legacy: ACP `char*` · v-next: UTF-8 `char*` |
| Path length | legacy: `< MAX_PATH` · v-next: up to OS max |
| Core → legacy plugin adaptation | UTF-8 → ACP conversion; if lossy (any character unrepresentable) or path ≥ MAX_PATH ⇒ refuse item per FR-014 |
| Legacy plugin → core adaptation | ACP → UTF-8 (always lossless) |

All 35 bundled plugins compile against `v-next` in this feature.

### 7. Persisted path (unchanged shape, new encoding contract)

History, hot paths, configuration, session state. Stored via the
existing registry layer; string values are written/read so that any
Unicode path round-trips (research.md decides the exact mechanism —
W registry APIs or UTF-8 process code page). Existing ACP-encoded
values from older versions MUST load correctly (one-way upgrade:
read as ACP when written by old versions, write back Unicode-safe).

## State transitions — name lifecycle

```
OS (UTF-16, any composition form)
  → enumerate (W API) → transcode UTF-16→UTF-8 (lossless)
  → CFileData.Name (UTF-8, exact)
  → display: UTF-8→UTF-16 → W text-out (GDI W variants)
  → match/sort: UTF-8→UTF-16 → normalize/collate (transient copies)
  → operation: UTF-8→UTF-16 → \\?\ normalize → W file API
  → plugin v-next: UTF-8 as-is · plugin legacy: →ACP (or refuse)
  → persist: Unicode-safe registry write
```

No stage may substitute, normalize, or truncate the name; the only
lossy edge (legacy plugin/ACP) is guarded by detect-and-refuse.
