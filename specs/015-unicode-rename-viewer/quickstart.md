# Quickstart: Unicode Rename + Viewer Verification (Feature 015)

## Part A — Rename field (F2) shows the real name

Test dirs in `%LOCALAPPDATA%\Temp\salamander-test\010\`:
`emoji-🙂-dir`, `Тест-Ελλάδα-测试`, `Můj disk qšěščrč`, `č-dir`.

| Step | Expected |
|------|----------|
| F2 on `emoji-🙂-dir` | edit field shows `emoji-🙂-dir` (not `emoji-??-dir`) |
| F2 on `Тест-Ελλάδα-测试` | field shows the exact name (not `????-??????-??`) |
| Confirm F2 without editing | on-disk name unchanged |
| Type non-ANSI chars + confirm | file renamed to exactly those characters |
| F5/F6 target dialog with a Unicode path | path shown/entered without `?` |

## Part B — Viewer (F3) encoding

Reference files in `%LOCALAPPDATA%\Temp\salamander-test\010\viewer-encodings\`
(regenerate with scratchpad `gen-viewer-tests.ps1`). Reference Czech sentence:
`Příliš žluťoučký kůň úpěl ďábelské ódy.`

| File | Expected |
|------|----------|
| `ref-utf8.txt` | Czech + other scripts render correctly; title shows `[UTF-8]` |
| `ref-utf8-bom.txt` | recognized by BOM; renders correctly; `[UTF-8]` |
| `ref-ascii.txt` | identical to before (legacy path) |
| `ref-win1250.txt` | correct via the Windows-1250 code table (auto or manual coding) |
| `ref-iso8859-2.txt` | correct via the ISO-8859-2 code table (manual coding) |
| `bad-utf8.txt` | not garbage-forever; falls back to legacy/default coding |
| `binary.bin` | shown as **hex**, not auto-forced to text (FR-009) |
| `ref-utf16le/be.txt` | shown as hex (UTF-16 render is a documented follow-up) |
| Coding menu → **UTF-8** on any file | re-decodes as UTF-8 immediately; file on disk unchanged; menu item checked |
| Coding menu → a single-byte table | switches back to legacy; UTF-8 unchecked |

## Regression checks

- ASCII file view: identical to before; search, scroll, PageUp/Down, long
  lines, large file open: unchanged.
- Binary file: hex mode as before.
- ASCII-name rename: identical to before.

## Automated / headless checks (this session)

1. Clean Debug + Release x64 build (Part A `1fcde51`; Part B).
2. Encoding detection reasoning against the 9 reference files (research.md R3).
3. Interactive walkthrough (open each file / F2 each dir / switch coding) is the
   user's final step — the environment cannot drive the GUI.

## Known limitations (documented)

- Caret/selection on a line containing multi-byte characters is approximate
  (reading is correct; sub-range copy may be off). Pure-ASCII lines exact.
- CJK double-width glyphs placed one cell each (may overlap); European text exact.
- UTF-16 rendering deferred (needs 16-bit EOL scanner); currently hex.
