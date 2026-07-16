# Quickstart: Plugin Build Policy

## Using the policy

```batch
:: Build the product with the committed policy (18 plugins)
build.cmd full

:: Disable a plugin: edit plugins.cfg, flip one line
::   renamer=on  ->  renamer=off
build.cmd            :: any flavor — output reconciles immediately

:: Re-enable: flip back to =on, rebuild
```

- `plugins.cfg` (repo root) is the only file to touch; syntax is
  `name=on|off`, `#` comments — see `contracts/plugins-cfg.md`.
- Disabled plugins stay in `salamand.sln` and can still be built
  manually from Visual Studio; they are just not part of the product
  output.
- Adding a new plugin to the repo? The build fails until you add its
  line to `plugins.cfg` — that is intentional.

## Verification matrix (acceptance)

| # | Check | Expected | Covers |
|---|---|---|---|
| 1 | Clean checkout → `build.cmd full` | success; summary reports 18 plugins; `<out>\plugins\` has exactly the 18 enabled dirs; `plugins.ver` = 18 entries | SC-001, US3 |
| 2 | `grep -ri` the 8 removed names over `src/`, `help/src`, `translations/`, `build.cmd`, `salamand.sln` | no functional references (git history/changelogs excepted) | SC-002, US1 |
| 3 | Flip `renamer=off` → `build.cmd` (incremental) | renamer not compiled; `<out>\plugins\renamer\` gone; `plugins.ver` has no renamer line | SC-003, US2, Q3 clarification |
| 4 | Flip back `renamer=on` → `build.cmd` | renamer compiled and present again | FR-007 |
| 5 | Repeat 3 with `rebuild`, `full`, `full release` | same behavior in every flavor | SC-006 |
| 6 | Delete `plugins.cfg` → `build.cmd` | stops before MSBuild; error names the expected file | SC-004 (V1) |
| 7 | Add line `bogus=on` | stops; error names `bogus` | SC-004 (V3) |
| 8 | Duplicate `zip=on` line | stops; error names `zip` | SC-004 (V4) |
| 9 | Remove the `tar=` line | stops; error names `tar` as unlisted | SC-004 (V5) |
| 10 | Line `tar=maybe` | stops; error cites the line | SC-004 (V2) |
| 11 | `ZIP=ON` (case variants) | accepted, treated as `zip=on` | FR-004 |
| 12 | Launch built Salamander | no plugin error dialogs; Plugin Manager lists exactly the 18 enabled | SC-005, FR-011 |
| 13 | Launch over a profile that previously had the 8 removed plugins registered | silent cleanup, no dialogs | US1 edge case, R6 |
| 14 | All 28 entries `off` → `build.cmd` | build succeeds; core app runs with empty plugin set | edge case |

Cleanup after 6–10: `git checkout -- plugins.cfg`.

## Key implementation files

| File | Role |
|---|---|
| `plugins.cfg` | committed policy (28 entries) |
| `src/vcxproj/gen_plugins_filter.ps1` | validation V1–V5, `.slnf` generation, output reconciliation |
| `src/vcxproj/salamand.gen.slnf` | generated per-build, gitignored — never edit or commit |
| `build.cmd` | orchestrates: policy stage → MSBuild on `.slnf` → full-build extras |
| `src/plugins2.cpp` | standard-plugin table minus PAK/IEViewer; silent-uninstall suppress list + 8 removed plugins |
