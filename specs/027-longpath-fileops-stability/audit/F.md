# Pass F — Main window / persistence / history / title (mainwnd1-5, salamdr3 history)

CRASH: 13 lines / 7 sites — all FIXED.
- mainwnd5.cpp:806/810 message[MAX_PATH+200] <- GetPath (Compare Dirs) -> lstrcpyn
- mainwnd5.cpp:996/1001 left/rightFilePath[2*MAX_PATH] <- GetPath -> lstrcpyn
- mainwnd5.cpp:1366/1367 left/rightFilePath[MAX_PATH] <- GetPath -> lstrcpyn
- mainwnd5.cpp:1539/1545/1563/1569 left/rightSubDir[MAX_PATH] <- Name/GetZIPPath -> lstrcpyn
- mainwnd4.cpp:390/411 GetNextFileFromPanel path/name <- GetPath/Name -> lstrcpyn MAX_PATH; :506 memcpy guard
- mainwnd3.cpp:7029 FMExt buff[MAX_PATH] <- GetPath+Name -> lstrcpyn+SalPathAppend (WinFile ABI)
- salamdr3.cpp:2098/2107/2116 (Save) + 2143 (Load) CPathHistory path[2*MAX_PATH] -> SAL round-trip

FIXED-confirmed: UpdateDefaultDir DefaultDir bound + GetRootPath fallback (rows stay MAX_PATH by design D1);
EditWindowSetDirectory dir[SAL]; panel-path restore left/rightPanelPath[SAL]; LoadPanelConfig SAL;
change-notif dispatch path[SAL].
BOUNDED: SetWindowTitle stdWndName (display truncation); user-menu FullPath*/CompareName* MAX_PATH.
