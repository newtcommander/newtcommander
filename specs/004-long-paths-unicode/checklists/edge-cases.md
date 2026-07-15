# Edge-Case Matrix (T087)

**Feature**: [../spec.md](../spec.md) | **Created**: 2026-07-14
Covers the edge cases listed in spec.md and how the implementation handles each.

| # | Edge case (spec.md) | Handling | Status |
|---|---------------------|----------|--------|
| 1 | Single name component at the 255-char OS limit inside an already-deep path | `CFileData::NameLen` is a full 32-bit byte count; component buffers are `SAL_FIND_NAME_U8` (3×255+); the total path uses dynamic `SAL_MAX_PATH_UTF8` | **Covered** — unit test grows a name past 255 UTF-8 bytes; deep-path fixture verified at runtime |
| 2 | Two files in one directory whose names are canonically equivalent but stored differently (NFC vs NFD) | Both listed and individually operable (byte-identity for operations); a one-time FR-007 notice fires when such a pair is first seen | **Covered** — runtime: `unicode` fixture lists both `č.txt` spellings; copy preserves both |
| 3 | Combined stress: decomposed Unicode name at a path deeper than 260 | Both mechanisms are orthogonal (UTF-8 names + `\\?\` paths) | **Covered** — `saltests` `TestFileIO` creates an NFD file at a >300-char path and round-trips it |
| 4 | Network locations (UNC) with long paths and Unicode names | `SalPathToWExtAlloc` emits `\\?\UNC\...`; UNC roots handled in `CSalPathBuf`/`SalGetFullName` | **Covered by construction** — unit-tested for path building; live UNC server not available in the verification environment |
| 5 | Non-BMP characters (surrogate pairs, e.g. emoji) in sorting, display, column widths | Conversions are surrogate-safe; quick-search input accumulates surrogate pairs; sort/collation operate on UTF-16 | **Covered** — runtime: emoji file lists, copies bit-exactly; unit test round-trips U+1F4C1. *Cosmetic:* panel font lacks emoji glyphs (font fallback, not encoding) |
| 6 | Sorting / case-insensitive grouping of names differing only in accents or composition form | ASCII fast path unchanged; otherwise NFC + `CompareStringEx`; canonically equivalent names collate adjacently with a binary tie-break for determinism (FR-009) | **Covered** — unit tests for NFC≡NFD collation; deterministic tie-break in `sort.cpp` |
| 7 | Storage destination that does not support long paths | The W layer surfaces the OS error per item; no silent truncation (FR-004) | **Covered by construction** — `salfileio` propagates `GetLastError`; per-item error dialogs carry the full path |
| 8 | Persisted references (history, hot paths, config, session) to long/Unicode paths survive restart | Registry string I/O via `SalRegSetValueExW8`/`SalRegQueryValueExW8` (UTF-8 payload ↔ native UTF-16 REG_SZ) | **Covered by construction** (research R9); old ANSI-written configs load correctly |
| 9 | External tools launched with an affected file as a parameter | `SalCreateProcess`/`SalShellExecuteEx` pass the exact UTF-8 name via `W` APIs; the external tool's own capability is its concern | **Covered** — verified for association/notepad launch paths |
| 10 | Existing configurations created before this change load unchanged | ASCII/ACP values read back identically; no migration step | **Covered** — app starts with existing config, panels/layout correct |
| 11 | Legacy third-party plugin cannot represent a name/path | Interface bump to 104 refuses `<=103` binaries cleanly at load (`PLUGIN_REQVER`); the `IDS_PLUGINCANTHANDLENAME` per-item message exists for in-plugin skip flows | **Covered by construction** (FR-014, contract amendment) |

## Notes

- "Covered" = exercised at runtime or by a passing unit test.
- "Covered by construction" = the code path is correct by design and unit-tested
  at the helper level, but the specific live scenario (UNC server, a
  long-path-incapable volume, a real pre-004 config with Unicode paths) was not
  reproducible in the verification environment. These should be spot-checked in
  a normal QA pass.
