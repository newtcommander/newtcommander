# Phase 0 Research: SFTP Dialogs — Ephemeral Quick Connect, Empty Bookmarks, Untruncated Texts

**Feature**: 053-sftp-connect-dialog · **Date**: 2026-08-06
**Spec**: [spec.md](spec.md) (clarified 2026-08-06 — see its Clarifications section)

Decisions D1–D3 below were verified directly against the translation pipeline
source; D4–D7 come from the code-map investigation of the plugin (see
[investigation.md](investigation.md)).

## D1 — Where the room for full localized texts comes from

**Decision** (user decision, clarification 2026-08-06): widen the dialogs.
Label/checkbox controls get a wider column, input fields shift right and keep
their current width, and each dialog's own width grows to absorb it. No control
is shrunk; no field is narrowed; the two-column "label beside field" layout and
the standard themed control set stay exactly as they are.

**Rationale**: guarantees the longest shipped translation fits (French is the
worst case), keeps every input field as usable as today (FR-011), and leaves
room for the currently disabled non-Latin languages if they are re-enabled.
These are popup dialogs, so extra width costs the user nothing.

**Alternatives considered**:
- *Shrink the input fields inside the current dialog size* — arithmetically
  possible (about 12 units would have to come out of the 120-unit fields) but
  leaves no reserve for a longer language and makes the key-file path field the
  tightest control in the dialog. Rejected by the user.
- *Move labels above their fields* — gives labels effectively unlimited width
  but makes the dialogs much taller and breaks the house style shared with the
  rest of the application (constitution VI). Rejected by the user.
- *Measure the text at runtime and lay the dialog out dynamically* — adapts to
  any future language automatically, but puts new layout logic in a plugin,
  makes the dialog a different size in every language, and is the riskiest of
  the four. Rejected by the user.

## D2 — Why the English template alone cannot fix it (the mandatory `.slt` refresh)

**Finding**: per-language control geometry lives in the **translation source**,
not in the English template. A `.slt` dialog row is
`id,x,y,cx,cy,flag,"text"` — e.g. `translations/czech/sftp.slt:31` reads
`612,125,89,40,8,1,"Soubor s klíč&em:"`. `-quiet-import-slt` writes those
coordinates into a copy of `english.slg`
(`src/vcxproj/build_langs.ps1:340-360`), so **the `.slt` wins over the
template** for every language. Editing `lang.rc2` alone would fix English and
leave all eight other languages exactly as clipped as they are now.

**Decision**: the change is two-part and both parts are mandatory:
1. `src/plugins/sftp/lang/lang.rc2` — the English template gets the new
   geometry (wider dialogs, shifted fields, wider label columns).
2. `translations/<lang>/sftp.slt` — every language's geometry is refreshed from
   that new template, with its translated **texts preserved byte-for-byte**.

