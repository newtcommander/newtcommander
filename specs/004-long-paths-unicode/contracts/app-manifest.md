# Contract: Application Manifest

**Feature**: [../spec.md](../spec.md) | **Decisions**: [../research.md](../research.md) R2, R3
**Evidence**: [../code-analysis.md](../code-analysis.md) §I

The manifest is embedded as an `RT_MANIFEST` resource from
`src/manifest.xml` (`src/salamand.rc2:12`); linker manifest generation
is disabled (`src/vcxproj/sal_base.props:9-10`). Changes happen in
`src/manifest.xml` only — no build-pipeline change (Constitution I).

## Required change

Add to the existing `<application>`/`<windowsSettings>` area:

```xml
<asmv3:application xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
  <asmv3:windowsSettings
      xmlns:ws2="http://schemas.microsoft.com/SMI/2016/WindowsSettings">
    <ws2:longPathAware>true</ws2:longPathAware>
  </asmv3:windowsSettings>
</asmv3:application>
```

- **`longPathAware=true`** — defense-in-depth only. The primary
  long-path mechanism is the explicit `\\?\` + `W`-API layer (R2),
  which works regardless of the system `LongPathsEnabled` registry
  value. The manifest flag additionally un-caps any stray un-migrated
  Win32 call on systems where the registry opt-in is present.

## Explicitly NOT added

- **`activeCodePage=UTF-8`** — rejected (R3): it would flip the
  process code page for in-process legacy third-party plugins whose
  own `-A` API usage depends on the system ACP, breaking today's
  working behavior (FR-015). GDI would ignore it anyway.

## Unchanged (out of scope, keep as-is)

- comctl32 v6 dependency (`manifest.xml:12-19`)
- `asInvoker` execution level (`manifest.xml:25-28`)
- `supportedOS` list (`manifest.xml:34-38`)
- Legacy `dpiAware=true` (`manifest.xml:43`) — DPI modernization is a
  separate concern, not part of this feature.

## Verification

- Dump the built binary's manifest (`mt.exe -inputresource:salamand.exe;#1 -out:check.xml`)
  and assert the `longPathAware` element is present.
- Quickstart scenario #1–2 (deep-path operations) MUST pass on a
  machine with `LongPathsEnabled` **absent or 0** — proving the app
  does not depend on the manifest flag + registry combination.
