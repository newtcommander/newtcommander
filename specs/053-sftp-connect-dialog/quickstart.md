# Quickstart: Validating the SFTP Dialog Changes

**Feature**: 053-sftp-connect-dialog · Phase 1 artifact

> **Implementation results (2026-08-06)**
>
> - **Layout, mechanically verified — PASS.** The estimator reports **0 clipped
>   controls across all 11 registered languages**, down from 124 in the 8 shipped
>   ones. A template-level check confirms no control extends past its dialog, no
>   two controls overlap (the configuration dialog's pre-existing 44-unit overlap
>   is gone), and every English caption fits.
> - **FR-008, mechanically proven — PASS.** 2229 rows compared across 11
>   languages before/after the refresh: **0 text differences, 0 state
>   differences**. `translate.slt --verify` round-trips all 290 committed files
>   byte-exactly.
> - **`--check-layout` rejected as a gate.** It reports `LAYOUT WARNINGS` for
>   every language of an **untouched** module too (verified against `renamer`),
>   because its non-clean exit is the build watchdog killing an interactive
>   fallback. Informational only — see research D4.
> - **Scenario 1 (Quick Connect) — PASS.** With Quick Connect selected every field
>   is empty and "Uložit heslo" / "Uložit heslo ke klíči" / "Uložit" are all
>   greyed out (verified by reading each control's enabled state, not just by
>   eye). After connecting with a typed password and exiting, the plugin's config
>   key contains **no `QuickConnect` subkey at all** and `Version = 2`. Selecting
>   a bookmark re-enables the same three controls — the rule follows the
>   selection.
> - **Purge, including with no ordinary save — PASS.** A pre-053 state was
>   recreated (`QuickConnect` with `Save Password = 1` and a password blob,
>   `Version = 1`); the plugin was loaded and the process then **killed**, so no
>   ordinary configuration save could run. The subtree was gone anyway and the
>   version was 2, while both bookmarks and the known-hosts records survived and
>   bookmark `a2` kept its stored password. Note the purge happens on **plugin
>   load**, not at application start — see research D5a.
> - **Scenario 2 (bookmarks unaffected) — PASS.** Bookmark `a2` keeps
>   `Save Password = 1` and its blob across all of the above.
> - **Scenario 3 (empty bookmark) — PASS.** "New" with every field empty
>   succeeded (no missing-host error), the entry shows its **name** in the list
>   rather than a blank row, Connect on it is refused with the existing
>   "V zadané cestě chybí název hostitele.", and after a clean exit it is in the
>   registry as `Name='Prazdna-zalozka'` with no address and `Port=22` — the
>   normalized default, not 0.
> - **Full connection end to end — PASS.** Quick Connect to the reference server
>   (`127.0.0.1:2222`, `tctest`, typed password) connected; the panel showed
>   `sftp:tctest@127.0.0.1:2222/` with the remote root listing.
> - **Scenario 6 (connect harness) — PASS.** `run_keyauth.cmd`: 7 passed,
>   0 failed.
> - **Scenario 4 visual pass — PARTIAL.** The connect dialog was verified in
>   Czech in both states (screenshots), and every label renders in full
>   ("Soubor s klíčem:", "Heslo ke klíči:", "Počáteční cesta:"). The other
>   dialogs were not seen: the configuration page **cannot be opened at all**
>   (pre-existing defect, investigation.md §9a), and chmod / owner-group /
>   symlink / host-key / rename / logs need specific remote-file flows. Their
>   geometry is covered mechanically (estimator 0, template check OK).
> - Two pre-existing defects found while verifying, both out of scope and
>   recorded: the configuration page is invisible (§9a) and one black-holed
>   address consumes the whole connect timeout, so `localhost` fails where
>   `127.0.0.1` works (§9b).
> - Builds green: `build.cmd`, `build.cmd full`, clean `rebuild`,
>   `full release`, `build_langs.cmd --module sftp --force`.

## Prerequisites

- Windows 11, VS2022 C++ workload, Windows SDK (repo standard)
- **Python 3.x on PATH** — mandatory since feature 052 (`build.cmd` fails
  without it)
