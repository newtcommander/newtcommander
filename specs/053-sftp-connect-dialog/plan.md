# Implementation Plan: SFTP Dialogs — Ephemeral Quick Connect, Empty Bookmarks, Untruncated Localized Texts

**Branch**: `053-sftp-connect-dialog` | **Date**: 2026-08-06 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/053-sftp-connect-dialog/spec.md`
**Decisions**: [research.md](research.md) · **Code map**: [investigation.md](investigation.md)

## Summary

Three independent changes in the SFTP plugin:

1. **Quick Connect becomes ephemeral** — its registry subtree is no longer
   written (the existing `DeleteKey` call becomes the automatic purge of stale
   data), it is no longer loaded, its in-memory entry is reset whenever the
   dialog presents it, and the save-password/save-passphrase checkboxes are
   disabled while it is selected so no secret can ever reach the password
   manager.
2. **A bookmark can be created empty** — address/port validation moves from
   "read the dialog fields" to "connect", so creating or saving a bookmark
   needs only a non-empty name while the connect path keeps today's checks and
   messages.
3. **Localized texts stop being clipped in all four SFTP dialogs** — the
   English dialog template gets wider label columns, fields shifted right and
   wider dialogs; then every language's `.slt` geometry is refreshed from that
   template with its texts preserved byte-for-byte, which lets the build's
   existing widener give each language exactly the width its own text needs.

## Technical Context

**Language/Version**: C++ (C++20, `/std:c++latest`), MSVC v143 (VS2022); Windows resource script (`lang.rc2`) for dialog templates; Python 3.13 for the offline translation tooling; PowerShell 5.1 / Batch for build orchestration
**Primary Dependencies**: Pure WinAPI; plugin SDK (interface version 105/106, unchanged); `CSalamanderRegistryAbstract` for persistence; `CSalamanderPasswordManagerAbstract` for secrets; `translator.exe` + `tools/translate/` for language modules
**Storage**: plugin configuration under the app's registry root via the plugin registry interface — bookmarks, known hosts, options; the `QuickConnect` subtree is being **removed** (deleted, never rewritten). No format change to anything that stays.
**Testing**: manual validation per [quickstart.md](quickstart.md) against the local Docker SFTP reference server (container `tandem-sftp`, `localhost:2222`); static layout check via `translate.layout.estimate_width`; authoritative layout check via `build_langs.cmd --module sftp --check-layout`; existing SFTP harness `src/plugins/sftp/test/` for connect regressions
**Target Platform**: Windows 11+, x64
**Project Type**: Desktop application plugin (`sftp.spl`) + its language modules
**Performance Goals**: N/A — dialog layout and one fewer registry subtree
**Constraints**: plugin ABI unchanged; no new external dependencies; dialogs stay `DIALOGEX` + `DS_SHELLFONT` + `FONT 8, "MS Shell Dlg"` with standard themed controls (constitution VI); translated **wording** must not change (FR-008); input fields must not get narrower (FR-011); no configuration migration — the only stored-data change is a deletion (FR-004)
**Scale/Scope**: 9 dialogs measured × 8 shipped languages → 26 clipped controls in 6 dialogs (plus one run-time overflow and one pre-existing overlap) must reach zero; ~8 code sites in `dialogs.cpp` + 4 in `sftp.cpp`; 1 English template re-laid-out; 8 `.slt` files refreshed (geometry only, texts byte-identical)

## Constitution Check

*GATE: evaluated against Tandem Commander Constitution v3.1.0 — PASS (pre-Phase-0 and re-checked post-Phase-1).*

| Principle | Verdict | Note |
|---|---|---|
| I. Build Reproducibility | PASS | No build-pipeline change. The `.slt` refresh is a maintainer step whose output is committed, exactly as feature 038 established; the build keeps consuming committed source offline. One optional tooling improvement (make the translation client lazy so a geometry-only refresh needs no network) *increases* reproducibility. |
| II. Backward Compatibility | PASS (deliberate, documented) | Two intentional user-visible behaviour changes, both requested: Quick Connect no longer remembers anything (its stored data is deleted, not migrated), and bookmark creation no longer demands an address. Bookmarks, known hosts and all other settings keep their format and behaviour. Plugin ABI untouched. |
| III. Incremental Modernization | PASS | Three small, independently revertable changes; no adjacent refactoring. The dialog-geometry edit touches only coordinates. |
| IV. Windows Platform Commitment | PASS | WinAPI dialogs and registry only. |
| V. Plugin Architecture Preservation | PASS | No interface change; the plugin's persistence contract for its own data is documented in [contracts/](contracts/) before modification. |
| VI. UI Consistency | PASS | Dialogs stay `DIALOGEX`/`DS_SHELLFONT`/`MS Shell Dlg` with standard themed controls, labels beside fields (FR-012). Widening a dialog and disabling an inapplicable checkbox are both existing house patterns. |
| Release Documentation | PASS | `CHANGELOG.md` entry under the existing `[Unreleased]` heading (research D7); version bump stays with the release change. |

## Project Structure

### Documentation (this feature)

```text
specs/053-sftp-connect-dialog/
├── spec.md              # Feature specification (with Clarifications)
├── plan.md              # This file
├── research.md          # Phase 0 decisions D1–D7
├── investigation.md     # Code map: persistence, dialog wiring, layout numbers
├── data-model.md        # Phase 1: entities, stored-data shape, layout data
├── quickstart.md        # Phase 1: validation guide
├── contracts/
│   └── sftp-plugin-persistence.md   # Phase 1: what the plugin stores, and what it must not
└── tasks.md             # Phase 2 (/speckit-tasks — not created here)
```

### Source Code (repository root)

```text
src/plugins/sftp/
├── sftp.cpp             # LoadConfiguration (517-524): stop reading QuickConnect;
│                        #   detect config version < 2 -> needs purge
│                        # SaveConfiguration (593-601): keep DeleteKey at 596, drop 597-601;
│                        #   bump CURRENT_CONFIG_VERSION to 2 (39)
│                        # EncryptPasswords (293-294): drop QuickConnect from the sweep
│                        # Connect/Release (632/455): one-time forced purge save via
│                        #   CallLoadOrSaveConfiguration (pattern: checkver.cpp:202)
├── sftp.h               # CSFTPServer / CSFTPConfig: document Quick Connect as transient
├── dialogs.cpp          # ConnectReadFields (777-796): host/port checks gated on forConnect;
│                        #   port normalized + host/user NULL-normalized at 791; save-flags
│                        #   forced FALSE for Quick Connect (a disabled checkbox still
│                        #   reports its old state)
│                        # ConnectUpdateButtons (965-972): disable the two save-secret
│                        #   checkboxes AND Save while the Quick Connect row is selected
│                        #   (runs after selection, unlike the field-load path)
│                        # ConnectSetAuthMode (725-734, 3 call sites): must not re-enable them
│                        # WM_INITDIALOG (1003-1030): reset the Quick Connect entry to
│                        #   constructor defaults before list/fields are populated
│                        # ConnectFillBookmarkList (904): label fallback checks Address[0]
│                        # IDB_CONNECT (1190-1210): validation unchanged; LastBookmark (1203)
│                        #   moved after successful validation; clear Quick Connect after
│                        #   the result is copied out (1205)
└── lang/lang.rc2        # New geometry only - NO text, NO accelerator changes:
                         #   IDD_CONNECT    340 -> 408   label col 40->99,  fields x->226
                         #   IDD_CONFIG     260 -> 350   label col ->176,   fields x->185
                         #                               + resolve the pre-existing 44-unit
                         #                                 overlap on the "show octal" row
                         #   IDD_HOSTKEY    300 -> 355   buttons 76->123/87 (must also fit
                         #                                 the run-time "accept new key" text)
                         #   IDD_CHMOD      210 -> 257   label col 30->46,  checkboxes x->55
                         #   IDD_SYMLINK    240 -> 294   label col 50->107, fields x->116
                         #   IDD_OWNERGROUP 220 (same)   label 108->127,    fields x->136
                         #   IDD_PASSWORD   220 (same)   prompt static enlarged (its run-time
                         #                                 message overflows two lines today)
                         #   IDD_RENAME, IDD_LOGS        unchanged

