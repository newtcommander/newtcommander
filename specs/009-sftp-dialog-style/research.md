# Phase 0 Research: Consistent SFTP Plugin Dialog Appearance

**Feature**: 009-sftp-dialog-style
**Date**: 2026-07-17

## Problem restatement

SFTP plugin text-input (EDIT) controls render with the Windows 11 "modern" focused-state
decoration (an accent-colored line along the bottom edge) while the FTP plugin and the core
application render the classic themed border around the whole field. Both run inside the
same `salamand.exe` process (which activates comctl32 v6 via its embedded manifest). The
user reports the divergence persists on a clean build.

## Key finding — the initial hypothesis is not confirmed

An earlier analysis identified `ICC_STANDARD_CLASSES` (passed to `InitCommonControlsEx`
only by the SFTP plugin) as the differentiator, on the theory that it makes comctl32 v6
superclass the standard `Edit` window class process-wide, giving Windows 11 modern
rendering.

A repo-wide search performed for this plan shows that, after the initial fix, **no module
loaded into `salamand.exe` passes `ICC_STANDARD_CLASSES`**:

- Only occurrence in the tree is `src/translator/translator.cpp:576` — a separate
  build-time translation tool, not loaded into the file-manager process.
- The shared WinLib used by the FTP plugin registers
  `ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_COOL_CLASSES`
  (`src/plugins/shared/winliblt.cpp:86`) — no standard classes.
- The core registers `ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES |
  ICC_COOL_CLASSES | ICC_DATE_CLASSES | ICC_USEREX_CLASSES`
  (`src/salamdr1.cpp:3965`) — no standard classes; core WinLib calls plain
  `InitCommonControls()` (`src/common/winlib.cpp:61`).
- The SFTP plugin no longer passes it (`src/plugins/sftp/sftp.cpp:101`, fixed on this
  branch).

**Implication**: if the user's clean build contained the fix, then removing
`ICC_STANDARD_CLASSES` alone did NOT eliminate the modern decoration — so it was not the
sole/true cause. Two possibilities remain and MUST be distinguished before designing the
final fix:

- **(a)** the tested clean build did not actually contain the fix (stale/other branch/DLL
  not reloaded), or
- **(b)** the true runtime trigger is a different factor.

**Decision D1**: Do not proceed on the unverified hypothesis. The first implementation task
is an empirical root-cause spike (below). Rationale: the user's report has already
falsified one plausible-looking fix; shipping another unverified change risks a second
non-fix.

## Candidate root-cause factors (to test in the spike)

| # | Factor | Why it could flip EDIT focus rendering |
|---|--------|----------------------------------------|
| 1 | `ICC_STANDARD_CLASSES` registration | comctl32 v6 superclasses `Edit` process-wide; order-dependent (controls created after registration render modern). |
| 2 | DPI-awareness context at control creation | Windows 11 applies modern common-control visuals under Per-Monitor-V2; legacy visuals under System-aware. Process manifest is `dpiAware=true` (System-aware); must confirm no per-thread override reaches SFTP dialogs. |
| 3 | Dialog creation path | FTP dialogs go through the shared WinLib `CDialog` framework; SFTP uses raw `DialogBoxParam`. Confirm neither applies/omits a theme or activation context that flips rendering. |
| 4 | Activation context / comctl32 version resolved at creation | Determines whether v5 vs v6 (and which v6 build) handles the `Edit` class. |
| 5 | Theme actually applied to the control | Compare `GetWindowTheme`/theme class on an SFTP edit vs an FTP edit at runtime. |
| 6 | Font / `DS_SHELLFONT` side effects | Now aligned (`DS_FIXEDSYS` added); confirm it does not affect focus decoration (expected: no effect). |

## Empirical spike (the method that resolves D1)

1. Build clean from this branch (fix present). Confirm the loaded `sftp.spl` contains no
   `ICC_STANDARD_CLASSES` path (source is authoritative; optionally verify the loaded DLL
   is the freshly built one).
2. Launch Salamander; open an SFTP dialog with an edit field and an equivalent FTP/core
   dialog; give each edit keyboard focus; compare the decoration side by side.
3. **If SFTP now matches FTP/core** → the fix works; the earlier tested build simply lacked
   it. Remaining work is deployment/build confirmation, order-independence check, and the
   durability record (already added).