- Local SFTP reference server for the connect scenarios: Docker container
  `tandem-sftp` on `localhost:2222`, login `tctest` / `tandem123`
- Czech UI language available (built by default)

## Build

```batch
build.cmd full                                   :: Debug x64 + runtime data + language modules
src\vcxproj\build_langs.cmd --module sftp        :: rebuild just the sftp language modules
```

## Scenario 1 — Quick Connect keeps nothing (US1, SC-001/002/005)

1. Open the connect dialog (Ctrl+Shift+S). Select **Quick Connect**.
2. Confirm **Save password** and **Save passphrase** are greyed out and cannot
   be ticked, and that **Save** is greyed out too.
3. Fill in host `localhost`, port `2222`, user `tctest`, password
   `tandem123`; connect. Confirm the panel opens.
4. Reopen the dialog, select Quick Connect. **Expected**: every field empty.
5. Exit the application completely (so the configuration is saved), restart,
   open the dialog, select Quick Connect. **Expected**: still empty.
6. Inspect stored settings — the plugin's config key must have **no
   `QuickConnect` subkey at all**:

   ```powershell
   Get-ChildItem 'HKCU:\Software\Tandem Commander\0.1\Plugins*' -Recurse |
     Where-Object { $_.Name -match 'QuickConnect' }
   ```

   **Expected**: no output. (Find the plugin's own key first with
   `Get-ChildItem 'HKCU:\Software\Tandem Commander\0.1' -Recurse -Depth 3 |
   Select-String -Pattern 'SFTP'` if the exact path is unknown.)
7. **Purge check (SC-005)**: before installing the fixed build, run the old
   one, use Quick Connect with a saved password, exit — confirm the
   `QuickConnect` subkey exists (with a `PasswordE`/`PasswordS` value in it).
   Then run the fixed build once and exit. **Expected**: the subkey is gone,
   with no user action.
8. **Purge with auto-save OFF** — this is the case the plain
   delete-on-save would miss, so it must be tested explicitly: in
   Configuration, turn **off** saving the configuration on exit; recreate the
   stale `QuickConnect` subkey (import the `.reg` you exported in step 7);
   start the fixed build, then close it **without** saving configuration.
   **Expected**: the subkey is gone anyway (the one-time forced purge ran), and
   bookmarks and known hosts are untouched.

9. **No master-password prompt for Quick Connect**: configure a master
   password in the password manager, then use Quick Connect with a typed
   password. **Expected**: no prompt to unlock the password store appears — a
   Quick Connect secret never reaches it.

## Scenario 2 — Bookmarks keep working (US1 scenario 5, FR-005)

1. Create a bookmark for the test server with **Save password** ticked;
   connect once so the password is stored; exit and restart.
2. Select the bookmark. **Expected**: fields restored, the secret placeholder
   present, save options available and ticked, Save enabled.
3. Switch to Quick Connect and back. **Expected**: the bookmark's save options
   and stored secret are unaffected; Quick Connect stays blank with its options
   greyed out.

## Scenario 3 — An empty bookmark can be created (US2, SC-003)

1. With every connection field empty, press **New**, type the name
   `Prepared`, confirm. **Expected**: the bookmark is created and appears in
   the list as `Prepared` — no "host name missing" message.
2. Close and reopen the dialog. **Expected**: `Prepared` is still listed with
   empty fields and port 22.
3. Select `Prepared` and press **Connect**. **Expected**: the existing
   "host name is missing" message appears and no connection is attempted.
4. Fill in the test server's details and press **Save**. **Expected**: accepted;
   reopening the dialog shows the saved values under the same name.
5. Fill in only a user name (no host) and press **Save**. **Expected**:
   accepted — a partially filled bookmark is savable.
6. Press **New** with an empty name. **Expected**: nothing is created (blank
   names stay rejected).
7. Exit and restart, then reopen the dialog. **Expected**: the empty bookmark
   survived the save/load round trip with its name intact.

## Scenario 4 — No text is clipped anywhere in the plugin's dialogs (US3, SC-004)

