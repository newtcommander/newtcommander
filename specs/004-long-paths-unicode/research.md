# Research: Long Path and Unicode File Name Support

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)
**Code evidence**: [code-analysis.md](code-analysis.md)

Phase 0 output. Each item: Decision / Rationale / Alternatives
considered. Platform facts verified against Microsoft documentation
(July 2026).

---

## R1. Internal string representation: UTF-8 in existing `char*` plumbing

**Decision**: Keep all in-program file names and paths as narrow
`char*` strings, but change their encoding contract from "active ANSI
code page (ACP)" to **UTF-8**. Convert to UTF-16 only at OS
boundaries (file APIs, GDI text-out, window text, registry) through a
small, central conversion layer.

**Rationale**:
- The codebase has ~4,900 `MAX_PATH` sites and thousands of `char*`
  string operations (`strcpy`, `sprintf`, byte-indexed parsing). A
  UTF-8 contract keeps all of that code *mechanically valid* (UTF-8
  is NUL-terminated, backslash bytes never appear inside multibyte
  sequences), so the migration concentrates on OS boundary calls
  instead of every string manipulation.
- `CFileData::Name` and the whole plugin structure family stay
  `char*` — the v-next plugin interface is a semantic + limits bump,
  not a wholesale re-typing (see R8, contracts/).
- UTF-8 encodes any Unicode name losslessly, including decomposed
  (NFD) sequences and non-BMP characters — the two reported defects.

**Alternatives considered**:
- *Full UTF-16 conversion (`UNICODE` build, `WCHAR*` everywhere)*:
  the "textbook" fix, but a big-bang re-typing of the entire core +
  plugin SDK + all 35 plugins in one stroke; contradicts the
  constitution's incremental modernization and multiplies regression
  risk (every `sprintf`/`strlen`/byte-parse touched).
- *Dual pipelines (keep ANSI, add parallel wide path)*: preserves the
  legacy path but doubles every code path and leaves the ANSI trunk
  as a permanent data-corruption hazard; unbounded scope.

## R2. Long paths: W APIs + extended-length normalization, no registry dependency

