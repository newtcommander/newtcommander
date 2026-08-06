# Data Model: Dialog Presentation and Connection Attempts

**Feature**: 054-fix-sftp-config-dialog · Phase 1 artifact

This feature adds no persisted data. Its "entities" are the two things whose
*shape* changes: how a dialog is presented, and how a connection attempt
progresses.

## Entity: settings window presentation

| Aspect | Today (broken) | After |
|---|---|---|
| Template style | child window: no caption, no frame | ordinary modal dialog with the standard frame |
| Caption | none (its `.slt` caption row exists but is empty) | the plugin's existing, already-translated configuration title |
| Confirm / cancel buttons | none in the template, although the code already handles both commands | present, following the shape used by the module's other dialogs |
| Position | computed in screen coordinates, applied relative to the parent → lands outside it and is clipped away | centred on the parent, on screen, fully visible |
| Result for the user | invisible modal window; the application appears frozen | a normal settings window that can be confirmed or cancelled |

**Invariants**
- Values shown are the stored settings; confirming persists them, cancelling
  leaves them untouched (no change to what is stored or where).
- The controls inside keep the layout feature 053 gave them — that work becomes
  visible for the first time.

## Entity: connect attempt

A single logical connect to a host name, which fans out over the addresses the
name resolves to.

| Field | Meaning |
|---|---|
| address list | the addresses the host name resolved to, in the system's preferred order |
| total budget | the configured connect timeout; an upper bound on the **whole** attempt, not per address |
| stagger delay | how long after starting one address the next one starts (~0.25 s) |
| in-flight attempts | the attempts currently open; more than one may exist briefly |
| winner | the first attempt to be accepted; every other is closed |

**State transitions**

```
resolve name ─┬─ fails ──────────────► report "cannot resolve"
              └─ succeeds
                    │
                    ▼
   start first address; every ~0.25 s start the next one too
                    │
      ┌─────────────┼──────────────┬───────────────────┐
      ▼             ▼              ▼                   ▼
 one accepts   all refused   budget exhausted     user cancels
      │             │              │                   │
      ▼             ▼              ▼                   ▼
 keep it, close  report        report "timed out"   report "cancelled"
 the others      "unreachable"  (bounded by the      (all attempts
 → session       (no wait)      configured timeout)   abandoned)
```

**Invariants** (full statement in
[contracts/connect-attempt-behaviour.md](contracts/connect-attempt-behaviour.md))
- exactly one socket survives; losers are closed, never leaked;
- the total is bounded by the configured timeout regardless of address count;
- the surviving socket keeps the non-blocking mode the SSH layer depends on;
- cancellation abandons every in-flight attempt;
- no message text changes.

## Entity: connect dialog label column

The run of field labels whose width decides where the input fields begin.

| Aspect | Today | After |
|---|---|---|
| Width source | fixed in the template, sized for the longest translation of **all** languages (100 units) | measured at open time from the labels of the **loaded** language, plus a small margin |
| Field column start | fixed (230 units) | follows the measured label width |
| Field width | 120 units | unchanged — fields never shrink |
| Dialog width | fixed (420 units) | follows the label column: narrower for short-label languages, wider for long ones |

**Labels that participate**: the field labels in the right-hand column — host,
user, password, key file, passphrase, initial path. The port label sits at the
end of the host row and is measured with it rather than being part of the
column; the bookmarks label belongs to the left-hand list and is not affected.

**Invariants**
- no label is clipped in any language (feature 053's guarantee is preserved);
- fields are never narrower than today;
- controls stay aligned in one column and never overlap;
- the dialog remains usable if the computed width exceeds the screen;
- computed, not hardcoded — languages that are not currently shipped need no
  further work when they are re-enabled.

**Scope**: the connect dialog only. The module's other dialogs keep the static
widths feature 053 gave them.
