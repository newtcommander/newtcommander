# Phase 0 Research: SFTP — Reachable Settings, Reliable Connect, Tight Dialog Layout

**Feature**: 054-fix-sftp-config-dialog · **Date**: 2026-08-07
**Spec**: [spec.md](spec.md) (three clarifications recorded 2026-08-06/07)
**Prior evidence**: `specs/053-sftp-connect-dialog/investigation.md` §9 — all three
defects were measured there, so Phase 0 starts from established causes rather
than from hypotheses.

## D1 — Making the settings window a normal dialog (US1)

**Decision** (user decision, clarification): `IDD_CONFIG` becomes an ordinary
titled dialog — the same shape every other plugin's settings use — rather than a
page hosted inside the application's own Configuration window.

**What that means concretely**: the template stops being `WS_CHILD`, gains a
caption and the standard modal-frame style, and gains the confirm/cancel buttons
it currently lacks (`ConfigProc` already handles `IDOK`/`IDCANCEL`, so only the
buttons themselves are missing). The centring call must also be reconsidered:
`MultiMonCenterWindow` works in screen coordinates, which is correct for a
top-level dialog and wrong for a child — that mismatch is precisely what put the
window outside its parent.

**Rationale**: smallest change that removes the defect, and it makes SFTP
consistent with the rest of the product (constitution VI). Confirmed by survey:
24 plugins implement a configuration entry point, and **every one that shows a
real dialog uses the same shape** — top-level, captioned, modal frame, with its
own OK/Cancel (7-Zip, ZIP, checksum, dbviewer, peviewer, nethood; FTP and
filecomp use property sheets). 22 templates repo-wide carry `WS_CHILD` and 21 of
them are property-sheet *pages* (`DS_CONTROL | WS_CHILD | WS_CAPTION`). SFTP's
`IDD_CONFIG` has neither `DS_CONTROL` nor `WS_CAPTION`, so it is not even a valid
page — it is the sole outlier, and its own eight sibling dialogs in the same file
all use the standard popup style. Its source comment already flags it as a
workaround.

**Exact change** (the sibling dialogs' style verbatim, which also satisfies
constitution VI — `DIALOGEX` + `DS_SHELLFONT` + `FONT 8, "MS Shell Dlg"`):

```
STYLE DS_SETFONT | DS_MODALFRAME | DS_FIXEDSYS | DS_CENTER | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "SFTP Configuration"
```
plus `DEFPUSHBUTTON "OK", IDOK` and `PUSHBUTTON "Cancel", IDCANCEL` appended.
The dialog is 366×250 and its content ends at y=219, so the buttons fit at
y=229 with no size change. No `IDHELP` — SFTP ships no help file, unlike the
plugins that have one.

**The centring call needs no change.** `MultiMonCenterWindow` reads and writes
**screen** coordinates (`src/common/multimon.cpp:138,193`), and `SetWindowPos`
interprets those as parent-client coordinates for a `WS_CHILD` window — that
mismatch alone produced the off-parent placement. Once the template is a popup,
the existing call becomes correct, and it is character-for-character what the
other eight SFTP dialogs already do.

**Alternatives considered**:
- *Keep the child template and only fix the positioning* — the window would
  become visible but still have no title bar, no close button and no buttons of
  its own, which is neither idiomatic nor obviously closable. Rejected.
- *Integrate as a page in the application's Configuration window* — tidier in
  principle, but a far larger change, and no other plugin does it. Rejected by
  the user.

**Measured structural impact** (from the committed translation source):

- **The caption costs nothing structurally.** `[DIALOG 530]` already carries a
  caption row — `366,250,1,""` — it is simply empty because the template has no
  `CAPTION`. Giving the dialog a title fills that existing row instead of adding
  one (`translations/czech/sftp.slt:88`).
- **The two buttons are two new rows.** Section 530 currently has 24 control
  rows and no buttons. The shape to copy is the rename dialog's
  (`translations/czech/sftp.slt:126-127`): `1,…,0,"OK"` and `2,…,1,"Zrušit"` —
  note the OK row's state flag is 0 (untranslated: "OK" is the same everywhere)
  while Cancel's is 1.