4. **If SFTP still differs** → instrument dialog init to log, for the edit control in both
   an SFTP and an FTP/core dialog: the thread DPI-awareness context
   (`GetThreadDpiAwarenessContext`) and the applied theme (`GetWindowTheme` handle / theme
   class name). The factor that differs between the two is the trigger; fix by removing that
   difference.

## Decisions

- **D1** (above): root cause resolved empirically as implementation Task 1.
- **D2 — Fix strategy = remove the divergence, not mask it.** Make SFTP create its dialogs
  and controls under conditions identical to FTP/core (e.g. matching plugin-init flags,
  matching creation path, or matching DPI/theme context — whichever the spike identifies),
  rather than adding SFTP-only theming overrides. Rationale: constitution principle VI
  forbids per-plugin theming hacks and process-wide side effects; alignment is more robust
  and maintainable. *Alternative considered*: force a specific theme on each SFTP edit via
  `SetWindowTheme` / `SalamanderGUI->DisableWindowVisualStyles` — rejected as a masking hack
  that could over-correct (un-theme the field so it no longer matches FTP/core); keep only
  as a last-resort fallback if alignment proves infeasible.
- **D3 — Durability via documentation, no tooling.** Consistency for future extensions is
  guaranteed by constitution principle VI (already added) upheld through code review. No
  automated check or visual-regression test (per spec clarification 2026-07-17).
- **D4 — Verification is manual and includes order-independence.** Side-by-side human
  comparison on a clean build (per clarification); explicitly test that using SFTP first
  does not modernize subsequently opened FTP/core dialogs and vice-versa (spec US2).

## Spike results (2026-07-17)

An isolated Win32 harness (`editprobe.c`, session scratchpad) was built with the same
comctl32 v6 dependency as the host, creating a focused single-line EDIT and capturing it,
while toggling `ICC_STANDARD_CLASSES` (0/1) and the DPI-awareness context
(unaware/system/pmv2).

- **Reproducible (focused-but-inactive) state**: captured via `PrintWindow`
  (`PW_RENDERFULLCONTENT`). Every combination rendered a **classic themed EDIT** (uniform
  thin border, no accent underline) — byte-identical across all `ICC_STANDARD_CLASSES` and
  DPI variants. → In the reproducible state, **`ICC_STANDARD_CLASSES` and DPI mode have no
  effect** on the EDIT decoration.
- **Active-window state not reproducible headlessly**: the Windows 11 accent-underline
  appears only when the top-level window is the *active foreground* window with the field
  focused. A background-launched process could not take the foreground (`isFG=0` even after
  `AttachThreadInput` + disabling the foreground-lock timeout), and a screen `BitBlt` then
  captured the desktop rather than the window. So the harness could neither reproduce nor
  measure the active-focus accent that the user observes.

**Interpretation**: the spike **refutes `ICC_STANDARD_CLASSES` / DPI as the driver in the
reproducible state**, but is **inconclusive for the active-focus accent** (the exact state
the user sees), because that state cannot be reproduced in a headless harness. Note also
that FTP and SFTP create their EDIT controls via an equivalent path — same `DialogBoxParam`
API, same module handle (`HLanguage`), same `DIALOGEX`/`EDITTEXT` template style — so no
source-level divergence remains beyond the (now-fixed) font.

**Consequence for the fix**:
- The `DS_SHELLFONT` font alignment is a **confirmed, correct** change (the font mismatch
  was real and is resolved; build-verified).
- Removing `ICC_STANDARD_CLASSES` is **retained** because it is required by constitution
  principle VI (no per-plugin process-wide standard-class registration), aligns SFTP init
  with FTP/core, and is the best-supported hypothesis for the accent (comctl32-v6
  superclassing effects can be active-state-specific, which the harness cannot observe). It
  is **not empirically confirmed** to fix the accent.
- **Most likely explanation for "still inconsistent on a clean build"**: the reported build
  did not include this branch's fix (the change lives only on `009-sftp-dialog-style`,
  unmerged; a `build.cmd rebuild` on `main`/`008` still has `ICC_STANDARD_CLASSES`).
- **Remaining confirmation is a live visual check** on a build from this branch (open the
  SFTP connect dialog and an FTP/core dialog, focus a field, compare) — a human/active-window
  step that could not be completed headlessly.

## Resolved unknowns

- "How is consistency enforced going forward?" → constitution principle VI + review (D3).
- "What is the acceptance-defining difference?" → focused EDIT decoration (spec
  clarification / FR-002).
- "What is the exact runtime trigger?" → to be confirmed by the spike (D1); the fix follows
  D2 once known.
