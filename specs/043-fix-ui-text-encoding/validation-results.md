# Validation Results (FR-015)

**Feature**: `043-fix-ui-text-encoding`
**Date**: 2026-07-27
**Builds**: Debug x64 and Release x64 (full), both `Počet chyb: 0`
**Tests**: `saltests` — **1126 checks, 0 failed** (1118 before; +8 for this feature)
**Guard**: `python tools/check_encoding.py --strict` → exit 0, with 5 rules

---

## The three reported defects

### Report 1 — language selection list

Read directly out of the control with `LVM_GETITEMTEXTW`:

```
Čeština (Česko)
Holandština (Nizozemsko)
Angličtina (Spojené státy)
Francouzština (Francie)
Němčina (Německo)
Maďarština (Maďarsko)
Rumunština (Rumunsko)
Slovenština (Slovensko)
Španělština (Španělsko, mezinárodní řazení)
```

**PASS — 9 language names, 0 mojibake.** Was `ÄŚeÅˇtina (ÄŚesko)`,
`AngliÄŤtina (SpojenĂ© stĂˇty)`, `NÄ›mÄŤina (NÄ›mecko)`.

### Report 2 — configuration language field

Fixed with the same value and the equivalent helper
(`SalSetDlgItemTextU8`). See the honest limitation in "Not verified" below.

### Report 3 — Quick Rename caption (F2), and F5/F6 by the same family

Caption read with `GetWindowTextW`; the quoted name extracted and compared
**case-sensitively against the file system**:

| Language | Caption | Verdict |
|---|---|---|
| english | `Rename directory "Тест-Ελλάδα-测试 +ěš - Copy" to` | PASS |
| czech | `Přejmenovat adresář "Тест-Ελλάδα-测试 +ěš - Copy" na` | PASS |
| german | `Verzeichnis "Тест-Ελλάδα-测试 +ěš - Copy" umbenennen in` | PASS |
| french | `Renommer répertoire "Тест-Ελλάδα-测试 +ěš - Copy" en` | PASS |
| dutch | `naam map "Тест-Ελλάδα-测试 +ěš - Copy" wijzigen naar` | PASS |
| hungarian | `"Тест-Ελλάδα-测试 +ěš - Copy" mappa átnevezése erre` | PASS |
| romanian | `Redenumeste director "Тест-Ελλάδα-测试 +ěš - Copy" in` | PASS |
| slovak | `Premenovať adresár "Тест-Ελλάδα-测试 +ěš - Copy" na` | PASS |
| spanish | `Cambiar nombre de directorio "Тест-Ελλάδα-测试 +ěš - Copy" a` | PASS |

**9/9.** In each case the extracted name is byte-identical to the directory on
disk, is non-ASCII, and contains no mojibake sequence. Was
`Přejmenovat adresář "Ð¢ÐµÑ…"`.

F5 Copy, F6 Move, F8 Delete, pack, unpack, the NTFS attribute confirmation and
the Find-log Ignore prompt all build their caption through the **same**
`CTruncatedString` family and were repaired in the same change.

---

## Regression matrix (FR-013) — nothing that worked before is broken

The user's condition was that no already-working surface may break. Every
scenario below was re-run **after** all changes.

| Feature | Scenario | Result |
|---|---|---|
| 042 | Reported Find search, exact code points | PASS (3/3 names, 4 items) |
| 042 | 83 fixture names verbatim, all scripts | PASS |
| 042 | No `?`, no `U+FFFD`, no mojibake across 83 names | PASS |
| 042 | Find Path column rooted | PASS |
| 042 | Type-to-search: Cyrillic / Czech / emoji / ASCII / no-match | PASS (5/5) |
| 042 | Duplicate-name notice, all 9 languages | **PASS 9/9** |
| 041 | Panel information line, reported `.mkv` | PASS (unchanged code path) |
| 004/005 | Panel file lists, long paths | PASS |

**Zero regressions.**

---

## The feature 042 regression this work found and fixed

`dialogs5.cpp` composed a plugin's name into a template that feature 042 had
converted to `LoadStrU8`. The substituted argument there is a local variable
`name`, a copy of the ANSI `p->Name`, which 042's classifier did not recognise as
plugin metadata — so it treated the site as a file-name composition. The result
was a message mixed the *other* way round: the plugin name would render and the
localized words would not.

Reverted to `LoadStr` and annotated. A sweep over every site 042 converted
confirmed this was the **only** such misclassification; three other candidates
flagged by the sweep (`fileswn6.cpp`, `fileswn7.cpp` ×2) were checked and are
genuine file names (`char name[SAL_MAX_PATH_UTF8]`, `f->Name`), correctly
converted.

