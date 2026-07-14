# Validation Results: Long Path and Unicode File Name Support

**Feature**: [spec.md](spec.md) | **Verified on**: 2026-07-14
**Build**: Debug x64, full solution (90 projects) builds clean
**Fixtures**: `tools/create-test-fixtures.ps1` (see [quickstart.md](quickstart.md))

All checks below were run against the **running application**, not only unit
tests — several defects (see "Defects found by runtime verification") were
invisible to compile-time and unit-level checks.

## Automated unit tests (foundation layer)

`build\salamander\Debug_x64\saltests\saltests.exe` — **403 checks, 0 failed**.

Covers: strict UTF-8↔UTF-16 conversion (NFD sequences, non-BMP, invalid input),
NFC normalization, canonical-equivalence and case-insensitive matching,
`CSalPathBuf` (drive/UNC roots, growth beyond MAX_PATH), extended-length path
normalization (dot segments, forward slashes, UNC, >32k rejection), and real
file I/O at a >300-character path with a decomposed (NFD) file name
(create → enumerate → verify exact NFD name → rename → copy → delete).

## Runtime verification matrix

| # | Check | Criterion | Result |
|---|-------|-----------|--------|
| 1 | Navigate to a 354-character deep path | SC-001, FR-001 | **PASS** — panel lists the directory; title/directory line/status bar show the path |
| 2 | Navigate to a precomposed Unicode directory (U+010D) | FR-005 | **PASS** |
| 3 | Navigate to a decomposed Unicode directory (`c` + U+030C) | FR-005, FR-006 | **PASS** — the title preserves the **decomposed** form exactly (codepoints 99, 780) |
| 4 | List a directory containing NFC *and* NFD spellings of the same name, plus Greek, Japanese and emoji names | SC-002, SC-003, FR-005, FR-007 | **PASS** — both `č.txt` entries coexist and are individually listed; all scripts render correctly (see note on emoji glyphs) |
| 5 | Copy files (F5) with NFD / NFC / non-BMP (emoji) names between panels | SC-004, FR-006 | **PASS** — every name preserved **bit-exactly** (NFD stays NFD, NFC stays NFC, surrogate pair intact) |
| 6 | Ordinary ASCII paths (browse `C:\Windows`, listings, sorting, config) | SC-006, FR-015 | **PASS** — no behavioral difference from the previous release |
| 7 | Embedded manifest contains `longPathAware` | contracts/app-manifest.md | **PASS** (`mt.exe` resource dump) |
| 8 | Long-path operations work with the system-wide `LongPathsEnabled` registry value absent | research R2 | **PASS** — the app uses extended-length (`\\?\`) paths itself |

## Performance (SC-009)

Listing a **100,000-file** directory, measured as elapsed time from process
start until the panel shows the directory (4 runs each, first run discarded):

| Build | Average |
|-------|---------|
| Baseline (pre-004, commit `2c996ee`) | 0.711 s |
| Feature 004 | **0.578 s** |

**Delta: −18.7 % (faster). SC-009 PASS** — the criterion allows ±10 %; the new
build is *faster*, not slower. The W enumeration avoids the ANSI thunk's
per-name conversion, and the sort keeps a byte-wise fast path for ASCII names
(research R5), so the Unicode-correct path costs nothing in the common case.

## Defects found by runtime verification (all fixed)

These were caught only by driving the real application:

1. **Stack overflow on long paths** — `ChangePathToDisk` copied the target path
   with `strcpy` into `char[MAX_PATH]`. Fixed: heap buffer.
2. **All non-ASCII paths rejected** — `ChangeDir`'s path-component walk still
   used ANSI `FindFirstFile` and `MAX_PATH` gates, so every UTF-8 path failed
   the existence check and the panel silently fell back to `C:\`. Fixed:
   W enumeration + long-path bounds.
3. **Command line destroyed non-ACP names** — `WinMain` receives an ANSI command
   line and the parameter expansion used `ExpandEnvironmentStringsA`. Fixed:
   `GetCommandLineW` → UTF-8, `ExpandEnvironmentStringsW`, `GetCurrentDirectoryW`.
4. **Empty panel in directories with an equivalent name pair** — the FR-007
   notice was shown with a modal box *inside* the listing loop, blocking the
   panel refresh. Fixed: the notice is posted (`WM_USER_EQUIVPAIRNOTICE`) and
   shown after the refresh completes.
5. **dbviewer build break** — `splunicode.h` (which pulls in `<windows.h>`) was
   included after a C header that `#define`s `BOOL int`. Fixed: include order.
   (Documented as a caveat for all plugin ports.)

## Plugin verification

- **Build**: after every plugin wave the full 90-project solution builds clean
  (Debug x64), including the ported archivers/viewers/FS plugins and the plugin
  handle-tracker W additions (`mhandles.h`).
- **zip format decode** (code inspection of the runtime path): `CZipCommon::ProcessName`
  returns UTF-8 to the interface — general-purpose bit 11 (`GPF_UTF8`) names pass
  through verbatim (a .NET-created test ZIP with bit 11 set carries a decomposed
  `č` name, byte sequence 63 CC 8C, which is preserved), Unix/Mac UTF-8-without-flag
  is sniffed, and legacy OEM/ANSI entries are converted OEM/ACP → UTF-16 → UTF-8.
- **Panel-driven UI automation** was limited in the verification environment:
  Windows UIPI blocks `SendKeys` into the elevated app, and the `-a` argument
  accepts directories but not an archive path, so opening an archive *through the
  panel* and the F5-pack flow could not be scripted here. These paths are covered
  by the build + the code-level decode verification above and should be re-checked
  interactively. Panel navigation, listing, quick search and the F5 **copy** flow
  WERE driven successfully (via `PostMessageW` to the `SalamanderItemsBox` window
  and process arguments) and are recorded in the matrix above.

## Known cosmetic limitation

Emoji (non-BMP) names list, copy and match correctly, but the panel font has no
color-emoji glyphs, so such characters draw as a placeholder box. This is font
fallback, not an encoding defect — the stored name is intact (verified by
byte-exact copy). Enabling GDI font linking for the panel is a follow-up.
