# Guard lifecycle correction — 2026-11-21

Scope: only the resource supervisor's parent-exit lifecycle. No application,
FX compiler, database, WAL, service, account, network, or existing test contract
was changed. These files were prepared in the existing isolated review copy.
The previously untracked Guard and its two baseline synthetic test files are
included with this correction; unrelated pending frontend/TTY work is excluded.

## Cause and behavior

The approved diagnostic observed seven rejections with
`before_child_list/PARENT_EXIT_OBSERVED`: a pinned parent had exited normally,
but the live-parent child-list absence check treated its exit as an error.
The diagnostic and original three-failure stop records remain unchanged.

The fix distinguishes `PARENT_REJECTED`, `PARENT_LIVE`, and `PARENT_EXITED`.
An ESRCH child-pin failure combined with the parent's pinned exit observation
now requires another owned/adopted descendant traversal, starting at the
subreaper. It stops consuming that parent's stale list. It does **not** infer
that a particular child exited, record zero RSS, or establish a measured peak.

A PASS still requires the driver's actual zero wait status, stable owned
identities, final `ECHILD` drain, zero remaining children, no pending lifecycle
work, and successful evidence synchronization. Still-live unreadability,
malformed inputs, identity mismatch, invalid pidfds, evidence errors, and
nonzero driver status continue to block. No raw-PID kill path was added.

## Direct verification

All builds used `cc -O2 -std=c11 -Wall -Wextra -Werror`; the selftest translation
unit additionally used the existing unused-static suppressions. Five C/ELF
builds exited zero with no diagnostics. No Node/JS/TS was used or added.

- Existing parser/measurement/evidence selftests: **15/15**.
- Added lifecycle unit checks using real `/proc`, pidfds, and child processes:
  **25/25**. No syscall mocks or injected storage faults.
- Synthetic process scenarios: **8/8 expected outcomes**:

| Scenario | Guard exit | Expected result / direct evidence |
| --- | ---: | --- |
| Normal detached descendant | 0 | PASS; descendant measured and reaped |
| RSS budget breach | 2 | BLOCKED; actual peak 71,483,392 B above 16 MiB; owned TERM only |
| Inherited ignored SIGCHLD | 0 | PASS; normal wait-status handling restored |
| Non-fresh evidence directory | 2 | Refused before target launch |
| Parent exits before living adopted descendant | 0 | PASS; adopted descendant measured, reaped after driver |
| Nonzero driver | 2 | BLOCKED; actual driver exit 7 retained |
| Fast fork/reap | 0 | PASS; 256/256 synthetic children normal; final drain complete |
| Evidence-coordinated parent exit during child recheck | 0 | PASS; 64/64 children normal; one parent-exit rescan observed |

The last scenario took approximately 0.097 s and exercised
`PARENT_EXIT_RESCAN_REQUIRED` → `OWNED_DESCENDANTS_RECHECKED` → `FINAL_DRAIN`.
RSS for the unpinned child remained null. Every completed post-launch scenario
recorded zero remaining children and a complete final drain. Event SHA-256 and
strict event sequence were checked. An unrelated sentinel was excluded from
Guard events and exited normally on its controller's request.

One initial background-controlled fast scenario lost its controller before a
final receipt. Its incomplete evidence is preserved and is **not** a PASS.
The already completed builds/checks were not repeated; the fast scenario was
then executed once under foreground control in a fresh evidence directory.
The separate exit-window scenario exists to prove the new branch, which that
completed fast run did not observe. No failed result or original counter was
reset, relabeled, or removed.

## Reproduction (fresh private output directory)

Commands below describe the test interfaces. Replace `<output>` with a fresh
private directory; placeholders are not actual environment paths. Process inspection must
precede execution. Use `flock -o /tmp/fl-heavy.lock` and an inherited 2 GiB
address-space limit. Do not rerun the original blocked workload automatically.

```sh
cc -O2 -std=c11 -Wall -Wextra -Werror tools/native-resource-guard.c -o <output>/guard -lcrypto
cc -O2 -std=c11 -Wall -Wextra -Werror tests/native-resource-fixture.c -o <output>/fixture
cc -O2 -std=c11 -Wall -Wextra -Werror tests/native-resource-fast-fixture.c -o <output>/fast
cc -O2 -std=c11 -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable tests/native-resource-guard-lifecycle.c -o <output>/lifecycle -lcrypto
cc -O2 -std=c11 -Wall -Wextra -Werror tests/native-resource-guard-exit-window.c -o <output>/exit-window
<output>/lifecycle
python3 tests/native-resource-guard.py --guard <output>/guard --fixture <output>/fixture --evidence <output>/existing
python3 tests/native-resource-guard-lifecycle.py --guard <output>/guard --lifecycle <output>/lifecycle --fast <output>/fast --evidence <output>/lifecycle-checks
<output>/guard --evidence <output>/window --interval-ms 10 --max-ms 3000 --grace-ms 1000 -- <output>/exit-window <output>/window/events.jsonl
```

