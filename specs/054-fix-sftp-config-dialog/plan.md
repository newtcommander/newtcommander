# Implementation Plan: SFTP — Reachable Settings, Reliable Connect, Tight Dialog Layout

**Branch**: `054-fix-sftp-config-dialog` | **Date**: 2026-08-07 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/054-fix-sftp-config-dialog/spec.md`
**Decisions**: [research.md](research.md) · **Prior evidence**: `specs/053-sftp-connect-dialog/investigation.md` §9

## Summary

Three independent fixes in the SFTP plugin, all with causes already measured:

1. **Settings become reachable.** `IDD_CONFIG` stops being a `WS_CHILD`
   template and becomes an ordinary captioned popup with OK/Cancel, exactly
   like the plugin's other eight dialogs and like every other plugin's
   settings. The existing centring call then becomes correct on its own.
2. **Connect stops being hostage to one address.** The sequential address loop
   in `CSFTPSession::OpenSocket` becomes an overlapping (Happy-Eyeballs) race:
   the next address starts ~250 ms after the previous without abandoning it,
   the first to answer wins, the losers are closed. Feature 051's bounded total
   and prompt cancellation are preserved.
3. **The connect dialog fits its language.** At open time the label column is
   measured from the loaded language's texts, the field column and the dialog
   width follow, and the fields keep their width.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022); Windows resource script for dialog templates; Python 3.13 for the offline translation tooling
**Primary Dependencies**: Pure WinAPI (dialog manager, GDI text measurement, Winsock); plugin SDK (`MultiMonCenterWindow`, `LoadStr`); vendored libssh2 (unchanged); `tools/translate/` for the language refresh
**Storage**: none — this feature persists nothing new and changes no stored format
**Testing**: live validation per [quickstart.md](quickstart.md) against the Docker reference server (`tandem-sftp`, `localhost:2222`, with `::1:2222` black-holed — a ready-made regression case); existing harness `run_keyauth.cmd` must stay green; run-time measurement of the rendered dialog for the layout work
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application plugin (`sftp.spl`) + its language modules
**Constraints**: plugin ABI unchanged; no new strings (the caption text already exists and is translated); constitution VI (`DIALOGEX` + `DS_SHELLFONT` + `MS Shell Dlg`, standard themed controls); feature 051's guarantees — bounded total connect time, prompt cancel, non-blocking winning socket — must survive; feature 053's guarantee that no label is clipped must survive
**Scale/Scope**: 1 dialog template (style + caption + 2 buttons), 1 connect helper rewritten, 1 dialog gains run-time sizing, 11 `.slt` files refreshed, ~3 code files touched

## Constitution Check

*GATE: evaluated against Tandem Commander Constitution v3.1.0 — PASS (pre-Phase-0 and re-checked post-Phase-1).*

| Principle | Verdict | Note |
|---|---|---|
| I. Build Reproducibility | PASS | No build-pipeline change. The translation refresh is a maintainer step whose output is committed, as feature 038 established. |
| II. Backward Compatibility | PASS | Three defect fixes; nothing stored changes format or location; plugin ABI untouched. The only visible behaviour changes are the defects disappearing. Connect semantics stay within feature 051's contract — see the risk note below. |
| III. Incremental Modernization | PASS | Three small, independently revertable changes in one plugin; no adjacent refactoring. Run-time dialog sizing is a new pattern for the project (no precedent found), so it is kept local to one dialog and to one helper. |
| IV. Windows Platform Commitment | PASS | WinAPI/GDI/Winsock only. |
| V. Plugin Architecture Preservation | PASS | No interface change. The connect behaviour users depend on is documented before modification in [contracts/connect-attempt-behaviour.md](contracts/connect-attempt-behaviour.md). |
| VI. UI Consistency | PASS | The settings dialog adopts the *same* style line the plugin's other dialogs use; no new control types, no local visual overrides. Run-time sizing changes geometry only. |
| Release Documentation | PASS | `CHANGELOG.md` entries under `[Unreleased]` (research D6); version bump stays with the release change. |

**Principal risk, recorded**: the connect rewrite must not regress feature 051,
which exists because users hit multi-minute waits. FR-005 (bounded total,
independent of address count) and FR-007 (prompt cancel) are therefore treated
as invariants to prove, not as side effects — see quickstart scenarios 4–6.

## Project Structure

### Documentation (this feature)

```text
specs/054-fix-sftp-config-dialog/
├── spec.md              # Feature specification (3 clarifications)
├── plan.md              # This file
├── research.md          # Phase 0 decisions D1–D6
├── data-model.md        # Phase 1: presentation + attempt state
├── quickstart.md        # Phase 1: validation guide
├── contracts/
│   └── connect-attempt-behaviour.md   # Phase 1: what a connect promises
└── tasks.md             # Phase 2 (/speckit-tasks — not created here)
```

### Source Code (repository root)

```text
src/plugins/sftp/
├── lang/lang.rc2        # IDD_CONFIG: WS_CHILD -> standard popup style,
│                        #   CAPTION from the existing IDS_CONFIGTITLE,
│                        #   + DEFPUSHBUTTON OK / PUSHBUTTON Cancel at y=229
│                        #   (content ends at y=219 in a 250-tall dialog,
│                        #   so no size change is needed)
├── dialogs.cpp          # ShowConfigDialog: drop the now-false "child template"
│                        #   comment; the DialogBoxParam call itself is correct
│                        # ConfigProc: MultiMonCenterWindow call unchanged - it
│                        #   becomes correct once the template is a popup
│                        # ConnectProc WM_INITDIALOG: measure the loaded
│                        #   language's label texts, set the label column, move
│                        #   the field column, resize the dialog (US3)
└── session.cpp          # OpenSocket (173-311): sequential address loop becomes
                         #   an overlapping race - stagger ~250ms, select() over
                         #   all pending candidates, first writable wins, close
                         #   the losers, keep the shared budget and the cancel
                         #   poll, move freeaddrinfo past the wait, keep the
                         #   winner non-blocking (feature 051 D4/F3)

