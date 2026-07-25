# Validation Results: Markdown Viewer Dark-Mode Polish (037)

**Date**: 2026-07-25 · Debug x64 (`build.cmd`), all builds 0 errors
**Method**: scripted UI drive (SendKeys/mouse) + full-screen frame capture at
~65–130 ms intervals around every F3 open, plus pixel analysis (share of
near-white pixels, R,G,B > 230, inside the viewer window rect) — session
screenshots referenced below were reviewed frame by frame during the run.

## Success criteria

### SC-001 — zero white frames on open ✅ (measured)

Three instrumented open sequences with the dark **midnight** scheme
(45 + 45 + 30 frames). White-pixel share inside the viewer rect never
exceeded **0.38 %** (and that peak is the pre-open panel content under the
window position); from the first frame in which the viewer exists the
client area is the scheme background (average luma 29–31, white ≤ 0.29 %,
which is the rendered light text). For contrast, the pre-fix build showed a
fully white client (~90 % white) until WebView2 navigated — captured live
from the pristine build during root-cause isolation. Both defense layers
verified: the first visible frame (before WebView2 exists) is painted by the
`WM_ERASEBKGND` brush; the WebView2 surface honors
`put_DefaultBackgroundColor` afterwards.

### SC-002 — dark menus consistent with the app ✅

Captured dark: menu **bar** (File/Edit/View/Help, engine highlight on the
active item), **View** drop-down (selection color, right-aligned
accelerators, separators, mnemonic underlines), **Help** drop-down,
**Color Scheme submenu** with the radio dot on *Midnight (dark)* and the
*Follow System Theme* row below the separator. Keyboard access verified:
Alt + mnemonic opens bar popups (owner-drawn mnemonics are routed through
the new `WM_MENUCHAR` handler), arrows/Esc navigate normally. Caption
(system) menu: the main window's own Alt+Space menu is the native light
popup even in Dark theme (028/036 OS boundary); the viewer shows the same
native popup — parity per spec FR-004/research R5.

### SC-003 — scheme change applies immediately ✅

Live change: Shift+F9 in the open viewer re-rendered midnight → graphite
with the new background at once, no white transition (screenshot). The
changed scheme persists and drives the next window (registry showed the
changed scheme until restored). Reopen first-paint was re-measured in the
second 45-frame sequence: 0 white frames, scheme background from the first
frame.

### SC-004 — Default (light) theme unchanged ✅

With Theme Mode = Default the viewer opens with the **native light** menu
bar and popups (no owner-draw, byte-identical code path — `DarkMenus`
snapshot false), light title bar, and the document still renders with the
selected Markdown scheme (midnight background — scheme is independent of
the app theme by design). No dark artifacts, no behavior change.

## Defect found and fixed during verification

**Pre-existing Debug-only crash (the "Microsoft Visual C++ Runtime Library —
Run-Time Check Failure #1" dialog on every F3)**: plugin debug builds compile
with `/RTCc` (`plugin_debug.props` `SmallerTypeCheck`), and the WinAPI macro
`GetGValue` contains a `(WORD)` cast that loses the blue byte of any
`COLORREF` at runtime. Two sites:

- `src/plugins/mdview/htmlgen.cpp` `HexColor()` — **pre-existing** (fires for
  every scheme color on every open; proven by reproducing the dialog on a
  pristine checkout with feature 037 fully stashed). Release builds never
  run RTC checks, which is why normal usage never showed it.
- the new `CMdWebHost::SetBackgroundColor` initially used the same macro and
  was fixed the same way during development.

Both now extract channels with explicit masked shifts. After the fix the
viewer opens cleanly in Debug; the dark-menu code was explicitly ruled out
as a cause (the dialog reproduced with Dark theme off, menu code inert, and
on the pristine build without any 037 code).

## Notes

- User state restored after the runs: Theme Mode = 1 (Dark),
  mdview ColorScheme/SchemeDark = midnight, right panel path returned to its
  original directory.
- Evidence frames/screenshots live in the session scratchpad
  (`v2-open1/`, `v2-open2/`, `v4-open3/`, `shots2/`, `shots3/`, `probe/`);
  key captures: first-frame dark window, View menu dark, scheme submenu with
  radio dot, graphite live-switch, light-mode native menus, pristine-build
  white client + RTC dialog (before), clean render (after).
