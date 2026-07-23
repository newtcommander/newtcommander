# Contract: Newt Commander Branding Identity

**Single source of truth** for every new identifier introduced by feature 032.
All implementation tasks MUST use these values verbatim. Any deviation is a defect.

## Product

| Attribute | Value |
|-----------|-------|
| Product display name | `Newt Commander` |
| Executable | `newtcommander.exe` |
| Version (display) | `0.1.0` — window title/About show `Newt Commander 0.1.0 (x64)` |
| Version (numeric FILEVERSION/PRODUCTVERSION) | `0,1,0,184` |
| Manifest assemblyIdentity | `name="NewtCommander.NewtCommander" version="0.1.0.0"` |
| Manifest description | `Newt Commander File Manager` |
| CompanyName / publisher | `Newt Commander Project` |
| ProductName (all VERSIONINFO) | `Newt Commander` |
| Main-app FileDescription | `Newt Commander, File Manager` |
| VERSINFO_INTERNAL / VERSINFO_ORIGINAL | `NEWTCOMMANDER` / `NEWTCOMMANDER.EXE` |
| Website | `https://newtcommander.org` |
| Repository / issues | `https://github.com/newtcommander/newtcommander` |

## Copyright (year-split rule, FR-017/FR-021)

| Component | LegalCopyright string |
|-----------|----------------------|
| Main app, inherited components | `Copyright © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors` |
| Inherited plugin with own start year Y | `Copyright © Y-2026 Open Salamander Authors, © 2026 Newt Commander Authors` |
| sftp, mdview (new in this project) | `Copyright © 2026 Newt Commander Authors` |
| pictview (modified by this project) | `Copyright © 2000-2026 Open Salamander Authors, © 2026 Newt Commander Authors` |
| salmon | `Copyright © 2012-2026 Open Salamander Authors, © 2026 Newt Commander Authors` |
| shellext | `Copyright © 2003-2026 Open Salamander Authors, © 2026 Newt Commander Authors` |

## Registry

| Purpose | Old | New |
|---------|-----|-----|
| Configuration root (HKCU) | `Software\Open Salamander\5.0` (+82 legacy roots) | `Software\Newt Commander\0.1` (single root, no legacy chain) |
| Config version label | `5.0` | `0.1` |
| Bug reporter (HKCU) | `Software\Open Salamander\Bug Reporter` | `Software\Newt Commander\Bug Reporter` |

## Filesystem

| Purpose | Old | New |
|---------|-----|-----|
| Crash dump folder | `%APPDATA%\Open Salamander` | `%APPDATA%\Newt Commander` |
| mdview WebView2 cache | `…\Open Salamander\mdview.WebView2` | `…\Newt Commander\mdview.WebView2` |

## IPC / discovery namespace

| Purpose | Old | New |
|---------|-----|-----|
| Main window class (FindWindow single-instance) | `SalamanderMainWindowVer25` | `NewtCommanderMainWindowVer01` |
| Process-list shared memory | `AltapSalamander3bProcessList` | `NewtCommander01ProcessList` |
| (mirrored variants) | `AltapSalamander3bProcessList{Mutex,Event,EventProcessed}` | `NewtCommander01ProcessList{Mutex,Event,EventProcessed}` |
| First-instance mutex | `AltapSalamanderFirstInstance` | `NewtCommanderFirstInstance` |
| Config load/save mutex | `AltapSalamanderLoadSaveRegistry` | `NewtCommanderLoadSaveRegistry` |
| Salmon main-dialog mutex | `AltapSalamanderSalmonMainDialog` | `NewtCommanderSalmonMainDialog` |
| Bug-reporter registry mutex | `Global\AltapSalamanderBugReporterRegistryMutex` | `Global\NewtCommanderBugReporterRegistryMutex` |

## Shell extension

| Attribute | Old | New |
|-----------|-----|-----|
| CLSID | `{c78b614f-f3ea-11d2-94a1-00e0292a01e3}` | `{A6D5A8E2-D69F-4E03-8396-781909E7A3AE}` |
| Registration name | `OpenSalamanderVer500` | `NewtCommanderVer010` |
| Shared-names appendix | `500` | `010` |
| Shared memory / mutex / event | `SalExten_SharedMem4` / `SalExten_SharedMemMutex4` / `SalExten_DoPasteEvent4` | `NCExten_SharedMem1` / `NCExten_SharedMemMutex1` / `NCExten_DoPasteEvent1` |
| Description | `Shell Extension (%s) for Open Salamander …` | `Shell Extension (%s) for Newt Commander 0.1.0` |
| DLL file names | `salextx86.dll` / `salextx64.dll` | unchanged (D05) |

## Crash reporter behavior

| Aspect | Old | New |
|--------|-----|-----|
| App name | `Open Salamander Bug Reporter` | `Newt Commander Bug Reporter` |
| Upload | POST to `reports.altap.cz` | **disabled — no network transmission**; dumps stay local |
| User guidance | vendor server | attach dump to GitHub issues |

## Visual assets

| Asset | Content |
|-------|---------|
| `src/res/salamand.ico` (name kept) | 16 (favicon variant), 24, 32 (simplified), 48, 64, 128, 256 px (full) — 32-bpp; BMP entries ≤ 64 px, PNG ≥ 128 px |
| `src/res/sal_r.ico` / `sal_g.ico` / `sal_b.ico` (names kept) | 16 + 32 px, plates tinted red `#EF4444/#B91C1C`, green `#22C55E/#15803D`, blue `#60A5FA/#1D4ED8` |
| `src/res/logo.svg` (name kept) | new icon SVG (96 viewBox, nanosvg-compatible) |
| `src/res/gradspl.svg`, `gradabt.svg` (names kept) | brand gradient band `#3B82F6 → #F97316` |
| `src/res/os.svg` | retired from rendering path (wordmark drawn via GDI) |
| Wordmark (GDI) | "Newt " + "Commander", Segoe UI bold; dark theme: `#EAF2FB` + `#F97316` on `#0A1424`; light theme: `#0A1424` + `#EA6A0B` on white |
| Tagline (GDI, optional) | `TWO-PANE FILE MANAGER`, letter-spaced; dark `#8FA6C4`, light `#5D82B8` |

## Strings

| Constant | New value |
|----------|-----------|
| `MAINWINDOW_NAME` | `"Newt Commander"` |
| `SALAMANDER_TEXT_VERSION` | `"Newt Commander " VERSINFO_VERSION` |
| Plugin refusal message | `This plugin requires Newt Commander 0.1.0 (build 184) or later.` (keep original phrasing pattern) |
| Config version display | `Newt Commander 0.1` pattern in import-config labels (dead code path, single root) |
