# Quickstart (Phase 1): mdview

## Build

1. Ensure `plugins.cfg` (repo root) contains `mdview=on`.
2. From repo root:
   ```
   build.cmd              :: Debug x64 incremental (compiles mdview + lang_mdview)
   build.cmd full         :: also regenerates plugins.ver so mdview auto-registers
   build.cmd full release :: Release x64
   ```
   Output: `build\newtcommander\<cfg>_x64\plugins\mdview\mdview.spl` (+ `english.slg`).

   Note: the Release `salamand.exe` link fails with `LNK1104` while a built
   Salamander is running (feature-019 pitfall) — close it first. Debug uses a
   separate path and is unaffected.

## First run / registration

- A `build.cmd full` writes `plugins.ver`; on next start Salamander auto-installs
  mdview → it appears in **Plugins → Plugin Manager**, and its first install
  registers the viewer masks `*.md;*.markdown` (top priority).
- If testing a plain (non-full) build against an existing config, add mdview once
  via Plugin Manager → Add, or drop the `.spl` into the plugins dir.

## Smoke test (P1)

1. In a panel, select a `.md` file (use `specs/020-mdview-plugin/fixtures/`
   once created) and press **F3** → the mdview window opens with rendered
   Markdown (headings styled, lists, code blocks shaded, no visible `#`/`*`).
2. **Esc** closes; the panel regains focus with the file still selected.
3. **Alt+F3** on the same file → internal text viewer shows raw source.
4. **View → Color Scheme** → pick a dark scheme → applies immediately, scroll
   kept; close + reopen → same scheme (persisted).
5. **Ctrl+F** find; **F3/Shift+F3** next/prev; **Ctrl+wheel** / **Ctrl+±/0**
   zoom; **Ctrl+C** copies plain text.

## Security spot checks (P1)

- Open a fixture containing `<script>`, `javascript:` links, and a remote image
  ref; idle 30 s and click around → no execution, no navigation, **no network**
  (verify with an unroutable remote host = no stall). Remote images show a
  placeholder until "Load remote images" is chosen.

## Robustness spot checks

- Binary file renamed `.md` → text/hex viewer takes over (CanViewFile declined)
  or an explicit "not text / open as text" state; never a crash.
- Pathological nesting fixture → UI stays responsive; Esc works.
- Corrupt the `ColorScheme` registry value → next open uses the default scheme,
  no crash.

## Fixtures

Hand-written fixtures + a `gen-fixtures.ps1` generator live under
`specs/020-mdview-plugin/fixtures/` (see analysis/testing.md for the full list).
For v1 smoke testing, `01-basic.md`, `03-blocks.md`, `07-czech-utf8.md`, and
`16-html.md` are the minimum.
