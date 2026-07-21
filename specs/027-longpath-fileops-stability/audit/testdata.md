# Feature 027 — Test data layout

Root: `%LOCALAPPDATA%\Temp\salamander-test\010\long-paths\`
(regenerate helper: session scratchpad `extend-testtree.ps1`; pre-existing
tree from feature 010)

| Entry | Purpose |
|---|---|
| `L1-dlouhy-nazev-ěščřžýáíé-…\L2-…\L3-…` | Unicode tree, dir paths up to ~319 chars; files inside L1 and L3 (`uvnitř.txt`, `soubor-ěščřxxx….txt`, …) |
| `L1-dlouhý-název-složky-ěščřžýáíéůťďň-…\L2-…\L3-…` | Denser-diacritics Unicode tree, ~417-char dir paths (~570 UTF-8 bytes) — the tree from crash dump D1 |
| `ascii-level-zzz…\ascii-level2-zzz…` | ASCII long tree (~281-char dirs) with copy-source files |
| `edge-259-xxx…` | full path exactly **259** chars (last sub-MAX_PATH) + `file-at-259.txt` |
| `edge-260` | legacy boundary dir (feature 010) with a 200-char `é…` file |
| `edge-261-xxx…` | full path exactly **261** chars (first over-MAX_PATH) + `file-at-261.txt` |
| `maximální-komponenta-xxx…` | single 255-char component |
| `rekurze-ěščřžýáíé\vnitřní-úroveň-1-…ř×60\…2-…ž×60\…3-…č×60` | **recursive folder with long Unicode inner relative paths** (deepest dir 358 chars; short root → long relative content); 3 files at two levels — the US3 folder-copy / clipboard-recursive case |
| `copy-target-dir`, `aa` | short-path targets/sources for the {long→normal, normal→long} matrix |

Matrix mapping: data-model.md §2. All names NFC; NFD/collation cases are
covered by saltests (feature 004 helpers), not by the file tree.
