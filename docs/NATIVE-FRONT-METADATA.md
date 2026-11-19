# Native source metadata and store declarations

The Native web compiler now records real contract readiness, store declarations,
store-key warnings, diagnostic items and the complete 19-entry diagnostic code
catalog. `"$FREELANG_NATIVE_WEB_TOOL" --codes` prints that catalog without building or
executing application code. The tool's `--metadata <bundle>` reads an existing plan and
successful AST inputs. The default development entry still awaits remaining
static diagnostics and headless execution.

## Contract readiness

`tools/front-contracts.fl` parses the existing comment annotation format,
including repeated nested keys and multiple blocks. It preserves contract
fields, check summaries, read/write route links and verification metadata.
A contract is complete only when it has at least one check, all checks pass,
its route is nonempty and its verification commit exactly matches the build's
HEAD. Missing, stale and failed verification cannot produce a complete row.
The summary counts complete, in-progress, missing and verified rows separately.

The supervisor captures a short Git HEAD for each build and passes it as data
through `FL_NATIVE_FRONT_COMMIT`; state uses the same captured value. Direct
build callers can set that variable explicitly. Missing HEAD cannot verify a
contract. This is annotation readiness, not evidence that declared checks were
executed by the metadata reader or that the working tree equals a Git commit.

## Stores and diagnostics

Top-level `(defstore :key initial)` declarations are removed from generated
server code. Each route's generated browser program initializes helper and
route defaults after HMR restoration and before mounting islands. Defaults
apply only to null/missing values; false, zero, empty collections and restored
values remain intact. Another route's declarations enter the global registry
without being initialized on the current route.

The existing literal defaults are supported: numbers, strings, booleans, nil,
empty arrays and empty maps. Other initializer expressions use the legacy null
fallback and are never evaluated. This includes file and process expressions.
The registry retains first declaration order and deduplicates keys. Undeclared
literal store-key calls produce the existing warning when declarations exist.

`tools/front-diagnostic-codes.fl` is the Native catalog data source. All six
fields and the order of its 19 entries match the unchanged legacy catalog.
Diagnostic items carry code, severity, file/position, message, hint, fixability
and applicability. Unknown codes retain the legacy empty hint/manual fallback.
The earlier temporary `diagnostics` status object becomes an item array;
`diagnosticStatus` separately records implemented and pending coverage.

The subsequent [static diagnostic implementation](NATIVE-STATIC-DIAGNOSTICS.md)
adds 17 catalog checks and actual SDK declaration metadata. Console-only
advisories, automatic fixes and island tests remain open. A complete catalog does not establish
complete detection, and contract annotations do not establish commercial or
application acceptance. Function-level source maps and live runtime translation
also need their remaining work.

## File notification correction

A timed Bash `read` can consume part of a line before returning a timeout.
The old host discarded those bytes, so the next suffix could appear as an
invalid notification and terminate development. The reader now retains partial
bytes until the newline, validates the fixed `CHANGE` protocol and records a
failed WATCH state before exiting on malformed input. The debounce and
readiness/stop bounds are unchanged.

Private pipe controls reproduce the failure in the old reader with two and
three fragments, and verify the corrected reader on the same inputs. A malformed
message still fails; the actual notification host and its protocol are unchanged.

## Evidence and scope

Receipt: `native-dev-metadata-20261120`; paths/owners: <가림>. The
[evidence](evidence/native-dev-metadata-20261120.json) preserves hashes, failures
and exact qualification scopes.

Twenty-five annotation cases compare complete results with output from the
unchanged legacy parsing/readiness functions. The legacy Node reference is a
private diagnostic oracle only. Native product build/runtime traces are audited
separately. Catalog and diagnostic conversion add two controls. The first edge
comparison found a scalar `checks: pass` count difference; the corrected Native
implementation preserves the original Object.values behavior and all 27 pass.

Real generated applications and Chrome controls cover initial defaults, duplicate
declarations, Unicode/escaped strings, false/empty values, nonexecution of an
initializer, real DOM clicks, automatic CSS reload without app/DOM replacement,
automatic source restart with restored store/atoms, stale verification after a
new Git commit, and recovery after updating verification. These are functional
DOM controls with layout checks; screenshot and physical pointer acceptance
are not added by this receipt. Existing standalone island output is compared
byte-for-byte with the previously accepted SDK.

An initial candidate passed the combined 24 controls. A relocated SDK repetition
then exposed the timed-notification failure; that failed run remains retained.
Final package/functional results are recorded separately in the evidence and do
not relabel the failed run. Protected tests, expectations, runners, time/memory
bounds, seed and foreign changes remain unchanged. The overall goal is active,
commercial readiness is HOLD, and production deployment has not run.
