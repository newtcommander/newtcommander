# Feature Specification: SFTP — Reachable Settings, Reliable Connect, Tight Dialog Layout

**Feature Branch**: `054-fix-sftp-config-dialog`
**Created**: 2026-08-06
**Status**: Draft
**Input**: User description: "Nyni vyřešíme problematické okno s nstavením pluginu SFTP, když kliknu na tlačítko \"Konfigurovat\", tak se nic nestane, ale cely program se zasekne. Dale vyresime opravu a pripojovani na localhost, resp. celou 2. Jedna black-holed adresa spolkne celý timeout připojení. localhost selže („Došlo k vypršení časového limitu\"), 127.0.0.1 se připojí okamžitě. localhost se rozkládá nejdřív na ::1, kde SYN padá do prázdna (Hyper-V firewall), a čekací smyčka drží tuto adresu, dokud nevyčerpá celý společný rozpočet — IPv4 se pak nezkusí. Zavedla to feature 051 záměrně (aby mrtvé adresy nekumulovaly timeouty), vedlejším efektem je tohle. Zaslouží si vlastní opravu. Posledni uprava se tyka pripojovaciho dialogu SFTP / uprav logku zobrazeni tak, ze nejprve nactou lokalizovane texty pro prvni sloupec textovych poli a sirka sloupce se pak nastavi podle nejdelsiho z nich (plus nejaka ochranna zona, padding). Nyni to nevypada dobre, je tam v podstate velka mezera."

## Problem Statement

Three independent defects in the SFTP plugin, all confirmed by measurement
during feature 053 (see `specs/053-sftp-connect-dialog/investigation.md` §9):

1. **The plugin's settings are unreachable and the application appears
   frozen.** Pressing "Configure" for the SFTP plugin opens a settings window
   that is never visible: it has no title bar or frame, and it is positioned
   outside the window it belongs to, so it is clipped away entirely. Because it
   is nevertheless a *modal* window, the application stops responding to the
   user — there is no visible window to close and no way back except killing
   the process. The user therefore cannot change any SFTP setting at all.
2. **Connecting to a host with more than one address can fail even when the
   host is reachable.** If the first address the system offers is silently
   unreachable (packets dropped, no rejection), the whole connect budget is
   spent waiting on it and the remaining addresses are never tried. Concretely:
   `localhost` fails with "The operation timed out" while `127.0.0.1` connects
   instantly, because `localhost` offers its IPv6 address first and that route
   is silently blocked.
3. **The connect dialog wastes horizontal space.** Its label column is sized
   for the longest translation across *all* shipped languages, so in any given
   language most labels are followed by a large empty gap before the input
   fields.

## Clarifications

### Session 2026-08-06

- Q: When a host name offers several addresses and the first one stays silent,
  how long should the program wait on it before trying the next? → A: Do not
  wait — start the next address shortly after the first (about a quarter of a
  second) without giving up on it, and take whichever answers first. A dead
  IPv6 address then costs almost nothing.
- Q: What should the SFTP settings window look like once it is visible? → A: An
  ordinary dialog window with a title bar and OK/Cancel, exactly like every
  other plugin's settings (e.g. 7-Zip) — not a page inside the application's own
  Configuration window.
- Q: What happens when a language's labels need more room than the dialog has?
  → A: The dialog itself follows the language — narrower where labels are short,
  wider where they are long. The input fields keep their width and nothing is
  ever clipped; only the window's own width varies between languages.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - SFTP settings can be opened and changed (Priority: P1)

A user opens Plugins Manager, selects the SFTP plugin and presses Configure.
A settings window appears, with a title and a frame, where they change a value
and confirm. The setting takes effect and the application keeps responding
throughout.

**Why this priority**: Today this action makes the application unusable and
requires killing it; no SFTP setting can be changed by any means. It is both
the most severe symptom and the one the user reported first.

**Independent Test**: Press Configure for the SFTP plugin, verify a window
appears, change the log size and confirm, then reopen it and verify the new
value is shown — with the application responsive at every step.

**Acceptance Scenarios**:

1. **Given** the SFTP plugin is selected in Plugins Manager, **When** the user
   presses Configure, **Then** a settings window appears on screen, fully
   within view, with a title and the usual window frame.
2. **Given** the settings window is open, **When** the user changes a value and
   confirms, **Then** the window closes, the value is kept, and reopening the
   window shows it.
3. **Given** the settings window is open, **When** the user cancels or closes
   it, **Then** the window closes, no value is changed, and the application
   responds normally.
