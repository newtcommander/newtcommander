# Audit B — Connect Window UX / Bookmark Lifecycle (Feature 017)

Scope: the Ctrl+Shift+S "Connect to SFTP Server" dialog (`IDD_CONNECT`) and the
bookmark create/edit/save/delete lifecycle. Files:
`src\plugins\sftp\dialogs.cpp`, `dialogs.h`, `lang\lang.rc2`, `lang\lang.rh`,
`sftp.h`, plus the consumers `fs.cpp` and `sftp.cpp`.

Bottom line: the dialog can **load** a bookmark into the fields but has **no way
to commit field edits back into a bookmark**. There is no Save action, and the
one code path that could persist edits (Connect) writes back only to
`QuickConnect`, never to a selected bookmark. This single structural gap causes
BOTH reported problems: the bookmark UX feels "muddled" (edits vanish), and a
typed+saved password is never stored on a bookmark.

---

## 1. Current dialog map (resource template)

Template `IDD_CONNECT` (id 500), `lang\lang.rc2:104-141`, size 340x232 DLU,
two-column layout: bookmark list on the left, connection fields on the right.
Control IDs from `lang\lang.rh:103-129`.

Left column (bookmark management):

| Control | ID (rh) | Type | Text | Rect (rc2) |
|---|---|---|---|---|
| `IDT_BOOKMARKS` | 625 | LTEXT | "&Bookmarks:" | 7,7 `lang.rc2:109` |
| `IDL_BOOKMARKS` | 600 | LISTBOX (LBS_NOTIFY) | — | 7,17,110,190 `lang.rc2:110` |
| `IDB_NEWBOOKMARK` | 620 | PUSHBUTTON | "&New" | 7,210 `lang.rc2:111` |
| `IDB_RENAMEBOOKMARK` | 621 | PUSHBUTTON | "&Rename" | 43,210 `lang.rc2:112` |
| `IDB_REMOVEBOOKMARK` | 622 | PUSHBUTTON | "Re&move" | 83,210 `lang.rc2:113` |

Right column (connection fields):

| Control | ID | Type | Text | Rect |
|---|---|---|---|---|
| `IDT_HOSTADDRESS`/`IDE_HOSTADDRESS` | 601/602 | LTEXT/EDIT | "&Host:" | `lang.rc2:115-116` |
| `IDT_PORT`/`IDE_PORT` | 603/604 | LTEXT/EDIT(ES_NUMBER) | "P&ort:" | `lang.rc2:117-118` |
| `IDT_USERNAME`/`IDE_USERNAME` | 605/606 | LTEXT/EDIT | "&User:" | `lang.rc2:119-120` |
| `IDC_AUTHPASSWORD` | 607 | AUTORADIO (WS_GROUP) | "&Password" | `lang.rc2:122` |
| `IDC_AUTHKEY` | 608 | AUTORADIO | "Private &key" | `lang.rc2:123` |
| `IDT_PASSWORD`/`IDE_PASSWORD` | 609/610 | LTEXT/EDIT(ES_PASSWORD) | "Pass&word:" | `lang.rc2:125-126` |
| `IDC_SAVEPASSWORD` | 611 | AUTOCHECKBOX | "&Save password" | `lang.rc2:127` |
| `IDT_KEYFILE`/`IDE_KEYFILE` | 612/613 | LTEXT/EDIT | "Key &file:" | `lang.rc2:129-130` |
| `IDB_BROWSEKEY` | 614 | PUSHBUTTON | "..." | `lang.rc2:131` |
| `IDT_PASSPHRASE`/`IDE_PASSPHRASE` | 615/616 | LTEXT/EDIT(ES_PASSWORD) | "Pass&phrase:" | `lang.rc2:132-133` |
| `IDC_SAVEPASSPHRASE` | 617 | AUTOCHECKBOX | "Save p&assphrase" | `lang.rc2:134` |
| `IDT_INITIALPATH`/`IDE_INITIALPATH` | 618/619 | LTEXT/EDIT | "&Initial path:" | `lang.rc2:136-137` |

Bottom-right (commit):

| Control | ID | Type | Text | Rect |
|---|---|---|---|---|
| `IDB_CONNECT` | 624 | DEFPUSHBUTTON | "Co&nnect" | 165,210 `lang.rc2:139` |
| `IDCANCEL` | — | PUSHBUTTON | "Cancel" | 283,210 `lang.rc2:140` |

Declared-but-unused / dead resources:
- `IDB_COPYBOOKMARK` (623, `lang.rh:127`) — a Duplicate/Copy button ID exists but
  there is **no control in the template and no handler**. Feature was planned,
  never wired.
