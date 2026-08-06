# Code Map: SFTP Connect Dialog, Persistence and Dialog Geometry

**Feature**: 053-sftp-connect-dialog · **Date**: 2026-08-06
**Method**: three parallel read-only investigations (Quick Connect persistence,
bookmark validation flow, dialog layout measurement), cross-checked against the
translation pipeline source. Line numbers are from the working tree at the time
of writing; treat them as anchors, not addresses.

## 1. How the dialog identifies Quick Connect

The entry list mixes one pseudo-entry with the real bookmarks, distinguished by
listbox item data:

- `QC_ITEM (-1)` marks the Quick Connect row (`dialogs.cpp:891`); row 0 is added
  with that data in `ConnectFillBookmarkList` (`dialogs.cpp:893-908`), every
  other row carries its bookmark index.
- `ConnectSelectedItemData` (`dialogs.cpp:912-919`) returns an index, `QC_ITEM`,
  or `LB_ERR` when nothing is selected.
- `ConnectEntryForItemData` (`dialogs.cpp:922-929`) maps `QC_ITEM` to
  `&Config.QuickConnect` and an index to `Config.Bookmarks[i]`.
- `ConnectUpdateButtons` (`dialogs.cpp:965-972`) is the only existing place that
  branches on "a bookmark is selected", and it runs *after* the selection is
  set — which is why the new enable/disable rules belong there.

## 2. Quick Connect persistence (what makes it non-ephemeral today)

| Concern | Site |
|---|---|
| Registry key name | `CFG_QUICKCONNECT = "QuickConnect"` (`sftp.cpp:61`) |
| Written | `SaveConfiguration` (`sftp.cpp:593-601`): `DeleteKey` at 596, then `CreateKey` + `SaveServer` at 597-601 |
| Read | `LoadConfiguration` (`sftp.cpp:517-524`) → `LoadServer` (`sftp.cpp:390-415`) |
| Values written per entry | name, address, user, key file, initial path, target panel path (skipped when empty); port, auth method, both save-flags, compression (always); `PasswordE`/`PasswordS` and `PassphraseE`/`PassphraseS` blobs when the matching save-flag is set (`sftp.cpp:417-442`) |
| Location | the plugin's private config key (`…\Plugins Configuration\SFTP\QuickConnect`) |
| Re-encryption sweep | `CSFTPServerList::EncryptPasswords` processes Quick Connect explicitly (`sftp.cpp:293-294`), reached from `PasswordManagerEvent` (`sftp.cpp:668-673`) |

Feature 017 introduced this deliberately ("persist the quick-connect entry …
so a saved quick-connect password survives a restart", comment at
`sftp.cpp:593-594`). This feature reverses that decision.

**Nothing else derives from Quick Connect**: there is no host MRU
(`ClearHistory` is a no-op, `sftp.h:168`), logs are memory-only, and FS
reconnect matches bookmarks only (`fs.cpp:530-541`), so removing the entry
cannot break reconnect. The last-used-entry marker (`CFG_LASTBOOKMARK`,
`sftp.cpp:58`) stores only a selection index. Known-hosts records list hosts
reached through Quick Connect, and stay (out of scope).

## 3. Traps that shape the Quick Connect design

- **A — the entry is also the connect staging buffer.** `IDB_CONNECT`
  (`dialogs.cpp:1199-1209`) commits the fields into `Config.QuickConnect` and
  only then copies to the result. Removing the registry write alone leaves the
  typed values in memory for the rest of the session, so a reopened dialog still
  pre-fills. → reset the entry when the dialog opens.
- **B — `Clear()` is not a reset.** `sftp.cpp:183-195` frees strings and blobs
  and leaves `Port`, `AuthMethod`, both save-flags, keepalive overrides and
  compression stale. An empty entry needs the constructor's values
  (`sftp.cpp:157-176`).
- **C — a disabled checkbox still reports its old state.**
  `IsDlgButtonChecked` (`dialogs.cpp:795-796`) ignores `EnableWindow`, so the
  save-flags must be forced FALSE in the read path, not merely greyed out.
- **D — `ConnectSetAuthMode` runs from three sites** (`dialogs.cpp:756`, `1035`,
  `1038`), so a rule applied only in the field-load path is undone by the auth
  radio buttons.
