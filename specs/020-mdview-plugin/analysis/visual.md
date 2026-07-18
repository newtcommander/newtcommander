# Agent 5 — Visual Design & Color Schemes Analysis (raw report, specify phase)

Independent analysis. Ratios = WCAG 2.x contrast for indicative anchors (final
hex = design phase); anchors prove each concept can meet targets.

## Scheme-by-scheme validation

**Light**
1. **Paper** — OK, zero risk. ~#FFFFFF bg / ~#333 text (≈12.6:1), classic blue
   links; the "GitHub light"; safe default light.
2. **Soft Gray** — OK. ~#F2F3F5 / ~#2B2B2B (≈11.9:1). Risk: code-block bg can
   vanish on gray page → require visible bg delta or 3:1 border.
3. **Warm Sepia** — OK. ~#F6EEDD / ~#4A3B2C (≈9–10:1). Risk: warm accents trend
   low-contrast; links stay darkened blue/teal, not brown; borders ≥3:1.
4. **Solar Light** — viable **only as darkened derivative**: stock Solarized
   body #657B83 on #FDF6E3 ≈3.9:1 **fails AA**; stock yellow #B58900 ≈2.9:1.
   Mandate body ≥ ~#586E75 (recommend ~#4A5F66) + darkened accents; flag in
   spec so design doesn't copy stock values.