**Decision**: All file-system access in the core goes through a new
I/O wrapper layer that converts UTF-8 → UTF-16 and normalizes
absolute paths to extended-length form (`\\?\C:\…`, `\\?\UNC\…`)
before calling `W` APIs (`CreateFileW`, `FindFirstFileW`,
`MoveFileExW`, …). The `\\?\` prefix is applied uniformly at the
boundary and never surfaces in UI or persisted data. The application
manifest additionally declares `longPathAware=true` as
defense-in-depth for any stray un-migrated call.

**Rationale** (verified): the `longPathAware` manifest lifts
`MAX_PATH` only when the system-wide registry value
`HKLM\SYSTEM\CurrentControlSet\Control\FileSystem\LongPathsEnabled=1`
is also set — which is **not** guaranteed on user machines and is not
default-on. `-A` functions never reliably support long paths. The
`\\?\` + `W` route works on every supported Windows regardless of
registry state. Sources:
[Maximum Path Length Limitation](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation),
[LongPathAware discussion](https://forum.linqpad.net/discussion/3404/longpathaware-manifest).

**Alternatives considered**:
- *Rely on `longPathAware` + registry opt-in*: rejected — silently
  broken on machines without the registry value; A-variant thunks
  stay capped anyway.
- *Keep the existing 8.3 tmp-rename workaround* (`safefile.cpp:152`):
  rejected — fragile, 8.3 generation is often disabled on modern
  volumes; superseded and removed by this feature.

**Consequences**: `\\?\` paths disable automatic normalization —
the wrapper must pre-normalize (`GetFullPathNameW` on the un-prefixed
form, strip trailing dots/spaces handling stays explicit) before
prefixing. Relative paths and drive-current-directory forms are
resolved before prefixing.

## R3. Process code page: leave the system ACP alone

**Decision**: Do **not** set `activeCodePage=UTF-8` in the manifest.
The process ACP remains the system ACP. The core stops depending on
ambient ACP semantics entirely: every core call site that passes
names to an `-A` API is migrated to the W wrapper layer within this
feature.

**Rationale**:
- Legacy **third-party plugins** run in-process and call `-A` file
  APIs with ACP strings they derived themselves. Flipping the process
  code page to UTF-8 would silently change what those APIs do with
  their bytes — breaking names that work today and violating FR-015
  (no regressions for legacy plugins). Keeping the ACP intact
  preserves their current capability exactly, with the core's
  detect-and-refuse shim (FR-014) guarding the new name space.
- GDI does **not** honor a per-process UTF-8 code page (verified:
  [Use UTF-8 code pages in Windows apps](https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page)),
  so the "free" rendering benefit of the manifest route doesn't exist
  anyway — drawing must move to W calls regardless (R9).

**Alternatives considered**:
- *`activeCodePage=UTF-8` manifest*: attractive on paper (residual
  `-A` calls become UTF-8-correct), rejected for the third-party
  plugin regression risk and the GDI gap above.

## R4. Canonical-equivalence matching: normalize to NFC at compare time

**Decision**: Introduce two helpers in the shared string utilities:
- `SalNormalizeNFC(utf16)` — wraps `NormalizeString(NormalizationC)`.
- Equivalence/matching compare: normalize both sides to NFC, then
  `CompareStringEx`/`LCMapStringEx` with linguistic casing for
  case-insensitive matching (quick search, Find, masks — FR-008).
Stored names are never modified (FR-006); normalization output is
transient. An ASCII fast path (both strings pure ASCII ⇒ current
byte-wise logic) keeps the common case at today's speed.

**Rationale**: `NormalizeString` is the supported canonical
normalization API (supersedes the `FoldString(MAP_PRECOMPOSED)`
experiment abandoned in `strutils.cpp:324`); `CompareString*` alone
does not guarantee canonical-equivalence handling, so explicit NFC
before comparison is the deterministic contract the spec requires.

**Alternatives considered**:
- *`FoldString(MAP_PRECOMPOSED)`*: legacy pre-Vista API, incomplete
  (fails on ligatures/newer compositions); the codebase's own dead
  experiment documents its dead end.
- *Normalize names once at directory-read time and cache NFC form for
  every item*: rejected as default (doubles per-item memory for a
  rare need); instead lazily computed where matching demands it.

## R5. Sorting: wide collation with cached fast path

**Decision**: `CmpNameExt`/`RegSetStrICmp*` (`sort.cpp`) gain
UTF-8-aware variants: pure-ASCII pairs use the existing byte-wise
comparators (unchanged speed); otherwise compare via UTF-8→UTF-16
conversion and `CompareStringEx(LOCALE_NAME_USER_DEFAULT, …)`;
canonically equivalent names tie-break by binary (byte) order so the
result is deterministic and equivalent forms collate adjacently
(FR-009). If the SC-009 benchmark (±10% on 100k items) fails with
on-the-fly conversion, add a per-item cached UTF-16 name or
`LCMapStringEx` sort key generated during directory read (memory
trade-off documented in data-model.md).

**Rationale**: >90% of real-world directory entries are ASCII-only;
the fast path preserves today's sort cost. The fallback cache is a
contained, measurable optimization — decided by benchmark, not
guesswork.

## R6. Display: W text APIs at draw/measure sites

**Decision**: Panel item painting and text measurement switch to
`ExtTextOutW`/`TextOutW`/`GetTextExtentPoint32W`/`DrawTextW` fed by
UTF-8→UTF-16 conversion (per-draw transient buffer or per-item cache
per R5). Dialog/control text carrying names uses W getters/setters
(`GetWindowTextW`/`SetWindowTextW` and dialog-item equivalents) —
USER32 windows are internally Unicode, so W calls work even on
windows created through A entry points.

**Rationale**: GDI ignores per-process UTF-8 (R3), so W calls are the
only way to render non-ACP and decomposed names correctly; USER32's
internal Unicode storage makes per-call W access a *local* change,
not a window-architecture change.

## R7. Keyboard input: Unicode window classes for typed-name paths

**Decision**: The panel window class (quick search) and any window
that consumes typed file-name characters via `WM_CHAR` is registered
with `RegisterClassW` so `WM_CHAR` delivers UTF-16 code units;
surrogate pairs are accumulated before matching. Edit-control input
(rename, path fields, Find masks) is read via W text APIs (R6), which
already yields full Unicode regardless of the control's creation
path.

**Rationale**: An ANSI window procedure receives `WM_CHAR` squeezed
through the ACP — decomposed input and non-ACP characters are
destroyed before the app sees them (quick search even hard-gates
`wParam < 256`, `fileswn0.cpp:894`). Flipping the *class* to W changes
only message decoding at the affected window procedures (the window
procedure signature is identical), a contained change. The shared
window framework already contains a dormant, compiled-but-unused W
path (`RegisterClassW`/`CWindowProcW`/`CreateExW`,
`src/common/winlib.cpp:499-521,181-232`) — the migration activates
existing scaffolding rather than building new plumbing.

**Alternatives considered**: handling `WM_UNICHAR`/`WM_IME_CHAR` in
ANSI classes — partial coverage, more special cases than switching
the class.

## R8. Plugin interface: version bump + load-time adaptation shim

**Decision**: Bump the plugin interface version. `v-next` plugins
receive/return UTF-8 names and long paths (same `char*` structure
shapes; widened `NameLen`; new limits documented in
contracts/plugin-interface-vnext.md). At plugin load, the core reads
the plugin's built-against version:
- `v-next` → direct pass-through (UTF-8).
- legacy → adaptation shim: core→plugin strings converted UTF-8→ACP
  with a lossless check; any lossy conversion or path ≥ `MAX_PATH`
  refuses that item with the FR-014 per-item message.
  plugin→core strings converted ACP→UTF-8 (always lossless).
All 35 bundled plugins are ported to `v-next` in this feature; the
legacy shim exists for third-party binaries.

**Rationale**: The existing SDK already version-gates plugins at load
(interface version constants in `spl_base.h`; exact mechanics cited
in contracts/), so a version bump rides the established compatibility
mechanism — constitution II's "binary compatibility or documented
migration path" is satisfied: legacy binaries keep working at current
capability; the migration path for third parties is recompile against
the v-next SDK.

## R9. Configuration persistence: switch registry string I/O to W

**Decision**: The configuration layer reads/writes `REG_SZ` values
via W registry APIs, converting UTF-8 ↔ UTF-16 at that boundary. All
string config flows through a single façade (`SetValueAux`/
`GetValueAux` in `src/regwork.cpp:163-249`), so the switch is one
chokepoint, not a sweep.

**Rationale**: `REG_SZ` is stored natively as UTF-16 by the OS.
Values written by *old* versions went through `-A` APIs, so the OS
already converted them ACP→UTF-16 at write time — reading them back
with W APIs yields the correct Unicode string with no migration
logic. Old versions reading configs written by the new version will
see best-fit ACP conversions of the UTF-16 store — acceptable
(downgrade is not a supported path). FR-010 round-trip achieved
structurally.

**Alternatives considered**: keep `-A` registry I/O with UTF-8 bytes
in `REG_SZ` — rejected: corrupts the registry's native UTF-16 store
with double-encoded text and breaks every external registry viewer.

## R10. Where `MAX_PATH` remains valid

**Decision**: `MAX_PATH` stays for: single name components (255-char
OS component limit — spec assumption), 8.3 `DosName`, drive-root
strings, and non-path uses. Everything holding a *full path* migrates
to `SalPathBuf`/`SAL_MAX_PATH_UTF8` (data-model.md §1). The migration
inventory (which of the ~4,900 sites are full-path buffers) is
produced mechanically during implementation via annotated sweeps per
subsystem, sequenced in tasks.md.

**Rationale**: A blanket constant swap would balloon stack frames
(3×32 767-byte worst case) and hide real semantic differences;
per-site classification with a dynamic buffer type is safer and
reviewable subsystem by subsystem.

---

## Sources

- [Maximum Path Length Limitation — Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation)
- [Use UTF-8 code pages in Windows apps — Microsoft Learn](https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page)
- [The activeCodePage manifest element — The Old New Thing](https://devblogs.microsoft.com/oldnewthing/20220531-00/?p=106697)
- [LongPathAware / manifest? — LINQPad forum](https://forum.linqpad.net/discussion/3404/longpathaware-manifest)
- [Unicode in Microsoft Windows — Wikipedia](https://en.wikipedia.org/wiki/Unicode_in_Microsoft_Windows)
