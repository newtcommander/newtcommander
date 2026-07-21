# Pass E — Core path/error primitives (salamdr1/2/3/5/6, salpath.cpp, salfileio.cpp)

CRASH: 3 — all FIXED.
- salamdr5.cpp:687 SalParsePath sprintf(errBuf, path, NOTPLUGINFS) -> _snprintf_s (sibling :910 was already hardened; this one was missed)
- salamdr3.cpp:1539 sprintf(errBuf, in-archive path) -> _snprintf_s
- salamdr3.cpp:3534/3542/3570 CFileHistory::FillPopupMenu name[2*MAX_PATH] <- lstrcpy+sprintf FileName -> widen SAL + bounded ops
(plus CPathHistory Save/Load 2107/2116/2143 path[2*MAX_PATH] -> SAL, listed by pass F check)

FIXED-confirmed: BuildName heap+gate; SalCheckPath ThreadPath SAL; GetErrorText FormatMessageW;
SalSplitWindows/GeneralPath newDirs SAL + SalCreateDirectory (027); CheckAndCreateDirectory SAL;
SalParsePath existence-walk gates removed; SalPathToWExtAlloc canonicalization pre-scan (027).
BOUNDED-safe: SalGetFullName gated; GetRootPath; CutDirectory; SalPathAppend family; all error-text
_snprintf_s. EXTERNAL: SalGetTempFileName MAX_PATH base cap; archive-timestamp ANSI SHFileOperation
subsystem (salamdr3 3050/3219/3275).
