# Pass B — Copy/Move/Delete/Pack engine + workers (worker.cpp/.h, fileswn6.cpp)

CRASH: 0. Recursion-stack risk: 0.

The engine is fully long-path clean: COperation SourceName/TargetName heap char*;
one 320KB copy buffer allocated once (async pool reused); all file I/O via Sal*/W
wrappers through SalPathToWExtAlloc (both source and target). The ONLY recursive
frame (BuildScriptDir) correctly heap-backs its SAL scratch (finalName, CFinalNameBuf);
the worker Do* routines execute a FLAT script (non-recursive). BuildScriptMain2's 4
path bufs are heap (CPasteBufs). All gates raised to SAL by 011/012/014/027; all
*Copy[3*MAX_PATH] bounded by MakeCopyWithBackslashIfNeeded; recycle nameList gated
(EXTERNAL, ANSI SHFileOperation).

Confirms the F5/F6 core is solid — the user's "works only in some cases" was the
BuildScriptDir gates (fixed in 027 US3), not overflow.