4. **Given** the settings window is open, **When** the user looks at it in any
   shipped language, **Then** every label is fully readable and nothing
   overlaps (the layout corrected in feature 053 becomes visible for the first
   time).
5. **Given** any point in this flow, **When** the user interacts with the
   application, **Then** it never stops responding.

---

### User Story 2 - A reachable host connects even when one of its addresses is dead (Priority: P2)

A user connects to a host name that resolves to several addresses, one of
which silently drops traffic. The connection succeeds through a working
address, within the normal connect time.

**Why this priority**: Makes a common configuration (a dual-stack name such as
`localhost` on a machine whose IPv6 loopback is filtered) usable again. Ranks
below US1 because a workaround exists — type the address directly.

**Independent Test**: With the reference server reachable on IPv4 only,
connect to `localhost` and confirm it succeeds in about the same time as
`127.0.0.1`.

**Acceptance Scenarios**:

1. **Given** a host name whose first address silently drops traffic and whose
   second address accepts connections, **When** the user connects, **Then** the
   connection succeeds.
2. **Given** the same host, **When** the user connects, **Then** it succeeds in
   under 2 seconds — the dead address does not hold up the working one.
3. **Given** a host where **every** address is unreachable, **When** the user
   connects, **Then** the attempt fails within the configured timeout — not a
   multiple of it — with the existing timeout message.
4. **Given** a host that is simply refusing connections, **When** the user
   connects, **Then** it fails promptly with the existing message, as it does
   today.
5. **Given** a connection attempt in progress, **When** the user cancels,
   **Then** it stops promptly, as it does today.

---

### User Story 3 - The connect dialog fits its labels (Priority: P3)

A user opens the connect dialog and sees each label close to its input field,
with a small, even gap — not a wide empty band. The dialog is only as wide as
the language in use needs.

**Why this priority**: Cosmetic. The dialog is fully usable and readable
today; it is merely wider than necessary in most languages.

**Independent Test**: Open the connect dialog in Czech and in English and
confirm the gap between the label column and the fields is small and similar
in both, with no text clipped.

**Acceptance Scenarios**:

1. **Given** any shipped UI language, **When** the user opens the connect
   dialog, **Then** the space between the longest label and the input fields is
   a small, deliberate margin rather than a wide gap.
2. **Given** any shipped UI language, **When** the user opens the connect
   dialog, **Then** no label is clipped — the guarantee from feature 053 still
   holds.
3. **Given** a language whose labels are short, **When** the dialog opens,
   **Then** the dialog window itself is narrower than for a language whose
   labels are long — the window follows the language.
4. **Given** the dialog after adjustment, **When** the user looks at it,
   **Then** the input fields are no narrower than before, the controls stay
   aligned in one column, and nothing overlaps.

---

### Edge Cases

- The settings window opened twice in a row, or while a connection is active.
- A host name that resolves to a single address (the common case) — connect
  behaviour and timing must not change.
- A host where the *working* address is offered first — must still connect
  immediately, with no penalty from the new behaviour.
- All addresses dead: the total wait must stay bounded by the configured
  timeout, which is what feature 051 set out to guarantee.
- Cancelling while several addresses are being attempted at once — every one of
  them must be dropped.
- Two addresses answering at almost the same moment — exactly one connection is
  kept and the other is closed, never left dangling.
- A language whose longest label is much wider than the dialog would otherwise
  need — the dialog grows rather than clipping or squeezing the fields.
- A dialog that grows wider than the screen it opens on — it must stay usable
  and fully reachable.
- Languages not currently shipped (Russian, Ukrainian, Simplified Chinese)
  whose source is retained: sizing must be computed, not hardcoded, so
  re-enabling them needs no further layout work.
- Users with a saved SFTP configuration: their stored settings must be
  preserved and shown when the settings window finally opens.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The SFTP settings window MUST appear on screen when the user
  presses Configure — visible in its entirety, positioned within the visible
  desktop area, as an ordinary dialog window with a title bar naming the plugin
  and confirm/cancel buttons, matching how every other plugin presents its
  settings.
- **FR-002**: The application MUST remain responsive while the settings window
  is open, and the user MUST be able to close it by the usual means (confirm,
  cancel, window close).
- **FR-003**: Confirming the settings window MUST apply and persist the changed
  values; cancelling MUST leave them unchanged. Previously stored settings MUST
  be preserved and displayed.
