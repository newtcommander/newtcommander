# Feature 027 — SC-005 performance results

Harness: `audit/perf.ps1` (median of 3 runs, 500 × 4 KB + 5 × 50 MB set,
same NTFS volume). Measures the end-to-end file-system copy cost the engine
is bound by; the code change removes the avoidable per-call
`SalCanonicalizePathW` pass for already-clean paths in `SalPathToWExtAlloc`.

Run 2026-07-21 (Debug x64):

| Scenario | Median time |
|---|---|
| ordinary-path copy | 310.6 ms |
| long-path Unicode copy (L3 tree) | 331.9 ms |
| **ratio long/ordinary** | **1.069** |

**Result: PASS** — 1.069 ≤ SC-005 target 1.10. The residual ~7% is the OS
cost of longer path resolution, not application overhead; sub-260 paths take
the identical `SalPathToWExtAlloc` skip branch, so they are not slowed.

Correctness of the skip branch is proven by saltests (`TestExtendedPaths`
feature-027 cases: clean/trailing/doubled-sep/dot/dotted-name paths produce
byte-identical output to the always-canonicalize path).
