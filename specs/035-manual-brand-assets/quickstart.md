# Quickstart: Verifying feature 035

## Prerequisites

- VS2022 + C++ workload, `OPENSAL_BUILD_DIR` set (or default `.\build\`)
- Python 3 + Pillow (asset authoring only)

## 1. Regenerate assets from the new source layout

```batch
python tools\brand\gen_icons.py
python tools\brand\gen_icons.py --verify
```

Expect: 4 ICOs + `src/res/logo.png` written; verify all OK. First run after
migration must reproduce the previously shipped ICO bytes for salamand /
salmon / setup / icon1 (same source pixels, same encoder).

## 2. Build

```batch
build.cmd
```

Expect: clean build; no references to `sal_r/g/b.ico`, `logo.svg`,
`IDB_LOGO_HAND`, `IDI_SALAMANDER_RED/GREEN/BLUE` remain (grep check).

## 3. Run verification

Launch `newtcommander.exe` from the build output:

- **Splash** (enable "Show splash screen at startup" if off): artwork
  renders from PNG; copyright shows TWO lines — "Copyright © 1997-2026
  Open Salamander Authors" / "© 2026 Newt Commander Authors" — neither
  truncated; status line below them; no overlap. (SC-004, US3)
- **Window/taskbar**: new-pipeline icon top-left and in taskbar. (US1)
- **Explorer**: `newtcommander.exe` shows the icon in all view sizes. (US1)
- **About** (Help → About): PNG artwork top-right, undistorted; wordmark
  and gradient line unchanged; both light/dark themes OK. (US2)
- **Configuration → Main Window**: the "Main Window Icon" color combo is
  gone; page lays out cleanly. (FR-003)
- **Stale config fallback**: `reg add "HKCU\Software\Newt Commander\0.1\Configuration" /v "Main window icon index" /t REG_DWORD /d 2 /f`,
  start app → default icon, no error; combo absent. (edge case)
- **File properties**: exe → Properties → Details → Copyright is the full
  single-line string (unchanged). (FR-010)

## 4. Manual-swap dry run (the actual user story)

1. Replace `tools/brand/icon-master.png` and `tools/brand/about.png` with
   visibly different test images (delete `icon-<N>.png` overrides).
2. `python tools\brand\gen_icons.py` && `build.cmd`
3. Confirm new artwork in window, taskbar, Explorer, About, splash.
4. Break it on purpose: rename `icon-master.png` away → rerun → expect
   `error:` naming the missing file, exit ≠ 0. (FR-007)
5. Restore original assets, regenerate, rebuild.

## 5. Docs check

`tools/brand/README.md` alone must walk a newcomer through steps 1–3 of
section 4 (SC-001/002/006); cross-check it against
`contracts/asset-layout.md`.
