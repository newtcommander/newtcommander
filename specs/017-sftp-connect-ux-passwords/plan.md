# Implementation Plan: SFTP Connect Window UX + Password Persistence

**Branch**: `017-sftp-connect-ux-passwords` | **Date**: 2026-07-18 | **Spec**: [spec.md](spec.md)

## Summary

Two orchestrator-confirmed defects plus whatever the three parallel audits add:

1. **Password not remembered** — `Config.QuickConnect` is never written by
   `SaveConfiguration` nor read by `LoadConfiguration` (sftp.cpp), although
   `EncryptPasswords` already re-encrypts its blobs (proving intent). A saved
   quick-connect password lives only in memory and is lost on restart. Fix:
   persist/restore the quick-connect entry under a `CFG_QUICKCONNECT` subkey via
   the existing `SaveServer`/`LoadServer`.

2. **Bookmark lifecycle incomplete/muddled** — no "Save (update) selected
   bookmark" action; `IDB_COPYBOOKMARK` (623) has no handler (dead control);
   editing a bookmark and connecting discards the edits. Fix: add an explicit
   **Save/Update** action, implement **Duplicate** (Copy), keep New/Rename/
   Delete, and clarify labels + the stored-password indicator; commit changes
   only via explicit actions (Cancel mutates nothing).

The audits (research.md) may add crypto/leak/consistency fixes → folded into
the same pass (FR-009).

## Technical Context

**Language/Version**: C++20, MSVC v143, plugin (.spl) | **Deps**: none new
**Files (expected)**:
- `src/plugins/sftp/sftp.cpp` — Save/LoadConfiguration: add QuickConnect
  round-trip (`CFG_QUICKCONNECT`); any SaveServer/LoadServer symmetry fixes.
- `src/plugins/sftp/dialogs.cpp` — connect dialog: Save/Update + Duplicate
  handlers; stored-secret indicator; commit semantics; wire IDB_COPYBOOKMARK.
- `src/plugins/sftp/lang/lang.rc` + `lang.rh` — connect dialog resource: add a
  "Save"/"Update" button (and relabel as needed); the Copy button already
  exists (IDB_COPYBOOKMARK) and just needs a handler.
- possibly `fs.cpp`/`session.cpp` per audit findings.
**Testing**: Debug + Release plugin build; registry round-trip of a saved
quick-connect + bookmark (write config → read back → blob present, no
plaintext); dev harness if touched. Live SFTP connect = user's final test.
**Constraints**: no plugin-ABI change; no plaintext secret at rest; do not
break host-key store or transfers; keep UTF-8 control-text helpers (feature 010).

## Constitution Check

| # | Principle | Verdict |
|---|-----------|---------|
| I | Build Reproducibility | PASS — code + plugin resource only |
| II | Backward Compatibility | PASS — adds a config subkey (older config simply lacks it → defaults); existing bookmarks unaffected |
| III | Incremental Modernization | PASS — reuse SaveServer/LoadServer + existing password-manager calls |
| IV | Windows Platform Commitment | PASS |
| V | Plugin Architecture Preservation | PASS — internal to the SFTP plugin; no core/ABI change |
| VI | UI Consistency | PASS — clarifies the dialog; house-style edit boxes kept (feature 009) |

## Phasing

1. **Persistence fix (P1)**: QuickConnect Save/Load round-trip — smallest,
   directly fixes the reported password loss, registry-verifiable.
2. **Audit consolidation**: read A/B/C, fold confirmed bugs into research.md.
3. **UX fix (P1)**: dialog Save/Update + Duplicate + indicator + resource button.
4. **Verify**: builds + registry round-trip + re-check audits addressed.

## Complexity Tracking

> Low risk. The persistence fix is a mechanical reuse of existing helpers. The
> UX change is contained to one dialog. Main care: commit-semantics (explicit
> Save only; Cancel mutates nothing) and not persisting a secret when its Save
> flag is off (clear the stale blob on update).