- **FR-004**: A connection to a host with several addresses MUST succeed when
  any one of them accepts connections, including when an earlier address
  silently drops traffic. Addresses MUST be attempted in overlapping fashion —
  the next attempt starts a short delay (about a quarter of a second) after the
  previous one, without abandoning it — and the first address to answer wins.
- **FR-005**: The total time spent failing to reach a completely unreachable
  host MUST remain bounded by the configured connect timeout, and MUST NOT
  grow with the number of addresses (the guarantee feature 051 introduced).
- **FR-006**: Connecting to a host whose first address works MUST NOT become
  slower than it is today.
- **FR-007**: Cancelling a connection attempt MUST remain prompt, and MUST
  abandon every address being attempted at that moment.
- **FR-011**: Once one address succeeds, all other attempts for that connection
  MUST be abandoned, leaving no connection to the losing addresses.
- **FR-008**: The connect dialog's label column MUST be sized from the labels
  of the language actually in use, plus a small margin — not from the longest
  translation across all languages.
- **FR-009**: No label in the connect dialog may be clipped in any shipped
  language after the change (feature 053's guarantee is preserved).
- **FR-010**: Input fields in the connect dialog MUST NOT become narrower than
  they are today, and the dialog MUST stay in the product's house style with
  its controls aligned and non-overlapping.
- **FR-012**: The connect dialog's own width MUST follow the label column —
  shrinking for languages with short labels and growing for languages with long
  ones — so the gap stays small in every language without ever clipping a label
  or narrowing a field.

### Key Entities

- **Plugin settings window**: the window presenting the SFTP plugin's options
  (timeouts, keepalive, retries, resume, permissions column, logging); reached
  from Plugins Manager.
- **Host address list**: the set of network addresses a host name resolves to;
  attempted in order until one accepts.
- **Connect timeout**: the user-configurable upper bound on how long reaching a
  host may take, in total.
- **Dialog label column**: the run of field labels in the connect dialog whose
  width determines where the input fields begin.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of attempts to open the SFTP settings result in a visible,
  usable window; the application never has to be killed to recover.
- **SC-002**: A user can change and persist an SFTP setting in under 30
  seconds, from Plugins Manager to a confirmed change — a task that is
  impossible today.
- **SC-003**: Connecting to a host name whose first address silently drops
  traffic succeeds in under 2 seconds when a working address exists (with the
  default 20-second timeout) — down from a guaranteed failure today.
- **SC-004**: A completely unreachable host fails within the configured connect
  timeout in 100% of attempts, regardless of how many addresses it has.
- **SC-005**: In every shipped language, the gap between the longest label and
  the input fields is at most a small margin, no label is clipped, and no input
  field is narrower than before — verified by opening the dialog in each
  language.

## Assumptions

- The settings window's *contents* are already correct: feature 053 sized every
  control for the longest shipped translation. This feature makes that window
  visible; it does not revisit its labels beyond what FR-009 requires elsewhere.
- "The whole program freezes" is the user-visible symptom of a modal window
  that cannot be seen or closed; the fix is to make the window a normal,
  visible one rather than to change how modality works.
- Address-attempt order is left to the system's own preference (as today); this
  feature changes only that a slow address no longer blocks the ones behind it.
- Bounding the total connect time remains a hard requirement — feature 051
  introduced it deliberately after users reported multi-minute waits, and
  FR-005 keeps it. Overlapping attempts do not conflict with it: they shorten
  the successful case and leave the all-dead case bounded as before.
- Briefly holding more than one connection attempt open is acceptable; the
  losing ones are dropped as soon as a winner appears (FR-011). This is the
  same approach mainstream browsers use for dual-stack hosts.
- Sizing the label column "for the language in use" is a run-time decision
  about layout, not a change to any translated text (feature 053's FR-008
  still applies: no wording changes).
- The small margin after the longest label is a fixed, modest amount chosen for
  visual comfort; its exact value is an implementation decision.
- A dialog whose width follows the language means the window is a different
  size in different languages. That is accepted as the price of a tight,
  consistent gap — the alternative (fixed window, moving divider) would narrow
  the input fields in verbose languages, which FR-010 forbids.
- Feature 053 widened these dialogs to fit the longest translation of all
  languages. This feature supersedes that sizing for the connect dialog only;
  the other SFTP dialogs keep the fixed widths 053 gave them.
- Scope is the SFTP plugin. The same run-time sizing idea might suit other
  dialogs, but applying it product-wide is separate work.