5. **Arctic Light** — OK. ~#ECEFF4 / ~#2E3440 (≈10.8:1). Risk: Nord frost cyan
   #88C0D0 as link ≈1.9:1 — catastrophic; mandate dark blue link
   (~#205081–#3B6398).

**Dark**
6. **Graphite** — OK, safe default dark. ~#2B2B2B / ~#D6D6D6 (≈9.5:1),
   light-blue links. Token palette must be deliberate (all-neutral base).
7. **Midnight** — OK. ~#101A2B / ~#C9D4E5 (≈11:1). Risk: saturated blues sink
   into navy — links light (#7FA8FF+); blue-tinted borders need 3:1 checks.
8. **Solar Dark** — viable **only as brightened derivative**: stock body
   #839496 on #002B36 ≈4.7:1 borderline (mandate ≥ #93A1A1 ≈5.8:1); stock
   comment #586E75 ≈2.9:1 **fails** (classic dim-comment defect). Forbid stock
   values for body + comments.
9. **Nordic Dark** — OK. #2E3440 / #D8DEE9 (≈9.3:1); frost accents pass
   (6–7:1). Risk: stock Nord comment #4C566A ≈1.9:1 — mandate lightened
   (~#8590A6+).
10. **High Contrast Black** — OK, trivially AAA. #000/#FFF (21:1); bright
    tokens (yellow ≈19.6:1, cyan ≈16.7:1). Ban pure blue #0000FF (2.4:1);
    links bright cyan underlined; heavier borders/underlines (2 px @96 DPI).

Verdict: all 10 sound; **Solar Light, Solar Dark, Arctic Light (links),
Nordic Dark (comments), Midnight (links)** carry named contrast risks the spec
must call out.

## Mandated color roles (every scheme defines all)

1. Document background; 2. Body text; 3. Heading text (base; optional per-level
overrides); 4. Link; 5. Link hover/active; 6. Visited link (*optional*;
fallback = Link); 7. Blockquote text + accent (bar/tint); 8. Inline code fg +
bg; 9. Code block bg + default text; 10. Table border + header-row bg;
11. Horizontal rule; 12. Selection bg + fg; 13. **Keyboard focus indicator**
(missing from the brief's own list; required by §15 keyboard control);
14. Image-error placeholder text + border; 15. Syntax token set (9 classes).

Explicit non-roles: scrollbars, menu bar, toolbars, dialogs — standard
system-drawn per constitution VI; schemes color **only the document canvas**.

## Measurable contrast criteria (unit-testable: iterate schemes × roles with
the WCAG relative-luminance formula)

- **SC-C1**: body text vs bg ≥ 4.5:1 in all 10 schemes.
- **SC-C2**: headings ≥ 3:1 at large size (≥ ~24 px or ≥ ~18.7 px bold);
  body-size headings 4.5:1.
- **SC-C3**: links underlined in every scheme (more than color alone) AND
  ≥ 4.5:1 vs bg.
- **SC-C4**: selected text vs selection bg ≥ 4.5:1 AND selection bg vs
  document bg ≥ 3:1.
- **SC-C5**: code text ≥ 4.5:1 vs own bg; code region distinguishable from
  document bg (visible delta or ≥3:1 border).
- **SC-C6**: non-text elements (table borders, rules, blockquote bars, focus
  indicator) ≥ 3:1 vs adjacent bg (WCAG 1.4.11).
- **SC-C7**: every syntax token ≥ 4.5:1 vs code-block bg; **comments
  explicitly included**.
- **SC-C8**: High Contrast Black: every text role + token ≥ 7:1 (AAA).
- **SC-C9**: scheme switch immediate, no reopen, scroll preserved.
- **SC-C10**: corrupted/unknown persisted value → default scheme; no crash,
  no error dialog.

## Syntax token classes (9)

keyword, string, number, comment, type/class, function, operator/punctuation,
diff-added, diff-removed. Code-block default text = fallback for unclassified.
Comments must remain readable — forbid the dim-gray failure; stock Solarized
Dark (#586E75) and stock Nord (#4C566A) comment values fail and must not be
copied. Diff add/remove differ by more than hue (keep +/- gutter markers).

## Picker UX + persistence (house-style compliant)

- **In-viewer picker (primary)**: demoview pattern — rebar +
  `CGUIMenuBarAbstract` from `MENU_TEMPLATE_ITEM` arrays via
  `SalamanderGUI->CreateMenuBar` (demoview/menu.cpp:24–65; viewer.cpp:559,
  rebar ~487, SetFont refresh 864). **View → Color Scheme** submenu, 10
  radio-checked items (light group, separator, dark group) + checked toggle
  "Follow system theme". Mouse + keyboard for free; instant apply on
  WM_COMMAND (repaint only, scroll preserved). Optional cycle hotkey. Plain
  text items (localizable via .slg); no owner-drawn swatches.
- **Optional config page**: exact house template — `DIALOGEX`, `STYLE
  DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD | WS_CAPTION`, `FONT 8,
  "MS Shell Dlg"` (demoview IDD_CFGPAGEVIEWER, lang.rc:61–70; SFTP
  IDD_CONFIG, lang.rc2:226). Standard themed ComboBox. Constitution VI: no
  ICC_STANDARD_CLASSES, no manifest, no subclass/owner-draw restyling; a
  preview pane only as the plugin's OWN window class painting itself. Dark
  schemes affect the document canvas only — menu bar/scrollbars/dialogs stay
  standard; write into the spec as intended behavior.
- **Persistence**: `LoadConfiguration`/`SaveConfiguration(HWND, HKEY, ...)`
  with Version DWORD guard (demoview.cpp:214–237; mmviewer.cpp:429–454
  persists REG_BINARY LOGFONT — precedent for zoom/font persistence). Values:
  `Version` (DWORD), `ColorScheme` (REG_SZ **stable ASCII identifier** —
  "paper", "graphite"… NOT localized name, NOT integer index — index coupling
  breaks on reorder; feature-007 pitfall class), `FollowSystemTheme` (DWORD
  0/1), `SchemeLight`/`SchemeDark` (REG_SZ, last-used per polarity for auto
  mode). Any missing/unknown/unparsable → that value's default; never crash.
- **Defaults + auto mode**: default light = **Paper**, default dark =
  **Graphite** (the two zero-risk neutrals). Auto ("follow OS theme" — the
  app has no theme of its own; only OS `AppsUseLightTheme` exists): default
  **off**. When on: OS state selects SchemeLight/SchemeDark; manual pick
  while auto-on updates the matching polarity slot and keeps auto on; react
  live to WM_SETTINGCHANGE "ImmersiveColorSet". Auto supplements, never
  replaces, the 10 schemes.

## Typography / layout requirements (roles, not concrete fonts)

- Proportional body face; monospaced code face; sizes derived from one base.
- Heading hierarchy distinct by size+weight (H1 ≈2.0×, H2 ≈1.5×, H3 ≈1.25×,
  H4–H6 1.0× bold), never color alone; H1/H2 may carry bottom rule.
- Max content width: cap reading measure (~70–90 chars / ~900 px @96 DPI) on
  wide windows — owner confirmation (Q3); tables/code may exceed with
  element-scoped horizontal scroll.
- Images: max-width = content width, aspect preserved, no upscale beyond
  natural size by default; failed image → placeholder meeting SC-C6.
- DPI: all sizes/margins/rules/underlines scale with window DPI; no hardcoded
  px; follow the host's existing DPI model (constitution VI — changing it is
  app-wide, not this plugin's call).
- Zoom (if accepted): scales typography uniformly; contrast requirements
  zoom-invariant.

## Open questions (ranked)

1. Follow-OS-theme auto mode: include in v1 with default off — confirm.
2. Default schemes: confirm Paper (light) + Graphite (dark).
3. Max content width: capped (~900 px @96 DPI, DPI-scaled) vs full width;
   if capped — fixed or toggleable?
4. Visited-link state: drop for v1 (recommended; scope creep) or per-session?
5. Dark-scheme chrome: confirm intended behavior = dark canvas inside
   standard light chrome (constitution VI).
