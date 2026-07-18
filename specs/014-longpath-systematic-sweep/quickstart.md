# Quickstart: Long-Path Operation Verification Matrix (Feature 014)

## Test data

`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\` — ASCII (≈291-char) and
Unicode (≈540-byte UTF-8) directory trees plus boundary lengths (just over
260, over 520, near extended-length max). Regenerate if a prior operation
displaced it.

## Matrix — run each on {ASCII long, Unicode long} and a sub-260 control

| Operation | Key | Expected (long) | Expected (sub-260 control) |
|-----------|-----|-----------------|----------------------------|
| Enter directory | Enter | opens, no crash, no "too long" | unchanged |
| View file | F3 | internal viewer shows content, no crash | unchanged |
| Edit file | F4 | opens (internal/external), no crash | unchanged |
| Open (assoc) | Enter on file | opens or bounded external message | unchanged |
| Rename | F2 | renames, no crash | unchanged |
| Copy | F5 | copies byte-exact, no crash, source intact | unchanged |
| Move | F6 | moves, no crash, no partial loss | unchanged |
| Delete | Del | deletes (or bounded recycle-bin message) | unchanged |
| Copy to clipboard | Ctrl+C | no crash | unchanged |
| Cut to clipboard | Ctrl+X | no crash | unchanged |
| Paste | Ctrl+V | pastes, no crash | unchanged |
| Drag-and-drop | mouse | copies/moves, no crash | unchanged |
| Create directory | F7 | creates, or bounded message beyond external limit | unchanged |
| Change attributes | | applies, no crash | unchanged |
| Properties | Alt+Enter | shows, no crash | unchanged |
| Calculate size | | recurses without stack overflow | unchanged |
| Pack | Alt+F5 | archive created, no crash | unchanged |
| Unpack | Alt+F6 | extracts, no crash | unchanged |

## Automated / forensic checks (headless)

1. **Build**: `build.cmd` (Debug) and `build.cmd release` (Release) both clean.
2. **Static exhaustion** (after full sweep): re-run the census grep; zero
   CRASH-verdict buffers remain unresolved (research.md R1/R4).
3. **Dump forensics**: if a new crash dump appears in `%LOCALAPPDATA%\CrashDumps`,
   symbolicate it (scratchpad `parse-dump3.ps1` + `resolve-rel.ps1` against the
   Release PDB) — the fixed frames must not reappear.
4. **CLI navigation**: launch with `-a <long path>` across the tree — survives.

## This session's confirmed results

- F3 crash (`viewer2.cpp:676`, 07:40 dump) — fixed; Debug build clean.
- Unicode navigate/parse "too long" (`fileswn9.cpp:60`) — widened; Debug clean.
- Release build — verify (see build log).
- Full matrix interactive walkthrough — user follow-up (headless env).