- `IDM_SRVCONTEXTMENU` (1100) / `IDM_SRVCONTEXTMENU2` (1101) (`sftp.rh2:24,26`) —
  bookmark-list right-click context menus (connect vs organize variants) are
  declared but **no menu resource exists in lang.rc2 and no WM_CONTEXTMENU handler
  exists** in `ConnectProc`. Dead declarations copied from the FTP plugin.

Model fields present in `CSFTPServer` (`sftp.h:54-90`) but with **no UI control**:
`TargetPanelPath`, `KeepAliveSendEvery`, `KeepAliveStopAfter`, `UseCompression`.
(Per-bookmark keepalive/compression can only ever take defaults today.)

---

## 2. Current runtime behavior (`ConnectProc`, `dialogs.cpp:717-855`)

### WM_INITDIALOG (`dialogs.cpp:722-744`)
- `ConnectFillBookmarkList` (`684-695`) fills `IDL_BOOKMARKS` from
  `Config.Bookmarks`, storing the array index in each item's `LB_SETITEMDATA`.
  **No "Quick Connect" pseudo-item is added** — quick-connect is an invisible
  mode entered only by having no selection.
- Organize mode: retitles the window and relabels `IDB_CONNECT` -> "Close"
  (`728-732`). Nothing else changes; all connection fields remain visible and
  editable.
- Seeds the fields: if `Config.LastBookmark` in range, loads that bookmark and
  selects it in the list (`734-739`); otherwise loads `Config.QuickConnect`
  with no list selection (`741`).

### Selecting a bookmark — `IDL_BOOKMARKS` / `LBN_SELCHANGE` (`757-768`)
- Calls `ConnectLoadServerToFields` (`563-577`) which overwrites every field
  from the selected bookmark. **It always blanks the password/passphrase edit
  boxes** (`570,573`) and restores the Save checkboxes from the bookmark.
- This **silently discards any edits** the user made in the fields for the
  previously shown entry. There is no dirty check, no prompt, no auto-commit.

### "New" — `IDB_NEWBOOKMARK` (`769-786`)
- Prompts for a name (`ShowRenameDialog`), then `new CSFTPServer` +
  `ConnectReadFields(hwnd, s, NULL, FALSE)` — i.e. it **snapshots the CURRENT
  field contents** into the new bookmark, sets `ItemName`, `Config.Bookmarks.Add`,
  refills, selects the new row.
- Semantics are actually "Save current fields AS a new bookmark", not "create a
  fresh blank entry". A user who wants a blank starting point instead gets a copy
  of whatever is on screen; a user who wants to save what they typed must know
  that "New" is the only way to do it. This is the crux of the muddle.

### "Rename" — `IDB_RENAMEBOOKMARK` (`787-805`)
- Prompts and updates `ItemName` **only**. Does not touch host/user/auth/etc.

### "Remove" — `IDB_REMOVEBOOKMARK` (`806-817`)
- `Config.Bookmarks.Delete(bi)` immediately, **no confirmation, no undo**.

### "Connect" (or "Close" in organize) — `IDB_CONNECT` (`818-847`)
- Organize mode: `EndDialog(IDCANCEL)` (`820-824`).
- Connect mode: reads the current selection; if a bookmark is selected sets
  `bookmark` = `Config.Bookmarks[bi]` and `Config.LastBookmark = bi+1` (`828-836`),
  else `LastBookmark = 0` (`838`).
- `ConnectReadFields(hwnd, d->Result, bookmark, TRUE)` (`839`) reads the fields
  into the **transient result object** `d->Result`, and:
  - if `bookmark == NULL` (quick-connect) it does
    `Config.QuickConnect.CopyFrom(d->Result)` (`842-843`) — quick-connect IS
    persisted.
  - **if a bookmark IS selected, nothing is written back to the bookmark.**
- `EndDialog(IDOK)`.

### Cancel (`848-850`)
- `EndDialog(IDCANCEL)`. **Does NOT undo New/Rename/Remove** — those already
  mutated the live `Config.Bookmarks` in place. Only field edits and the connect
  result are dropped.

### `ConnectReadFields` (`583-682`) — field -> object mapping
- Validates host/port (`591-602`), fills `s` via `s->Set(NULL,...)` (note
  `ItemName` forced NULL — fine for `d->Result`, re-set afterwards for New).
- For password auth: if the password box is non-empty and `SavePassword` is set,
  encrypts via the password manager and stores the blob on `s`
  (`620-642`). If the box is empty, **reuses the selected bookmark's existing
  blob** (`643-649`). Passphrase path is symmetric (`651-680`).
