# Contract: Tandem Commander Identity Map

**Feature**: 046-tandem-commander-rebrand · **Date**: 2026-08-01

This is the definitive old→new mapping. Every value on the left MUST be absent
from tracked files after the rename (outside `specs/0*` history); every value
on the right is the single authoritative new form. The "MUST NOT change" list
at the end is binding in the other direction.

## 1. Product & binary

| Element | Old | New |
|---|---|---|
| Product name | Newt Commander | Tandem Commander |
| Company / project | Newt Commander Project | Tandem Commander Project |
| Main binary | `newtcommander.exe` | `tandemcommander.exe` |
| MSBuild TargetName | `newtcommander` | `tandemcommander` |
| VERSINFO_INTERNAL / OriginalFilename | `NEWTCOMMANDER` / `NEWTCOMMANDER.EXE` | `TANDEMCOMMANDER` / `TANDEMCOMMANDER.EXE` |
| Usage-text exe | `NEWTCOMMANDER.EXE` | `TANDEMCOMMANDER.EXE` |
| Version / build / plugin interface | 0.1.0 / 184 / 105 | **unchanged** |

## 2. Registry & per-user file system

| Element | Old | New |
|---|---|---|
| Config root | `HKCU\Software\Newt Commander\0.1` | `HKCU\Software\Tandem Commander\0.1` |
| Bug Reporter key | `Software\Newt Commander\Bug Reporter` | `Software\Tandem Commander\Bug Reporter` |
| Roaming folder | `%APPDATA%\Newt Commander` | `%APPDATA%\Tandem Commander` |
| WebView2 profile | `%LOCALAPPDATA%\Newt Commander\mdview.WebView2` | `%LOCALAPPDATA%\Tandem Commander\mdview.WebView2` |
| Migration | — | **None.** Old keys/folders are never read, written, or deleted. |

## 3. OS-visible named objects

| Element | Old | New |
|---|---|---|
| Main window class | `NewtCommanderMainWindowVer01` | `TandemCommanderMainWindowVer01` |
| Process list shm | `NewtCommander01ProcessList` | `TandemCommander01ProcessList` |
| Process list mutex | `NewtCommander01ProcessListMutex` | `TandemCommander01ProcessListMutex` |
| Process list event | `NewtCommander01ProcessListEvent` | `TandemCommander01ProcessListEvent` |
| Process list event (processed) | `NewtCommander01ProcessListEventProcessed` | `TandemCommander01ProcessListEventProcessed` |
| Single-instance mutex | `NewtCommanderFirstInstance` | `TandemCommanderFirstInstance` |
| Load/save registry mutex | `NewtCommanderLoadSaveRegistry` | `TandemCommanderLoadSaveRegistry` |
| Bug-reporter registry mutex | `Global\NewtCommanderBugReporterRegistryMutex` | `Global\TandemCommanderBugReporterRegistryMutex` |
| Salmon main-dialog mutex | `NewtCommanderSalmonMainDialog` | `TandemCommanderSalmonMainDialog` |
| Shell-ext shared mem | `NCExten_SharedMem1` | `TCExten_SharedMem1` |
| Shell-ext shm mutex | `NCExten_SharedMemMutex1` | `TCExten_SharedMemMutex1` |
| Shell-ext paste event | `NCExten_DoPasteEvent1` | `TCExten_DoPasteEvent1` |
| Shell-ext registry value | `NewtCommanderVer<appendix>` | `TandemCommanderVer<appendix>` |
| App manifest identity | `NewtCommander.NewtCommander` | `TandemCommander.TandemCommander` |
| Salmon manifest identity | `NewtCommander.BugReporter` | `TandemCommander.BugReporter` |
| TWAIN identity | Manufacturer `Newt Commander`, family `Newt Commander plugin` | `Tandem Commander` / `Tandem Commander plugin` |

The `tools/salbreak/tasklist.cpp` mirror MUST stay byte-identical to
`src/tasklist.cpp` for these names.

## 4. URLs

| Element | Old | New |
|---|---|---|
| Website (bare) | `newtcommander.org` | `tandemcommander.org` |
| Website (www) | `www.newtcommander.org` | `www.tandemcommander.org` |
| Website (https) | `https://newtcommander.org[/]` | `https://tandemcommander.org[/]` |
| Repository | `github.com/newtcommander/newtcommander` | `github.com/tandemcommander/tandemcommander` |
| Issues / Releases | `.../issues`, `.../releases` | same suffixes, new org/repo |

## 5. Build tree & installer

