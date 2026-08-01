# Research: Tandem Commander Rebrand

**Feature**: 046-tandem-commander-rebrand · **Date**: 2026-08-01

All Technical Context entries were resolvable from the repository itself (the
feature is a rename of known, inventoried surfaces); no external research was
required. Decisions below resolve every open "how" from the spec.

## R1. Replacement strategy — ordered token map, category by category

**Decision**: Perform the rename as deliberate per-category edits (not one
blind repo-wide sed), using a fixed, ordered token map applied longest-first:

| # | Old token | New token | Scope note |
|---|---|---|---|
| 1 | `NEWTCOMMANDER.EXE` | `TANDEMCOMMANDER.EXE` | usage strings, `VERSINFO_INTERNAL` derivative |
| 2 | `newtcommander.exe` | `tandemcommander.exe` | code, scripts, installer |
| 3 | `NewtCommander.exe` | `TandemCommander.exe` | rebrand.py rule output |
| 4 | `newtcommander.org` | `tandemcommander.org` | all scheme/`www` variants survive as prefix/suffix |
| 5 | `github.com/newtcommander/newtcommander` | `github.com/tandemcommander/tandemcommander` | `/issues`, `/releases` suffixes survive |
| 6 | `NEWTCOMMANDER` | `TANDEMCOMMANDER` | `VERSINFO_INTERNAL` |
| 7 | `NewtCommander` | `TandemCommander` | window class, kernel objects, manifests, mutexes |
| 8 | `newtcommander` | `tandemcommander` | build-dir segment, shortcut name, file names |
| 9 | `Newt Commander` | `Tandem Commander` | display name, registry root, folders, prose (inflected suffixes preserved by leaving trailing `\w*` intact) |
| 10 | `NEWT COMMANDER` | `TANDEM COMMANDER` | uppercase prose if present |
| 11 | identifier fragments: `VERSINFO_HOLDER_NEWT`→`VERSINFO_HOLDER_TANDEM`, `VERSINFO_COPYRIGHT_NEWT`→`VERSINFO_COPYRIGHT_TANDEM`, `NCDrawWordmark`→`TCDrawWordmark`, `NC_COLOR_*`→`TC_COLOR_*`, `NCExten_*`→`TCExten_*`, `newtClr`→`tandemClr` | per clarification (all brand-derived identifiers) |

**Rationale**: Longest-first prevents partial rewrites (`newtcommander.org`
must not become `tandemcommander.org` via token 8 leaving `.org` handling
inconsistent, and token 9 must not fire inside token 5). Category-by-category
keeps each commit reviewable (Principle III) and lets the two hazard zones
(build props + `Directory.Build.targets`; translations) be verified with their
own gates.

**Alternatives considered**: One repo-wide scripted replace — rejected: it
would hit historical `specs/`, `.git`, and the `newText`/`NewTable`-class
false positives, and a single 400-file commit with no per-area verification
contradicts Principle III.

## R2. Translation archives — extend the proven rebrand.py machinery

**Decision**: Update `tools/translate/rebrand.py`:
- Constants: `PRODUCT = "Tandem Commander"`, `PROJECT = "Tandem Commander
  Project"`, `WEB = "www.tandemcommander.org"` (existing Salamander→PRODUCT
  rules then emit the new name automatically for any future re-import).
- Add previous-identity rules ahead of the Salamander rules, following the
  existing pattern: `www.newtcommander.org` → `WEB`,
  `NEWTCOMMANDER.EXE`/`newtcommander.exe`/`NewtCommander.exe` → Tandem
  equivalents, `\bNEWT COMMANDER(\w*)` → upper PRODUCT + suffix,
  `\bNewt Commander(\w*)` → PRODUCT + suffix, `\bNewt Commanderu\b`-style
  forms are covered by the suffix-preserving group (Czech "Newt Commanderu" →
  "Tandem Commanderu" — both names are masculine *-er* nouns, so declension
  carries over, same argument as the 038 Salamander→Commander move).
- Extend `find_residue()` to also flag `[Nn]ewt\s*[Cc]ommander|NEWTCOMMANDER|
  newtcommander` so the SC-005 gate covers the predecessor identity.
