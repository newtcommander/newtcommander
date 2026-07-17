# UI Contract: SFTP dialog house style

**Feature**: 009-sftp-dialog-style
**Date**: 2026-07-17

This project exposes no programmatic API for this feature. Its "contract" is a UI contract:
the observable rules an SFTP dialog must satisfy to be considered consistent with the rest
of the application. These rules are verifiable by inspection and by side-by-side comparison
on a clean build.

## C1 — Same font as the application

- **Given** any SFTP dialog, **it MUST** be a `DIALOGEX` declared with `DS_SHELLFONT`
  (`DS_SETFONT | DS_FIXEDSYS`) and `FONT 8, "MS Shell Dlg"`.
- **Verification**: the rendered typeface matches a core/FTP dialog opened next to it (no
  Microsoft Sans Serif vs shell-font mismatch).

## C2 — Same text-field frame and focus decoration

- **Given** an SFTP text-input field, **when** it has keyboard focus, **it MUST** show the
  same decoration as a focused text field in a core or FTP dialog — a frame consistent with
  the rest of the field, with **no** divergent Windows 11 accent underline or modern frame.
- **Verification**: focus an SFTP edit and a core/FTP edit; the focused-state appearance is
  visually indistinguishable. (Primary acceptance item — spec FR-002 / clarification.)

## C3 — No process-wide or per-plugin styling side effects

- The SFTP plugin **MUST NOT** register `ICC_STANDARD_CLASSES`, embed its own manifest, or
  apply per-plugin theming/subclassing that changes standard-control rendering.
- Using the SFTP plugin **MUST NOT** change the appearance of any other dialog opened later
  in the same session, and prior use of other plugins **MUST NOT** change SFTP's appearance.
- **Verification**: in one session, open SFTP and FTP/core dialogs in several orders; every
  dialog keeps its appearance regardless of order (spec US2 / SC-003, SC-004).

## C4 — Uniform across all SFTP dialogs

- Every SFTP dialog listed in `data-model.md` **MUST** satisfy C1–C3 (text-field rules apply
  to dialogs that contain text input).
- **Verification**: walk each SFTP dialog on a clean build; all conform.

## C5 — Durable convention

- The house style (C1–C3) **MUST** be recorded in the project constitution so future SFTP
  (and other) dialogs inherit it by default and deviations are caught in code review.
- **Verification**: constitution contains the UI-consistency principle (principle VI); a
  proposed deviating dialog is flagged in review against it. No automated check is required
  (spec clarification 2026-07-17).
