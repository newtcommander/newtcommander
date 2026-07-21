# Quickstart: Theme-Adaptive Toolbar Icons

**Feature**: 029-dark-toolbar-icons

## Build & test

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\   :: or leave default .\build\
build.cmd full                            :: Debug x64 + runtime data (deploys toolbars\ and toolbars\dark\)
```

Unit tests (includes the new `TestDarkIconColorAdaptation` suite):

```batch
"%OPENSAL_BUILD_DIR%salamander\Debug_x64\saltests.exe"
:: expected: "saltests: N checks, 0 failed"
```

## Manual verification (GUI)

1. Start `salamand.exe`, Options → Theme → **Dark**.
2. Audit every toolbar (top toolbar, directory lines, middle toolbar,
   bottom bar) and the Find window (Ctrl+F/F3 window toolbar): every glyph
   must be clearly visible on the dark background; colored accents stay
   colored; nothing is a dark blob (SC-001).
3. Open drop-down menus — same adapted icons, legible (FR-003).
4. Hover/press buttons; check a disabled button (e.g. Forward with empty
   history) is muted but visible (FR-006).
5. Options → Theme → **Default**: icons identical to the previous release
   (SC-003). Toggle themes 10× — no artifacts, icons always match (SC-004).
6. Cut icon check: the Clipboard Cut button now renders its SVG glyph in
   both themes (typo fix `CilpboardCut.svg` → `ClipboardCut.svg`).

## Adding a hand-tuned dark icon (override)

1. Create `src\res\toolbars\dark\<Name>.svg` — `<Name>` = the standard
   glyph's file name (e.g. `Copy.svg`). Design against background
   RGB(45,45,45); keep the motif of the light icon.
2. Re-run `build.cmd full` (deploys the file), or copy it manually next to
   the exe into `toolbars\dark\` for a quick experiment.
3. Restart the app or switch Default↔Dark to rebuild the image lists.
   The override is used verbatim in Dark; Default is unaffected.
   Details: [contracts/dark-icon-override.md](contracts/dark-icon-override.md).
