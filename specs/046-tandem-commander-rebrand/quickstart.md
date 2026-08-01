# Quickstart: Verifying the Tandem Commander Rebrand

**Feature**: 046-tandem-commander-rebrand

## Prerequisites

- Windows 11, VS2022 with C++ Desktop workload
- Python 3 + Pillow (`pip install pillow`) — brand tooling only
- Inno Setup 6 (for the installer step)
- `set OPENSAL_BUILD_DIR=<your build dir>` (or default `.\build\`)

## 1. Static gates (no build needed)

```powershell
# G1 — old identity gone from tracked files (allowed only in specs/ history and
# the four deliberate predecessor references: the FTP .str import-compat
# constant (FR-013), rebrand.py's rewrite rules for the old identity, and the
# rename history recorded in CLAUDE.md + the constitution)
git grep -iIl -e "newt commander" -e "newtcommander" -- ':!specs' ':!src/plugins/ftp/ftp2.cpp' ':!tools/translate/rebrand.py' ':!CLAUDE.md' ':!.specify/memory/constitution.md'
# expected: no output

# G2 — brand-derived identifiers renamed (FR-017)
git grep -nE "VERSINFO_(HOLDER|COPYRIGHT)_NEWT|NCDrawWordmark|NC_COLOR_|NCExten_" -- ':!specs'
# expected: no output

# G3 — build-tree token lockstep (Directory.Build.targets self-check)
git grep -n "newtcommander" -- "*.props" "*.targets" "*.vcxproj"
# expected: no output

# G4 — icon/artwork structure
python tools\brand\gen_icons.py --verify
# expected: every line "OK"

# G5 — translation residue (old AND predecessor identity)
python -m tools.translate.rebrand
# expected: "no predecessor identity remains", exit code 0
```

## 2. Build

```batch
build.cmd full release
```

Verify:
- `%OPENSAL_BUILD_DIR%tandemcommander\Release_x64\tandemcommander.exe` exists;
  no `%OPENSAL_BUILD_DIR%newtcommander\` is created.
- Release intermediates live under `%OPENSAL_BUILD_DIR%obj\Release_x64\`
  (NOT inside `tandemcommander\Release_x64\`) — proves the
  `Directory.Build.targets` token still matches (R5 hazard).
- Explorer → `tandemcommander.exe` → Properties → Details: Product name
  "Tandem Commander", company "Tandem Commander Project", original filename
  `TANDEMCOMMANDER.EXE`; the icon is the new orange folder at all sizes.
- Run an **incremental** `build.cmd` afterwards; it must be fast (object/PCH
  cache preserved), not a from-scratch rebuild.

## 3. Runtime smoke (SC-003, SC-006)

Launch `tandemcommander.exe` and check:

| Surface | Expect |
|---|---|
| Splash screen | new artwork + "Tandem Commander" two-tone wordmark |
| Main window title + taskbar | "Tandem Commander", new icon |
| Help → About | new artwork, wordmark, `tandemcommander.org` link, copyright lines unchanged (Open Salamander Authors + Pavel Stupka) |
| Second launch | activates the running instance (renamed mutex works) |
| Left/Right → Task List | caption "Tandem Commander Task List" |
| Registry (regedit) | config appears under `HKCU\Software\Tandem Commander\0.1`; nothing new under `Software\Newt Commander` |
| Language switch (e.g. Czech) | About/menus show inflected "Tandem Commander*u*" etc. |
| Plugins → Plugin Manager | plugins loaded (interface 105 intact), home pages `www.tandemcommander.org` |

FTP compat (SC-007): in a pre-rename build export a server type (`.str`),
then import it in the renamed build — must succeed; export from the new build
carries the new header.

## 4. Installer (SC-004)

```batch
iscc setup\tandemcommander.iss
```

- Output: `setup\output\tandemcommander-0.1.0-x64-setup.exe`.
- Install on a clean profile: lands in `C:\Program Files\Tandem Commander`,
  Start-menu/desktop entries "Tandem Commander" with the new icon; app runs.
- If an old Newt Commander install exists, it is untouched (different AppId,
  different directory).
- Uninstall removes the installed files and shortcuts.

## 5. Docs & governance

- `README.md` / `CLAUDE.md` / `AUTHORS` / `architecture/*` read "Tandem
  Commander"; `.specify/memory/constitution.md` Principle II is anchored to
  Tandem Commander 0.1.0 with version 3.0.0 and a fresh Sync Impact Report.
- `specs/030-*`, `specs/032-*` still mention Newt Commander — by design.
