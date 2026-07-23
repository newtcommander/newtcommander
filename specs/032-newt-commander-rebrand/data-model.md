# Data Model: Newt Commander Application Rebrand

Feature 032 manipulates *identity data*, not domain data. Entities below map the
spec's Key Entities to concrete artifacts; canonical values live in
[contracts/branding-identity.md](contracts/branding-identity.md).

## Entity: Product Identity

| Field | Owner artifact | Notes |
|-------|----------------|-------|
| Display name | `src/salamdr1.cpp` (`MAINWINDOW_NAME`), `src/mainwnd1.cpp` (`SALAMANDER_TEXT_VERSION`), `src/lang/lang.rc` | drives titles, captions, tray |
| Binary name | `src/vcxproj/salamand.vcxproj` (`TargetName`) | consumers: salmon relaunch, fcremote, SalPath props, texts.rc2 usage strings |
| Version (display) | `src/plugins/shared/spl_vers.h` | 3-component composition fix; consumed by `VERSINFO_VERSION`, title, About, splash |
| Version (numeric) | `src/plugins/shared/versinfo.rc2` tuples, `src/manifest.xml` | `0,1,0,184` / `0.1.0.0` |
| Publisher/copyright metadata | `src/versinfo.rh2`, per-plugin `versinfo.rh2`, `shellext.rc`, `salmon.rc` | year-split rule (FR-017/FR-021) |
| OS manifest identity | `src/manifest.xml`, `src/salmon/manifest.xml` | assemblyIdentity + description |

**Validation**: after build, `newtcommander.exe` VERSIONINFO reports ProductName
"Newt Commander", version 0.1.0, year-split copyright; no shipped binary reports
ProductName "Open Salamander".

## Entity: Configuration Root

| Field | Owner artifact | Notes |
|-------|----------------|-------|
| Active root path | `src/mainwnd2.cpp` `SalamanderConfigurationRoots[0]` | `Software\Newt Commander\0.1` |
| Root count | `src/consts.h` `SALCFG_ROOTS_COUNT` | 83 → 1 (kills import chain; parallel arrays stay consistent) |
| Version label | `src/mainwnd2.cpp` `SalamanderConfigurationVersions` | `{"0.1"}` |
| Bug-reporter key | `src/salmoncl.cpp`, `src/salmon/config.cpp` | `Software\Newt Commander\Bug Reporter` |

**State transitions (first run)**:
`no NC config present` → app starts with defaults → on exit writes only under
`Software\Newt Commander\0.1`. Pre-existing Open Salamander keys are never read,
enumerated, or written (FR-009/FR-010); the import dialog is unreachable (single root).

## Entity: IPC Namespace

Owner artifacts: `src/tasklist.cpp` (+ `salbreak` mirror if present),
`src/salamdr1.cpp` (window class), `src/salmoncl.cpp`, `src/salmon/salmon.cpp`.
Full mapping in the contract. **Invariant**: no runtime object name shared with
Open Salamander 5.0 → the two products cannot detect/block/signal each other (FR-011).

## Entity: Shell Integration Identity

Owner artifacts: `src/shexreg.h` (CLSID, appendix, shared names), `src/shexreg.c`
(description, registration), `src/shellext/shellext.rc` (metadata).
**Invariant**: registering/unregistering NC's extension leaves OS 5.0's registration
untouched (distinct CLSID + name, FR-012).

## Entity: Visual Asset Set

| Asset | Source of truth | Generated output |
|-------|-----------------|------------------|
| Icon geometry/palette | `tools/brand/*.svg` (from `temp/visual_style/` + authored simplified/favicon/tray variants) | `src/res/salamand.ico`, `sal_r/g/b.ico` |
| About/splash artwork | `tools/brand/` SVGs | `src/res/logo.svg`, `gradspl.svg`, `gradabt.svg` (RCDATA, names kept) |
| Wordmark | GDI text in `src/logo.cpp` (no font dependency) | theme-dependent colors per contract |
| Generator | `tools/brand/gen_icons.py` (Pillow) | re-runnable; committed for reproducibility |

**Validation**: icon renders at 16/24/32/48/64/128/256 with size-appropriate variant
(SC-004); About/splash correct in both themes (SC-005).

## Entity: Version Identity

Marketing `0.1.0` + internal build `184` (continues) + plugin ABI `104` (frozen).
Owner: `src/plugins/shared/spl_vers.h`. **Invariant**: plugins built before/after
this feature load identically (FR-015, SC-007); only the refusal-message text changes.
