# Research: Dark Theme for Plugin Windows and Dialogs (036)

Sources: `specs/028-visual-themes/research.md` (theme-engine decisions
D1–D11), `src/themes.h` (verified current API), `src/plugins/shared/`
survey (spl_gen.h, spl_vers.h, winliblt.cpp), per-plugin dialog census
(rc-template count + winliblt usage), `plugins.cfg`.

## R1 — Exporting the theme engine: append to CSalamanderGeneralAbstract

**Decision**: Append six pure virtuals at the END of
`CSalamanderGeneralAbstract` (`src/plugins/shared/spl_gen.h`):

```cpp
virtual BOOL     WINAPI IsDarkThemeActive() = 0;
virtual COLORREF WINAPI GetThemeSysColor(int index) = 0;
virtual HBRUSH   WINAPI GetThemeSysColorBrush(int index) = 0;
virtual void     WINAPI ThemeApplyToDialog(HWND hDialog) = 0;
virtual void     WINAPI ThemeApplyToTopLevel(HWND hWindow) = 0;
virtual BOOL     WINAPI ThemeHandleCtlColor(UINT uMsg, WPARAM wParam,
                                            LPARAM lParam, INT_PTR* result) = 0;
```

Implemented by `CSalamanderGeneral` (`src/plugins.h:1871`) as one-line
delegations to the existing `src/themes.cpp` functions of the same names
(`ThemeSysColor`/`ThemeSysColorBrush` for the color getters). Bump
`LAST_VERSION_OF_SALAMANDER` 104 → 105 in `spl_vers.h` with a history row.

**Rationale**: Appending virtuals to an interface that plugins only *call*
(never implement) preserves the vtable layout of every existing slot — old
binaries keep working; this is the interface's own documented evolution
pattern (spl_vers.h history 1→104). Delegation keeps `themes.cpp` the
single owner of the dark palette (no drift between core and plugins).

**Alternatives**: a separate `CSalamanderThemeAbstract` obtained via a new
getter — more API surface for no benefit at 6 methods; exporting C
functions from the exe — breaks the established all-services-via-interfaces
architecture; duplicating the palette in shared plugin code — guaranteed
drift.

## R2 — winliblt central hook: the 028-D6 pattern replicated

**Decision**: `src/plugins/shared/winliblt.{h,cpp}` gets a module-level
provider `void SetupWinLibTheme(CSalamanderGeneralAbstract* sal)` (same
pattern as the existing `SetupWinLibHelp`). The two central procs —
`CDialog::CDialogProc` (winliblt.cpp:495) and
`CPropSheetPage::CPropSheetPageProc` (winliblt.cpp:754) — add:

- `WM_INITDIALOG`: `sal->ThemeApplyToDialog(HWindow)` (dark title bar +
  per-child `SetWindowTheme` + listview/treeview colors), before the
  object's own `DialogProc` runs so dialogs can still override.
- `WM_CTLCOLORDLG/STATIC/EDIT/LISTBOX/BTN`: `sal->ThemeHandleCtlColor(...)`
  first; when it returns TRUE the central proc returns the brush, else
  falls through to existing behavior.

Provider unset (old plugin, or plugin author opted out) ⇒ both hooks are
no-ops — winliblt keeps today's behavior byte-for-byte.

**Rationale**: Identical to how 028 themed 100% of core dialogs with two
hook points. The census shows 12 of the 19 enabled plugins compile
winliblt (ftp 55 dialog templates, pictview 17, regedt 9, renamer 9,
dbviewer 6, undelete 6, filecomp 5, 7zip 4, checksum 3, peviewer 1,
uniso 1, mdview/folders/portables/sftp winliblt-linked for non-dialog
helpers) — one shared change + one `SetupWinLibTheme` line per plugin
covers the bulk of ~150 templates.

## R3 — Raw-DialogBoxParam dialogs: two touchpoints per dialog proc

**Decision**: Plugins that create dialogs directly (census: sftp 8 procs +
1 modeless log window, zip 26 templates across dialogs*.cpp, uncab 4,
diskmap config) add to each dialog proc:

- `WM_INITDIALOG`: `SalamanderGeneral->ThemeApplyToDialog(hwnd);`
- `WM_CTLCOLOR*`: `INT_PTR r; if (SalamanderGeneral->ThemeHandleCtlColor(uMsg, wParam, lParam, &r)) return r;`

Wrapped per-plugin in a tiny local helper where a plugin has many procs
(zip) to keep the diff mechanical.

**Rationale**: The exact contract `ThemeHandleCtlColor` was designed for in
028 (msgbox.cpp precedent). No behavioral change in Default theme (the
function returns FALSE and `ThemeApplyToDialog` early-outs).

## R4 — Top-level plugin windows (viewers, logs, custom frames)

**Decision**: Every plugin-created top-level window applies
`SalamanderGeneral->ThemeApplyToTopLevel(hwnd)` at creation (dark DWM
title bar) and swaps its draw-time `GetSysColor/GetSysColorBrush` chrome
calls to `GetThemeSysColor/GetThemeSysColorBrush`. Content areas per the
clarification:

- **Text/document content dark**: mdview (markdown/HTML document
  background/foreground), dbviewer (table view), filecomp (compare panes),
  regedt (value views), sftp log window, renamer preview lists.
- **Image/binary content untouched**: pictview canvas (image + its
  checkerboard/paper background stay as configured), thumbnails.

Theme state is read at window creation only (clarified: reopen suffices);
plugins that already handle `PLUGINEVENT_COLORSCHANGED` may refresh
sooner, but no new notification plumbing is added.

**Rationale**: Matches 028's treatment of the core viewer (dark chrome +
`DarkViewerColors`); mdview and dbviewer already draw from configurable
colors, so the dark variants slot into existing color-resolution points.

## R5 — ABI safety and interface documentation

**Decision**: `spl_vers.h` gains history row `105 — 0.1.0 build 184:
theme services for plugin UI (feature 036; 6 methods appended to
CSalamanderGeneralAbstract)`. The new methods are fully documented in
`spl_gen.h` comments (contract mirrored in
`contracts/plugin-theme-api.md`) BEFORE any plugin change (constitution V).
Plugins built with SDK 105 and calling the new methods MUST declare they
require version 105 (standard `SalamanderPluginGetReqVer` mechanism), so
they refuse to load on older cores instead of jumping into missing vtable
slots. All shipped plugins already return `LAST_VERSION_OF_SALAMANDER`,
which becomes 105 by the same rebuild.

**Rationale**: This is precisely how versions 1→104 evolved; the loader
already enforces the minimum-version handshake.

## R6 — What is explicitly NOT done

- No live repaint of open plugin windows on theme switch (clarified);
  the existing `PLUGINEVENT_COLORSCHANGED` broadcast stays as-is.
- No theming of OS-owned surfaces launched by plugins (common dialogs,
  shell menus) — 028 boundary.
- No forced dark for plugins that never call the new API (third-party) —
  they stay light and functional.
- No changes to `themes.cpp` palette values — plugins consume the 028
  palette verbatim.
- Disabled-in-default-build plugins (automation, mmviewer, nethood, unchm,
  unmime, unole, unrar, checkver, demo*) receive the same treatment ONLY
  where the change is shared (winliblt, spl_gen.h); their plugin-local
  sweeps are out of scope until they are re-enabled.