- Run `python -m tools.translate.rebrand --apply` to rewrite all 220 `.slt`
  files in place (it deliberately includes disabled languages, which is what
  we want — ru/zh/uk source must not accumulate residue).

**Rationale**: The module already solves the two hard problems — inflectional
suffix preservation and accelerator-marker handling (`Salamand&er`-class
splits) — and has a residue reporter that becomes the acceptance gate.
`languages.cfg` (11 × `web =`, 1 × `author =`, header comment) is a plain
config file edited directly; `.slt` `WEB,"..."` metadata lines are rewritten
by the same rules.

**Alternatives considered**: Plain sed over `.slt` — rejected: would miss
accelerator-split forms and provides no residue verification. Re-translation
via the API — rejected: nothing needs translating; this is a string rewrite.

## R3. Brand assets — full-bleed renders through the feature-035 pipeline

**Decision** (per clarification: full-bleed for all icon sizes):
- `temp/tandem_design/png/tandem-commander-icon-full-1024.png` →
  `tools/brand/icon-master.png`
- `tandem-commander-icon-full-{16,24,32,48,64,128,256}.png` →
  `tools/brand/icon-{N}.png` (per-size overrides — the designer's per-size
  renders beat a Lanczos downscale of the master)
- `tandem-commander-icon-1024.png` (margin + drop-shadow variant) →
  `tools/brand/about.png` (About/splash artwork source; the shadow belongs to
  the artwork context per DESIGN_README)
- Run `python tools/brand/gen_icons.py` → regenerates
  `src/res/salamand.ico`, `src/salmon/res/salmon.ico`,
  `src/setup/res/setup.ico`, `src/setup/remove/icon1.ico`, `src/res/logo.png`
  (shipped file names deliberately keep upstream names — no project-file
  edits, per feature 035 design).
- `setup/setup.ico` (Inno Setup icon, separate from `src/setup/res/`) is also
  regenerated from the same frames (copy of the new `setup.ico`).
- Reference vectors: delete `tools/brand/newt-commander-icon.svg`; add
  `tandem-commander-icon.svg`, `tandem-commander-icon-full.svg`,
  `tandem-commander-lockup-light.svg`, `tandem-commander-lockup-dark.svg`
  from `temp/tandem_design/`; carry the DESIGN_README palette/usage content
  into `tools/brand/README.md`.
- `gen_icons.py` needs only its docstring updated — no paths reference the
  brand name.