- Key point: this function writes into the object it is *handed*. Connect hands it
  `d->Result` (transient), New hands it a fresh object. **Nothing ever hands it a
  selected bookmark**, so a selected bookmark's stored blob can be *read/reused*
  but never *updated* from the dialog.

### Consumer (`ConnectSFTPServer`, `fs.cpp:74-114`)
- `CSFTPServer srv;` is a **local**; `ShowConnectDialog(parent, FALSE, &srv)`
  fills it, `FillParamsFromServer` copies it into `g_PendingParams`, then `srv`
  is destroyed at function return (`fs.cpp:76-114`). It is **never copied back
  into `Config.Bookmarks`**. Confirms: the connect result is throw-away for
  bookmarks.

### Persistence (`sftp.cpp`)
- `SaveConfiguration` writes `Config.Bookmarks` and `Config.QuickConnect`-adjacent
  data to the registry (`sftp.cpp:548-581`, `SaveServer` `416-441`). It faithfully
  persists whatever is in the bookmark objects — but since the dialog never puts
  the new password/edits into the selected bookmark object, there is nothing to
  save. (`SaveServer` itself looks correct: it stores the blob under
  `SRV_PASSWORDE`/`SRV_PASSWORDS` when `SavePassword && EncryptedPassword != NULL`.)

---

## 3. Findings — what is confusing or broken

### F1 (structural bug, ties to reported problem #2). No write-back to a selected bookmark.
`IDB_CONNECT` persists field edits only to `QuickConnect`
(`dialogs.cpp:842-843`); when a bookmark is selected it writes into the transient
`d->Result` and drops it. Scenario: user opens the dialog with a bookmark
auto-selected (because `LastBookmark>0` selected it at `dialogs.cpp:738`), types a
password, ticks "Save password", clicks Connect. The encrypted blob lands on
`d->Result`, is used for this session, then discarded; `Config.Bookmarks[bi]`
keeps its old (empty) password. Next connect → password gone. **This is the
mechanism behind "password not saved".** (Agent A owns the encrypt/registry chain;
this is the UX-layer half: even a perfectly-encrypted blob is written to the wrong
object.)
Fix direction: give the dialog an explicit action that runs `ConnectReadFields`
against the *selected bookmark object* (see redesign). At minimum, in
`IDB_CONNECT`, when `bookmark != NULL`, also
`bookmark->CopyFrom(d->Result); bookmark->SetString(&bookmark->ItemName, <name>)`
so edits+password reach the stored bookmark.

### F2 (UX, primary muddle). No explicit "Save" / "Edit" action.
The only way to persist connection fields is "New" (snapshot as a new bookmark) or,
for quick-connect only, Connect. There is no "Save"/"Update bookmark". Editing an
existing bookmark's host/user/auth/password is effectively **impossible** — you
must recreate it and delete the old one. This is exactly the user's complaint that
create/edit/save are "pomíchané".

### F3 (UX). Selecting a bookmark silently discards unsaved edits.
`LBN_SELCHANGE` -> `ConnectLoadServerToFields` overwrites the fields with no
dirty check (`dialogs.cpp:757-768`). Combined with F2, edits are volatile and can
vanish on a stray click in the list.

### F4 (UX). "New" conflates two intents.
"Save what I typed" vs "start a fresh entry" both map to one button whose actual
behavior (snapshot current fields under a new name) matches neither label
cleanly (`dialogs.cpp:769-786`).

### F5 (UX). Quick-connect vs bookmark is invisible / ambiguous.
Quick-connect is a hidden mode (no selection) with no list item and no label. The
user cannot tell whether the fields currently represent a saved bookmark or the
scratch quick-connect entry, and cannot deliberately switch to quick-connect once
a bookmark is selected (clicking empty list space does not reliably deselect a
single-select listbox). The FTP sister plugin solves this by showing a literal
"Quick Connect" row as list item 0 (`ftp\dialogs1.cpp:894`, `IDS_QUICKCONNECT`).

### F6 (UX). Connect double-books as an implicit Save (for quick-connect only).
Connect silently updates `QuickConnect` and `LastBookmark`
(`dialogs.cpp:834,838,843`). Users reasonably assume "Connect" also saved their
bookmark edits (it did not — F1). "Connect" and "Save" semantics are entangled and
asymmetric between quick-connect and bookmarks.

### F7 (UX). Remove has no confirmation.
`IDB_REMOVEBOOKMARK` deletes on a single click (`dialogs.cpp:806-817`).

