# Data Model: Fix About Dialog Copyright Notice

**Feature**: 040-fix-about-copyright
**Date**: 2026-07-26

This feature has no runtime data. Its "model" is three build-time string
constants and the surfaces that consume them.

## Entities

### 1. `VERSINFO_COPYRIGHT_NEWT` — Newt Commander notice (display)

| Attribute | Value |
|-----------|-------|
| Defined in | `src/versinfo.rh2` |
| Value | `Copyright © 2026 Newt Commander Authors` |
| Self-contained | Yes — reads correctly on its own |
| Language | English, always; never localized |
| Display position | **First** (upper) line, both in About and on the splash |
| Consumers | `CAboutDialog::DialogProc` (`IDC_STATIC_1`), `CSplashScreen::PrepareBitmap` (`CopyrightR`) |
| Replaces | `VERSINFO_COPYRIGHT2` (`© 2026 Newt Commander Authors` — lacked the word "Copyright") |

### 2. `VERSINFO_COPYRIGHT_OPENSAL` — Open Salamander notice (display)

| Attribute | Value |
|-----------|-------|
| Defined in | `src/versinfo.rh2` |
| Value | `Copyright © 1997-2026 Open Salamander Authors` |
| Self-contained | Yes |
| Language | English, always; never localized |
| Display position | **Second** (lower) line, both in About and on the splash |
| Consumers | `CAboutDialog::DialogProc` (`IDC_STATIC_2`), `CSplashScreen::PrepareBitmap` (`Copyright2R`) |
| Replaces | `VERSINFO_COPYRIGHT1` (same value, previously first line) |

### 3. `VERSINFO_COPYRIGHT` — combined notice (metadata only)

| Attribute | Value |
|-----------|-------|
| Defined in | `src/versinfo.rh2` |
| Value | `Copyright © 1997-2026 Open Salamander Authors, © 2026 Newt Commander Authors` |
| Consumer | `src/plugins/shared/versinfo.rc2:57` → `VALUE "LegalCopyright"` in the version resource of `newtcommander.exe` and `english.slg` |
| Displayed to the user? | No — file properties metadata only |
| Changed by this feature? | **No.** FR-012 requires byte-identity. |

## Relationships

```text
src/versinfo.rh2
├── VERSINFO_COPYRIGHT ────────────► plugins/shared/versinfo.rc2  ─► LegalCopyright (metadata, unchanged)
├── VERSINFO_COPYRIGHT_NEWT ───┬───► CAboutDialog   → IDC_STATIC_1  (About, line 1)
│                              └───► CSplashScreen  → CopyrightR    (splash, line 1)
└── VERSINFO_COPYRIGHT_OPENSAL ┬───► CAboutDialog   → IDC_STATIC_2  (About, line 2)
                               └───► CSplashScreen  → Copyright2R   (splash, line 2)
```

Note the deliberate asymmetry: the display constants are *not* derived from
`VERSINFO_COPYRIGHT`, and `VERSINFO_COPYRIGHT` is *not* derived from them. The
two display lines each carry `Copyright ©`; the combined metadata string carries
it once. Deriving one from the other would require multi-level macro expansion
inside `rc.exe` for a string that must stay byte-identical — rejected in
research.md finding 5. A maintenance comment in `versinfo.rh2` records that the
three move together when the copyright year changes.

## Control slots (translation archive)

These are not data the product reads at runtime any more, but they still occupy
positional slots that the import depends on.

| Archive row key | Control | Geometry | Text after this feature | State |
|-----------------|---------|----------|-------------------------|-------|
| `1150,10,97,196,8` | `IDC_STATIC_1` in `IDD_ABOUT` | unchanged | `""` (empty) | `1` |
| `1151,10,108,196,8` | `IDC_STATIC_2` in `IDD_ABOUT` | unchanged | `""` (empty) | `1` |

Applies to all 11 archives under `translations/*/salamand.slt`, and to the
English original in `src/lang/lang.rc`.

State stays `1` ("translated") to match every other empty-caption row in the
archives — e.g. `101,8,18,189,86,1,""`. An empty original with an empty
translation is consistent, and leaves nothing for a translation round to fill.

## Invariants

- **INV-1**: `VERSINFO_COPYRIGHT` is byte-identical before and after this
  feature. (FR-012)
- **INV-2**: Both display constants begin with `Copyright © ` and end with an
  `Authors` attribution; neither depends on the other to read correctly. (FR-006)
- **INV-3**: The Newt Commander constant is displayed above the Open Salamander
  constant on every surface that shows both. (FR-002, FR-003, FR-011, SC-007)
- **INV-4**: No `salamand.slt` archive contains a non-empty text for the two row
  keys above. (FR-009a, SC-004a)
- **INV-5**: Each edited archive keeps its UTF-8 BOM, CRLF line endings, and
  exact row count. (Positional import requirement.)
- **INV-6**: `IDD_ABOUT` keeps twelve controls with unchanged IDs and geometry;
  only two captions change. (FR-008)