- **E — ordering inside `WM_INITDIALOG`.** Fields are loaded
  (`dialogs.cpp:1019/1024`) *before* the row is selected (`1020/1025`), so a
  selection query inside the load path sees `LB_ERR`.
- **F — the load key is read-only.** `LoadConfiguration` receives a key opened
  with `KEY_READ` (`src/regwork.cpp:77` via `src/plugins1.cpp:2341/2944`), so a
  delete cannot happen there.
- **G — `SaveConfiguration` is not guaranteed to run.** The application calls it
  on exit only when auto-save is enabled (`src/mainwnd3.cpp:6764-6765`).
  → a forced one-time purge is needed for FR-004 (research D5a); in-repo
  precedent `src/plugins/checkver/checkver.cpp:202`.
- **H — the config version is never compared.** `CURRENT_CONFIG_VERSION`
  (`sftp.cpp:39`), `CFG_VERSION` (`sftp.cpp:45`), `Config.Version` read at
  `sftp.cpp:471` and written at `sftp.cpp:560`, but no upgrade branch exists —
  this feature adds the first one.

## 4. Bookmark validation: one gate, three callers

All "must have an address / valid port" logic is in `ConnectReadFields`
(`dialogs.cpp:777-788`); there is no second validator anywhere. The function
already takes a `forConnect` flag that is TRUE on exactly one path, which makes
it the natural gate.

| Caller | Site | `forConnect` | Validation after this feature |
|---|---|---|---|
| Connect | `dialogs.cpp:1204` via `ConnectCommitToEntry` (`935-946`) | TRUE | **keep** — unchanged messages |
| Save bookmark | `dialogs.cpp:1142` | FALSE | drop (FR-007) |
| New bookmark | `dialogs.cpp:1094` | FALSE | drop (FR-006) |

Second line of defence outside the dialog: the FS path parser refuses a
zero-length host (`fs.cpp:347`, message at `fs.cpp:507`), so no hostless
connection can start even if the dialog check were bypassed.

**Consequences that must be handled with it**
- Port: `GetDlgItemInt` returns 0 for an empty field (`dialogs.cpp:776`), which
  would be stored as `Port = 0`. Normalize to 22 on the non-connect path.
- Empty vs absent: `dialogs.cpp:791` passes raw buffers, so an empty host is
  `""` in memory but `NULL` after a reload (the registry writer skips empty
  strings, `sftp.cpp:360-364`). The neighbouring fields already normalize
  (`dialogs.cpp:793-794`); host and user should match.
- List label: `dialogs.cpp:904` falls back to the address testing only
  `!= NULL`, so an `""` address would render a blank row. Tighten to check for
  emptiness. (The English literal `"(unnamed)"` on the same line is a
  pre-existing localization gap, unreachable while blank names are rejected —
  left as follow-up.)
- Last-used marker is written before validation (`dialogs.cpp:1203`), so a
  failed connect from an empty bookmark would still re-seed the dialog to it.
- Save on the Quick Connect row would become able to "save" into a transient
  entry → disable Save for Quick Connect (research D8).
- Secrets could be attached to an address-less bookmark; harmless and allowed
  (the user opted in), noted so it is a decision rather than an accident.

**Persistence already supports empty entries**: `SaveServer` has no address
guard, the per-bookmark subkey is created regardless of content
(`sftp.cpp:579-589`), and the load loop stops only when the next index subkey is
missing (`sftp.cpp:494-513`) — there is no "skip entries without an address"
guard. An empty named bookmark round-trips today. The constructor already
defaults `Port` to 22 (`sftp.cpp:157-176`), and the visible 22 comes from
`ConnectLoadServerToFields` (`dialogs.cpp:742`), not the template.

**Deliberately unchanged**: "New" snapshots the current fields under a new name
(feature 017 semantics), rather than creating a blank record. Blank names stay
rejected — silently — at all three call sites (`dialogs.cpp:1091`, `1118`,
`1160`).

## 5. Why the localized texts are clipped

Per-language control geometry lives in the **translation source**, not the
template: a `.slt` dialog row is `id,x,y,cx,cy,flag,"text"` and
`-quiet-import-slt` writes those coordinates into a copy of `english.slg`
(`src/vcxproj/build_langs.ps1:340-360`). So the `.slt` overrides the template
for every language, and editing `lang.rc2` alone would fix English only.