- **The title text already exists and is already translated.**
  `IDS_CONFIGTITLE` = "SFTP Configuration" (`lang/lang.rc2:26`, id 7) is present
  in every language — Czech "Konfigurace SFTP", German "SFTP-Konfiguration",
  French "Configuration SFTP" — and is referenced **nowhere in the plugin's
  code**. It was evidently added for this dialog and never wired up. The caption
  can use it, so no new string is invented and no language needs new wording.

So the refresh is a narrow case: geometry for existing rows, one already-present
caption text, and two button captions ("OK" / "Cancel") whose translations exist
in every other dialog of this module. See D4 for how that is sequenced.

## D2 — Overlapping connection attempts (US2)

**Decision** (user decision, clarification): attempt addresses in overlapping
fashion — start the next address about 250 ms after the previous one without
abandoning it, take the first that answers, drop the rest. This is the Happy
Eyeballs approach (RFC 8305) that mainstream browsers use for dual-stack hosts.

**Rationale**: it is the only option that makes a dead first address cost
essentially nothing (spec SC-003: under 2 seconds) while preserving feature
051's guarantee that the *total* is bounded by the configured timeout. Dividing
the budget per address — the obvious simpler fix — still makes the user wait
half the timeout on a dead IPv6 address, and gives a slow-but-alive address less
room the more addresses a host has.

**Where the code is**: not in `Connect` but in the worker-thread helper
`CSFTPSession::OpenSocket` (`src/plugins/sftp/session.cpp:173-311`). The defect
is the interaction of two lines: the inner wait sets `timedOut = TRUE` when the
*shared* budget is exhausted (`:229-233`), and the outer address loop's condition
includes `!timedOut` (`:204`) — so the budget running out on one address ends
the whole list.

**Constraints the implementation must respect** (all from feature 051, which
this must not regress):
- total time bounded by the configured connect timeout, independent of address
  count (spec FR-005);
- cancellation stays prompt, and must abandon *every* in-flight attempt
  (FR-007). Note the trap: `RequestCancel` only shuts down the **member**
  socket (`session.cpp:320-323`), which is `INVALID_SOCKET` during the whole
  address loop — in-flight candidates are local. Cancel is therefore honoured
  *only by polling*, so the ≤250 ms poll cadence must survive the rewrite, and
  it must now poll while several sockets are pending;
- exactly one socket survives; losers are closed, never leaked (FR-011). There
  is no close helper: `Disconnect` closes only the member socket, so anything
  the loop opens the loop must also close;
- the winning socket stays non-blocking (`session.cpp:211-212` and the
  `feature 051 (D4/F3)` comment at `:268-273`), because that is what makes
  libssh2 honour its own timeout;
- `freeaddrinfo` currently runs immediately after the loop (`:275`); keeping
  candidate `addrinfo` pointers alive across an overlapping wait means moving
  it.

**No in-repo idiom to follow.** A tree-wide search for `select(`, `FD_SET`,
`WSAEventSelect` and `WSAAsyncSelect` found the only multi-socket wait outside
the vendored libssh2 is the very loop being fixed. The FTP plugin uses a
different, message-pump model (`WSAAsyncSelect`, one socket per object, IPv4
only, no address list); checkver uses WinInet. So the local conventions to
inherit are the ones already inside `OpenSocket`: sliced waits with a cancel
poll each slice, `GetTickCount()`-relative budget arithmetic, `SO_ERROR` via
`getsockopt` to classify a writable socket, publishing under `SocketLock`, and
the `// feature NNN (ID):` comment style for non-obvious choices.

**Alternatives considered**: divided budget (rejected above); a short first
round followed by a long second round (better than dividing, but more moving
parts than overlapping attempts and still slower in the common case).

## D3 — Sizing the connect dialog from the active language (US3)

**Decision** (user decision, clarification): at dialog-open time, measure the
label texts of the language actually loaded, set the label column to the widest
of them plus a small fixed margin, move the input column to follow, and resize
the dialog itself so the fields keep their width. The window is therefore
narrower in English than in Czech or French.

**Rationale**: the labels' rendered width is only knowable at run time — it
depends on the language, the dialog font and the DPI — so any static value is
either wasteful (today: sized for the worst language across all of them) or
unsafe. Measuring removes both failure modes at once and makes languages that
are not currently shipped a non-issue.

