# Quickstart: Verifying feature 010 (display encoding)

## Build

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd full              :: Debug x64 + runtime data + plugins.ver
```

Run `%OPENSAL_BUILD_DIR%salamander\Debug_x64\salamand.exe`.
(Beware: a locally installed Salamander 4.0 shares the process name —
make sure you are testing the freshly built binary.)

## Test data (sample-name set, feature 005 protocol)

Create a scratch tree, e.g. `C:\Users\<you>\AppData\Local\Temp\salamander-test\010\`:

| Directory / file | Exercises |
|------------------|-----------|
| `Můj disk\AI\` | the reported Directory Line defect (Czech diacritics in path) |
| `č-dir` (composed U+010D) and `č-dir` (decomposed U+0063 U+030C) | composed/decomposed pair |
| `Тест-Ελλάδα-测试\` | Cyrillic/Greek/CJK |
| `emoji-🙂-dir\` | outside any code page |
| `plain-ascii\` | regression guard (must look exactly as before) |

## P1 walkthrough — Directory Line / chrome (User Story 1)

1. Navigate into `...\Můj disk\AI` — Directory Line must show the
   exact path; compare with the panel rendering.
2. Shrink the window until the path ellipsizes — visible portions stay
   correct; the trailing component must not vanish.
3. Hover the Directory Line → tooltip shows the full correct path.
4. Click individual path segments (hot-track) — navigation targets the
   segment you clicked; drag a segment — the dragged text is correct.
5. Focus a file in `emoji-🙂-dir` — bottom Info Line renders the name
   correctly (check the highlighted sub-texts too).
6. Window title shows the correct path (regression — was already fixed).

## P2 walkthrough — packer lists (User Story 2)

1. Delete/rename the app's registry config so defaults regenerate
   (fresh-config case), start the app with a **Czech** `.slg` active.
2. Select files → Alt+F5 — every packer combo entry must be readable
   (e.g. "externí", "testováno" with correct diacritics). Alt+F6 the same.
3. Configuration → Pack/Unpack (custom packers) and External Archivers
   pages — identical correct titles.
4. Add a custom packer named `Můj balíčkovač`, save config, restart —
   the entry must round-trip exactly (FR-004).
5. Legacy-reset case: with a config whose version predates the UTF-8
   baseline (or by temporarily lowering the stored version value),
   start the app — packer sections must be rebuilt from defaults, no
   garbled entries anywhere (clarified reset policy).

## P3 walkthrough — audit inventory (User Story 3)

Work through `surface-inventory.md` area by area with the test tree:
menus (dir history, drive menus Alt+F1/F2 with volume labels, hot
paths, user menu), toolbars/hot-path bar, tooltips (panel long-name),
Find window, archive browsing (zip with Unicode entry names), plugin
surfaces (ftp bookmarks + log, sftp log/connect, regedt path fields;
spot-check the remaining enabled plugins). Record a verdict per row;
every `defective` row gets a fix + re-verification.

## Regression pass (SC-005)

Repeat browsing, Alt+F5 pack, copy/move, rename in `plain-ascii\` —
behavior and rendering must be indistinguishable from the pre-change
build. The already-fixed references (panel list, window title, F2
dialog) must remain correct.
