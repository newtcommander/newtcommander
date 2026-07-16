# Data Model: Plugin Build Policy

## Entities

### PluginConfigEntry

One non-comment, non-blank line of `plugins.cfg`.

| Field | Type | Constraints |
|---|---|---|
| `name` | string | Must equal a directory name under `src/plugins/` (case-insensitive); `shared` is not a valid name; must be unique across the file (case-insensitive) |
| `state` | enum `on` \| `off` | Literal, case-insensitive; nothing else accepted |

Validation rules (violations abort the build before compilation):
- **V1 missing-file**: `plugins.cfg` absent from repo root.
- **V2 syntax**: line is neither blank, nor `#`-comment, nor `name=on|off` (surrounding whitespace tolerated).
- **V3 unknown**: `name` matches no plugin directory.
- **V4 duplicate**: same `name` appears twice (case-insensitive).
- **V5 unlisted**: a plugin directory exists with no entry.

### Plugin

A directory `src/plugins/<name>/` (excluding `shared`), owning
`vcxproj/<name>.vcxproj` (+ `vcxproj/lang_<name>.vcxproj` and possible
satellite projects referenced via `ProjectReference`). When built it
yields `plugins\<name>\<name>.spl` + `lang\english.slg` in the output.

Derived states (computed each build from `plugins.cfg` × directory scan):

| State | Meaning | Build behavior |
|---|---|---|
| `enabled` | entry `on` | in `.slnf`; compiled; outputs present; in `plugins.ver` |
| `disabled` | entry `off` | not in `.slnf`; not compiled; outputs deleted from build dir; not in `plugins.ver`; stays in `salamand.sln` (IDE-buildable) |
| `removed` | no directory (this feature deletes 8) | absent everywhere; leftover output directories deleted by reconciliation; silent-uninstall suppress list handles stale user registrations |

State transitions: `enabled ↔ disabled` = edit one line + next build
(any flavor). `→ removed` = repository change (this feature; not a
config operation).

### PluginRegistrationList (`plugins.ver`)

Build-generated file in `<out>\plugins\`. Line 1 = monotonically
increasing version token; lines 2+ = `<version>:<relative .spl path>`.
Invariant after every `build.cmd` run: contains exactly the enabled
plugins' `.spl` paths (reconciliation filters it; `full` regenerates it
from the reconciled tree).

### SolutionFilter (`salamand.gen.slnf`)

Generated JSON build artifact (gitignored). Contains `salamand.sln`
reference + the project paths of every solution project **except**
those under `..\plugins\<name>\` for disabled plugins. Regenerated on
every build; never committed; never hand-edited.

## Relationships

```text
plugins.cfg (1) ──validates against──> src/plugins/* directories (28)
plugins.cfg (1) ──filters──> salamand.sln (77 projects) ──> salamand.gen.slnf
enabled Plugins (18) ──produce──> <out>\plugins\<name>\*.spl ──enumerated by──> plugins.ver
build.cmd ──invokes──> gen_plugins_filter.ps1 ──emits──> .slnf + reconciled output tree
```

## Initial content (the committed policy)

`off` (10): automation, checkver, demomenu, demoplug, demoview,
mmviewer, nethood, unchm, unmime, unole
`on` (18): 7zip, checksum, dbviewer, diskmap, filecomp, folders, ftp,
peviewer, pictview, portables, regedt, renamer, tar, uncab, undelete,
uniso, unrar, zip

Removed (no entry, no directory after this feature): ieviewer, pak,
splitcbn, unarj, unfat, unlha, winscp, wmobile.