The exit-window fixture reads only its own run's event log and exits after
its short-lived children were reaped and a child-pin attempt was recorded.
Its source is test synchronization, not production branching or a stub.
The exact preserved 256-child fixture source is included as
`tests/native-resource-fast-fixture.c`, with source SHA-256
`6840b19cbe66e250df1b2fdf90d6a2f68cbe0ff5de153c1c46a43a0bba0d881d`.
A redacted machine-readable result is recorded in
`docs/evidence/guard-lifecycle-20261121.json`.
Raw commands, exits, timestamps, input/ELF hashes, approvals, incomplete
receipts and the original failed diagnostic remain in private evidence.
No private filesystem location is committed.

## Provenance

| Artifact | SHA-256 |
| --- | --- |
| `tools/native-resource-guard.c` | `d7d3e8ac4cce69461b1a4cf18cf7749856a0a5b43e4d0b0cc4a4a88e2174f81c` |
| `tests/native-resource-fixture.c` | `fb5ae51ee40f027da9dcb951f7a60d076b441a6bd5cbcb314fe3378d9041dba9` |
| `tests/native-resource-guard.py` | `214f9cb9eea0655ac2e3d21a6897932d65cc451a38ab7bd8945af706692651f9` |
| `tests/native-resource-guard-lifecycle.c` | `5df03cdac342fc76d6a26e72e50f385c9d56c7c3c928044c34d3091bb8807cbb` |
| `tests/native-resource-guard-lifecycle.py` | `24693d520794fc4c5e3ab4b29feb6a7b4ef6e37859791b609132efe629f5295f` |
| `tests/native-resource-guard-exit-window.c` | `44b351d83370f53adaa3a9e3bf2f9f667bdee0d8a5e9ecbf92914c669b30070f` |
| `tests/native-resource-fast-fixture.c` | `6840b19cbe66e250df1b2fdf90d6a2f68cbe0ff5de153c1c46a43a0bba0d881d` |
| Guard ELF | `61d9a4d63cab417a74cf37c62f27d9f0726db23a94a63b54cbde1c170e8c3b9f` |
| Approved diagnostic baseline source | `cf3bb7db5f19051c9f73b2cad59966ca4306d5974b3f42fef5e726f85883a92a` |

An inherited extra blank EOF line was removed after the runtime checks, without
changing any C token. The tested source SHA was
`0e440eb208120a74e7a407847a00940f650d3b964d3633f7b01338daed939208`;
the table records the final source. Two additional warning-free equivalence
builds produced Guard and lifecycle selftest ELFs byte-identical to the tested
artifacts. This proves the final source-to-tested-ELF link without repeating
runtime checks.
Original and normalized source snapshots are both retained. A 45-second lock
wait occurred before the final builds; no test or build ran during that wait.

## Independent QA and limits

Independent static QA: **PASS**, covering the tested Guard source hash stated above.
Independent execution-evidence QA: **PASS** — 40/40 unit assertions,
8/8 expected scenario outcomes, 7/7 complete result streams with 489 events,
15/15 original receipts and 30/30 stdout/stderr hashes checked. The reviewer
verified the actual parent-exit event at sequence 25, successful traversals at
27 and 43, and final drain at 46. Protected originals remain 5/5 unchanged.
The redacted independent report SHA-256 is
`7a236cd5afe44872b1c961e5962e90deaa5ed8baa6440d71be65b614d2f282c3`.

The supervisor remains a sampled resource observer, not a physical-memory
cgroup or an independent administrator/Guard/Anchor trust boundary. Short-lived
unpinned child peaks remain **UNVERIFIED**. This correction does not establish
commercial DB durability, frontend integration PASS, or independent disk/power
isolation. Existing three-failure history remains three; a fresh general-workload
resume is not implied by these narrow synthetic checks.

`GUARD_LIFECYCLE=PASS` applies only to the completed checks and independent
execution review above; `CRASH_TEST=HOLD`; `RELEASE=BLOCKED`.
