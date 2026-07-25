# Per-Plugin Dark-Theme Audit (036)

**Date**: 2026-07-25 · Debug x64 build, Dark theme active (`Theme Mode` = 1).
Two verification levels: **runtime** (opened live, screenshot in the session
scratchpad) and **mechanism** (surface flows through a runtime-verified code
path — winliblt central procs or the `*ThemeDlgMsg` guard pattern — and the
plugin compiles with the change).

## Mechanism verification anchors (runtime-proven)

| Anchor | Proof |
|--------|-------|
| winliblt central procs (`SetupWinLibTheme`) | FTP "Connect to FTP Server" + Renamer "Batch Rename" + Renamer close-confirm — all fully dark with ZERO per-dialog edits in those plugins (screenshots `t018-ftp-connect-dark.png`, `t018-renamer-dark.png`, `t018-renamer-confirm.png`) |
| Raw-proc guard pattern | SFTP: all surfaces (Connect, Logs) dark (`t008-sftp-connect-dark.png`, `t008-sftp-logs-dark.png`) |
| `ThemeApplyToTopLevel` | DiskMap frame: dark title bar (`t018-diskmap-dark.png`); mdview/pictview/dbviewer/filecomp/ftp-wait windows use the identical call |
| Default-theme passthrough | SFTP Connect reopened under Default = classic light look (`t020-sftp-connect-default.png`) |
| Plugin menus (core-drawn) | Plugins menu + submenus dark (`t018-popup-a.png`) |

## Per-plugin status

| Plugin | Surfaces | Level | Result |
|--------|----------|-------|--------|
| sftp | 8 raw dialogs + Logs window | runtime | ✅ dark, readable |
| ftp | 55 winliblt dialogs + wait/list windows | runtime (Connect) + mechanism | ✅ |
| renamer | Batch Rename window + dialogs | runtime | ✅ (menu bar, combos, preview list dark) |
| diskmap | map frame | runtime | ✅ dark title bar; map visualization = its own design (intended); **native Win32 menu bar stays light — OS-drawn, 028 boundary** |
| zip | 18 raw dialog procs | mechanism | ✅ guard in every proc (same pattern as SFTP) |
| uncab | 4 raw dialog procs + custom static | mechanism | ✅ guard + themed custom control |
| pictview | 17 winliblt dialogs; viewer frame | mechanism | ✅ central + dark title bar; image canvas untouched (intended) |
| dbviewer | winliblt dialogs; table window | mechanism | ✅ central + renderer colors → GetThemeSysColor (dark table, light text) + dark title bar |
| filecomp | winliblt dialogs; compare panes | mechanism | ✅ central + SCF_DEFAULT color seeding → GetThemeSysColor (dark panes) + dark title bar |
| mdview | viewer window (WebView2 document) | mechanism | ✅ dark title bar; follow-sys scheme now keys off the app's Dark theme first (dark document palette) |
| regedt | 9 winliblt dialogs | mechanism | ✅ central (submenu verified dark at runtime) |
| undelete | 6 winliblt dialogs | mechanism | ✅ central |
| checksum | 3 winliblt dialogs | mechanism | ✅ central |
| 7zip | 4 winliblt dialogs | mechanism | ✅ central |
| peviewer | 1 winliblt dialog (content shown in core viewer) | mechanism | ✅ central; core viewer dark since 028 |
| uniso | 1 winliblt dialog | mechanism | ✅ central |
| folders | winliblt dialogs (drop-down UI) | mechanism | ✅ central |
| portables | winliblt dialogs (Fx framework) | mechanism | ✅ central |
| tar | no own UI (uses core dialogs) | n/a | ✅ core themed since 028 |

## Theme-switch semantics (US3, runtime)

- Dark → Default with DiskMap (modeless plugin window) open: no crash, main
  window live-switched, DiskMap kept its previous consistent appearance
  (`t019-main-default.png`, `t019-diskmap-after-switch.png`).
- Newly opened plugin window after the switch matches the new theme
  (`t020-sftp-connect-default.png`).
- Switched back to Dark the same way; app closed cleanly, `Theme Mode` = 1
  persisted.

## Known limitations (accepted, recorded)

1. **Native Win32 menu bars** on plugin frame windows (diskmap, mdview,
   dbviewer, filecomp, pictview, renamer window menus) stay light — the OS
   draws them; darkening needs undocumented APIs rejected in 028 (the core
   app's own menus are owner-drawn and dark). Same boundary as OS common
   dialogs.
2. Already-open plugin windows keep the old theme until reopened — by
   design (clarification 2026-07-25).
3. Third-party plugins without the new API stay light — by design (FR-006).