---

## The guard, widened and proven (FR-011, FR-012, SC-006, SC-007)

Feature 042's guard passed cleanly while all three of these defects were
present. That is not a small miss — it is evidence that the rules described two
*examples* rather than the defect. Two rules were added:

- `utf8-to-legacy-sink` — a value from a known UTF-8 source reaching a
  byte-oriented list-view, window-text, status-bar or combo call.
- `ansi-template-caption` — a composed caption whose template is the ANSI
  `LoadStr` and whose substituted value is a name.

**SC-007 — would the widened guard have caught the reported defects?** Run
against the tree *before* any fix:

```
dialogs2.cpp:883  [utf8-to-legacy-sink]     ListView_SetItemText(HListView, i, 0, buff);
dialogs4.cpp:871  [utf8-to-legacy-sink]     SetDlgItemText(HWindow, IDE_LANGUAGE, buff);
fileswn5.cpp:2385 [ansi-template-caption]   subject.Set(buff, editName);
```

**All three detected.** The guard was widened and proven *before* the fixes were
written, deliberately — a guard authored afterwards cannot be shown to detect
anything.

The guard also learned the codebase's real design shape: a legacy call that is
the `else` branch of a wide attempt is **not** a finding. Without that, ~60
correct sites would be flagged and everyone would learn to ignore the guard,
which is worse than having none.

Final state: **0 findings across all 5 rules**, with 17 sites annotated in place
with a reason.

---

## Plugin boundary (FR-014)

| Check | Result |
|---|---|
| files changed under `src/plugins/` | **0** |
| `src/plugins.h` diff | **empty** |
| `src/spl_gen.h` diff | **empty** |
| `src/msgbox.cpp` diff | **empty** |

`plugins3.cpp` (`CSalamanderGUI::SetSubjectTruncatedText`) *was* changed — it had
no wide path at all, so any plugin showing a non-ASCII file name in its subject
line was broken even in English. It now uses `SalSetWindowTextU8`, which takes
the wide path for UTF-8 and falls back to the legacy call for plugins that
compose in the legacy code page. No interface changed; plugin output that was
correct stays correct, and output that was broken is now correct.

---

## What was NOT verified, and why

1. **The configuration page's language field was not read at runtime with a
   value in it.** `CCfgPageRegional::LoadControls()` runs only when that page is
   activated, and the automation opened the sheet on a different page, so the
   control was legitimately empty — an empty field proves nothing. What *is*
   verified: the same `GetLanguageName()` value renders correctly in the
   language list (above), and the field now uses `SalSetDlgItemTextU8`, the same
   helper family used correctly at 20+ existing sites. Recorded as
   verified-by-shared-mechanism, not by direct observation.

2. **Not every one of the ~25 window-text/number sites was triggered
   individually.** Reaching all of them means provoking volume-information
   dialogs, occupied-space calculations, drive-not-ready states and beta-expiry
   paths. What was verified: the mechanism (unit tests), the three reported
   surfaces end-to-end in 9 languages, the full 042/041 regression matrix, and
   the guard proving no legacy sink remains on a UTF-8 path.

3. **The drag image (`fileswn9.cpp`) was not verified by dragging a file.**
   Mouse-drag automation was out of reach for this session. The change follows
   the same wide-primary/legacy-fallback shape as ~60 existing sites and is
   covered by the guard.

4. **Plugin dialogs were not exercised at runtime** — checked by diff, as in
   feature 042.

5. **13 composed-message sites remain deliberately unconverted** (plugin
   metadata, a network share name, a configuration name), each annotated with
   its reason. Their values are not produced by this application, so converting
   the template around them could mix the message the other way — precisely the
   mistake this feature had to undo in `dialogs5.cpp`.

---

## Note for whoever meets this class next

Two things worth carrying forward:

**English cannot reproduce any of it.** English resources are pure ASCII, ASCII
is valid UTF-8, so a composed string converts cleanly and every one of these
surfaces looks correct. All three defects survived this long for that reason
alone. Any future check of this class has to run in a localized build.

**A guard is only as good as the shape it describes.** Feature 042's guard was
written around the two defects then in hand and passed while three more of the
same class sat in the tree. The rules now describe *a UTF-8 value reaching a
byte-oriented call*, which is the defect itself — and the "would it have caught
the reported bugs" check (SC-007) is now part of the process, not an
afterthought.
