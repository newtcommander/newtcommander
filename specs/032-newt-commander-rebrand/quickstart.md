# Quickstart: Verifying the Newt Commander Rebrand

## Build

```batch
build.cmd            :: Debug x64 incremental (repo root)
build.cmd full       :: + runtime data (convert, toolbars, plugins.ver)
```

Output: `%OPENSAL_BUILD_DIR%\newtcommander\Debug_x64\newtcommander.exe` (directory
names unchanged — only the binary is renamed).

## Regenerating icon assets (only when the visual identity changes)

```batch
python tools\brand\gen_icons.py src\res
```

Requires Python 3 + Pillow on the dev machine (not a build dependency); outputs
`salamand.ico`, `sal_r/g/b.ico` in place.

## Verification walkthrough

### 1. Binary identity (SC-003)

- Explorer → build output → `newtcommander.exe` exists; `salamand.exe` does not.
- Properties → Details: Product name **Newt Commander**, version **0.1.0**,
  copyright shows the year-split rule, company **Newt Commander Project**.
- The file icon is the new Split Disc tile at all Explorer view sizes.

### 2. Icon surfaces (SC-004)

- Explorer small/medium/large/extra-large icons → size-appropriate variant
  (16 px favicon-style, 24–32 simplified, ≥ 48 full detail).
- Run the app: taskbar, Alt-Tab, and window caption show the new icon.

### 3. Runtime identity (SC-001)

- Main window title: `Newt Commander 0.1.0 (x64)` (+ path/admin decorations).
- Help → About: new artwork, name wordmark, version 0.1.0, year-split copyright,
  link to `newtcommander.org`.
- Toggle dark theme (Options) → About/splash render the dark variant (SC-005).
- Trigger any message box (e.g., invalid path) → caption says Newt Commander.

### 4. Registry separation (SC-002)

```powershell
# before first run — optionally plant a fake OS root to prove isolation:
reg add "HKCU\Software\Open Salamander\5.0" /v Marker /d untouched /f
# run the app, change a setting, exit, then:
reg query "HKCU\Software\Newt Commander\0.1"          # exists, holds config
reg query "HKCU\Software\Open Salamander\5.0" /v Marker  # still exactly 'untouched'
```

No import prompt appears on first run even with Open Salamander config present.

### 5. Crash reporter (SC-006)

- `salmon.exe` metadata reads Newt Commander Bug Reporter; dumps land in
  `%APPDATA%\Newt Commander`; no connection attempt to `reports.altap.cz`
  (verify: no upload UI path / endpoint code disabled).

### 6. Plugins (SC-007)

- Plugin Manager: all 18 enabled plugins load; plugin metadata shows
  ProductName Newt Commander and per-plugin copyright per contract
  (sftp/mdview solely Newt Commander Authors).

### 7. Coexistence (optional, needs an OS 5.0 install)

- Run both apps simultaneously — each keeps its own single instance;
  neither activates the other.
