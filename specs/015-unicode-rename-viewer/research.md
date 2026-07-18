# Research: Unicode Rename Field + Viewer Encoding (Feature 015)

## R1 — Rename field `?` : ANSI dialog window (Part A, fixed)

- The panel (`CFilesBox`) and the in-place quick-rename edit (`viewer`-style
  `CreateExW`) are already Unicode windows, so the panel shows non-ANSI names
  correctly (features 004/005).
- The **Rename dialog** used by F2 (`CFilesWindow::RenameFile` →
  `CCopyMoveDialog`, `IDD_RENAMEDIALOG`, with `Configuration.QuickRenameHistory`)
  places the UTF-8 name into its combo via the wide helpers
  `SalSetWindowTextU8` / `SalComboAddStringU8` (features 004/005). Those call
  `SetWindowTextW` / `CB_ADDSTRING`-W.
- But `CDialog::Execute()` calls the **ANSI** `DialogBoxParam` unless the
  object's `UnicodeWnd` flag is set (`common/winlib.cpp`). `CCommonDialog`/
  `CDialog` default `UnicodeWnd = FALSE`, so the dialog and its controls are
  ANSI windows. A wide `SetWindowTextW` on an ANSI control is down-converted
  through the system ANSI code page → every character outside it becomes a
  literal `?` (emoji = surrogate pair → `??`; each Cyrillic/Greek/CJK letter →
  `?`). This matched the report exactly.
- **Fix**: thread an optional `unicodeWnd` flag through `CCommonDialog` to
  `CDialog` (defaulted FALSE — all other dialogs unchanged) and set it TRUE in
  `CCopyMoveDialog` and `CCopyMoveMoreDialog`; `CEditNewFileDialog` inherits.
  Now Rename/Copy/Move/Edit-New are `DialogBoxParamW` Unicode windows and the
  existing wide helpers work. Committed `1fcde51`.

## R2 — Viewer text pipeline (Part B)

The internal viewer reads the file as **raw bytes** into a 60 000-byte sliding
`Buffer`, optionally applies a 256-entry byte→byte `CodeTable`, and paints one
byte per fixed column via the ANSI `TextOut` (`MyTextOut`). The whole geometry
(EOL scan, seeks `SeekY`/`FirstLineSize`, `LineOffset` triples, selection
`StartSelection`/`EndSelection`, hit-testing `GetOffsetOrXAbs`, search) is
byte-based; there is no UTF-8/UTF-16/BOM notion. A UTF-8 file is therefore
rendered as single-byte → mojibake for any non-ASCII character.

**Design chosen (risk-managed, gated).** Preserve the byte-offset engine
entirely and add a **decode-for-display** layer that activates only when a
Unicode content encoding is detected — so every existing (ASCII/legacy) file is
byte-for-byte unchanged (zero regression), and the Unicode path only touches
files that are broken today.

- **Detection** (`ViewerDetectEncoding`, viewer2.cpp): BOM first
  (UTF-8/UTF-16LE/UTF-16BE); else strict whole-sample UTF-8 validation
  (rejects overlong forms, surrogates, out-of-range; tolerates a sequence
  truncated by the sample boundary); a NUL byte marks the sample binary
  (FR-009). Runs on open in `FileChanged`, independent of the legacy code-page
  auto-select. A positive detection forces text mode and bypasses the
  single-byte `CodeTable`.
- **Decoding** (`ViewerDecodeChar`): one code point → up to two UTF-16 units;
  always makes forward progress (invalid → U+FFFD, 1 byte) and signals an
  incomplete trailing sequence so callers can supply more bytes.
- **Rendering** (viewer.cpp `Paint`): for UTF-8 content the visible line's byte
  segments are decoded and drawn packed with `ExtTextOutW` (`MyTextOutSeg`);
  segment X positions use decoded cell counts (`Utf8SegCells`) so the selection
  highlight aligns with the glyphs.
- **Column hit-testing** (`GetOffsetOrXAbs`): gated code-point stepping (tabs
  stay ASCII; re-Prepare on a code point split at the buffer window edge).
- **Reported encoding** (viewer3.cpp `SetViewerCaption`): shows `UTF-8`
  (or the legacy code-table name) actually used.
- **Manual switch** (FR-008): new `CM_VIEWER_CODING_UTF8` menu item at the top
  of the Coding menu re-decodes without touching disk; choosing any single-byte
  coding (or None) resets to legacy (`SetCodeType`).

### R2a — Scope decision: UTF-8 now, UTF-16 follow-up

UTF-8 preserves the byte-based EOL/seek model (its `\n`/`\r` are single 0x0A/
0x0D bytes; valid UTF-8 has no embedded NUL), so only rendering + column
mapping change — safe and self-contained. **UTF-16** additionally needs the
EOL/line scanner to be 16-bit-aware (its `\n` is `0x0A 0x00`, and every ASCII
char carries a `0x00`), which the byte scanner mis-reads. Rather than render
UTF-16 worse than today, detection deliberately leaves UTF-16 to the existing
path (shown as hex, unchanged) — a documented follow-up. `ViewerDecodeChar`
already handles UTF-16, so the follow-up is the EOL/line scanners only.

### R2b — Known limitations (documented, not silent)

- **Caret/selection precision on lines with multi-byte characters** is
  approximate: the byte-based loop computes selection spans in bytes; rendering
  packs glyphs by cell. Reading is correct; copy/selection of a sub-range on a
  multi-byte line may be off by the multi-byte delta. Pure-ASCII lines are
  exact. (This is the plan's accepted fallback.)
- **Double-width (CJK) glyphs** are placed one cell each (fixed-pitch
  assumption); they render but may visually overlap. European text is exact.
- **UTF-16** — see R2a.
- **CreateDirectory on a long NEW target path** and other external limits are
  unrelated (feature 014).

## R3 — Verification (headless)

- Clean Debug **and** Release x64 builds (Part A `1fcde51`; Part B).
- Reference test files generated at
  `%LOCALAPPDATA%\Temp\salamander-test\010\viewer-encodings\` (UTF-8, UTF-8+BOM,
  UTF-16 LE/BE, Windows-1250, ISO-8859-2, ASCII, invalid-UTF-8, binary) —
  scratchpad `gen-viewer-tests.ps1`.
- Detection reasoning against those files: ref-utf8/ref-utf8-bom → UTF-8;
  ref-win1250/ref-iso8859-2 (not valid UTF-8) → legacy tables; ref-ascii →
  legacy (unchanged); bad-utf8 → legacy (no garbage-forever); binary.bin (NULs)
  → legacy → hex (FR-009); ref-utf16* → legacy/hex (documented follow-up).
- Interactive walkthrough (open each file, switch coding) is the user's final
  step — the environment cannot drive the GUI.
