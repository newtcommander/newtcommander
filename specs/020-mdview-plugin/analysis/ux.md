# Agent 1 — Product & UX Analysis (raw report, specify phase)

Independent analysis; evidence cites the codebase as of branch base.

## Findings

- **F1 — F3 mental model**: instant, read-only, disposable window; Esc closes
  (`Esc → CM_EXIT`, src/salamand.rc:153); internal viewer persists window
  placement (src/viewer3.cpp:3531–3535). Startup latency violates expectation.
- **F2 — Inside a viewer, F3 = Find Next, never close** (src/salamand.rc:145–163:
  `F3 → CM_FINDNEXT`, `Shift+F3 → CM_FINDPREV`, `Ctrl+F3/F7 → CM_FINDSET`).
  If mdview has search, F3 = find next; if not, F3 must be a no-op.
- **F3 — Internal viewer key map** (viewer3.cpp:3311–3507 + salamand.rc):
  arrows = line/column scroll; Shift+arrows extend selection; PgUp/PgDn page;
  **Home/End = document begin/end**; Ctrl+A select all; Ctrl+C/Ctrl+Insert copy;
  Ctrl+F find; Ctrl+R re-read; **Backspace/Space = previous/next file in panel**
  (plugin-wide convention via GetNext/PreviousFileNameForViewer — demoview
  viewer.cpp:753–779, also pictview/mmviewer/dbviewer); F11 full screen;
  F8/Shift+F8 encoding (reserved); "Auto-Copy Selection" global habit
  (CM_VIEWER_AUTOCOPY, viewer3.cpp:3304).
- **F4 — Wheel**: system scroll-lines with sub-line accumulation
  (viewer3.cpp:479–515); Shift+wheel horizontal (517–541); Ctrl+wheel unhandled
  in internal viewer (no zoom); PictView has full zoom UI (precedent for plugin
  zoom, pictview.cpp:157–171).
- **F5 — Chrome**: internal viewer has menu bar File/Edit/Search/View/Options/Help
  (src/lang/lang.rc2:30–98), right-click context menu (100–120), **no status
  bar**, tooltip window for transient info (viewer3.cpp:558–573); plugins add
  rebar+toolbar; context menu via Shift+F10/Apps (demoview viewer.cpp:743–751).
- **F6 — Title convention**: `<full path> - <viewer name> - [encoding]`
  (SetViewerCaption, viewer3.cpp:23–68), Unicode-safe.
- **F7 — DPI reality**: process manifest declares system-DPI awareness only
  (src/manifest.xml:43); no PerMonitorV2, no WM_DPICHANGED anywhere. mdview can
  rely on system-DPI scaling at startup; per-monitor rescale is beyond the host.
- **F8 — No dark mode / OS-theme integration in the host** (zero matches).
  Only "follow OS theme" (AppsUseLightTheme + WM_SETTINGCHANGE) is
  implementable; dark document will sit in light-classic chrome.
- **F9 — External mental models**: GitHub README rendering (GFM, capped-width
  column ~900 px, URL on hover) is the reference look; VS Code preview (local
  links navigate in-pane, source one toggle away); Obsidian (theming, in-place
  internal links); Typora ("source always visible somewhere").
- **F10 — Copy expectation**: plain text is the primary clipboard format for
  file-manager users; rich text at most a bonus.
- **F11 — Keyboard-only link operation has no host convention** (internal viewer
  has no links) → needs Tab/Shift+Tab focus cycle + visible focus ring + Enter.
- **F12 — Persistence is the norm**: plugin registry config + internal viewer
  persists placement/mode on close (viewer3.cpp:3531–3547).

## Recommendations for v1

### MUST
1. Esc and Alt+F4 close; F3 never closes.
2. Internal-viewer key map verbatim (arrows, PgUp/PgDn, Home/End = doc
   start/end + Ctrl+Home/End aliases, wheel per system, Shift+wheel horizontal).
3. Mouse selection + Ctrl+C/Ctrl+Insert (plain text) + Ctrl+A.
4. Menu bar modeled on internal viewer + right-click context menu (Shift+F10).
5. Title `<full path> - Markdown Viewer`, Unicode-correct.
6. View → Color Scheme submenu, 10 radio-checked schemes, immediate apply,
   scroll preserved, persisted, corrupt value → default.
7. Scheme cycling F9/Shift+F9 (F9 unused in viewer accel table; F8 reserved).
8. Ctrl+R refresh (CM_REREADFILE convention).
9. Anchors navigate in-document; external links only on explicit click via
   system browser; target shown on hover (tooltip precedent viewer3.cpp:558).
10. Tab/Shift+Tab link focus + Enter activate, visible focus ring.
11. Broken image → placeholder with alt text, filename, reason.
12. "Open in text viewer" escape hatch (menu + error-state button).
13. System-DPI-scaled fonts; reflow on resize; no horizontal body scroll (code
    blocks/tables get inner horizontal scroll).
14. Persist window placement.

### SHOULD
15. Search: Ctrl+F, F3/Shift+F3 next/prev (+ F6/Ctrl+N/Ctrl+P legacy aliases) —
    highest-value optional; ship in v1 if renderer can enumerate laid-out text.
16. Zoom: Ctrl+wheel, Ctrl+plus/minus/0, ~50–300 %, persisted (accessibility
    lever; PictView precedent).
17. Space/Backspace next/prev file in panel (host convention; tension with
    browser Space=page-down — host wins, see Q4).
18. Copy Link Address context item.
19. Capped reading measure (~42–50 em, centered; Options toggle "full width").
20. Register `.markdown` too; `.mdown`/`.mkd` = PO taste (vanishingly rare).
21. Optional "Automatic (follow Windows)" scheme entry; DWM dark title bar when
    dark scheme active.

### COULD
22. In-window rendered/source toggle (Ctrl+U). 23. F11 full screen.
24. Rich-text secondary clipboard format. 25. Auto-refresh on file change.
26. Local .md in-place navigation w/ single-level Alt+Left back (never
    Backspace).

## Explicitly deferred beyond v1
Full back/forward history across files; PerMonitorV2 DPI; UI Automation
exposure of rendered content; Windows High Contrast (forced colors) integration
(High Contrast Black scheme is the v1 answer); editing/export/print/Mermaid/
math/AV per brief §16; per-scheme custom color editor.

## Risks
- **R1 — Rendering technology dominates the UX ceiling**: selection, Ctrl+F,
  zoom, accessibility are ~free with a browser component and very expensive in
  a custom renderer. The quality-bar answer must precede engine choice.
- **R2 — Convention collisions**: F3 / Space / Backspace (host conventions must
  win, and be documented).
- **R3 — Scope creep through links** (multi-file nav, history, non-.md files).
- **R4 — 10-scheme contrast QA**: 150+ color decisions; WCAG AA gate required;
  links underlined (not color alone).
- **R5 — Perceived performance**: internal viewer's instant open sets the bar.
- **R6 — Dark scheme in light chrome**: mitigate with DWM dark title bar.

## Open questions (ranked)
1. Selection/find/zoom quality bar → engine class (browser-grade vs
   viewer-grade)?
2. Local .md link behavior in v1: tooltip-only / new window / in-place nav +
   minimal back?
3. Which optional controls are v1 (search=SHOULD-strong, zoom=SHOULD, source
   toggle=COULD, auto-reload=COULD)?
4. Space/Backspace: Salamander convention (recommended) vs reader convention?
5. Remote image default: blocked + per-document "Load remote images" action,
   no global always-allow in v1 (final wording with security agent).