**Rationale**: The pipeline exists precisely for this swap ("replace a file
here → run one command → rebuild"); `--verify` is the structural gate
(FR-010). Palette check: the new design's colors are **identical** to the
current `NC_COLOR_*` values (navy `#0A1424`, orange `#F97316`/`#EA6A0B`,
muted `#8FA6C4`/`#5D82B8`, text `#EAF2FB`) — the wordmark change in
`logo.cpp` is therefore text + identifier renames only (`part1 = "Tandem "`),
and the existing shrink-to-fit loop absorbs the longer word automatically.

**Alternatives considered**: Rendering the SVG at build time — rejected
(feature 035 decided committed PNG/ICO, build never runs Python). Using the
lockup SVGs in-app — rejected: About/splash wordmark is deliberately GDI-drawn
(no shipped font); lockups stay as reference/marketing assets.

## R4. Registry, kernel objects and IPC — mechanical map, no migration

**Decision**: Rename every OS-visible name by token map (see
`contracts/identity-map.md` for the full table): registry roots
`Software\Tandem Commander\0.1` / `...\Bug Reporter`; window class
`TandemCommanderMainWindowVer01`; the six `tasklist.cpp` kernel objects (+
`tools/salbreak` mirror); salmon mutexes; `NCExten_*` → `TCExten_*` shared
memory/events; shexreg registry value `TandemCommanderVer...`; manifest
identities `TandemCommander.TandemCommander` / `TandemCommander.BugReporter`;
TWAIN app identity; `%APPDATA%`/`%LOCALAPPDATA%` folder names. No import of
old registry data (clarified); `dialogs2.cpp` root-sniffing literals updated
in the same commit as `mainwnd2.cpp` so the three sites cannot disagree.
The shell-extension CLSID `a6d5a8e2-...` is **not** brand-derived and stays.

**Rationale**: Same one-time-break shape as feature 032; coexistence of an
old and new build as "unrelated applications" is the accepted (spec edge
case) consequence of renaming the single-instance mutex.

## R5. Build output tree — lockstep rename with grep gate

**Decision**: Rename the output segment `newtcommander\` →
`tandemcommander\` in the same commit across: 20 `*_base.props` +
`salopen_debug/salspawn_debug/shellext_debug.props`, `x86.props`/`x64.props`
(`SalPath` — dir **and** exe name), `saltests.vcxproj`,
`salamand.vcxproj` `<TargetName>`, `Directory.Build.targets` IntDir token,
`build.cmd`, `!clean_all_interm.cmd`, `!populate_build_dir.cmd` (incl.
`tandemcommander.lnk`), `build_langs.cmd/.ps1`, `signslgs.cmd`,
`verify_slg.ps1`, `makeall.bat`, `help/src/*.bat`,
`translations/!update_langs_from_translator.bat`. Then run the
`Directory.Build.targets` self-check it prescribes: grep for the old segment
across `*.props/*.targets/*.vcxproj` and confirm **zero hits**, followed by a
clean Release build confirming intermediates land in `obj\` and not inside
the shipped tree. Known dormant drift: `salpvenv_base.props` hardcodes `_x64`
— rename its segment too but do not fix the pre-existing drift (out of
scope, Principle III).

**Rationale**: `Directory.Build.targets:22-29` documents that a mismatch is a
silent no-op that quietly destroys incremental Release builds — the grep gate
it prescribes becomes an explicit verification task.

## R6. Installer — rename, rebrand, new AppId

**Decision**: `git mv setup/newtcommander.iss setup/tandemcommander.iss`;
update `MyAppName`, `MyAppURL`, `MyAppExeName`, `Source:` paths
(`..\build\tandemcommander\Release_x64\...`), `OutputBaseFilename=
tandemcommander-{#MyAppVersion}-x64-setup`; generate a fresh `AppId` GUID
(PowerShell `[guid]::NewGuid()` at implementation time) so the package is a
distinct product that never upgrades a Newt Commander install.
`MyAppPublisher` stays "Pavel Stupka". `setup.ico` replaced per R3.

## R7. Data-file headers — accept old, write new

**Decision**: Headers written into user files switch to the new name; the one
**validated** header keeps reading the old one:
- `ftp2.cpp` `.str` import (`ftp2.cpp:1197` exact `strncmp` signature check):
  keep `STR_FILE_HEADER` (new text) for export, add
  `STR_FILE_HEADER_OLD = "Newt Commander - FTP Client - Exported Server
  Type"` accepted as an alternate signature on import (FR-013 / SC-007).
- `callstk.cpp` bug-report header, `checksum` `.sfv`/`.md5` comment, zip SFX
  vendor/WWW defaults: write-only strings, no import validation — plain
  rename.

## R8. Constitution amendment

**Decision**: Amend `.specify/memory/constitution.md` as part of the
documentation work: title and Principle II re-anchored to **Tandem Commander
0.1.0** (binary `tandemcommander.exe`, registry root `HKCU\Software\Tandem
Commander`, IPC names, shell-extension identity), recording this feature
(`specs/046-tandem-commander-rebrand/`) as the second deliberate, documented,
one-time identity change; plugin ABI 105 carried over unchanged. Version bump
per the constitution's own policy: MAJOR (principle redefined) →
**3.0.0**, with a Sync Impact Report and a corrected footer (the current file
says 1.1.0 in the footer while its own report says 2.0.0 — fix in passing).

## R9. Verification gates (SC-001/FR-015 grep discipline)

**Decision**: The final acceptance grep is case-insensitive
`newt\s*commander|newtcommander` over tracked files, with allowed remainder
only under `specs/0*` (history). The bare token `newt` is **not** the gate
(false positives: `newText`, `NewTable`, `IDS_MENUNEWTITLE`,
`IDS_SRVTYPENEWTITLE`, `IDS_HOSTKEY_NEWTEXT`, sqlite3.c, wil). A second
identifier sweep greps `_NEWT\b|NCDrawWordmark|NC_COLOR_|NCExten_` to prove
FR-017. Untracked/gitignored trees (`build/`, `temp/`, `setup/output/`) are
exempt.
