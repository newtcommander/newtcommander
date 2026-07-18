# Agent 6 — Testing & Edge-Case Analysis (raw report, specify phase)

Grounding: `CanViewFile` FALSE ⇒ next viewer in priority list (spl_view.h);
`ViewFile` receives char* path; archive/FS files = temp extracted copy under a
lock event (sibling images absent). Long-path pitfall pattern per specs/014
(fixed MAX_PATH buffers; Unicode 3× byte length; test tree
`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`, 291-char ASCII +
~540-byte Unicode). Local SFTP test server exists (Docker atmoz/sftp,
localhost:2222, sftptest/sftptest123, feature 017).

## Acceptance test catalog (Setup → Action → Expected; AC-n = brief §17)

### A — Basic rendering
- **TC-A01** (AC-1,2): `01-basic.md`, F3 → mdview opens (not text viewer/
  Explorer); rendered output, no visible `#`/`*`/backtick source markers.
- **TC-A02** (AC-4): H1–H6 distinct; paragraph spacing; `---` = rule;
  double-space = in-paragraph break.
- **TC-A03** (AC-4): bold/italic/both/strike/inline code render; `\*not
  italic\*` literal; entities `&amp;`→&, `&copy;`→©, `&#269;`→č.
- **TC-A04** (AC-4): `02-lists.md` — ordered start at 7 numbers 7,8,9;
  4-level nesting indents; task checkboxes drawn, **non-interactive**.
- **TC-A05** (AC-4,5): `03-blocks.md` — nested quotes (left border per
  level); fenced c/cpp/js/python/json/xml/bash colorized; unknown `zzz` +
  untagged = plain mono, no error; fence content verbatim.
- **TC-A06** (AC-4): `04-tables.md` — alignments honored; ragged row padded;
  over-wide table contained (no paint outside client area).
- **TC-A07** (AC-3): `07-czech-utf8.md` (no BOM) — all diacritics correct
  everywhere (headings/body/table/code/link text), no mojibake/`?`.
- **TC-A08** (AC-3): `08-utf8-bom.md` — identical to A07; BOM invisible;
  first heading still parsed.
- **TC-A09** (AC-11): `16-html.md` (script, img-onerror, iframe, object,
  embed, autosubmit form, SVG-script, javascript: href) — 30 s idle + click
  everything: no execution, no network, no navigation; HTML per spec policy;
  javascript: dead with indication.

### B — Images
- **TC-B01** (AC-6): relative `assets/architecture.png` displays, correct
  aspect.
- **TC-B02** (AC-6): `sub dir/with space.png`, Unicode `óbrázek-žluťoučký.png`,
  `../shared/up.png` all load; resolution relative to the .md directory.
- **TC-B03**: one of each PNG/JPEG/GIF/SVG/WebP/BMP renders — or the defined
  placeholder if a format is excluded; never crash. Animated GIF: first frame
  minimum.
- **TC-B04** (AC-8): `missing.png` alt "Diagram" → in-flow placeholder with
  alt + "not found"; layout intact.
- **TC-B05** (AC-8): truncated `corrupt.png` → placeholder + decode-error;
  no crash/hang.
- **TC-B06** (AC-7): `wide-4000x500.png` + `huge-20000x20000.png` in ~1200 px
  window → downscaled to content width, aspect preserved; resize re-fits;
  20000² must NOT materialize ~1.6 GB bitmap (PR-6).
- **TC-B07** (§6 decision): `https://example.com/logo.png` — 30 s idle: **no
  network request on load** (verify via unroutable host = no stall, or local
  proxy/netstat); placeholder per policy.

### C — Links
- **TC-C01** (AC-9): TOC anchor click scrolls to "Section 9"; diacritic
  heading anchor resolves; duplicate-heading rule documented; missing anchor
  = harmless no-op/notice.
- **TC-C02** (AC-10): 30 s idle = nothing opens; links visually distinct
  (color + underline); click → system browser; viewer stays open.
- **TC-C03**: mailto: → mail client; javascript:/vbscript: blocked with
  indication; unknown `foo://bar` → blocked notice; file: per spec (blocked/
  confirm).
- **TC-C04**: `[readme](sub/other.md)` and `[data](data.csv)` behave per the
  consolidated decision, deterministically; if deferred → clear "not
  supported" notice, never silent wrong action.

### D — Schemes & persistence
- **TC-D01** (AC-12): picker (mouse AND keyboard path) lists ≥10 schemes
  (≥5 light, ≥5 dark, §10 names).
- **TC-D02** (AC-13): 1 MB doc @50 % scroll; switch all 10 → immediate
  (<~300 ms), no reopen, scroll unchanged; never blank/crash.
- **TC-D03**: scheme→close→F3 same session → retained.
- **TC-D04** (AC-14): scheme→exit app normally→relaunch→F3 → retained (config
  written on normal exit).