**Rationale**: this is the documented data flow
(`tools/translate/README.md` "Why `.slt` files are regenerated, never
hand-edited"), and the same document explicitly sanctions the narrow case we
need: *"Editing a committed `.slt` by hand is fine for a text or **width fix**.
Adding or removing rows is not."* No rows are added or removed here — only
numbers change.

**Proof that the automatic widener is not at fault** (run against the committed
Czech data using the build's own logic): for each clipped connect-dialog label
the widener's growth limit is *below its current width*, because the input
field sits immediately to the right —

| control | x | current cx | needed | blocked by | max cx the widener could give |
|---|---|---|---|---|---|
| key file (612) | 125 | 40 | ~66 | field 613 at x=165 | **38** |
| passphrase (615) | 125 | 40 | ~62 | field 616 at x=165 | **38** |
| initial path (618) | 125 | 40 | ~66 | field 619 at x=165 | **38** |
| port (603) | 289 | 18 | ~22 | field 604 at x=307 | **16** |

So the pipeline is behaving correctly and no amount of data regeneration alone
can fix this: the English template must first create the space (D1), and only
then does the refresh (D3) let every language claim it.

## D3 — How the `.slt` refresh is performed and proven text-neutral

**Decision**, in this order:
1. Rebuild so `english.slg` reflects the new template, then export a
   current-structure template:
   `src\vcxproj\build_langs.cmd --module sftp --export-templates`
   (writes `<build>\tandemcommander\translator\templates\sftp.slt`,
   `build_langs.ps1:248-284`).
2. Preview: `python -m translate.merge --module sftp --all --dry-run` — must
   report **zero gaps**. Zero gaps means no text will be fetched or replaced;
   the merge then only re-derives geometry from the template and applies
   `layout.widen()` per language.
3. Apply for real: `python -m translate.merge --module sftp --all`.
4. **Prove FR-008 mechanically**: diff each `translations/<lang>/sftp.slt`
   before/after and assert that every quoted string is unchanged and only
   numeric coordinate fields differ. This is a task in its own right, not an
   assumption.

**Rationale**: `merge` is the sanctioned producer of `.slt`; with no gaps it
makes no translation decisions at all. `layout.widen()`
(`tools/translate/layout.py:68-101`) grows a left-aligned text control into
*genuinely free space* — which is precisely what step 1 creates — so each
language automatically gets exactly the width its own text needs. The
`.origin` sidecars and the `ui-overrides.json` pins from feature 051 are
preserved by the same call.

**Trap found**: `merge` builds its DeepL client whenever `--dry-run` is absent
(`tools/translate/merge.py:362-370`), *before* it knows whether there are any
gaps, so a real run touches the network even with nothing to translate. A key
is present at `temp/deepl_key.txt`, so this works today; the plan still makes
the client lazy (construct only when gaps exist) so an offline, deterministic
geometry refresh is possible and this feature cannot be blocked by a missing
key. If that change is not wanted, the fallback is the README-sanctioned
width-only hand edit applied by a one-off script.

**The hand-edit fallback does not exist.** The committed `cx` values are
*generated output*, not maintained data — verified mechanically: taking the
English geometry from `lang.rc2`, attaching each language's text and running
`layout.widen()` reproduces **all 864 committed control rows across all eight
languages with zero mismatches**. `merge` re-runs `widen()` unconditionally on
every dialog section, so any hand edit is overwritten by the next refresh. (The
build even prints stale advice to "fix by widening the control (cx) in the
offending `.slt`" — `build_langs.ps1:399-401`; that advice predates the widener
and should not be followed.) The README's "a width fix by hand is fine" line is
therefore true only until the next merge. **The template is the only durable
place to change geometry.**

**Text survives the refresh by construction**: `match()` keys entries by
`(section, control id)` rather than position (`tools/translate/match.py:47-52`),
so a pure geometry change matches 100% and each hit is replayed from the
`.origin` sidecar as `human`/`machine` — zero API calls, zero quota. The 24
hand-curated Czech overrides for this module (12 of them on the very controls
being widened) win over everything and survive untouched
(`tools/translate/merge.py:260-276`).

**Correction made during implementation.** `merge --module sftp --dry-run`
reported **6 gaps per language**, not zero: entries whose committed translation is
still an English fallback, which a real run would send to the translation service.
That is a text change — exactly what FR-008 forbids — so `merge` is *not* the tool
for this job. Implemented instead: `tools/translate/relayout.py`, a small sibling
that copies dialog geometry from the template and never reads text from it, so it
cannot alter a character. It applies `layout.widen()` afterwards, so its output is
what `merge` would produce geometrically and a later `merge` stays idempotent about
layout. It defaults to **all** registered languages (like `rebrand`, unlike
`merge`), because leaving a disabled language on stale geometry would hand whoever
re-enables it a broken layout, and the pass costs nothing.

**Full command sequence** (as executed):
```bat
build.cmd full                                                :: english.slg must exist
src\vcxproj\build_langs.cmd --export-templates --module sftp
python -m translate.relayout --module sftp --dry-run           :: preview, writes nothing
python -m translate.relayout --module sftp
python -m translate.slt --verify                              :: writer round-trips byte-exact
src\vcxproj\build_langs.cmd --module sftp --force
```

**Alternatives considered**:
- *Hand-edit widths in eight `.slt` files* — **not viable**, see above: the next
  merge regenerates them.
- *A new dedicated `relayout` tool* — duplicates what `build_slt` already does
  for geometry. Rejected as unnecessary code.

## D4 — Verification tool for "nothing is clipped"

**Decision**: two independent checks.
- **Static, fast, offline**: the project's own estimator, used exactly as the
  build's widener uses it — `translate.layout.estimate_width(text) > cx` over
  every `translations/*/sftp.slt` dialog row. Target: zero findings (today: 11
  in Czech, 124 across the eight shipped languages).
- **Template-level geometry check**: parse `lang.rc2` and assert, per dialog,
  that no control extends past the dialog, no two controls overlap, and every
  English caption fits its own control. This is what catches the overlap the
  estimator is blind to.

**Rejected as a gate: the Translator's `--check-layout`.** Measured during
implementation: it reports `LAYOUT WARNINGS` for **every** language of an
untouched module too (verified against `renamer`, which this feature does not
touch). Its non-clean exit is the build's watchdog killing an interactive
fallback rather than a finding about the module, so it cannot distinguish "this
change is good" from "this change is bad". It stays available for a human QA
pass; it is not evidence.

**Rationale**: the estimator is what produced the requirement's numbers, so it
must reach zero for consistency; the Translator's validator is the ground truth
for overlap (FR-009/FR-010) and is already part of the build's vocabulary. A
product-wide guard stays out of scope per the spec (it would report hundreds of
pre-existing findings elsewhere).

## D5 — Making Quick Connect ephemeral

**Decision**: Quick Connect stops being a persisted entry, in five parts. The
code map showed that dropping the registry write alone is **not** sufficient —
each part below closes a specific hole.

1. **Not written**: delete the write of the `QuickConnect` subtree in
   `SaveConfiguration` (`sftp.cpp:597-601`) but **keep the `DeleteKey` that
   already precedes it** (`sftp.cpp:596`) — that existing line becomes the
   purge, so no migration code is written.
2. **Not read**: remove the load block (`sftp.cpp:517-524`). `LoadServer` stays
   for bookmarks.
3. **Not remembered in memory either**: `Config.QuickConnect` doubles as the
   dialog's *staging buffer* for the connection being started
   (`dialogs.cpp:1204-1205` commits into it, then copies to the result). So the
   entry is **reset to constructor defaults when the dialog opens**, before the
   list and fields are populated. Resetting matters because `Clear()`
   (`sftp.cpp:183-195`) frees only strings and blobs and leaves `Port`,
   `AuthMethod`, `SavePassword`, `SavePassphrase`, `KeepAlive*` and
   `UseCompression` stale — an "empty" entry needs the constructor's values
   (port 22, password auth, both save-flags FALSE). Clearing it again after a
   successful connect is cheap hygiene and is included.
4. **No secret can be stored**: the save-flags are forced FALSE for Quick
   Connect in the field-read path, not merely unchecked in the UI — a disabled
   checkbox still reports its last checked state to `IsDlgButtonChecked`
   (`dialogs.cpp:795-796`). With the flags FALSE the encryption branches
   (`dialogs.cpp:827-839`, `863-875`) never run, so no blob is created and the
   master-password prompt is never raised on Quick Connect's behalf. Quick
   Connect is also removed from the password-manager re-encryption sweep
   (`sftp.cpp:293-294`), which would otherwise process an entry that can no
   longer hold blobs.
5. **Visibly unavailable**: both save-secret checkboxes (and Save, per D8) are
   disabled while the Quick Connect row is selected. This is applied from
   `ConnectUpdateButtons` (`dialogs.cpp:965-972`), which already branches on
   "is a bookmark selected" and runs *after* the selection is set — the
   field-load path cannot do it, because at `WM_INITDIALOG` the fields are
   loaded (`dialogs.cpp:1019/1024`) **before** the row is selected
   (`1020/1025`), so a selection query there returns "no selection". The rule
   must also survive the auth-method radio buttons, which re-run
   `ConnectSetAuthMode` from three separate sites (`dialogs.cpp:756`, `1035`,
   `1038`).

**Rationale**: matches the user's framing ("z principu se nic neukládá") and
removes a secret at rest that no user asked for. Bookmarks are untouched, which
keeps the change small and reviewable (constitution III).

**Kept out**: known-hosts trust records still list every host connected to,
including via Quick Connect. The spec puts host-key trust out of scope, and
deleting those records would weaken host verification rather than privacy.

## D5a — Guaranteeing the purge actually happens (FR-004)

**Finding**: `SaveConfiguration` is not guaranteed to run — the application
calls it on exit only when auto-save is enabled (`src/mainwnd3.cpp:6764-6765`),
otherwise only on an explicit Save/Export Configuration. And the purge cannot
move into `LoadConfiguration`, because the registry key handed to it is opened
**read-only** (`src/regwork.cpp:77`, `KEY_READ`), so a delete there fails.
Relying on the retained `DeleteKey` alone would therefore satisfy FR-004 only
for users who happen to have auto-save on.

**Decision**: bump the plugin's config version to 2 and, when a configuration
older than that is loaded, force one purge-only configuration save through the
SDK's `CallLoadOrSaveConfiguration(FALSE, …)` callback — the save branch hands
the callback a writable key. The plugin already has the version machinery
(`CURRENT_CONFIG_VERSION`, `CFG_VERSION`, `Config.Version`); it is simply never
compared today, so this feature adds the first upgrade branch. In-repo
precedent for the forced-save call: `src/plugins/checkver/checkver.cpp:202`.
Trigger it once, from the plugin's own lifecycle (`Connect`/`Release`), guarded
by the version flag so it never repeats.

**Rationale**: FR-004 says "with no user action", and this is the only
mechanism that keeps that promise for every user. It is a purge, not a
migration: nothing is read from the old subtree before it is deleted.

**Correction after verification.** The purge runs the first time the **plugin is
loaded**, not at application start — measured: starting the application and
killing it leaves the stale subtree in place, because a plugin that loads on
demand has not executed any code yet. Loading the plugin (opening its connect
dialog) and then killing the application does purge it, which proves the forced
save path works rather than the ordinary exit save. This is the earliest moment
any plugin code can run, so it is the strongest guarantee a plugin can offer;
spec FR-004's "on first run" is corrected to "the first time the plugin is
loaded". A user who has stale quick-connect data has used the plugin before, and
the purge happens before they can interact with the dialog again.

**Alternatives considered**: purge on the next ordinary save (fails FR-004 when
auto-save is off); purge in `LoadConfiguration` (impossible — read-only key);
delete the whole plugin config key (destroys bookmarks — unacceptable).

## D6 — Empty bookmarks

**Decision**: move the address/port validation from "read the fields" to
"connect". All of it lives in one place — `ConnectReadFields`
(`src/plugins/sftp/dialogs.cpp:777-788`) — and that function already receives a
`forConnect` flag that is TRUE on exactly one path, so gating the two checks on
it is the whole change. Creating **or saving** a bookmark then accepts empty
fields (spec FR-007 says required "at the moment the user attempts to connect,
not when a bookmark is created or saved"); only a non-empty bookmark name is
required, as today. The connect path keeps today's checks and messages
verbatim.

**Rationale**: one gate, three callers, no new error paths. A partially filled
bookmark (name and user typed, host not yet known) must be savable for US2's
"complete it later" flow to work at all.

**Two corrections the change makes mandatory** (found by the code map):
- **Port normalization.** With the port field empty, `GetDlgItemInt` returns 0
  and the entry would store `Port = 0`. On the non-connect path the port is
  normalized to the default (22) instead, so nothing invalid is persisted.
- **Empty string vs no value.** `dialogs.cpp:791` passes raw buffers, so an
  empty host becomes `""` in memory but comes back as `NULL` after a restart
  (the registry writer skips empty strings). Normalizing to `NULL` at the same
  place the neighbouring fields already do keeps in-memory and post-restart
  state identical, and removes the only place where a nameless address-less
  entry could render as a blank list row (`dialogs.cpp:904` tests
  `Address != NULL` without checking for emptiness — tightened at the same
  time).

**Kept deliberately unchanged**: "New bookmark" keeps its feature-017 meaning —
it snapshots the *current* fields under a new name rather than creating a blank
record. US2 only requires that this succeed when the fields *are* empty, and
redefining the button would remove a working feature.

**Out of scope, recorded**: the list-label fallback contains a hardcoded English
`"(unnamed)"` (`dialogs.cpp:904`). It is unreachable while blank names are
rejected, and adding a string ID would introduce a translation row; left as
follow-up.

## D8 — Save on the Quick Connect row (US1 × US2 interaction)

**Finding**: the Save button writes the dialog fields into *whatever row is
selected*, including Quick Connect (`dialogs.cpp:1135-1151` →
`ConnectEntryForItemData` maps the Quick Connect row to `Config.QuickConnect`).
With D6 removing the validation from Save and D5 making Quick Connect
ephemeral, "Save" on the Quick Connect row would otherwise write values into an
entry that is by definition never stored — a control that appears to work and
silently does nothing.

**Decision**: Save is **disabled while the Quick Connect row is selected**,
using the same enable/disable pass that already governs Rename/Delete/Duplicate
(`ConnectUpdateButtons`, `dialogs.cpp:965-972`). Saving is a bookmark
operation; Quick Connect has nothing to save into.

**Rationale**: keeps the two stories coherent, matches the spec's requirement
that inapplicable controls are visibly unavailable rather than silently inert
(same treatment as the save-secret checkboxes), and needs no new strings.

**Alternatives considered**: leave Save enabled and let it commit to the
transient entry (a control that lies), or turn it into "Save as new bookmark"
for Quick Connect (new behaviour, new confirmation flow — out of scope).

## D10 — Text the estimator cannot see

**Finding**: three kinds of text never reach the width estimator, because it
only measures what is in the template:

1. **A button retitled at run time.** `IDB_HOSTKEY_TRUST` is re-captioned with
   `IDS_HOSTKEY_ACCEPTNEW` (`dialogs.cpp:182`), whose longest translation needs
   ~111 units. The button must therefore satisfy both that and its dialog-row
   need of ~123.
2. **Five statics that are empty in the template** and filled at run time:
   the password prompt, the host-key explanation, and the chmod / owner-group /
   rename target lines. The host-key text has ample line budget, but the
   **password prompt is one unit from overflowing**: its longest string
   (`IDS_ENTERPASSPHRASE_RETRY`) needs ~407 units against a 2×206 = 412-unit
   budget — *before* the `%s` file path is substituted into it. In practice it
   already overflows.
3. **Two hardcoded English captions** set at run time —
   `SetDlgItemTextA(hwnd, IDB_CONNECT, "Close")` (`dialogs.cpp:1012`) and the
   retitle above. The `"Close"` literal is a localization defect with no layout
   impact.

**Decision**: the host-key button is sized for the longer of its two captions,
and the password prompt static is enlarged (more height and/or width) so a
realistic passphrase-retry message with a path fits. Both are within FR-009 —
"no text … may be visually truncated" is about what the user sees, not about
what the estimator can measure. The hardcoded `"Close"` caption is recorded as
follow-up (fixing it needs a new string id, i.e. a translation gap, which this
feature deliberately avoids).

## D11 — A pre-existing overlap in the configuration dialog

**Finding**: in the English template the configuration dialog already has two
overlapping controls on the same row — the "rights" radio button spans x 14–174
while the "show octal" checkbox spans x 130–248 (`lang.rc2:258` and `:260`),
44 units of overlap. It also caps the widener on the radio button below its
authored width.

**Decision**: fix it as part of the relayout — FR-009/FR-010 require that
nothing overlaps after the change, and this row is one of the two that drive the
dialog's new width. Moving the checkbox below the radio buttons (where 31 units
of vertical space are free) would let the dialog grow by far less; the exact
arrangement is an implementation choice recorded in the task list.

## D12 — Accelerators: translations clean, English has three collisions

**Finding**: across all eight shipped languages there are **zero** duplicate
accelerators in any SFTP dialog — the merge's de-duplication pass already
resolved them. The **English template**, however, has three collisions, all in
the connect dialog: `New`/`Co&nnect` (n), `D&uplicate`/`&User` (u),
`&Password`/`Pass&phrase` (p).

**Decision**: keep every accelerator letter exactly as it is. Geometry-only
edits leave accelerators byte-identical, and the de-duplication pass prefers the
letter already marked, so a section with no duplicates is left alone. The three
English collisions are **recorded as follow-up, not fixed here**: fixing them
means changing English label text, which creates translation gaps and re-rolls
accelerator assignment across all eight languages — exactly the risk this
feature's FR-008 exists to avoid.

## D9 — Ordering trap on a failed connect

**Decision**: move the "remember which entry was last used" assignment so it
happens only after the connect fields validate
(`Config.LastBookmark` is set at `dialogs.cpp:1203`, *before*
`ConnectCommitToEntry` validates at `1204`).

**Rationale**: pre-existing, but empty bookmarks make it visible — a failed
connect attempt from an empty bookmark would still make that bookmark the entry
the dialog seeds next time. The spec's assumption is that selection restore is
unchanged in *behaviour*; restoring an entry whose connect just failed is not
that behaviour.

## D7 — Release documentation

**Decision**: a `CHANGELOG.md` entry under the existing `[Unreleased]` heading
(created by feature 052): Fixed — clipped texts in the SFTP dialogs in every
non-English language; Changed — Quick Connect no longer remembers anything and
cannot store secrets; Changed/Fixed — a bookmark can be created empty. Version
and build-number bumps stay with the release change, per the constitution.