### F8 (UX / consistency). Cancel does not cancel structural edits.
New/Rename/Remove mutate the live `Config.Bookmarks` immediately
(`dialogs.cpp:778,799,813`), so they survive Cancel; only field edits are dropped.
Behavior is inconsistent and surprising (a user who "changed their mind" and hit
Cancel still lost/added/renamed bookmarks). The FTP plugin avoids this by editing a
temp copy (`TmpFTPServerList`) committed only on OK/Close (`ftp\dialogs1.cpp:871,914`).

### F9 (UX). "Organize Bookmarks" cannot actually edit a bookmark.
Organize mode (`dialogs.cpp:728-732`) shows all connection fields but offers only
New/Rename/Remove; with no Save, the visible Host/User/Password fields are inert
for existing entries. The command name promises editing the mode cannot deliver.

### F10 (UX). No reorder, no duplicate, no context menu, no double-click-connect.
- No Move Up/Down; bookmark order is fixed at creation (FTP has
  `CBookmarksListbox::MoveUpDown`, `ftp\dialogs1.cpp:730`).
- `IDB_COPYBOOKMARK` (Duplicate) declared but unwired (F-dead above).
- `IDM_SRVCONTEXTMENU`/`2` declared but unwired.
- `LBN_DBLCLK` on the list is not handled, so double-clicking a bookmark does not
  connect (a near-universal expectation for a list of connections).

### F11 (minor). "Save password"/"Save passphrase" checkbox state is inert for existing bookmarks.
Because there is no write-back (F1/F2), toggling `IDC_SAVEPASSWORD` (611) on a
selected bookmark and pressing Connect changes nothing persistent on that
bookmark.

---

## 4. Redesign proposal (WinAPI-implementable)

Goal: make Create / Edit / Save / Delete / Connect explicit and unambiguous while
staying within a listbox + edit-fields + buttons dialog. The user explicitly wants
visible, distinct functions, so this favors an explicit **Save** button over the
FTP plugin's implicit auto-commit — but borrows FTP's proven pieces (visible Quick
Connect row, temp staging list, reorder, context menu).

### 4.1 List area — make quick-connect and selection explicit
- Add a permanent first row **"Quick Connect"** (new string, e.g.
  `IDS_QUICKCONNECT`) rendered by `ConnectFillBookmarkList`, mapping to
  `Config.QuickConnect` (item data = -1 / sentinel). Bookmarks follow, item data =
  array index (as today). `LastBookmark == 0` selects Quick Connect. This kills F5
  and makes "which entry am I editing" always visible.
- Header static shows current target, e.g. relabel `IDT_HOSTADDRESS` area or add a
  small "Editing: <name>" static above the fields, so Save's target is unambiguous.

### 4.2 Button set — explicit lifecycle
Left column under the list (bookmark management):
- **New** (`IDB_NEWBOOKMARK`) — creates a *blank* bookmark (prompt name), selects
  it, loads empty/default fields. Distinct from Duplicate.
- **Duplicate** (wire the existing `IDB_COPYBOOKMARK` 623) — copies the current
  fields/selected bookmark into a new named entry.
- **Rename** (`IDB_RENAMEBOOKMARK`) — unchanged.
- **Delete** (`IDB_REMOVEBOOKMARK`) — add a Yes/No confirmation (fixes F7);
  disabled when Quick Connect is selected.
- **Move Up / Move Down** (new IDs, e.g. `IDB_MOVEUPBOOKMARK`/`IDB_MOVEDOWNBOOKMARK`)
  — reorder within `Config.Bookmarks` (fixes F10). Optional but cheap.

Bottom row (commit):
- **Save** (new `IDB_SAVEBOOKMARK`) — commits the current fields into the selected
  bookmark object via `ConnectReadFields(hwnd, Config.Bookmarks[bi], /*self*/, FALSE)`
  (or a small `ConnectReadFieldsInto(bookmark)` that also encrypts+stores the typed
  password when "Save password" is checked). For Quick Connect it writes
  `Config.QuickConnect`. **This is the concrete fix for F1/F2/F11 and the reported
  password bug.** Enabled only when the field set is dirty.
- **Connect** (`IDB_CONNECT`, DEFPUSHBUTTON) — connect using the current fields.
  If a bookmark is selected and fields are dirty, **auto-Save first** (so a
  just-typed+saved password reaches the bookmark) then connect; keep the transient
  `d->Result`/`ConnectPlainPassword` path for the not-saved-password case
  (unchanged, `dialogs.cpp:612-616,626-627`).
