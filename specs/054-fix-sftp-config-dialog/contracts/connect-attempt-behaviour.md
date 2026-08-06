# Contract: How a Connection Attempt Behaves

**Feature**: 054-fix-sftp-config-dialog · Phase 1 artifact
**Amends**: feature 051, which made the connect timeout a budget for the whole
address list. That guarantee is kept; what changes is that a silent address no
longer blocks the addresses behind it.

## 1. What the user is promised

| Situation | Outcome | Timing |
|---|---|---|
| Host name resolves to one address, which answers | connects | as fast as today |
| Several addresses, the first answers | connects | as fast as today — no penalty for the new behaviour |
| Several addresses, an earlier one is silent, a later one answers | **connects** (today: fails) | under 2 seconds with the default 20 s timeout |
| Several addresses, an earlier one refuses outright, a later one answers | connects | promptly — a refusal is an immediate answer, not a wait |
| Every address unreachable | fails, with the existing timeout message | within the configured timeout, **never a multiple of it**, regardless of address count |
| Host name cannot be resolved | fails, with the existing resolve message | as today |
| User cancels mid-attempt | stops, with the existing cancelled message | promptly, as today |

## 2. Invariants

- **Bounded total** — the wall-clock time of a failing connect never exceeds the
  configured connect timeout, and does not grow with the number of addresses.
  This is feature 051's guarantee and the reason its shared budget exists.
- **One survivor** — exactly one socket is carried forward. Every other attempt
  is closed before the connect call returns; none is left open or leaked, even
  when two addresses answer at almost the same moment.
- **Cancellation reaches everything** — a cancel abandons every attempt that is
  in flight at that moment, not only the newest.
- **The winner stays non-blocking** — the socket handed on to the SSH layer
  keeps the non-blocking mode feature 051 relies on; without it the library's
  own timeout is not enforced and a black-holed connection can hang the session.
- **Messages are unchanged** — the existing resolve / unreachable / timeout /
  cancelled texts are reused verbatim. No new strings, so no translation work.

## 3. Deliberately unchanged

- The order in which addresses are tried is whatever the system prefers, as
  today. This feature does not re-rank IPv6 before IPv4 or vice versa.
- The retry loop around a failed connect (attempt count and delay from the
  plugin's settings) keeps its current meaning.
- Nothing about authentication, host-key handling or the session afterwards.

## 4. How it is verified

- `localhost` on this machine is a ready-made regression case: it offers a
  black-holed IPv6 address first and a working IPv4 address second. It must
  connect in under 2 seconds; today it fails after the full timeout.
- The existing connect harness must keep passing unchanged — in particular its
  silent-host scenario, which is what pins the bounded-failure invariant.
