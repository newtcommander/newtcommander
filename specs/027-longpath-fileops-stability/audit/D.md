# Pass D — Navigation / change-notify / directory line (fileswn0/1/2/3/9, snooper.cpp)

CRASH: 14 — all FIXED.
- fileswn0.cpp:337 FocusShortcutTarget junctionOrSymlinkTgt[MAX_PATH] <- fullName -> lstrcpyn+guard
- fileswn1.cpp:481 icon-reader path[MAX_PATH+10] <- GetPath (unguarded memmove) -> l>=MAX_PATH guard, skip icon
- fileswn1.cpp:665 icon-reader fileName[MAX_PATH] <- Name (up to 780) -> SAL_FIND_NAME_U8
- fileswn1.cpp:2273/2301 OpenActiveFolder dirName memmove (System32 redirect) -> length guard
- fileswn1.cpp:2316 OpenActiveFolder itemName[MAX_PATH] <- AlterFileName -> SAL_FIND_NAME_U8
- fileswn3.cpp:1264/1542 ReadDirectory ext-lowercase buf[MAX_PATH] -> SAL_FIND_NAME_U8
- fileswn3.cpp:2020/2116/2194/2422/2501/2525 ChangeDir errBuf sprintf(path) -> _snprintf_s
- fileswn9.cpp:1608 CreateDragImage buff <- AlterFileName -> SAL_FIND_NAME_U8
- fileswn9.cpp:1942/1964 CopyFocusedNameToClipboard itemName/fileName <- AlterFileName -> SAL_FIND_NAME_U8

FIXED-confirmed: ChangePathToDisk changedPath heap+errBuf bounded; Execute CPathBufs heap;
ReadDirectory buf:284 bounded; DirectoryLineSetText; ParsePath curPath; DropPath SAL;
snooper pathCopy MakeCopyWithBackslashIfNeeded-guarded.
BOUNDED: ChangePathToArchive backup1/2; ClipboardPastePath buff; hot-path save;
CopyUNCPathToClipboard buff. EXTERNAL: .lnk IShellLink MAX_PATH.