- **Cancel** — unchanged.

Double-click a list row = Connect (handle `LBN_DBLCLK`).

### 4.3 Unsaved-edit semantics (fixes F3)
Track a per-field "dirty" flag (set on `EN_CHANGE`/`BN_CLICKED` of the connection
controls). On `LBN_SELCHANGE`, if dirty, either (a) auto-commit to the
previously-selected entry (FTP behavior, zero nagging), or (b) prompt
"Save changes to <name>?" Yes/No/Cancel. Recommended: auto-commit for a saved
bookmark (matches FTP `Transfer` write-back, `ftp\dialogs1.cpp:914`) since Save is
now also explicit; this guarantees edits are never silently lost.

### 4.4 Password / "Save password" semantics (make persistence explicit)
- "Save password" now has teeth: when checked, Save/Connect runs the existing
  encrypt path (`dialogs.cpp:628-639`) but targets the **bookmark object**, so
  `SaveServer` (`sftp.cpp:425-430`) persists it. When unchecked, the password is
  used for the session only (existing `ConnectPlainPassword` path,
  `fs.cpp:84-88`) and any stored blob for that bookmark should be cleared on Save
  (add: if `!SavePassword`, `SetBlob(&EncryptedPassword, ..., NULL, 0)`).
- Because the password edit box is intentionally blanked on load
  (`dialogs.cpp:570`), Save must NOT wipe an existing stored blob just because the
  box is empty — reuse the "empty box → keep existing blob" rule already in
  `ConnectReadFields` (`643-649`). Document this so an edit-then-Save of *other*
  fields does not drop a saved password.

### 4.5 Atomic commit / real Cancel (fixes F8)
Stage bookmark edits in a temporary `CSFTPServerList` copy (mirror FTP's
`TmpFTPServerList`, `ftp\dialogs1.cpp:871`) and copy back to `Config.Bookmarks`
only on Connect/Close/Save-and-Close; on Cancel discard the temp copy. This makes
New/Rename/Delete/reorder all undoable via Cancel and matches user expectation.
(If staging is deemed too invasive, at minimum relabel Cancel behavior or keep
today's immediate-mutation but rename the button set accordingly.)

### 4.6 Organize mode (fixes F9)
With an explicit Save button, "Organize Bookmarks" becomes genuinely useful: the
same template now edits+saves bookmarks. Keep the "Connect" button relabeled
"Close" (`dialogs.cpp:731`) but ensure Save works there too. Wire the declared
`IDM_SRVCONTEXTMENU2` (1101) for right-click New/Duplicate/Rename/Delete/Move.

### 4.7 Resource template edits required
- New control IDs in `lang.rh`: `IDB_SAVEBOOKMARK`, `IDB_MOVEUPBOOKMARK`,
  `IDB_MOVEDOWNBOOKMARK`; reuse `IDB_COPYBOOKMARK` (623) for Duplicate.
- Add `IDS_QUICKCONNECT` string.
- `IDD_CONNECT` (`lang.rc2:104-141`): widen slightly / relayout the left button
  column to fit New | Duplicate | Rename | Delete and Move Up | Move Down (two
  rows), and add a Save button next to Connect at the bottom. Add an optional
  "Editing: <name>" static above the field block.
- Optionally add controls for the currently-hidden model fields
  (`UseCompression`, per-bookmark keepalive, `TargetPanelPath`) behind an
  "Advanced..." button — out of scope for the core fix.

### 4.8 Priority order for implementation
1. **F1/F2 — add Save + bookmark write-back** (also resolves the password-not-saved
   report at the UX layer). Highest value, smallest change.
2. **F5 — visible Quick Connect row**; **F3 — dirty-edit handling**.
3. **F7 — delete confirmation**; **F6/F9 — clarify Connect vs Save, fix organize**.
4. **F10 — Duplicate/Move/context-menu/double-click**; **F8 — temp-staging Cancel**.

---

## 5. Cross-references for the orchestrator
- The password-not-saved report (#2) has a UX root cause in addition to any
  password-manager issue: `dialogs.cpp:839-843` never copies the encrypted blob
  into the selected bookmark, and `fs.cpp:76-114` discards the local result. Even
  a correct encrypt chain (Agent A) cannot fix persistence without a write-back /
  Save action.
- `SaveServer`/`LoadServer` (`sftp.cpp:416-441`, `380-414`) and
  `SaveConfiguration` (`548-581`) appear correct and are NOT the bottleneck for
  bookmark passwords — the object handed to them simply never contains the new
  password.
