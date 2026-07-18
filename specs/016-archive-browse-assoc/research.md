# Research: Archive Browsing — Root Cause Forensics (Feature 016)

## R0 — Symptom and decision point

Enter on a file browses it as an archive iff `PackerFormatConfig.PackIsArchive`
matches its extension (salamdr5.cpp:833 and the panel Enter path). The user's
config had NO `zip` (or `7z`/`tar`/`iso`/`cab`) association → fallback to
"open by shell association" → Windows Explorer.

## R1 — Registry forensics (three sources, all consistent)

- **Current config** (created today, after a full key delete):
  `Archive Association` = exactly six entries (`j`, `rar;r##`, `arj;a##`,
  `lzh`, `uc2`, `ace;c##`) — `CPackerFormatConfig::AddDefault(0)` **minus
  `zip`**, the only default with a plugin reference (`-1`).
  `Custom Packers` = the 20 external defaults, **zero** plugin entries
  ("ZIP (Plugin)" etc. absent). 18 plugins registered (keys `Plugins\1..18`,
  zip = #1, `Functions=0x3f`, `Extensions=zip;pk3;pk4;jar`).
  `Configuration\Plugins.ver Version (x64)` = 13961445 while the on-disk
  `plugins.ver` = 13961426 (the build-time stamp regressed between builds).
- **Pre-reset backup** (149 KB, whole config history): same picture — six
  associations, only external customs. So plugin `Connect` adds **never**
  took effect at any point of the config's life.
- **Altap 4.0 source config**: full, healthy 15-entry association table
  including `zip;pk3;jar` — proving the data was destroyed on OUR side.

## R2 — The failure chain (code-confirmed)

1. **Import/upgrade**: the fresh start silently imported the Altap 4.0 config
   (18 surviving plugins with `Support*` flags TRUE and relative `.spl` paths
   valid for our build). Imported `ConfigVersion` = 104 (< 105).
2. **Feature-010 gate** (`mainwnd2.cpp:2893` `packersResetToDefaults`):
   discarded the stored Custom Packers/Unpackers and **Archive Association**
   (which contained the working `zip;pk3;jar`) and rebuilt them from
   `AddDefault(0)`. `AddDefault` (pack3.cpp:421) creates ZIP as the **legacy
   entry**: `SetFormat(index, "zip", TRUE, -1, -1, TRUE /*OldType*/)`.
3. **`CPlugins::CheckData()`** (mainwnd2.cpp:3009, right after):
   - the historical "remove old internal ZIP/TAR/PAK" cull deletes OldType
     entries with plugin (negative) references (plugins2.cpp:1956+), and
   - the index validation deletes entries whose `UnpackerIndex` doesn't
     resolve (`IsArchiveIndexOK`, plugins2.cpp:2148+).
   Either way the freshly re-created legacy `zip` entry is **deleted** →
   exactly the observed six-entry table. (On a truly fresh start the same
   validation deletes it too, because `CheckData` runs before any plugin is
   registered.)
4. **No re-add mechanism**: plugin associations are created only in
   `CSalamanderConnect::AddPanelArchiver` during a plugin's **first install**
   (upgrade-delta flags, plugins1.cpp:871 + 2312). All plugins were already
   installed/imported (`Support*` TRUE), so every later `Connect` is a no-op
   upgrade. The loss is permanent and survives restarts; a full config reset
   reproduces it via the import path. (Side finding: the imported
   `LastPluginVer` 13961445 > on-disk plugins.ver 13961426 also suppresses
   plugins.ver reprocessing; secondary, not the root cause.)

## R3 — Fix decision

**Standing self-heal in `CPlugins::CheckData()`** (the function whose existing
job is reconciling these tables), after the validation loop: for every
registered plugin with `SupportPanelView` and non-empty declared `Extensions`
that has **no** association entry (`UnpackerIndex == -idx-1` nowhere), add one
entry containing those of its extensions **not claimed by any existing entry**
(`UsePacker`/`PackerIndex` mirror `SupportPanelEdit`). Properties:

- Repairs the user's current broken config on the next start, in place.
- Idempotent (entry exists → skip; claimed extensions → never touched), so no
  oscillation across restarts and no interference with user remappings
  (e.g. `zip` deliberately mapped to an external archiver stays untouched).
- Covers fresh installs, imports, and any future table rebuild.
- Runs harmlessly with zero plugins (fresh-start `CheckData` before install).
- Documented trade-off: deleting a plugin's whole association row is undone on
  restart (reliability of defaults preferred; disabling the plugin or
  remapping the extension are the supported ways to opt out).

Alternatives considered: fixing only the 010 gate (would not repair already
damaged configs, and the legacy `AddDefault` zip entry would still be culled
on fresh starts); a one-time config-version migration (repairs once but does
not protect against recurrence); reworking install-time `AddPanelArchiver`
semantics (high risk, plugin-ABI adjacent).
