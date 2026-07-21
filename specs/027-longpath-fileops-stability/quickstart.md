# Quickstart: Feature 027 — Long-Path & Unicode File-Operation Stability

## Build

```batch
set OPENSAL_BUILD_DIR=E:\Projects\salamander\build\   :: or your dir
build.cmd                :: Debug x64 incremental
build.cmd release        :: Release x64 (user runs Release!)
```

Note (019 pitfall): a running Release `salamand.exe` blocks the Release
relink with LNK1104 — close the app first.

## Tests

- `saltests` builds with the Debug solution; run:
  `& 'E:\Projects\salamander\build\salamander\Debug_x64\saltests\saltests.exe'`
  (exit code = failed checks; feature 027 = **427 checks, 0 failed**).
  New 027 tests: wide DROPFILES build/parse round-trip with >260 Czech
  paths, path canonicalization pre-scan equivalence. (saltests is excluded
  from the Release tree by feature 023 — the Debug run is authoritative.)
- Static exhaustion check:
  `pwsh -File specs\027-longpath-fileops-stability\audit\check.ps1`
  (exit 0 = no CRASH-site regression).
- Performance (SC-005):
  `pwsh -File specs\027-longpath-fileops-stability\audit\perf.ps1`
  (measured ratio 1.069 <= 1.10; see `audit/perf-results.md`).

## Test data

`%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`
- `L1-dlouhy-nazev-ěščřžýáíé-…\L2-…\L3-…` — Unicode tree, ≥427-char paths
- `ascii-level-zzz…` — ASCII long tree
- `edge-260`, `copy-target-dir` — boundary/target helpers

## User walkthrough (after implementation — the interactive part)

In the **Release** build, panel inside the L3 Unicode directory:

1. **Alt+F1** (Change Drive menu) — must open, not crash (dump D1 fix).
2. **F4** on a file (external editor) — must launch or show a clear message,
   not crash (dump D2 fix).
3. **Ctrl+C** files → navigate to an ordinary dir → **Ctrl+V** — files
   arrive, names exact.
4. Ordinary dir: **Ctrl+C** → into L3 dir: **Ctrl+V** — files arrive.
5. **Ctrl+X** variant of (3) — source removed only after successful paste.
6. **F5 copy of a FOLDER** out of / into the long tree (recursive) — works
   (previously "name too long").
7. **F7** create a directory inside L3 — works.
8. Drag&drop a file **onto a subdirectory item** inside the long tree.
9. Explorer interop: Ctrl+C in Salamander (long dir) → paste in Explorer —
   valid data; Explorer may refuse >260 targets (its own limit, expected).
10. Sub-260 sanity: ordinary-path copy/paste/F5 behaves exactly as before.
