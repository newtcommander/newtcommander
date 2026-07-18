# FTP → SFTP file context menu + attributes — Audit Brief (Feature 018)

You are analyzing the **FTP plugin** of Open Salamander so we can implement an
equivalent for the **SFTP plugin**. Repo `E:\Projects\salamander`, plugins at
`src\plugins\ftp\` and `src\plugins\sftp\`.

## Goal (feature 018)

Right-clicking a file/dir in an SFTP panel currently does nothing. We must add a
context menu analogous to the FTP plugin, especially **Change Attributes**
(Unix mode drwxrwxrwx = chmod), plus a NEW **owner/group change** with a
**recursive** option for directories. The SFTP plugin already has a chmod dialog
(`ShowChmodDialog`), a `ChangeAttributes` FS method, and `Chmod` in session; the
context menu (`CPluginFSInterface::ContextMenu`) is an empty stub.

## The plugin-FS context-menu / command model (what to map out)

Salamander plugin FS (CPluginFSInterfaceAbstract) surfaces panel commands to a
plugin through specific virtual methods. Map EXACTLY how the FTP plugin:
- provides/handles the **right-click context menu** on panel items
  (`ContextMenu`), and/or the standard commands Salamander routes to the FS:
  `ChangeAttributes`, `ShowProperties`, `QuickRename`, plus any menu items the
  plugin adds (CPluginInterfaceForMenuExt / `ftp\menu.cpp`).
- builds the menu (which items, labels, command IDs, enable/disable by
  selection), and dispatches each item to its handler.

Cite file:line and the exact API/interface method names.

## Rules
- **Do NOT edit source.** Analyze only. Cite `file:line`.
- Note the concrete data structures + protocol calls used (for attributes: the
  octal/rwx model, the dialog, the recursive-apply walk, the wire operation).
- Distinguish what FTP does via its protocol (SITE CHMOD, etc.) from what SFTP
  will do differently (libssh2 setstat with PERMISSIONS / UIDGID).
- Return a compact summary; put detail in your `audit\<LETTER>.md`.

## Output
- `MECHANISM:` how the context menu / attribute command is wired (interface
  methods, command IDs, dispatch), file:line.
- `ATTRS:` the chmod dialog + recursive apply + wire op, file:line.
- `OWNER_GROUP:` whether FTP supports chown and how (or "not supported").
- `SFTP_MAPPING:` concrete recommendation for the SFTP equivalent.
- `NOTES:`
