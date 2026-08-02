# Quickstart: Hot Path Display Names and Custom Icons

**Feature**: 047-hot-path-names-icons

## Build

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd            :: Debug x64 incremental
```

Run `tandemcommander.exe` from the build output tree.

## Regenerate icon assets (only when artwork changes)

```batch
python tools/brand/gen_icons.py
git status           :: src/res/hotpath1..9.ico should be the only icon changes
```

## Scenario walkthroughs

### US1 — Naming (P1)

1. Navigate a panel to a deep directory; press **Ctrl+Shift+5**.
   - Hot Path Bar (enable via Plugins/toolbar menu if hidden) shows a button
     labeled with the **path**; Alt+F1 shows the same at the bottom with digit
     accelerator `5`. *(FR-011: no auto-name, default icon.)*
2. Open **Options → Configuration → Hot Paths**. The row shows the path; the
   **Name** field is **empty**; the Path edit holds the path.
3. Type `Acme assets` into Name, confirm the dialog.
   - Bar button and Alt+F1/F2 entry now read `Acme assets` — no restart
     (SC-001). Tooltip on the bar button still shows the full path (FR-003).
   - Check the Go menu and the top-toolbar hot paths drop-down: same label
     (SC-003).
4. Reopen settings, clear Name, confirm → all surfaces show the path again.
5. In settings, type a name into an **empty** slot without a path, confirm →
   validation error, page does not close (FR-004).
6. Name with only spaces → treated as empty (FR-005).
7. `&` in a name (`A&B`) → renders literally as `A&B` on bar and menus.

### US2 — Icons (P2)

1. In Hot Paths settings, select an assigned row; open the **Icon** combo:
   10 swatches, first = current default icon, current selection highlighted.
2. Pick the red variant; confirm.
   - Bar button, Alt+F1/F2 entry, Go-menu entry show the red icon; other
     entries keep the default (≤ 4 interactions: row, combo, swatch, OK —
     SC-002).
3. Settings list shows the chosen icon next to the row.
4. Delete the entry (Delete button) → row empties; re-assigning that slot
   starts with the default icon (FR-013).

### US3 — Upgrade & compatibility (P3)

1. Simulate a pre-047 config (or use a real one):

   ```batch
   reg add "HKCU\Software\Tandem Commander\0.1\Hot Paths\7" /v Name /t REG_SZ /d "C:\Temp" /f
   reg add "HKCU\Software\Tandem Commander\0.1\Hot Paths\7" /v Path /t REG_SZ /d "C:\Temp" /f
   reg add "HKCU\Software\Tandem Commander\0.1\Hot Paths\7" /v Visible /t REG_DWORD /d 1 /f
   ```

2. Start the app → slot 7 displays `C:\Temp` everywhere, default icon, and the
   settings Name field is **empty** (FR-010: `Name==Path` ⇒ unnamed).
3. Edit slot 7's path in settings → labels follow the new path (clarification #2).
4. Exit the app (saves config) and inspect:

   ```batch
   reg query "HKCU\Software\Tandem Commander\0.1\Hot Paths\7"
   ```

   - Unnamed slot: `Name` equals the path text; **no** `Icon` value (index 0).
   - Named slot with non-default icon: `Name` = custom label, `Icon` = index.
5. Reorder entries with Move Up/Down → name, path, checkbox and icon travel
   together; Ctrl+digit mapping follows position (FR-012).

## Non-functional checks

- **DPI (FR-014/SC-005)**: move the window to a 150 %/200 % monitor (or change
  scaling) → bar icons re-render crisp after the rebuild; menu icons crisp on
  next open.
- **Theme (SC-005)**: switch app light/dark theme → all 10 swatches legible in
  the settings combo and on the bar; the nine color variants mutually
  distinguishable at 16 px.
- **Jump list**: pin the app to the taskbar, right-click → visible hot paths
  titled by the same label rule.
- **Persistence (FR-009)**: restart the app → names and icons intact.

## Translation follow-up (after lang.rc changes)

Regenerate the English template and merge the new Hot Paths page strings into
enabled languages per feature-038 tooling (`translate` scripts; languages listed
in `translations/languages.cfg`). Verify the config page renders in at least one
non-English language build.