**In-repo idiom to follow**: measuring text with a device context is common in
this codebase (37 files use `GetTextExtentPoint32`); the closest example is
`src/dialogs5.cpp:3144-3155` — `GetDC`, `SelectObject` the dialog's own font,
measure, `SelectObject` back, `ReleaseDC`. **Repositioning a dialog's controls
at run time, however, has no precedent**: the one attempt in the tree
(`src/dialogs3.cpp:2205-2213`) is commented out. So this is a new pattern for
the project and must be kept small, local to this one dialog, and obvious.

**Constraints**: fields must not get narrower (FR-010), nothing may be clipped
(FR-009), controls stay aligned and non-overlapping, and the result must stay a
standard `DIALOGEX`/`DS_SHELLFONT` dialog in the house style (constitution VI).
The dialog must remain usable if the computed width exceeds the screen (spec
edge case).

**Scope note**: this supersedes feature 053's fixed width **for the connect
dialog only**. The other SFTP dialogs keep the static widths 053 gave them —
generalizing run-time sizing across dialogs is explicitly out of scope.

## D4 — Translation-source impact (cross-cutting)

**Decision**: treat the translation refresh as a first-class step of this
feature, not an afterthought, and prove text-invariance the same way feature 053
did (compare every quoted string before/after; zero differences expected for the
connect dialog, whose texts do not change).

**Why it needs care**: `.slt` import is strictly positional, so the *structural*
change in D1 (two new button rows) is not the same kind of edit as the
*geometry* change in D3. Geometry alone can go through
`tools/translate/relayout.py` (written in feature 053, offline and text-safe);
added rows cannot — they need `merge`, and feature 053 measured that `merge`
re-translates six English-fallback entries per language, which is exactly why
relayout exists.

**How the refresh is sequenced** (settled by the measurements above):
1. Edit the template (caption + two buttons + any geometry).
2. `build.cmd full`, then `build_langs.cmd --export-templates --module sftp`.
3. `python -m translate.merge --module sftp` — needed because rows were added.
   Matching is by identity, not position (`tools/translate/match.py:48-52` keys
   entries by kind/number/id), so all 24 existing rows keep their translations
   and only genuinely new entries become gaps.
4. **Bound and check what merge changed**: the expected new entries are the
   caption and "Cancel"; "OK" is on the keep-English list so it stays `"OK"`.
   Diff every quoted string before/after and confirm nothing else moved — the
   same proof feature 053 used. If merge disturbs the six English-fallback
   entries, pin or revert those individually.
5. Refresh the three disabled languages explicitly (`--language <folder>`), or
   their `.slt` no longer matches the resource layout and re-enabling them
   later would fail.

**Caption text**: use the existing `IDS_CONFIGTITLE`. Its translations are
already committed in every language, so the caption is not a new string to
translate — only the two button captions are, and one of those is "OK".

**Note on D3 and geometry**: if the label column is computed at run time, the
committed per-language geometry for those controls stops mattering for the
connect dialog — the code overrides it. The template still needs sane values so
the dialog looks right before the adjustment runs and so the layout checks from
feature 053 keep passing.

## D5 — Verification approach

**Decision**: reuse what feature 053 established, and add one new automated
check.
- **Settings window**: open it through Plugins Manager and confirm it is
  visible, titled, within the desktop, and closable — the exact flow that fails
  today. Feature 053's session showed the window can be found and inspected
  programmatically (enumerate the plugin's windows, read each control's text,
  state and rectangle), so this is scriptable.
- **Connect**: the reference server (`tandem-sftp`, `localhost:2222`) plus the
  fact that `::1:2222` is black-holed on this machine gives a ready-made
  regression environment — `localhost` must connect in under 2 seconds, and
  `127.0.0.1` must stay as fast as it is today. The existing harness
  (`run_keyauth.cmd`, 7 scenarios) must keep passing.

  **Correction to an assumption**: the harness's `timeout-silent` scenario does
  **not** cover this code. It binds a local listener that *accepts* and then
  stays quiet, and measures libssh2's handshake timeout — the plugin's
  `OpenSocket` is not even linked into the harness (it compiles only
  `key_auth.c` plus libssh2; `session.cpp` pulls in the whole plugin SDK).
  Passing it therefore proves nothing about this fix, and it cannot break
  either. Real coverage has to come from the live `localhost` case above.

  If a harness-level test is wanted, the in-repo precedent is the *other*
  harness (`build_and_run.cmd`), which compiles SDK-free plugin sources against
  a shim — which would mean extracting the candidate race into an SDK-free
  function. Recorded as an option for the task list, not a requirement:
  simulating a genuinely black-holed address needs a firewall rule, whereas the
  observable that matters (a stalled first candidate must not delay a working
  second one) is exactly what the live `localhost` case already demonstrates.
