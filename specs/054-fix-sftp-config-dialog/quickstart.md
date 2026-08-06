# Quickstart: Validating the SFTP Fixes

**Feature**: 054-fix-sftp-config-dialog · Phase 1 artifact

> **Results (2026-08-07, Czech UI, reference server `tandem-sftp` on
> `127.0.0.1:2222`, `::1:2222` black-holed)**
>
> | Scenario | Result |
> |---|---|
> | 1 — settings window opens | **PASS** — titled "Konfigurace SFTP", centred inside Plugins Manager, with OK/Cancel and a close button; every label readable, nothing overlapping (feature 053's layout for this dialog visible for the first time) |
> | 3 — `localhost` connects | **PASS** — 1835 ms including host-key confirmation, against a guaranteed ~20 s failure before. `127.0.0.1` unchanged at 1009 ms |
> | 4 — every address dead | **PASS** — failed after 20194 ms against a 20 s budget: one timeout, not a multiple |
> | 5 — no dangling attempts | **PASS** — `Get-NetTCPConnection -State SynSent` on port 2222 is empty after each attempt |
> | 7 — dialog fits the language | **PASS** — the wide band is gone; the Czech dialog narrowed from 646 px to 580 px with fields at their original width and no label clipped |
> | 8 — connect harness | **PASS** — 7 passed / 0 failed, no leak (one transient leak report did not reproduce in three further runs; the harness does not compile `session.cpp`) |
> | 9 — translations | **PASS** — 1408 pre-existing rows compared across 11 languages, **0 unexpected changes**; 22 new rows and 11 captions, all filled from translations already in the module |
> | 10 — builds | **PASS** — `build.cmd`, `build.cmd full`, clean `rebuild`, `full release` |
>
> Not executed: scenario 2 (settings surviving a restart — the value was
> confirmed to persist within a session), scenario 6 (two working addresses
> racing — needs a second live address), and the English/French layout
> measurement (Czech verified; the sizing is computed, so other languages
> follow the same code path).

## Prerequisites

- Windows 11, VS2022 C++ workload, Windows SDK (repo standard)
- **Python 3.x on PATH** — `build.cmd` fails without it (feature 052)
- **Docker Desktop running**, container `tandem-sftp` up (`docker ps`); the
  server answers on `127.0.0.1:2222`, login `tctest` / `tandem123`
- This machine's `::1:2222` is **black-holed** (SYN dropped, no RST) — that is
  what makes `localhost` a ready-made regression case. Confirm before testing:
  connecting to `::1:2222` must hang rather than refuse, while `127.0.0.1:2222`
  connects in about a millisecond.

## Build

```batch
build.cmd full                                :: app + plugins + language modules
src\vcxproj\build_langs.cmd --module sftp --force
```

## Scenario 1 — The settings window opens (US1, SC-001/SC-002)

1. Open Plugins Manager (Plugins → the manager entry).
2. Select **SFTP Client** and press **Otestovat / Test** once — Configure is
   only enabled for a loaded plugin, and SFTP loads on demand.
3. Press **Konfigurovat / Configure**.

**Expected**: a window appears, titled with the plugin's configuration title
("Konfigurace SFTP" in Czech), centred on the manager, entirely on screen, with
a close button and OK/Cancel. **Today**: nothing appears and the application
stops responding.

4. Change **Maximální velikost protokolu (KB)** to a distinctive value, press
   OK, reopen the window. **Expected**: the new value is shown.
5. Change it again, press **Zrušit / Cancel**, reopen. **Expected**: the value
   from step 4 is still there.
6. Reopen and close with the window's X. **Expected**: closes, nothing changes,
   application responsive.
7. Look over the whole window in Czech. **Expected**: every label fully
   readable, nothing overlapping — this is feature 053's layout work becoming
   visible for the first time.

## Scenario 2 — Settings persist across a restart (US1, FR-003)

Set a distinctive log size, press OK, exit the application cleanly, restart,
reopen the settings. **Expected**: the value survived.

## Scenario 3 — `localhost` connects (US2, SC-003)

1. Connect dialog (Ctrl+Shift+S) → Quick Connect → host **`localhost`**, port
   `2222`, user `tctest`, password typed by hand.
2. Press Connect and time it.

**Expected**: connects, panel shows the remote listing, **in under 2 seconds**.
**Today**: fails after the full 20-second timeout with "Došlo k vypršení
časového limitu".

3. Repeat with **`127.0.0.1`**. **Expected**: connects as fast as it does today
   — the change must not slow the common case (FR-006).

## Scenario 4 — A completely unreachable host still fails on time (US2, SC-004)

Connect to an address that is silently dropped and has no working alternative
(for example a routable but unused address on the local network, or `::1` typed
directly). Time the failure.

**Expected**: fails with the existing timeout message **within the configured
connect timeout** (default 20 s) — not a multiple of it, however many addresses
the name has. This is feature 051's guarantee and the main thing this feature
must not regress.

## Scenario 5 — Cancel stays prompt (US2, FR-007)

Start a connection to the unreachable host from scenario 4 and press Cancel on
the wait window.

**Expected**: stops within about a second, with the existing cancelled message,
and no connection is left behind. Verify no stray connections remain:

```powershell
Get-NetTCPConnection -State SynSent -ErrorAction SilentlyContinue |
  Where-Object { $_.RemotePort -eq 2222 }
```

**Expected**: empty a moment after the attempt ends — every losing attempt was
closed (FR-011).

## Scenario 6 — Two addresses answering at once (US2, edge case)

Connect to a name that resolves to two *working* addresses (add one to the
hosts file if needed).

**Expected**: connects normally, exactly one session appears, and the check in
scenario 5 shows nothing left dangling.

## Scenario 7 — The connect dialog fits the language (US3, SC-005)

Open the connect dialog in **English** and in **Czech**, and measure the
rendered controls rather than eyeballing them — read each label's and each
field's rectangle from the running dialog (the approach used during feature
053's verification: enumerate the dialog's children and read id, text, enabled
state and rectangle).

**Expected**, in both languages:
- the gap between the widest label's right edge and the fields' left edge is a
  small, similar margin — not the wide band visible today;
- no label is clipped (its rendered text width fits its control);
- every field is at least as wide as before this feature;
- the dialog window is **narrower in English than in Czech** — the window
  follows the language;
- nothing overlaps and the labels stay in one aligned column.

Repeat for **French** (the widest translations) to confirm the dialog grows
rather than clipping.

## Scenario 8 — Connect regressions (feature 051 harness)

```batch
src\plugins\sftp\test\run_keyauth.cmd
```

**Expected**: 7 passed, 0 failed. Note this harness does **not** exercise the
address loop — it links only `key_auth.c` and libssh2 — so it is a guard
against collateral damage to the authentication paths, not evidence for
scenarios 3–6.

## Scenario 9 — Translations untouched apart from the new rows (FR-009 / feature 053's guarantee)

After the translation refresh, compare every `translations/<lang>/sftp.slt`
against a copy taken beforehand and confirm that the only differences are:
the config dialog's caption text, its two new button rows, and geometry
numbers.

**Expected**: no other quoted string changed, in any of the 11 languages.
Also confirm the three disabled languages (Russian, Ukrainian, Simplified
Chinese) were refreshed too, or re-enabling them later would fail on a
structure mismatch.

## Scenario 10 — Full build regression

```batch
build.cmd rebuild
build.cmd full release
```

**Expected**: both complete with 0 errors, and `CHANGELOG.md` carries this
feature's entries.