The build's automatic widener (`tools/translate/layout.py:68-101`) grows a
left-aligned text control only into *genuinely free space*. Measured against the
committed Czech data with that same logic, every clipped connect-dialog label
has a growth ceiling **below its current width**, because its input field starts
immediately to the right:

| control | x | cx now | needed | blocked by | ceiling |
|---|---|---|---|---|---|
| key file (612) | 125 | 40 | ~66 | field 613 at x=165 | 38 |
| passphrase (615) | 125 | 40 | ~62 | field 616 at x=165 | 38 |
| initial path (618) | 125 | 40 | ~66 | field 619 at x=165 | 38 |
| port (603) | 289 | 18 | ~22 | field 604 at x=307 | 16 |

So the pipeline is behaving correctly; the space has to be created in the
template first (research D1), after which the refresh (research D3) lets every
language claim it.

**Scale**: 11 clipped controls in Czech across four dialogs (connect,
configuration, permissions, symlink); 124 across the eight shipped languages,
worst in French (23). The plugin's template defines **nine** dialogs
(`lang/lang.rc2`: connect 340×250, password 220×66, host key 300×150, chmod
210×176, owner/group 220×120, config 260×236, symlink 240×78, rename 240×60,
logs 400×260), so the measurement must cover all nine, not only the four that
happen to clip in Czech.

**Pipeline commands** (verified in `src/vcxproj/build_langs.ps1`):
- export a current-structure English template:
  `build_langs.cmd --module sftp --export-templates` (stage at lines 248-284)
- refresh committed translation source: `python -m translate.merge --module sftp
  --all` (`--dry-run` first; it makes no API calls and writes nothing)
- authoritative layout validation, opt-in because it is slow when it finds
  something: `build_langs.cmd --module sftp --check-layout` (lines 367-377)

**Trap**: `merge` constructs its DeepL client whenever `--dry-run` is absent
(`tools/translate/merge.py:362-370`), before it knows whether there are gaps, so
a geometry-only refresh currently needs network access even with nothing to
translate. A key exists at `temp/deepl_key.txt`; the plan still makes the client
lazy so the refresh is deterministic and offline.

## 6. The committed `.slt` widths are generated, not maintained

Verified mechanically: taking the English geometry from `lang.rc2`, attaching
each language's text and running `layout.widen()` reproduces **all 864 committed
control rows across all eight languages with zero mismatches**. `merge` re-runs
the widener on every dialog section, so a hand edit to a `.slt` width is
overwritten by the next refresh — the build's own message advising exactly that
(`build_langs.ps1:399-401`) predates the widener and is stale. Consequence: the
English template is the only durable place to change geometry, and the numbers in
§5 are *residual* clipping — what remains after the widener has already done
everything it can.

The refresh preserves text by construction: `match()` keys entries by
`(section, control id)` rather than by position
(`tools/translate/match.py:47-52`), so a geometry-only change matches 100% and
each hit is replayed from the `.origin` sidecar. The 24 hand-curated Czech
overrides for this module — 12 of them on the very controls being widened — win
over machine output and survive untouched (`tools/translate/merge.py:260-276`).

## 7. Measured geometry targets (all nine dialogs, eight languages)

`.slt` section ↔ symbol join, current width, and the minimum width that fits
every shipped translation (row-packing with the 4-unit right margin):

| section | dialog | cx now | cx min | grow | driver |
|---|---|---|---|---|---|
| 500 | `IDD_CONNECT` | 340 | **408** | +68 | host/port row |
| 510 | `IDD_PASSWORD` | 220 | 217 | 0 | *(prompt static overflows at run time — see §8)* |
| 515 | `IDD_HOSTKEY` | 300 | **355** | +55 | the three buttons |
| 520 | `IDD_CHMOD` | 210 | **257** | +47 | "set modification time" row |
| 530 | `IDD_CONFIG` | 260 | **350** | +90 | log-enable / log-size row |
| 540 | `IDD_SYMLINK` | 240 | **294** | +54 | link-name row |
| 550 | `IDD_RENAME` | 240 | 237 | 0 | — |
| 560 | `IDD_LOGS` | 400 | 397 | 0 | — |
| 570 | `IDD_OWNERGROUP` | 220 | 219 | 0 | *(controls still need re-laying: label 108→127)* |

