# Data Model: On-Demand Release Code Signing

## Entity: Signing profile (`tools/codesign/codesign.cfg`)

Committed key=value text file, ASCII, one entry per line, `#` comments and
blank lines ignored, keys case-insensitive, whitespace around `=` trimmed.

| Field | Type | Constraints | Meaning |
|---|---|---|---|
| `thumbprint` | string | exactly 40 hex chars, case-insensitive | SHA-1 thumbprint selecting the certificate from the Windows certificate store |
| `timestamp_url` | string | `http(s)://` URL | RFC 3161 timestamp authority passed to `/tr` |

Validation: both keys mandatory; malformed/missing file or values → the
signing run fails immediately with a message naming the file, before touching
any artifact. Rotation = editing `thumbprint` (single-entry change, FR-005 /
SC-005).

## Entity: Signable artifact set

Discovered at run time, never hard-coded (spec: Key Entities).

- **Universe**: all files under the sweep root (`-Root`), recursively.
- **Inclusion**: extension in `{ .exe, .dll, .spl, .slg }` (all four are PE
  images in this product).
- **Exclusion**: any path containing an `Intermediate` directory segment
  (defensive; feature 023/`:clean_release_tree` normally remove these).
- **Current population** (Release_x64, full build): 1 exe + `utils\salmon.exe`
  + `utils\sqlite.dll` + 19 `.spl` + `7za.dll` + `7zwrapper.dll` + `exif.dll`
  + `fcremote.exe` + 180 `.slg` = **206 files**; grows automatically with
  plugins.cfg / languages.cfg.

### Artifact states (drive per-file action)

| State | Detected by | Action |
|---|---|---|
| `Unsigned` | `Get-AuthenticodeSignature` Status `NotSigned` | sign |
| `SignedByCurrent` | Status `Valid` ∧ signer thumbprint = profile thumbprint | skip |
| `SignedByOther` | Status `Valid` ∧ different thumbprint | re-sign (primary signature replaced) |
| `Broken` | any other Status (HashMismatch, NotTrusted, …) | re-sign |

State transitions: `Unsigned/SignedByOther/Broken → SignedByCurrent` (the only
transition a run performs; failure leaves the file in its prior state — files
are never left half-modified because signtool writes atomically per file).

## Entity: Signing run report

Emitted by every sweep (stdout, consumed by build.cmd exit-code gating).

| Field | Type | Meaning |
|---|---|---|
| `signed` | int + file list | files transitioned to `SignedByCurrent` this run |
| `skipped` | int | files already `SignedByCurrent` |
| `failed` | int + file list | files still not `SignedByCurrent` after retries |
| `verified` | int | files passing the final verification pass |
| exit code | 0 / 1 | 0 iff `failed = 0` ∧ `verified = signed + skipped` |

## Entity: Non-distribution byproduct set (FR-013/FR-014)

- **Definition**: linker byproducts with extensions `{ .pdb, .lib, .exp }`
  inside the release output tree.
- **Producers**: MSVC linker (`ImportLibrary` → `.lib` + `.exp`,
  `ProgramDatabaseFile` → `.pdb`) for every DLL/EXE project.
- **Lifecycle after this feature**: created directly in
  `%OPENSAL_BUILD_DIR%obj\Release_x64\…` (redirect), i.e. never enter the
  shipped tree; any stragglers (legacy trees) deleted by
  `:clean_release_tree`; independently excluded by the installer
  (`Excludes: "*.pdb,*.lib,*.exp"`).
- **Retention**: `.pdb` files remain under `obj\` for crash-dump
  symbolication; `.lib/.exp` are disposable (nothing links against them —
  verified in research R3).

## Relationships

```text
codesign.cfg ──(read by)──> sign_release.ps1 ──(sweeps)──> Signable artifact set
                                   │                             │
                                   └──(emits)──> Signing run report
build.cmd [sign] ──(invokes)──> sign_release.ps1 -Root <tree>
build.cmd [setup] ──(invokes)──> setup\build_setup.cmd [sign]
build_setup.cmd [sign] ──(invokes sweep, then)──> ISCC /DSIGN=1 /Stcsign=…
sign_with_retry.cmd (props hooks) ──(TC_CODESIGN=1 only)──> sign_release.ps1 -File <target>
Directory.Build.targets ──(redirects)──> Non-distribution byproduct set → obj\
```