**Static check** (fast, offline — the same estimator the build's widener uses):

```bash
python - <<'EOF'
import sys, pathlib
sys.path.insert(0, 'tools')
from translate.layout import estimate_width
from translate.slt import load
from translate.config import load_languages
bad = 0
for lang in load_languages():
    p = pathlib.Path(f'translations/{lang.folder}/sftp.slt')
    if not p.is_file():
        continue
    for sec in load(p).sections:
        if sec.kind != 'DIALOG' or not sec.rows:
            continue
        for row in sec.rows[1:]:
            if len(row.numbers) == 6 and row.text.strip():
                cx = row.numbers[3]
                if estimate_width(row.text) > cx:
                    bad += 1
                    print(f'{lang.folder} dlg {sec.number} id {row.numbers[0]}: '
                          f'cx={cx} need~{estimate_width(row.text)} {row.text!r}')
print('clipped controls:', bad, '(was 124)')
EOF
```

**Expected**: `clipped controls: 0`.

**Template-level check** (catches overlap and out-of-bounds controls, which the
estimator cannot see) — parse `src/plugins/sftp/lang/lang.rc2` and assert, per
dialog, that every control stays inside the dialog, no two controls overlap, and
each English caption fits its own control.

**Expected**: `template geometry: OK`.

**Not a gate: `build_langs.cmd --module sftp --check-layout`.** It reports
`LAYOUT WARNINGS` for every language of an untouched module as well (verified
against `renamer`), so it cannot tell a good change from a bad one. Run it if you
want the Translator's own opinion, but do not read a warning as a regression.

**Visual pass — needs a human.** Automating it failed in this environment and the
reasons are worth recording so nobody repeats the attempt: the main window's menu
is owner-drawn (`GetMenu` returns 0), plugin menu command ids are assigned
dynamically at menu-build time, a synthetic Ctrl+Shift+S does not reach the
application, `PrintWindow` returns an all-black bitmap for a dialog instantiated
straight from a `.slg` resource (no owner, no message loop of its own), and screen
capture needs the foreground, which Windows denies a background script.

So: with the Czech UI, open **Ctrl+Shift+S** and the plugin's settings page, plus
the permissions, symlink, owner/group, host-key, password, rename and logs
dialogs, and confirm no text is cut off and nothing overlaps. Repeat for French
(the widest translations). The connect dialog is now 420×250 dialog units instead
of 340×250, and the settings page 366×250 instead of 260×236 — both are expected
to be visibly wider.

## Scenario 5 — Translated wording is untouched (FR-008)

Compare each `translations/<lang>/sftp.slt` against a copy taken before the
refresh and assert, per `(section, row index)`, that the quoted text **and** the
translation-state field are identical — differences must be confined to the
geometry numbers:

```bash
python - <<'EOF'
import sys, pathlib
sys.path.insert(0, 'tools')
from translate.slt import load
before_dir = pathlib.Path('<scratch>/slt')   # copies taken before the refresh
bad = checked = 0
for before in sorted(before_dir.glob('*.sftp.slt')):
    lang = before.name.split('.')[0]
    after = pathlib.Path(f'translations/{lang}/sftp.slt')
    b, a = load(before), load(after)
    bi = {(s.key, i): r for s in b.sections for i, r in enumerate(s.rows)}
    ai = {(s.key, i): r for s in a.sections for i, r in enumerate(s.rows)}
    assert set(bi) == set(ai), f'{lang}: row set changed'
    for k in bi:
        checked += 1
        if bi[k].text != ai[k].text or bi[k].numbers[-1] != ai[k].numbers[-1]:
            bad += 1
            print(f'{lang} {k}: CHANGED')
print(f'{checked} rows compared, {bad} differences')
EOF
python -m translate.slt --verify
```

**Expected**: `0 differences`, and the writer round-trips every committed file
byte-exactly. **Measured**: 2229 rows across 11 languages, 0 differences.

## Scenario 6 — Connect regressions (feature 051 harness)

```batch
src\plugins\sftp\test\run_keyauth.cmd
```

**Expected**: all scenarios pass, no hang, no leak — the dialog changes must
not disturb the authentication paths fixed in feature 051.

## Scenario 7 — Full build regression

```batch
build.cmd rebuild
build.cmd full release
```

**Expected**: both complete with 0 errors; `CHANGELOG.md` carries the entries
for this feature.
