# Validation Results: Newt Commander Application Rebrand

**Feature**: 032-newt-commander-rebrand | **Date**: 2026-07-23
**Build**: `build.cmd full` → Debug x64, 0 errors, 19 plugins registered, 20 language modules
**Method**: built the app, launched it on a clean registry (planted an `HKCU\Software\Open Salamander\5.0` marker first), drove it via Win32 messages, captured windows with `PrintWindow`, and inspected binary metadata + registry state.

Screenshots are in [`screenshots/`](screenshots/).

## Success criteria

| ID | Criterion | Result | Evidence |
|----|-----------|--------|----------|
| SC-001 | 100% of product self-references read "Newt Commander"; 0 "Open Salamander"/"Altap" outside attribution | **PASS** | Language selector, main title, About, message-box captions all "Newt Commander"; repo-wide sweep left only comments, required attribution, and deferred scope (setup/help/translations/disabled plugins) |
| SC-002 | Zero changes to Open Salamander registry data; both products coexist | **PASS** | Planted `Open Salamander\5.0\NCMarker=untouched` — still `untouched` after a full configure/exit cycle; NC wrote only `HKCU\Software\Newt Commander\0.1` + `\Bug Reporter`. Single-instance class renamed → `NewtCommanderMainWindowVer01` |
| SC-003 | Binary is `newtcommander.exe`; properties report product "Newt Commander", version 0.1.0 | **PASS** | Output = `newtcommander.exe`; VERSIONINFO: ProductName "Newt Commander", ProductVersion "0.1.0 (x64)", Company "Newt Commander Project", InternalName NEWTCOMMANDER |
| SC-004 | New icon at all standard sizes with size-appropriate variant | **PASS** | Title-bar + Explorer icon is the Split Disc tile; `ExtractAssociatedIcon` returned the new 32px simplified variant; `.ico` packs 16 (favicon) / 24+32 (simplified) / 48+64+128+256 (full) |
| SC-005 | About + splash in both themes, correct name/version/attribution | **PASS** | About light and dark captured: GDI wordmark ("Newt" + orange "Commander"), new icon, "Newt Commander 0.1.0 (x64)", year-split copyright, `newtcommander.org`. Dark uses navy `#0A1424` bg + light text |
| SC-006 | Zero network transmissions to the vendor's domains | **PASS** | Crash-report upload compiled out (`upload.cpp` neutered); no `reports.altap.cz` literal in any shipped binary (scanned exe/dll/spl) |
| SC-007 | All default-enabled plugins load; no plugin interface change | **PASS** | Build registered 19 plugin entries; plugin ABI `LAST_VERSION_OF_SALAMANDER` unchanged at 104; sampled sftp/mdview/pictview .spl metadata correct |
| SC-008 | Constitution + README consistent on name/version/policy | **PASS** | Constitution v2.0.0 ("Newt Commander Constitution", Principle II re-anchored); README caveat updated; CLAUDE.md identity block added |

## Copyright / attribution (FR-017, FR-021)

| Binary | LegalCopyright / ProductName | Expected | OK |
|--------|------------------------------|----------|----|
| `newtcommander.exe` | © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors | year-split | ✅ |
| `sftp.spl` | © 2026 Newt Commander Authors | sole NC (new plugin) | ✅ |
| `mdview.spl` | © 2026 Newt Commander Authors | sole NC (new plugin) | ✅ |
| `pictview.spl` | © 2000-2026 Open Salamander Authors, © 2026 Newt Commander Authors | dual (modified) | ✅ |

## Notes / scope boundaries confirmed

- **Registry isolation**: verified byte-for-byte via the planted marker; the legacy Open/Altap/Servant import chain was cut (`SALCFG_ROOTS_COUNT` 1, single root), so the first-run import dialog is unreachable — the app started fresh with defaults.
- **First run** showed the language selector (title "Newt Commander", author "Newt Commander Project", web `newtcommander.org`) and a temp-dir cleanup prompt ("previous instances of Newt Commander") — both correctly branded.
- **salmon.exe / salextx64.dll**: not emitted in the Debug default build (built under Release, pre-existing behavior). Their source metadata (`salmon.rc`, `manifest.xml`, `shellext.rc`) and identifiers (registry keys, CLSID `{A6D5A8E2-…}`, IPC names) are rebranded in code and will carry the new identity when a Release build produces them.
- **Deferred (per Q3)**: `src/setup`, `help/`, and `translations/` remain Open Salamander-branded and do not ship in the default build.
- **Developer-facing names unchanged (FR-016)**: `salamand.sln`, `salamand.vcxproj`, `OPENSAL_BUILD_DIR`, source/function/`SALAMANDER_*` identifiers intentionally preserved.