26 distinct controls clip. Uniform column targets: connect labels 40→99, chmod
labels 30→46, config labels →176, symlink label 50→107, owner/group label
108→127. Worst single control: the configuration dialog's "show octal in the
information line" checkbox, 118 units for text needing 208 (French).

## 8. Constraints and hidden requirements

- **Nothing repositions at run time.** No `SetWindowPos`/`MoveWindow`/`WM_SIZE`
  anywhere in the plugin; the only placement call is
  `MultiMonCenterWindow`, which centres the dialog as authored. No shared dialog
  base class, no owner-drawn controls, no width assumptions in code. Growing
  these dialogs is safe.
- **A button is re-captioned at run time.** `IDB_HOSTKEY_TRUST` gets
  `IDS_HOSTKEY_ACCEPTNEW` (`dialogs.cpp:182`), needing ~111 units — invisible to
  the estimator, so the button must satisfy the larger of that and its 123-unit
  row need.
- **Five statics are empty in the template** and filled at run time. The
  password prompt is the acute one: `IDS_ENTERPASSPHRASE_RETRY` needs ~407 units
  against a 2 × 206 = 412-unit budget, *before* the `%s` path is substituted —
  i.e. it already overflows in practice.
- **A pre-existing overlap**: in the configuration dialog the "rights" radio
  (x 14–174) and the "show octal" checkbox (x 130–248) overlap by 44 units
  (`lang.rc2:258`, `:260`). It also caps the widener on the radio button.
- **Accelerators**: zero duplicates in any shipped language (the merge's
  de-duplication pass resolved them). The **English** template has three
  collisions, all in the connect dialog — `New`/`Co&nnect`, `D&uplicate`/`&User`,
  `&Password`/`Pass&phrase`. Geometry-only edits leave every accelerator
  byte-identical; fixing the English collisions would require changing English
  text and is deliberately left as follow-up.
- **Also noted, out of scope**: `IDB_CONNECT` is re-captioned with a hardcoded
  English `"Close"` (`dialogs.cpp:1012`); the configuration dialog has no
  OK/Cancel in its template (it is a child page) with 31 units free below it;
  the logs dialog is resizable but has no `WM_SIZE` handler, so its controls
  never follow the frame.

## 9. Recorded follow-up work (deliberately not done in feature 053)

Each of these needs a text change, which would create translation gaps — the one
thing this feature's FR-008 exists to prevent — or is a separate concern:

1. **Three duplicate keyboard accelerators in the English connect dialog**:
   `&New` vs `Co&nnect`, `D&uplicate` vs `&User`, `&Password` vs `Pass&phrase`
   (`lang/lang.rc2`). All eight shipped languages are already duplicate-free; only
   English collides. Fixing it means re-lettering English labels, which re-rolls
   the merge's accelerator assignment across every language.
2. **Two hardcoded English strings** shown to users regardless of language: the
   `"Close"` caption the connect dialog sets in organize mode
   (`dialogs.cpp`, in `WM_INITDIALOG`) and the `"(unnamed)"` entry-list fallback
   (`ConnectFillBookmarkList`). Both need new string ids.
3. **The logs dialog is resizable but never lays itself out** — it carries
   `WS_THICKFRAME | WS_MAXIMIZEBOX` with no `WM_SIZE` handler, so its list and text
   box keep their authored size when the user drags the frame.
4. **`IDS_HOSTNAMEMISSING` is worded for the path parser** ("A host name is
   missing in the path specified."), and it is now the *only* place the connect
   dialog tells the user the address is missing. A dialog-appropriate wording would
   need a new string id.
5. **A product-wide truncation guard.** The estimator used here could run over
   every module, but today it would report hundreds of pre-existing findings
   elsewhere, so it is its own feature. Note that the Translator's
   `--check-layout` cannot serve as that guard: it warns for untouched modules
   too.
6. **The `.slt` layout advice printed by the build is stale** —
   `build_langs.ps1` tells the reader to widen `cx` in the offending
   `translations\<lang>\<module>.slt`, but `merge`/`relayout` regenerate those
   numbers from the template, so such an edit is overwritten. The message should
   point at the dialog template instead.
