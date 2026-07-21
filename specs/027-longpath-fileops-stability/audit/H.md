# Pass H — Dialogs / find / drive bar / plugin handoff (dialogs*, find*, finddlg*, drivelst.cpp, gui.cpp, plugins.cpp)

CRASH: 1 — FIXED.
- finddlg2.cpp:813 EDTLB_DISPINFO::Buffer[MAX_PATH] <- strcpy(CFindOptionsItem::ItemName up to 2*MAX_PATH+10)
  when Find-Options Manage dialog renders a saved item -> lstrcpyn MAX_PATH (shared edit-list-box ABI: bound, don't widen)

Incidental (non-path, FIXED): gui.cpp:1074 CStaticText WM_GETTEXT off-by-one -> [len] not [len+1]
(was unterminated + 1-byte OOB write on a full buffer).

FIXED-confirmed: drivelst CDrivesList::CurrentPath SAL+lstrcpyn (027, the Alt+F1 crash); Find results
CFoundFilesData::{Name,Path} heap char*; FocusPath[SAL]; MyEnumFileNamesBuffer[SAL]; Copy/Move/ChangeDir
dialogs size-parametrized (callers match); CPack/CUnpackDialog Path[MAX_PATH] safe-by-source.
BOUNDED: finddlg1 User-Menu enum (MAX_PATH, documented not long-path capable); Pack target prepend;
CChangeDirDlg manual entry (2*MAX_PATH). EXTERNAL/ABI: drivelst pluginFSNameBuf/remoteName/cloud paths
(MAX_PATH net/shell ABI, safe-by-source); plugin-FS absFSPath MAX_PATH convention.