- **TC-D05** (AC-15): corrupt stored value 3 ways across 3 runs — (a) unknown
  name, (b) wrong registry type (REG_BINARY garbage), (c) deleted value →
  default scheme every time; no crash; no dialog storm; self-heals on next
  change.
- **TC-D06**: kill salamand.exe before save → next run: any valid scheme,
  no crash.

### E — Controls
- **TC-E01** (AC-16): Down/Up/PgDn/PgUp/End/Home behave; Esc closes; focus
  returns to panel with original file selected.
- **TC-E02**: wheel per system scroll-lines; Ctrl+wheel zooms or does nothing
  harmful.
- **TC-E03** (AC-17): drag-select across heading+bold+code → visible
  highlight; Ctrl+C → Notepad paste = text content (plain text min); Ctrl+A;
  Czech chars survive round-trip.
- **TC-E04** (§15): resize narrow (~400 px)/wide/maximize; 100 %→150 % DPI
  monitor → reflow, no clipping, fonts scale, images re-fit, no blurry text.
- **TC-E05**: two instances (file A + B) — independent state, any close
  order, no cross-talk beyond persisted scheme.

### F — Robustness
- **TC-F01** (AC-18): `17-malformed.md` (unclosed fence at EOF, broken table
  row, unbalanced `**`, unclosed bracket) → best-effort render (CommonMark
  defines all); no crash/hang/error dialog.
- **TC-F02** (AC-18): `18-pathological-nesting.md` (10k quotes + 10k list) →
  opens or aborts within budget (PR-7 ≤5 s); UI responsive; NO stack overflow
  (verify in Release — feature-014 lesson).
- **TC-F03** (AC-18): `19-pathological-emphasis.md` (~100 KB a*b*c*…),
  `20-pathological-brackets.md` (50k `[` + 50k `]`) → linear-time; within
  budget; Esc always works.
- **TC-F04** (AC-18): `22-binary-garbage.md` (64 KB of EXE/ZIP) → no crash;
  either CanViewFile declines → cascade to text/hex viewer, or opens with
  "not text" notice + open-as-text path.
- **TC-F05** (AC-19): see Fallback verification.
- **TC-F06** (AC-20): 50× F3→render→Esc cycle over 4 fixtures (incl. images —
  classic GDI leak source); baseline vs end (10 s settle): GDI +10, USER +10,
  handles +20, private bytes +5 MB max; spot-check at 10/25/50; no orphan
  windows/processes.
- **TC-F07**: close main window with viewer open → follows app-wide viewer
  policy; no crash; config still saved.

## Edge-case inventory (case → expected)

E01 nonexistent file → clear error naming file; no crash. E02 access denied →
distinct message (icacls /deny test). E03 exclusive lock → open with read
share if possible; else clean "file in use". E04 empty file → empty document,
scheme bg, not an error. E05 binary .md → TC-F04. E06 invalid UTF-8 → defined
fallback per §9 (replacement or documented legacy decode), notice, text-viewer
option, no crash. E07 UTF-16 LE/BE, CP1250, ISO-8859-2 → per encoding-scope
decision: correct decode (if in scope) or graceful degraded + fallback offer;
UTF-16 BOM never renders as garbage. E08 BOM/no-BOM identical. E09 CRLF/LF/CR
mixed identical; lone CR mid-line harmless. E10 malformed → best-effort.
E11 10k nesting → bounded, NO stack overflow (Release build check). E12
pathological emphasis → linear time. E13 missing image → placeholder+alt.
E14 corrupt image → placeholder+decode error contained. E15 unsupported
format → placeholder consistent with spec list. E16 20000² image → scaled
decode (never ~1.6 GB); responsive. E17 changed on disk → snapshot stays;
refresh re-renders preserving position; no tearing. E18 deleted while open →
snapshot stays; refresh reports missing gracefully. E19 >260-char path →
works (291-char ASCII + ~540-byte Unicode trees); NO new fixed MAX_PATH
buffers (spec-014 defect class); relative image resolution from long base
must work. E20 Unicode filename (Czech/CJK/emoji) → opens, title correct
(features 005/010/015 bug class); `ViewFile` char* encoding convention must
be verified vs 015 viewer fix — high-risk seam; surrogate-safe. E21 .md in
ZIP → temp-copy view works; relative images legitimately missing →
placeholders (correct result); lock honored; close releases temp. E22 .md on
SFTP → same temp semantics; no image network attempts; test vs local Docker
server. E23 renderer internal error → contained; error surface +
text-viewer path. E24 OOM/too large → refuse with message + open-as-text;
mid-render alloc failure aborts cleanly. E25 read-only file/medium → opens
normally (guards accidental write-open).