- **Layout**: measure the rendered dialog — read each label's and field's
  rectangle at run time and assert the gap is small and no field shrank — in at
  least English and Czech. This is stronger than feature 053's static estimator
  and is the natural check for a run-time-computed layout.

**Rationale**: the static estimator from 053 cannot see a layout computed at run
time, so verification has to move to the running dialog. That is exactly what
the tooling built during 053's verification already does.

## Follow-ups this feature deliberately did not do

- **The connect address race has no automated coverage.** `run_keyauth.cmd`
  compiles only `key_auth.c` and libssh2, so `OpenSocket` is not linked into it;
  its `timeout-silent` scenario measures libssh2's handshake timeout, not the
  address loop. Making it testable would mean extracting the race into an
  SDK-free helper (`sftputils.cpp` exists for exactly that), plus a way to
  simulate a black-holed address — which needs a firewall rule. Verification
  today rests on the live `localhost` case.
- **`Config.ConnectRetries` and `Config.RetryDelay` are configurable,
  persisted, and never read by any connect code.** They appear in the settings
  dialog — which, until this feature, nobody could open. Either wire them up or
  remove them.
- **Run-time dialog sizing stayed in one dialog.** The same idea would suit the
  plugin's other dialogs (and other plugins), but generalising it is separate
  work; feature 053's static widths still serve them.
- **Two scenarios were not exercised**: a settings value surviving an
  application restart, and two live addresses racing each other. Neither is
  hard, both need a slightly different environment.

## Implementation notes (written during, not before)

Three things turned out differently from the plan and are worth recording.

**The translation refresh did not use `merge`.** `merge --module sftp --dry-run`
reported **8 gaps per language**, not the 2 rows actually added: the other 6 are
pre-existing English fallbacks that a real run would have sent for translation,
changing shipped text this feature never intended to touch. So a sibling of
feature 053's `relayout.py` was written — `tools/translate/addrows.py` — which
inserts only the genuinely missing rows, in template order, and nothing else.
The two texts were then filled from translations **already present in this
module**: the symlink dialog's "Cancel" in each language, and `IDS_CONFIGTITLE`
for the caption. Zero API calls, and the proof afterwards was exact: 1408
pre-existing rows compared across 11 languages, **0 unexpected changes**, 22 new
rows (2 per language), 11 captions filled.

**The layout measurement had to move to a label's own DC.** Measuring on the
dialog's DC returned the stock system font's metrics — every label came out too
narrow and the column would have been *shrunk* below its own text. Selecting the
label's font into the label's DC fixed it. The accelerator marker also has to be
stripped before measuring: `&` is not drawn, so measuring the raw text
over-estimates every label by one character.

**A measurement discrepancy that was not a bug.** A PowerShell verification
script measured the same labels as 106 px while the plugin measured 79 px. The
plugin is right: the control is 100 dialog units wide and reads back as 79 px,
so the plugin and the dialog agree — the script measures from a differently
DPI-aware host. Worth knowing before trusting an external script over the
process's own numbers.

**One transient harness result.** A single `run_keyauth.cmd` run reported a
16-byte leak. It did not reproduce in three further runs, and the harness does
not compile `session.cpp` at all (only `key_auth.c` plus libssh2), so it cannot
have come from this feature's code.

## D6 — Release documentation

**Decision**: `CHANGELOG.md` entries under the existing `[Unreleased]` heading:
Fixed — the SFTP settings could not be opened at all and the application stopped
responding; Fixed — connecting to a host whose first address is unreachable
(`localhost` on a machine with a filtered IPv6 loopback); Changed — the connect
dialog is sized for the language in use. Version bump stays with the release
change, per the constitution.