translations/
├── czech/sftp.slt       # geometry refreshed from the new template; texts byte-identical
├── slovak/sftp.slt      # (same for german, french, spanish, dutch, hungarian, romanian)
└── …

tools/translate/merge.py # lazily construct the translation client (only when gaps exist),
                         #   so a geometry-only refresh runs offline and deterministically

CHANGELOG.md             # Fixed/Changed entries (research D7)
```

**Structure Decision**: existing layout; no new projects or files. The only new
artifact is documentation. `lang.rh`/`lang.rh2` need no change — no controls are
added or removed, only moved and resized.

## Implementation Phases (for /speckit-tasks)

1. **Quick Connect ephemerality** (research D5): stop loading/saving the
   subtree (the retained `DeleteKey` purges old data), reset the in-memory entry
   when the dialog shows it, disable the two save-secret checkboxes for Quick
   Connect via the dialog's existing enable/disable pass, and confirm no
   quick-connect secret can reach the password manager.
2. **Empty bookmarks** (research D6): move address/port validation to the
   connect path; verify an empty named bookmark survives a save/load round trip
   and renders sensibly in the list.
3. **Dialog geometry** (research D1, D10, D11): re-lay-out the seven affected
   dialogs in `lang/lang.rc2` to the measured targets, resolve the
   configuration dialog's pre-existing overlap, size the host-key button for its
   run-time caption too, and enlarge the password prompt static. Fields never
   shrink; no text and no accelerator changes.
4. **Language refresh** (research D3): export the new template, dry-run the
   merge to confirm zero gaps, apply it, verify the writer round-trips, then
   **mechanically prove** that only coordinates changed in the eight `.slt`
   files.
5. **Verification** (research D4): estimator reaches zero findings for the sftp
   module; `--check-layout` reports no clipped or overlapping controls; visual
   pass in Czech plus French (the worst case for width); quickstart scenarios.
6. **Docs**: CHANGELOG entry; `clang-format` on touched C++.

## Complexity Tracking

No constitution violations to justify. The two deliberate behaviour changes
(Quick Connect persistence removed; bookmark address no longer required at
creation) are the feature's requested purpose and are recorded in the
Constitution Check above and in `CHANGELOG.md`.