## Measurable performance/robustness criteria (Release x64; F3-keypress →
first paint; recommend debug-trace parse+layout timestamps)

- **PR-1**: typical README 10–50 KB: first paint < 500 ms, target 200 ms
  (F3 = "instant" in this app; parse is sub-ms — budget is window+layout).
- **PR-2**: 1 MB doc: first paint < 2 s; fully navigable (End works) < 4 s.
- **PR-3**: 10 MB: opens < 10 s with UI responsive OR size gate triggers
  (proposed gate default **20 MB** → "too large — open as text?"); UI thread
  never blocked > 1 s without feedback; Esc aborts anytime.
- **PR-4**: scrolling 1 MB doc: no hitch ≥ 100 ms (hold PgDn 5 s + thumb-drag
  end-to-end; eye check, PresentMon/ETW if disputed); no blank flashes.
- **PR-5**: memory: peak private-bytes delta ≤ max(10× file size, 50 MB) for
  conforming docs; hard abort + error surface at 512 MB.
- **PR-6**: image decode off UI thread / bounded; 20000² never materializes
  full bitmap (scaled decode ≤ 2× viewport width); per-image decode budget
  ~2 s → placeholder.
- **PR-7**: pathological-input watchdog: parse+layout abort at 5 s (or
  deterministic iteration/depth cap) → error + text-viewer fallback; UI
  responsive (window movable, Esc closes) during attempt.
- **PR-8**: 50× open/close: GDI +10 / USER +10 / handles +20 / private
  +5 MB max drift.
- **PR-9**: scheme-switch repaint < 300 ms on 1 MB doc (supports AC-13).

## Fallback verification (AC-19)

Two routes, both specified and tested:
1. **Pre-open decline** (`CanViewFile` FALSE) → next viewer in priority list.
   Test: priority = mdview, then internal text viewer for `*.md`; open the
   decline fixture → text viewer opens seamlessly.
2. **In-viewer failure** → human-readable error naming file + reason
   (encoding/renderer/too-large/OOM) **plus explicit "view as plain text"
   action** (in-place source or internal text viewer on same path). Triggers:
   `12-invalid-utf8.md` (if routed to error), above-gate file, and a
   **debug-only forced-failure hook** (env var e.g. MDVIEW_TEST_FAIL_RENDER=1
   or magic fixture marker) — needs owner confirmation (Q2). Pass: specific
   error text (not "0x80004005"), fallback shows identical content, Esc
   works, no leaked resources (PR-8 spot-check).
3. **Negative check**: merely malformed Markdown must NOT trigger the error
   surface (best-effort contract).

## Test-asset pack

Committed under `specs/020-mdview-plugin/fixtures/`: 01-basic, 02-lists,
03-blocks, 04-tables, 05-links (TOC + diacritic/duplicate headings +
relative .md + other-file + http + bare URL + mailto + javascript: + file: +
unknown scheme), 06-images/ (+assets: PNG/JPEG/GIF/SVG/WebP/BMP, space path,
Unicode name, ../ ref, missing ref, corrupt.png, wide-4000x500), 07-czech-utf8,
08-utf8-bom, 09-utf16le, 10-utf16be, 11-cp1250, 12-invalid-utf8, 13-crlf,
14-lf, 15-mixed-eol, 16-html, 17-malformed, 21-empty.

Generated by checked-in `gen-fixtures.ps1` (deterministic; repo stays small):
18-pathological-nesting (10k quotes + 10k list), 19-pathological-emphasis,
20-pathological-brackets, 22-binary-garbage, 23-large-1MB, 24-large-10MB,
25-oversize-gate, huge-20000x20000.png.

Deployment sets (script-produced): fixtures.zip (E21); SFTP upload to
localhost:2222 (E22); long-path copies into the existing 291-char/540-byte
trees (E19); Unicode-name copies `příliš-žluťoučký.md`, `日本語ドキュメント.md`,
`🚀-doc.md` (E20); icacls-deny permission fixture (E02, removed after run).

## Open questions (ranked)

1. Large-document gate number (proposed 20 MB) + must 10 MB render fully or
   may it be gated?
2. Fallback authority (CanViewFile decline vs in-window error surface) + is
   the debug-only forced-failure hook acceptable? (Without it AC-19 is only
   partially verifiable.)
3. Perf targets binding vs advisory (PR-1/2/4) + is progressive/async
   rendering in scope for v1?
4. Remote images default (decides TC-B07 + no-network assertions).
5. Encoding scope beyond UTF-8: must fixtures 09–11 decode correctly, or
   only not-crash with clean fallback?

**Consolidator flag**: the two most recently broken seams in this codebase are
(a) the char*-path encoding convention for Unicode names crossing the viewer
API (features 005/010/015) and (b) fixed-size path buffers (011–014). Both
must be explicit requirements in the spec.