| Element | Old | New |
|---|---|---|
| Output root segment | `<OPENSAL_BUILD_DIR>newtcommander\` | `<OPENSAL_BUILD_DIR>tandemcommander\` |
| IntDir rewrite token (`Directory.Build.targets`) | `newtcommander\$(Configuration)_$(ShortPlatform)\` | `tandemcommander\$(Configuration)_$(ShortPlatform)\` — MUST equal the props segment |
| Launcher shortcut | `newtcommander.lnk` | `tandemcommander.lnk` |
| Installer script | `setup/newtcommander.iss` | `setup/tandemcommander.iss` |
| Installer AppId | `{D8FDDA80-E79F-4C37-BF39-03B6486C1ED1}` | `{35C0B0DC-DB73-429C-AAA8-FBC41C937F66}` (generated in T003) |
| Install dir | `{autopf}\Newt Commander` | `{autopf}\Tandem Commander` |
| Setup package | `newtcommander-0.1.0-x64-setup` | `tandemcommander-0.1.0-x64-setup` |
| Publisher | Pavel Stupka | **unchanged** |

## 6. Internal brand-derived identifiers (FR-017)

| Old | New |
|---|---|
| `VERSINFO_HOLDER_NEWT` | `VERSINFO_HOLDER_TANDEM` (value "Pavel Stupka" unchanged) |
| `VERSINFO_COPYRIGHT_NEWT` | `VERSINFO_COPYRIGHT_TANDEM` |
| `NCDrawWordmark` (+ `newtClr` param) | `TCDrawWordmark` (+ `tandemClr`) |
| `NC_COLOR_NAVY` … `NC_COLOR_MUTED_LIGHTBG` | `TC_COLOR_*` (RGB values unchanged) |
| `SALSHEXT_*` name **values** `NCExten_*` | `TCExten_*` (identifier names are upstream `SALSHEXT_*`, kept) |
| rebrand.py constants/rules | PRODUCT/PROJECT/WEB → Tandem forms; predecessor rules added |

## 7. Data-file headers (write new, read both)

| File kind | Written header (new) | Import behavior |
|---|---|---|
| FTP exported server type (`.str`) | `Tandem Commander - FTP Client - Exported Server Type` | Import MUST also accept the old `Newt Commander - ...` signature (validated at `ftp2.cpp:1197`) |
| Bug report | `Tandem Commander Bug Report File` | write-only, no validation |
| Checksum `.sfv`/`.md5` comment | `; Generated by Tandem Commander, https://tandemcommander.org` | write-only |
| ZIP SFX defaults | Vendor via holder macro, WWW `https://tandemcommander.org` | baked into new archives only |

## 8. Wordmark & palette (About / splash)

- Wordmark text: `"Tandem "` + `"Commander"`, GDI-drawn, Segoe UI bold,
  shrink-to-fit (existing loop handles the longer word).
- Palette (unchanged values, renamed macros): navy `#0A1424`; text dark-bg
  `#EAF2FB`; orange dark-bg `#F97316`; muted dark-bg `#8FA6C4`; text light-bg
  `#0A1424`; orange light-bg `#EA6A0B`; muted light-bg `#5D82B8`.
- Icon source: full-bleed variant renders for all 7 ICO sizes; margin+shadow
  variant is the About/splash artwork (`about.png` → `logo.png`).

## 9. MUST NOT change (binding non-goals)

- Upstream source/project names: `salamand.sln`, `salamand.vcxproj`,
  `salamand.rc2`, `SALAMANDER_*` constants, `salmon`, `salspawn`, `salopen`,
  `shexreg`, `spl_*` headers, `.spl`/`.slg` extensions.
- Shipped asset file names: `src/res/salamand.ico`, `salmon.ico`,
  `setup.ico`, `icon1.ico`, `logo.png` (feature-035 pipeline contract).
- Shell-extension CLSID `a6d5a8e2-d69f-4e03-8396-781909e7a3ae`.
- Version identity: 0.1.0, build 184, plugin interface 105 (pre-rename
  plugins keep loading).
- `OPENSAL_BUILD_DIR` environment-variable name.
- Copyright holders and year-split rule (up to 2026 Open Salamander Authors;
  2026+ Pavel Stupka), holder defined once.
- Historical `specs/0*` content and directory names; git history/branches.
- Incidental non-brand identifiers: `newText`, `NewTable`,
  `IDS_MENUNEWTITLE`, `IDS_SRVTYPENEWTITLE`, `IDS_HOSTKEY_NEWTEXT`, and all
  third-party code (`sqlite3.c`, wil, …).
