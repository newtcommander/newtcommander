# Quickstart: Verifying feature 036

## Build

```batch
build.cmd            :: Debug x64 — core + all enabled plugins must relink clean
```

## Runtime verification (Dark theme)

Launch `newtcommander.exe`, switch Options → Theme → Dark, then:

1. **SFTP (US1)**: Plugins → SFTP → Connect… — connect dialog dark
   (background, edits, list, buttons); open Organize bookmarks, a password
   prompt, the SFTP log window, the configuration dialog — all dark,
   all text readable. (FR-002)
2. **winliblt plugins (US2)**: open representative dialogs — FTP Connect,
   ZIP pack options, PictView config, Renamer window, CheckSum dialog,
   DBViewer table + its config, FileComp compare + panes, RegEdt view —
   dark chrome everywhere; text/document content areas (dbviewer table,
   filecomp panes, mdview document, regedt values, sftp log) dark with
   light text; PictView image canvas NOT recolored. (FR-001/008/009)
3. **Viewer windows**: F3 on a .md file (mdview) and an image (pictview):
   dark title bars, dark chrome; markdown document dark; image faithful.
4. **Switch semantics (US3)**: with a plugin dialog open, switch Dark →
   Default: open window keeps its consistent look, no crash; close +
   reopen → new theme. Newly opened plugin windows always match. (FR-005)
5. **Default regression**: switch to Default, reopen every surface from
   steps 1–3 — pixel-familiar light look, no dark leftovers. (FR-004)

## Compatibility checks

- Grep gate: no plugin adds `ICC_STANDARD_CLASSES` or a manifest.
- `spl_vers.h` = 105 with history row; spl_gen.h methods documented.
- A plugin binary built before 036 (if available) still loads — or,
  minimally: the loader's version handshake path is unchanged for ≤104.

## Audit artifact

`specs/036-plugin-dark-theme/audit.md` — per-plugin checklist (surface →
dark OK / N-A / issue) filled during US2; SC-001/002 evidence.
