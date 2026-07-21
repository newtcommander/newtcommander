# Pass C — File-operation UI (fileswn5.cpp, fileswn7.cpp, fileswn8.cpp)

CRASH: 10 — all FIXED. Dominant pattern: AlterFileName() copies a full name
UNBOUNDED into buffers left at MAX_PATH after feature 004 grew names to
SAL_FIND_NAME_U8.
- fileswn5.cpp:356 ChangeAttr path -> SAL_FIND_NAME_U8
- fileswn5.cpp:2530 RenameFile plugin newName -> lstrcpyn MAX_PATH (plugin ABI)
- fileswn7.cpp:401 UnpackZIP path -> SAL_FIND_NAME_U8+200
- fileswn7.cpp:752 DeleteFromZIP name -> SAL
- fileswn7.cpp:1298 Pack path -> SAL_FIND_NAME_U8 (+ fileBuf inherit gate)
- fileswn7.cpp:1696 Unpack fileName -> SAL_FIND_NAME_U8
- fileswn8.cpp:1003 copy/move subject sprintf -> _snprintf_s
- fileswn8.cpp:1326 EmailFiles path -> SAL + memmove safe (GetPath<=SAL)
- fileswn8.cpp:716 archive-error textBuf sprintf -> _snprintf_s
- fileswn8.cpp:113 recycle textBuf sprintf -> _snprintf_s (mirror :98)

FIXED-confirmed: ViewFile/EditFile (SAL), RenameFileInternal, F7 CreateDir,
FilesAction target path+gate, DeleteThroughRecycleBin guard.
BOUNDED: QuickRename inline buffers, reparse-delete detect, copy->pluginFS targetPath,
OpenFocusedInOtherPanel, CPanelTmpEnumData::WorkPath (MAX_PATH member).
