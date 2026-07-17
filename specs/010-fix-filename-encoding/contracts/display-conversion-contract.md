# Contract: Display-time text conversion (feature 010)

Every surface touched by this feature MUST satisfy the rules below.
They codify the boundary architecture established by features 004/005
(see research.md R1) so each fix is mechanical and reviewable. This is
an internal interface contract — the feature exposes no external API.

## C1. Encoding contract at the boundary

1. Any `char*` file name, path, or app-stored display string is
   **UTF-8** (feature 004). Never pass it to an `-A` text API or ANSI
   window message.
2. Rendering goes through the **`-W` API** on a converted buffer:
   `SalU8ToW`/`SalU8ToWAlloc` (core) or `SplU8ToW*` (plugins), then
   `ExtTextOutW` / `DrawTextW` / `SetWindowTextW` / `SendMessageW`.
3. Read-back goes wide-first: `GetWindowTextW` (or `-W` message), then
   `SalWToU8`/`SalWToU8Alloc`.
4. Prefer the packaged helpers over inline conversion where one exists:

   | Situation | Helper |
   |-----------|--------|
   | window/dialog-item text | `SalSetWindowTextU8`, `SalSetDlgItemTextU8`, `SalGet*U8` |
   | combo box item | `SalComboAddStringU8` |
   | list box item | listbox `U8` helper (add in this feature, same shape) |
   | ListView cell | `SalListViewSetItemTextU8` |
   | dialog data transfer | `CTransferInfo::EditLine` (already wide) |

## C2. Invalid-UTF-8 fallback (mandatory)

Strict conversion may fail (`SalU8ToWAlloc` → NULL). Every converted
site MUST keep the legacy ANSI call as the fallback branch — never
drop text, never substitute `?`. (Pattern: `winlib.cpp:1093-1105`;
panel reference: `fileswn4.cpp:689,773`.)

## C3. Measurement and indexing units

Wherever text is measured, truncated, ellipsized, hit-tested, or
offset-indexed, the units MUST be **WCHAR units of the converted
string** — never bytes of the UTF-8 original (`GetTextExtentExPointW`,
wide `AlpDX`, WCHAR offsets). Truncation must not split surrogate
pairs (mirror `CTruncatedString`, `salamdr4.cpp:157+`). Mixed
byte/WCHAR bookkeeping in one code path is forbidden (root cause of
the dropped `\AI` component — research.md R2).

## C4. Convert on change, not on paint

Conversion happens when the text is *set* (e.g. `SetText`,
`BuildHotTrackItems`, menu-item construction), cached, and reused by
paint/measure/hit-test. Converting inside `WM_PAINT` is acceptable
only for one-shot, low-frequency draws (tooltips, message boxes).

## C5. Round-trip fidelity

Any value extracted from a display structure for action (clipboard
copy, navigation, packer selection, drag-drop) MUST come from the
stored UTF-8/wide value, never from text re-read out of an ANSI
control or from a lossy rendering (FR-007).

## C6. Registry / configuration

REG_SZ config strings go exclusively through `SetValue`/`GetValue`
(→ `SalRegSetValueExW8`/`SalRegQueryValueExW8`) — UTF-8 in memory,
UTF-16 in the registry. Sections whose entries may predate the UTF-8
baseline are guarded by the `THIS_CONFIG_VERSION` gate: older
version ⇒ rebuild from defaults, no conversion attempts
(clarification 2026-07-17; data-model.md §5).

## C7. What must NOT change

- No `UNICODE`/`_UNICODE` build flip, no UTF-8 ACP manifest (004
  rejections stand).
- No visual restyling, no `ICC_STANDARD_CLASSES`, no manifests, no
  control subclassing for style (constitution VI).
- No behavior change for pure-ASCII strings (FR-008) — the ANSI
  fallback path preserves today's behavior bit-for-bit.
- Already-converted reference surfaces (panel drawing, window title,
  005-fixed dialogs) are regression guards — do not refactor them.

## Acceptance mapping

| Contract rule | Spec requirement |
|---------------|------------------|
| C1, C2 | FR-001, FR-002, FR-003, FR-006 |
| C3 | FR-002 (ellipsis/shortening), SC-001 |
| C4 | Performance constraint (plan Technical Context) |
| C5 | FR-007 |
| C6 | FR-004, SC-002 |
| C7 | FR-008, SC-005, constitution II/VI |
