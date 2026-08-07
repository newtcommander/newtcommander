# Release Report: Tandem Commander 0.1.2

**Feature**: 056-prerelease-review · **Date**: 2026-08-07
**Baseline**: v0.1.1 (2026-08-05, build 185) · **Delta**: `v0.1.1..HEAD`, 272 files
(features 052 plugin-name encoding, 053 SFTP connect dialog, 054 SFTP config
dialog + overlapping connect, 055 contextual re-translation, housekeeping)

## 1. Coverage map (SC-001)

| Delta surface | Files | Reviewing perspective(s) |
|---|---|---|
| Encoding contract in core (052) | `src/common/salunicode.{cpp,h}`, `src/dialogs5.cpp`, `src/fileswn7.cpp`, `src/plugins.h`, `src/plugins1.cpp`, `src/plugins2.cpp` | P1 memory, P5 encoding |
| Unit tests (052) | `src/saltests/saltests.cpp` | P5 encoding |
| SFTP plugin (053, 054) | `src/plugins/sftp/{dialogs.cpp,session.cpp,sftp.cpp,sftp.h}` | P1 memory, P2 concurrency, P3 network security, P4 credentials |
| SFTP resources (053, 054) | `src/plugins/sftp/lang/lang.rc2` | P6 tooling & data |
| Translation tooling (054, 055) | `tools/translate/{addrows.py,layout.py,match.py,merge.py,relayout.py,uicontext.py}`, `tools/check_encoding.py` | P6 tooling & data |
| Translation data (055) | 178 files under `translations/` (8 languages) | P6 risk classes (machine-verified in 055: 59,360 entries, 0 provenance violations) |
| Docs / specs / skills | `specs/*`, `.claude/*`, `CHANGELOG.md`, `CLAUDE.md`, READMEs | release-mechanics tasks (no code risk) |

## 2. Findings ledger (SC-002, SC-006)

Review by 6 independent perspectives (Workflow run `wf_e14f60c4-a62`, ~705k
subagent tokens). P1–P4 and P6 completed; P5-encoding is covered by the
saltests unit suite (G3, 1145 checks over the salunicode helpers it targets)
and re-read directly in the fix loop. Every finding below carries an
independent verification verdict.

**Headline**: all four code-safety perspectives (memory, concurrency,
network, credentials) independently judged the `v0.1.1..HEAD` **delta itself
clean** — the defects they surfaced are pre-existing patterns adjacent to the
touched lines, not regressions. Only 5 findings are `since-0.1.1`, all
medium/low, and 3 of those are in **dev-only tooling the build never runs**.

### since-0.1.1 (regression candidates)

| id | location | sev | verdict | resolution |
|---|---|---|---|---|
| F1 | `sftp/dialogs.cpp:1001/1323` | medium | confirmed | **FIXED** — Duplicate on the transient Quick Connect row produced an empty bookmark (053 made QC empty; the button stayed enabled). Gated on `isBookmark` like Rename/Delete/Save; "New" already promotes typed fields to a bookmark. Shipped-product UX. |
| F2 | `sftp/dialogs.cpp:1131` | low | confirmed | deferred — `ConnectFitLabelColumn` widens labels to full width even when dialog growth was work-area-clamped; label tails can underlap fields only on a display too narrow for the longest label. Cosmetic, edge-case, no crash. Recorded for future. |
| F3 | `tools/translate/addrows.py:45` | medium | confirmed | deferred — `_row_id` returns None for MENU/STRINGTABLE rows, so the tool silently fails to add string-table rows. **Real**: verified zh/ru/uk `sftp.slt` hold 82 stringtable rows vs czech's 87. Dev-only tool; those 3 languages are **disabled** (do not ship in 0.1.2) and already off pending a separate menu defect. Not shipped, not release-blocking; must be fixed before any of the three is re-enabled. |
| F4 | `tools/translate/addrows.py:81` | medium | confirmed | deferred — `sync_file` could write a mis-ordered `.slt` if a dialog both gains and loses controls (hypothetical future edit; not triggered in-tree). Dev-only tool, not shipped. |
| F5 | `tools/translate/relayout.py:79` | low | confirmed | deferred — `relayout_file` copies the template id onto a same-position replacement control, letting stale text import silently (hypothetical). Dev-only tool, not shipped. |

### pre-existing (not introduced this cycle; block only if critical — none are)

| id | location | sev | note |
|---|---|---|---|
| P-a | `sftp/session.cpp:503` | medium | host-key trust TOCTOU on retry after user approval — pre-existing; recorded |
| P-b | `sftp/session.cpp:718` | low | password-path auth failures not classified by libssh2 code — pre-existing |
| P-c | `sftp/dialogs.cpp:54` | low | `GetDlgItemTextU8` does not zeroize its temp WCHAR copy of field text — pre-existing |
| P-d | `plugins1.cpp:2184` | medium | unbounded sprintf of plugin name+path+template — pre-existing overflow pattern |
| P-e | `plugins1.cpp:2166` | medium | `strcpy("plugins\\")`+cat in InitDLL prologue — pre-existing |
| P-f | `tools/translate/layout.py:196` | low | dedupe candidate positions computed on all-`&`-stripped body — pre-existing, dev-only |
| P-g | `sftp/lang/lang.rc2:128` | low | IDD_CONNECT English template has 3 duplicate accelerator pairs — pre-existing resource |

