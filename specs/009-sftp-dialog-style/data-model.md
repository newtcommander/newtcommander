# Phase 1 Data Model: Consistent SFTP Plugin Dialog Appearance

**Feature**: 009-sftp-dialog-style
**Date**: 2026-07-17

This feature changes visual presentation, not application data. There are no persisted
entities, records, or state machines. In place of a traditional data model, the relevant
"entities" are the set of SFTP dialogs that must conform and the house-style attributes they
must match.

## Entity: SFTP dialog (presentation object)

The dialogs the feature must bring into visual conformance. Source: language module resource
`src/plugins/sftp/lang/lang.rc2`.

| Dialog | Resource ID | Has text input | Notes |
|--------|-------------|----------------|-------|
| Connect | `IDD_CONNECT` | Yes (host, port, user, password, key file, passphrase, initial path) | Primary user-facing window |
| Password / passphrase prompt | `IDD_PASSWORD` | Yes (one password field) | Interactive auth |
| Host-key verification | `IDD_HOSTKEY` | No | Buttons/icon/text only |
| Change permissions | `IDD_CHMOD` | Yes (octal, mtime) | |
| Configuration page | `IDD_CONFIG` | Yes (numeric fields) | Child template (`WS_CHILD`) |
| Symbolic link | `IDD_SYMLINK` | Yes (name, target) | |
| Rename | `IDD_RENAME` | Yes (name) | |
| Logs | `IDD_LOGS` | Yes (read-only multiline) | Resizable |

## Entity: House-style attributes (conformance target)

The observable attributes each dialog/control must match against the core application and
other bundled plugins (e.g. FTP). This is the "match target", not new data.

| Attribute | Required value / behavior | Baseline reference |
|-----------|---------------------------|--------------------|
| Dialog template kind | `DIALOGEX` | All core/plugin dialogs |
| Font selection | `DS_SHELLFONT` (`DS_SETFONT | DS_FIXEDSYS`) + `FONT 8, "MS Shell Dlg"` | Core `lang.rc`, FTP `lang.rc` |
| Typeface rendered | Same shell UI font as the rest of the app | Core/FTP dialogs |
| Text-field frame | Same border treatment as core/FTP edits | Core/FTP edits |
| Text-field focus decoration | Same as a focused core/FTP edit (no divergent accent underline / modern frame) | Core/FTP edits (FR-002) |
| Process-wide control registration | No per-plugin `ICC_STANDARD_CLASSES`, manifest, or theming side effects | Constitution principle VI |

## Relationships / rules

- Every SFTP dialog in the first table MUST satisfy every applicable row of the second
  table (a text-field row applies only to dialogs with text input).
- Conformance is assessed relative to the current shipped appearance of the core and other
  plugins (the "classic" look), not an independent visual target.
- No lifecycle/state transitions apply.
