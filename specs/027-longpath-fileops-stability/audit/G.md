# Pass G — Shell / clipboard / drag&drop / execute (shellib.cpp, shellsup.cpp, salshlib.cpp, execute.cpp, salclip.cpp)

CRASH: 2 — all FIXED.
- shellsup.cpp:2593 ExecuteAssociation execName[MAX_PATH+200] <- strcpy(path) (SalOpen route) -> widen SAL + lstrcpyn
- execute.cpp:1199 MFLFileDataExpPath strcpy(CFileDataExpData::Buffer[2000], Path+l) (Make File List) -> widen Buffer SAL
  (027 had widened CExecuteExpData::Buffer + Path but left the separate CFileDataExpData::Buffer at 2000)

EXTERNAL — VERIFIED: SalShExtSharedMem::TargetPath both writes (shellib.cpp ~1238/1316) have the 027
refusal gates (strlen(CurDir)<MAX_PATH / <2*MAX_PATH else IDS_TOOLONGPATH + DROPEFFECT_NONE); ABI unchanged.
FIXED-confirmed: CImpDropTarget CurDir/SrcPath/OldDataObjectSrcFSPath/dataObjectSrcFSPath SAL; GetShellFolder
root SAL; OpenFolderAndFocusItem mydir SAL; IsSimpleSelection prefix heap (027); own CF_HDROP copy-out
(027 salclip); execute.cpp CExecuteExpData::Buffer + CFileDataExpData::Path SAL (027); ResolveNetHoodPath guarded.
BOUNDED: salshlib CSalShExtPastedData ArchiveFileName/PathInArchive[MAX_PATH] (archive truncation);
CTmpDragDropOperData archive fields; execute.cpp:1223 MFLFileDataExpDOSPath (GetShortPathName MAX_PATH).
