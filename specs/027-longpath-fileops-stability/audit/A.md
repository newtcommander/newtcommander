# Pass A — Viewer (viewer.cpp/2/3, fileswnb.cpp)

CRASH: 1 — viewer3.cpp:1594 `sprintf(text[300], IDS_FILEALREADYEXIST, fileName)`
overflows for a save-target path >=249 (mirror at dialogs.cpp:1950). FIXED
(widen + _snprintf_s).

FIXED-confirmed (011/013/015/027): viewer.cpp:564 name[SAL]+heap FileName;
viewer2.cpp:234/243/876 name/captionBuf/FileName SAL/heap; viewer3.cpp fileName[SAL];
fileswnb.cpp buff heap + FileNamesEnumData SAL.

BOUNDED (truncating, no crash): viewer3.cpp caption title, tmpFile/path save-dialog
(MAX_PATH-capped by dialog), fileswnb.cpp:626 pathBackup change-detect. EXTERNAL:
DragQueryFile/GetOpenFileName MAX_PATH; fileswnb IPC targetPath.
