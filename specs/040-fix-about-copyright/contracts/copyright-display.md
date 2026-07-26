# Contract: Copyright Notice Display

**Feature**: 040-fix-about-copyright
**Type**: UI contract (desktop application surfaces)

This is the contract every surface that shows the product's copyright must
honour after this feature. It is the reference an implementer, a reviewer, or a
future translator should be able to check against without reading the code.

## C-1 — The two literals

Exactly two notices exist for display. They are fixed English text, defined once
at build time, and are **never** routed through the translation pipeline.

| Slot | Literal (exact bytes) |
|------|-----------------------|
| First (upper) | `Copyright © 2026 Newt Commander Authors` |
| Second (lower) | `Copyright © 1997-2026 Open Salamander Authors` |

The `©` is U+00A9. Source files are UTF-8-BOM.

## C-2 — Order

On every surface showing both notices, the Newt Commander line is above the
Open Salamander line. No surface may show them in the opposite order.

## C-3 — Surfaces

| Surface | Line 1 control/target | Line 2 control/target | Mechanism |
|---------|----------------------|----------------------|-----------|
| About dialog (`IDD_ABOUT`) | `IDC_STATIC_1` @ `10,97,196,8` | `IDC_STATIC_2` @ `10,108,196,8` | `SetDlgItemText` in `WM_INITDIALOG` |
| Splash screen (`IDD_SPLASH`) | `IDC_SPLASH_COPYRIGHT` @ `8,73,237,8` | `IDC_SPLASH_COPYRIGHT2` @ `8,83,237,8` | `PaintText` into the background bitmap |

Both surfaces read the same two constants. There is no third source of the
notice text.

## C-4 — Localization

- The displayed bytes MUST be identical under every user-interface language.
- The dialog template still comes from the active language module, so the
  *controls* are localizable — their **captions are empty** in the English
  resource and in every translation archive, and the running code supplies the
  text.
- A translation archive containing text for these controls MUST have no effect
  on what is displayed.

## C-5 — What must NOT change

| Item | Required state |
|------|----------------|
| `LegalCopyright` version-resource value | `Copyright © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors` — byte-identical to before this feature |
| `IDD_ABOUT` control set | Twelve controls, unchanged IDs, unchanged geometry, unchanged dialog size `299x184` |
| `IDD_ABOUT` style | `DIALOGEX`, `DS_SETFONT \| DS_FIXEDSYS`, `FONT 8, "MS Shell Dlg"` (Constitution VI) |
| Row count of every `salamand.slt` | Unchanged — the import is strictly positional |
| Encoding of every edited file | UTF-8 with BOM, CRLF line endings |
| Plugin `VERSINFO_COPYRIGHT` defines | Untouched — plugins are out of scope |

## C-6 — Rendering

- Neither line may be clipped, wrapped, truncated with an ellipsis, or
  overlapped by another control, in any shipped language.
- Both lines must be legible in the light and the dark theme. The About dialog
  achieves this through the dialog's existing `WM_CTLCOLORSTATIC` handler; the
  splash screen is always drawn on the brand dark background.
- The `©` must render as `©` — not as a substituted or garbled glyph — including
  under a CJK or Cyrillic user-interface language.

## C-7 — Maintenance rule

When the copyright year advances, three definitions in `src/versinfo.rh2` move
together:

- `VERSINFO_COPYRIGHT` (metadata)
- `VERSINFO_COPYRIGHT_NEWT` (display, line 1)
- `VERSINFO_COPYRIGHT_OPENSAL` (display, line 2)

No translation file is involved, and no other file needs editing for the
notice to update on both surfaces.

## Verification

| Contract | How to check |
|----------|--------------|
| C-1, C-2, C-3 | Open the About dialog and observe the splash on startup |
| C-4 | Repeat under each enabled language; bytes must match English |
| C-5 (LegalCopyright) | File properties → Details → Copyright on `newtcommander.exe` |
| C-5 (archives) | Row count and BOM/CRLF assertion after the edit; full build succeeds |
| C-6 | Visual check, light and dark theme |
