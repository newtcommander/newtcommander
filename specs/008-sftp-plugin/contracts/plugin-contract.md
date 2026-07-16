# Contract: SFTP Plugin ↔ Salamander Host

**Feature**: `008-sftp-plugin` | SDK: interface version **104** (UTF-8 `CFileData::Name`)

The plugin's external interface is the Salamander plugin ABI (headers in
`src/plugins/shared/spl_*.h`). This contract pins exactly what `sftp.spl`
exposes and consumes; reference implementation of each pattern is the
FTP plugin.

## DLL exports (`sftp.def`)

| Export | Contract |
|---|---|
| `SalamanderPluginGetReqVer` | returns `LAST_VERSION_OF_SALAMANDER` (104) — refuses older hosts |
| `SalamanderPluginEntry` | returns static `CPluginInterface*`; NULL on version/lang-module failure |

## Registration (`SetBasicPluginData`)

| Argument | Value |
|---|---|
| functions | `FUNCTION_FILESYSTEM \| FUNCTION_CONFIGURATION \| FUNCTION_LOADSAVECONFIGURATION` |
| regKeyName | `"SFTP"` |
| fsName | `"sftp"` (assigned name re-read via `GetPluginFSName`) |
| password manager | `SetPluginUsesPasswordManager()` called in entry |

Path syntax accepted from the user (Change Directory, command line,
Alt+F1 item): `sftp:user@host:port/remote/path` (port and path optional).
`ConvertPathToInternal/External` normalize between display and internal
forms. `ExecuteOnFS` handles Enter (dir/symlink navigation, file = view).

## FS services declared (`GetSupportedServices`) ↔ requirements

| FS_SERVICE_* | Implemented by | Spec |
|---|---|---|
| `COPYFROMFS` / `MOVEFROMFS` | `CopyOrMoveFromFS` (download, worker + progress + resume) | FR-011/012, US3 |
| `COPYFROMDISKTOFS` / `MOVEFROMDISKTOFS` | `CopyOrMoveFromDiskToFS` (upload) | FR-011, US3 |
| `DELETE` | `Delete` (recursive via queue; confirmation flow) | FR-013 |
| `QUICKRENAME` | `QuickRename` (rename/move; UTF-8) | FR-012 |
| `CREATEDIR` | `CreateDir` | FR-013 |
| `CHANGEATTRS` | `ChangeAttributes` → chmod dialog → setstat | FR-015/020, US2 |
| `VIEWFILE` | `ViewFile` via `CSalamanderForViewFileOnFSAbstract` disk cache | FR-031 |
| `SHOWINFO` | `ShowInfoDialog` (session info, octal mode display) | FR-018 |
| `CONTEXTMENU` | panel context menu (chmod, symlink target…) | US2/US3 |
| `GETCHANGEDRIVEORDISCONNECTITEM` | Alt+F1 item + disconnect | US1 |
| `GETFSICON`, `GETNEXTDIRLINEHOTPATH`, `GETPATHFORMAINWNDTITLE` | cosmetics parity with FTP | FR-027 |
| `COMMANDLINE` | quick `cd` in command line | FR-027 |
| `ACCEPTSCHANGENOTIF` | refresh coalescing after operations | US3 |

Not declared in v1: `SHOWPROPERTIES`, `OPENFINDDLG`, `CALCULATEOCCUPIEDSPACE`
(occupied-space needs full tree walk — reconsider post-v1), `SHOWSECURITYINFO`.

## Listing contract (`ListCurrentPath`)

- Fills `CSalamanderDirectoryAbstract` with `SetValidData(VALID_DATA_EXTENSION |
  SIZE | DATE | TIME | ATTRIBUTES | HIDDEN | ISLINK)`; `..` entry always present
  (except root).
- Returns `CSFTPListingData : CPluginDataInterfaceAbstract`; `iconsType =
  pitFromRegistry` (by extension), no plugin icons in v1.
- `SetupView` inserts custom columns (`ID = COLUMN_ID_CUSTOM`): **Rights**
  (symbolic, type char + rwx + s/S, t/T bits), **Owner**, **Group** — or leaves
  the standard attribute-style layout when the user toggled the view
  (FR-021). Column widths persisted via `ColumnFixedWidthShouldChange` /
  `ColumnWidthWasChanged`.
- `GetText` callbacks read `CSFTPItemData` via transfer variables; output
  ≤ `TRANSFER_BUFFER_MAX`.

## Host services consumed

| Service | Use |
|---|---|
| `CSalamanderPasswordManagerAbstract` | encrypt/decrypt stored passwords & passphrases; `PasswordManagerEvent` re-encrypts all blobs (main thread only) |
| `CSalamanderRegistryAbstract` (via Load/SaveConfiguration) | bookmarks, known hosts, settings — schema in [registry-schema.md](registry-schema.md) |
| `CSalamanderForViewFileOnFSAbstract` | F3: `AllocFileNameInCache` (key `"<fsname>:user@host:port/path"`) → download → `OpenViewer` → `FreeFileNameInCache`; purge on unload via `RemoveFilesFromCache("<fsname>:")` |
| `CSalamanderForOperationsAbstract` | progress dialogs for interactive ops |
| `CSalamanderGUIAbstract` | subclassed controls (bookmark listbox, hyperlinks) |
| `CSalamanderDebugAbstract::CallWithCallStack` | all worker threads |
| `PostChangeOnPathNotification` / `PostRefreshPanelFS` | refresh after mutations (any thread) |

## Threading contract

- `ChangePath`/`ListCurrentPath`/dialog methods: main thread; network I/O
  inside them uses a cancellable wait window (ESC aborts, FR-024/25).
- Transfers/deletes/chmod-trees: one worker thread per operation, own SSH
  session; UI via operation dialog thread (FTP `COperationDlg` pattern).
- Password manager, `ViewFileInPluginViewer`: main thread only (SDK rule).
- `Release(force)` terminates workers, closes sessions, purges disk cache.

## Error contract (FR-024)

Every failure surfaced to the user distinguishes at minimum: host
unreachable / host key rejected / authentication failed (wrong password
vs. key not accepted vs. passphrase wrong) / permission denied /
disk-or-quota full / timeout / connection lost (with reconnect offer).
SFTP status codes map to localized messages; raw code + server message
go to the session log (FR-032).
