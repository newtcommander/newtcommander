# Contract: Plugin Interface v-next (Unicode + Long Paths)

**Feature**: [../spec.md](../spec.md) | **Decisions**: [../research.md](../research.md) R1, R8
**Status**: Draft — documented before modification per Constitution V.

This contract defines the plugin-SDK changes that carry UTF-8 names
and long paths across the core↔plugin boundary, and the behavior
guaranteed to legacy binaries. Mechanics evidence:
[../code-analysis.md](../code-analysis.md) §H.

## 1. Version negotiation (existing mechanism, new number)

| Item | Today | v-next |
|------|-------|--------|
| `LAST_VERSION_OF_SALAMANDER` (`spl_vers.h:195`) | `103` ("5.0") | `104` (first Unicode/long-path version; exact number finalized per `doc\how_to_change.txt`) |
| `PLUGIN_REQVER` (`plugins.h:8`) | `103` | **stays `103`** — legacy plugins keep loading |
| Plugin's `SalamanderPluginGetReqVer` / `GetSDKVer` | returns ≤ 103 | returns ≥ 104 when built against v-next SDK |
| Core's per-plugin record | `BuiltForVersion` (`plugins1.cpp:2226-2239`) | unchanged field; `BuiltForVersion >= 104` ⇔ **UTF-8/long-path semantics**, else **legacy semantics + shim** |

No new export names; no change to `SalamanderPluginEntry` signature.
The existing too-old gate (`plugins1.cpp:2240`) is untouched.

## 2. String semantics by negotiated version

### v-next plugins (`BuiltForVersion >= 104`)

- Every `char*` name/path crossing the interface (both directions) is
  **UTF-8**. Structure shapes stay `char*` — no wide-string re-typing.
- `CFileData` (`spl_com.h:203`) revised in the v-next SDK header:
  - `Name`, `Ext`, `DosName` — unchanged types, UTF-8 content;
    `Name` remains allocated with `CSalamanderGeneralAbstract::Alloc`.
  - `NameLen` — widened from `unsigned :9` to a full 32-bit field
    (UTF-8 **byte** length; the `MAX_PATH-5` cap is removed; single
    component ≤ 765 bytes = 3×255). **This is the ABI-breaking
    layout change that motivates the version bump.**
- Paths may be up to the OS maximum (~32,767 UTF-16 units; up to
  `3×32767` UTF-8 bytes). Plugins MUST NOT assume `MAX_PATH`.
- Plugins receive paths in display form (no `\\?\`); the SDK provides
  helpers (new `spl` utility exports) for UTF-8↔UTF-16 conversion and
  extended-length normalization so plugin authors call `W` APIs
  correctly without reimplementing the rules.
- Registry/config access through `SalRegQueryValueEx`-family
  (`spl_gen.h:3321`) keeps `char*` signatures with UTF-8 payload; the
  core converts at its registry boundary (R9).

### Legacy plugins (`BuiltForVersion < 104`)

Core-side **adaptation shim**, applied at every interface crossing:

| Direction | Conversion | Failure rule |
|-----------|-----------|--------------|
| core → plugin | UTF-8 → system-ACP (`WideCharToMultiByte` on the UTF-16 form, no best-fit) | Any unrepresentable character **or** resulting path ≥ `MAX_PATH` ⇒ the item is refused before any data flows: per-item, actionable message naming the item (FR-014); remaining items continue |
| plugin → core | system-ACP → UTF-8 | always lossless; no failure path |

Guarantees to legacy binaries (FR-015): process code page unchanged
(R3), `CFileData` layout as published in SDK ≤ 103 is what the shim
materializes for them, allocation contract
(`CSalamanderGeneralAbstract::Alloc/Free`) unchanged. Anything that
works today keeps working identically.

## 3. Behavioral requirements on v-next plugins

- **Name fidelity**: a plugin MUST NOT normalize, case-fold, or
  otherwise alter name bytes it passes through (mirrors FR-006).
- **Byte vs glyph**: `NameLen` and all length fields are byte counts;
  display/measure inside plugin UI must convert to UTF-16 (SDK
  helpers provided).
- **Archives**: entry names stored in archives are converted at the
  plugin's format boundary (format-specific code pages ↔ UTF-8);
  the plugin declares per-item failures using the existing skip/error
  callbacks — same UX as FR-014.

## 4. Migration path for third-party authors

1. Recompile against the v-next SDK (headers in
   `src/plugins/shared/`); fix compile breaks from the `NameLen`
   widening and any local `MAX_PATH` assumptions.
2. Treat all interface strings as UTF-8; use the new SDK conversion
   helpers for own `W`-API calls.
3. Return `104` from `SalamanderPluginGetReqVer` (or keep a lower
   ReqVer and add `SalamanderPluginGetSDKVer`=104 per the existing
   dual-export scheme, `spl_base.h:824/839`).
4. Ship. Un-recompiled binaries continue to work under the shim at
   legacy capability.

## 5. Bundled plugin porting order (increments 7a–7d in plan.md)

| Wave | Category | Rationale |
|------|----------|-----------|
| 7a | Archivers (zip, tar, pak, …) | highest user impact — SC-008's archive scenarios |
| 7b | Viewers (pictview replacement, text/hex, …) | opens files at long/Unicode paths |
| 7c | Filesystem/network (ftp, wmobile, …) | path semantics differ per backend; needs the matured SDK helpers |
| 7d | Utilities (checksum, compare, …) | lowest coupling |

Each wave lands independently; an un-ported bundled plugin behaves —
temporarily, within the feature branch only — exactly like a legacy
third-party plugin (shimmed), so the app is shippable between waves.
Feature completion requires all 35 ported (FR-012).