translations/*/sftp.slt  # refreshed: 2 new button rows + the caption text;
                         #   the 24 existing rows keep their translations
CHANGELOG.md             # Fixed/Fixed/Changed entries (research D6)
```

**Structure Decision**: existing layout; no new files. Should the connect race
be extracted into an SDK-free helper for harness testing (optional, research
D5), it would join `sftputils.cpp`, which exists for exactly that purpose.

## Implementation Phases (for /speckit-tasks)

1. **Settings dialog reachable** (research D1): template style + caption +
   buttons; remove the stale comment; verify the dialog opens, is titled,
   centred, closable, and that OK/Cancel persist/discard as `ConfigProc`
   already implements.
2. **Translation refresh** (research D4): export template → `merge --module
   sftp` → bound and check what changed (expected: caption text + "Cancel";
   "OK" stays English by rule) → refresh the three disabled languages
   explicitly.
3. **Overlapping connect** (research D2): rewrite `OpenSocket`'s loop; preserve
   the budget, the cancel poll, the socket hand-off under `SocketLock`, the
   non-blocking winner and the error classification; close losers; move
   `freeaddrinfo`.
4. **Run-time dialog sizing** (research D3): measure labels with the dialog's
   own font, size the column, move the fields, resize the dialog; keep fields'
   width and clip nothing.
5. **Verification** (research D5): quickstart scenarios — settings window,
   `localhost` under 2 s, `127.0.0.1` unchanged, all-dead bounded, cancel
   prompt, layout gap measured in two languages, harness green, full builds.
6. **Docs**: CHANGELOG entries; clang-format on touched C++.

## Complexity Tracking

No constitution violations to justify. Two choices worth naming explicitly:

| Choice | Why | Simpler alternative rejected because |
|---|---|---|
| Overlapping connection attempts | Only option that makes a dead first address cost ~nothing while keeping the total bounded | Dividing the budget per address still costs half the timeout on a dead first address, and shrinks each address's share as the list grows |
| Run-time dialog sizing (new pattern here) | Label width is only knowable at run time — it depends on language, font and DPI | A fixed width is either wasteful (today) or unsafe; a fixed window with a moving divider would narrow the fields, which FR-010 forbids |