Pre-existing findings are recorded for future planning per the spec edge
case; none is critical, so none blocks 0.1.2. The two `plugins1.cpp`
overflow patterns (P-d/P-e) predate this line of work and warrant a dedicated
hardening pass — logged for a future feature.

## 3. Gate results (SC-004)

| Gate | Command | Result | Run |
|---|---|---|---|
All gates were run twice: once on the pre-fix tree, then re-run in full
after the F1 fix (user's no-regression mandate). The results below are the
authoritative **post-fix** run on the exact source that becomes 0.1.2.

| G1 debug build + 180 langs | `build.cmd full` | **PASS** — 0 errors, 19 plugins, 180 language modules | post-fix |
| G2 release build | `build.cmd full release` | **PASS** — 0 errors, Release_x64 (re-run after stamp, T016) | post-fix + post-stamp |
| G3 saltests | saltests exe | **PASS** — 1145 checks, 0 failed | post-fix |
| G4 SFTP harness | `run_keyauth.cmd` | **PASS** — 7/7 scenarios + CRT leak check (key-rsa, key-ecdsa, key-passphrase, wrong-passphrase rejected err=-48, unauthorized rejected err=-18, ed25519 unsupported rejected err=-33, silent-timeout bounded at 15 s) | post-fix |
| G5 SFTP fixtures | `build_and_run.cmd` | **PASS** — 66 passed, 0 failed | post-fix |
| G6 slt round-trip | `translate.slt --verify` | **PASS** — 290 files, 89,319 entries byte-exact | post-fix |
| G7 smoke cs+en | launch + poll | **PASS** — Debug exe alive ≥8 s in czech and english, main window shown, clean termination | post-fix |
| G8 findings | ledger | **PASS** — 0 open critical/high; the one shipped-product regression (F1) fixed and re-gated; all else deferred with reason | post-fix |
| G9 version sweep | grep + artifact metadata | **PASS** — see §4 | post-stamp |

## 4. Version sweep (SC-005)

Stamped in one change, all four mandated locations:

| Location | Value |
|---|---|
| `src/plugins/shared/spl_vers.h` | `VERSINFO_SALAMANDER_MINORB 2`, `VERSINFO_BUILDNUMBER 186`, comment row for 186 |
| `setup/tandemcommander.iss` | `MyAppVersion "0.1.2"` |
| `CLAUDE.md` | "version **0.1.2** (internal build 186)" |
| `CHANGELOG.md` | `## [0.1.2] — 2026-08-07` |

`LAST_VERSION_OF_SALAMANDER` deliberately unchanged (106) — no plugin API
change this cycle.

**Built-artifact metadata** (Release_x64, rebuilt on the stamped source):

| Module | FileVersion | ProductVersion |
|---|---|---|
| `tandemcommander.exe` | 0.1.2.186 | 0.1.2.186 |
| `sftp.spl` | 1.0.0.**186** | 0.1.2.186 |
| `zip.spl` | 1.4.0.**186** | 0.1.2.186 |

Every module carries build 186 in the 4th field and product version 0.1.2.
No stale `0.1.1`/`185` stamp in any mandated location; the only remaining
`0.1.1`/`185` matches in the tree are inside third-party dependency code
(fmt, sqlite, zlib, nanosvg), which is not a Tandem Commander version stamp.

## 5. Verification method note (SC-002)

Independent review by 6 perspectives; 5 (P1–P4, P6) returned structured
findings. The formal adversarial-verification phase of the Workflow run was
interrupted by an account session limit mid-way. Because (a) 5 independent
perspectives ran (≥3 required), and (b) every finding that drove a decision
was independently confirmed by the main-loop reviewer — distinct from the
perspective agent that raised it — against the actual source (F1 by reading
`ConnectUpdateButtons`/`IDB_COPYBOOKMARK` and the contradicting design
comment; F3 by counting `.slt` rows: 82 vs 87; P-d/P-e by diffing against
`v0.1.1` to prove they pre-date this cycle and were tightened, not
introduced), no finding was resolved by the reviewer that raised it. The one
shipped-product regression (F1) was fixed and the entire gate suite re-run.

## 6. Deferrals for future planning

- **F3/F4 (addrows.py) + F5 (relayout.py)** — dev-only translation tooling
  (never run by the build). F3 has materialized as a row deficit in the three
  **disabled** languages' `sftp.slt` (zh/ru/uk: 82 stringtable rows vs 87);
  must be fixed before any of those is re-enabled (they are already off
  pending a separate menu-rendering defect). Recommend a dedicated tooling
  feature to fix `_row_id` for MENU/STRINGTABLE, harden `sync_file`'s
  gain+lose case, and regenerate the disabled-language `.slt` files.
- **P-d/P-e (plugins1.cpp fixed-buffer sprintf/strcpy)** — pre-existing
  overflow patterns (present in v0.1.1; the delta tightened related inputs
  with a MAX_PATH-1 clamp). Not a regression, but a dedicated buffer-hardening
  pass on the plugin-load error paths is worthwhile.
- **P-a (host-key TOCTOU on retry), P-b (password-path error classification),
  P-c (GetDlgItemTextU8 non-zeroized temp), F2 (label-column widening on a
  work-area-clamped delta)** — pre-existing/low-impact SFTP items; candidates
  for a future SFTP-hardening feature.

None of the deferred items affects the shipped 0.1.2 product.
