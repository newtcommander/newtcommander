# Quickstart: Fix Information Line Encoding

**Feature**: 041-fix-infoline-encoding

## Build

The Debug tree was removed after feature 040, so start from a full build.

```batch
set OPENSAL_BUILD_DIR=D:\Build\OpenSal\
build.cmd full
```

Without `OPENSAL_BUILD_DIR` the output lands in `.\build\`. Invoke the script by
absolute path from a non-interactive shell:

```bash
cmd /c "E:\Projects\newtcommander\build.cmd full"
```

## Reproduce the defect (before the fix)

The reported file is already in the tree:

```text
temp\Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv
```

1. Point a panel at `E:\Projects\newtcommander\temp`.
2. Focus that file.
3. Read the information line directly above the command line.

Expected **before** the fix:

```text
Epizoda IV â€" NovÃ¡ nadÄ›je (Despecialized) - pÅ¯vodnÃ­ kinodabing  CZ dabing.mkv: 1 948 456 197, …
```

Note the panel above shows the same name correctly. That contrast is the defect.

**Confirming the trigger** — the file's size is what poisons the line:

```powershell
$ci = [System.Globalization.CultureInfo]::CurrentCulture
"group separator = U+{0:X4}" -f [int][char]$ci.NumberFormat.NumberGroupSeparator
"ANSI codepage   = $([System.Text.Encoding]::Default.CodePage)"
```

On a Czech system this prints `U+00A0` and `1250` — a single `0xA0` byte, which
is invalid UTF-8.

## Verify the fix

### 1. The reported case

Focus the same file. The information line must read:

```text
Epizoda IV – Nová naděje (Despecialized) - původní kinodabing  CZ dabing.mkv: 1 948 456 197, …
```

No `Ã`, `Ä`, `Å` or `â€` anywhere. The name must match the panel above
character for character.

### 2. The size threshold

Create two files with the same accented name pattern, one of 999 bytes and one
over 1 000 000 bytes. Both must display their names correctly — before the fix
only the small one does, because it has no thousands separator.

### 3. Selection summary, every language

Select several files totalling more than 999 bytes. The summary must be fully
readable. Repeat under each shipped language (`Language` under
`HKCU\Software\Newt Commander\0.1\Configuration`, e.g. `czech.slg`).

Then repeat inside the **Find** dialog, which shows the same summary.

### 4. A non-ASCII locale for dates

Temporarily switch Windows regional settings to a locale whose short date or
time format contains a non-ASCII character, restart the application, and confirm
the date and time fields display correctly and do not poison the name.

### 5. No regression elsewhere

Check on screen:

- panel columns — name, size, date, time;
- the directory line above the panel;
- dialogs reporting sizes and counts (occupied space, properties);
- the same line while browsing inside an archive.

### 6. Responsiveness

Hold an arrow key through a directory with several thousand items. Panel
navigation must feel exactly as before — the information line is rebuilt on
every focus change.

### 7. Truncation

Narrow the window until the line truncates with an ellipsis. No character may be
cut in half; a `�` may appear only if the name genuinely contains something
unrepresentable.

## Verify the contracts

### Plugin interface unchanged (FR-012)

```bash
git diff --stat src/plugins/shared/spl_gen.h    # must be empty
```

Every shipped plugin must still load: open **Plugins → Plugin Manager** and
confirm none reports a load failure.

### Locale call sites classified (FR-011)

```bash
grep -rn "GetDateFormat(\|GetTimeFormat(\|GetLocaleInfo(" src/*.cpp
```

Every remaining hit must appear in the Group C exemption list in
`validation-results.md`, with its reason.

### Strict helpers untouched (Constitution III)

```bash
git diff src/common/salunicode.h    # additions only; no change to SalU8ToW/SalWToU8
```

## Driving the GUI without a human

Match the main window by class `NewtCommanderMainWindowVer01` — early in startup
`Process.MainWindowHandle` is the splash screen. Capture with `PrintWindow`;
`CopyFromScreen` grabs whatever is physically on screen and silently captures
the wrong window when the application is not in the foreground.

Unlike the About dialog in feature 040, the information line is not a dialog
control, so its text is read from the panel's status window or verified from the
captured image.

## Rollback

Source-only:

```bash
git checkout -- src/
```

then rebuild.
