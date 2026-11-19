# Native development supervisor

`scripts/fl-dev-native` provides an explicit Native development entry. Activate
the matching SDK and run it against an existing web tree:

```sh
source "<matching-sdk>/env.sh"
./scripts/fl-dev-native "<project>/web" --artifacts "<existing-output-directory>"
```

The output directory must be outside the source web tree. The entry binds only
to loopback; application and HMR ports default to 40850 and 40851. `PORT` and
`FL_HMR_PORT` select distinct ports. Compiler, web, inspection, watcher, control,
archive and header inputs are explicit SDK configuration. There is no tool
search or Node fallback. The default `scripts/fl-dev` remains unchanged until
its full diagnostic and headless contracts have a verified Native path.

FreeLang owns content decisions, build state, source error extraction and error
resolution bookkeeping. The Bash host supplies process, pipe and file IO.
It preserves literal argument boundaries, including whitespace, apostrophes,
Korean and embedded LF. No handwritten product JavaScript or Python is added.

## Lifecycle

The supervisor starts its own HMR service and source notification host. Native
content snapshots ignore generated files; saves with identical content do not
rebuild. Changes debounce for 250 ms. Each build uses a fresh private directory
and the shared `fl-heavy.lock`, with the existing 2 GiB virtual memory cap.
Changes received during a build remain pending for the next content comparison.

A complete successful build is required before stopping the previous app. Each
started process has a fresh instance header, and readiness requires both its
owned child and that header within three monotonic seconds. A different
listener's HTTP 200 cannot establish readiness. A failed build keeps the prior
app running. A failed new app start restarts the previous complete bundle and
records the failure. Source rebuilds notify `soft-reload` after readiness.

CSS changes stage all copies and backups before publication, replace individual
files atomically, and roll back prior replacements if publication fails. This
is not an atomic transaction across multiple files. Public CSS additions and
removals are included. Style DSL changes use the existing stable development
class names. The app PID remains unchanged, generated CSS is served from the
active bundle without caching, and notification uses `css-reload`.

Normal termination stops only children owned by this supervisor. Application
and inspection processes receive their normal termination signal; a build's
own process group is stopped together. Each child retains the three-second
stop bound. Unexpected application, HMR or watcher exit fails the supervisor
and records a failed state. Notification failure also changes ready state to
an HMR failure, including the race immediately after application readiness.

The SDK's libcurl is for Native binaries. Host `curl` requests run without the
SDK `LD_LIBRARY_PATH`, so the host command resolves its matching system library.
No global loader configuration is changed.

## State and diagnostics

The artifact directory retains build logs, complete/failed bundles and state
for diagnosis. There is no automatic artifact retention limit yet. State records
successful generation, active bundle, route/helper metadata, function-level
source mappings, build issues and error transitions. Failed builds preserve the
last active bundle and successful generation. Existing `HANDOFF.airc` files
retain unrelated fields, deduplicate open errors, resolve repaired errors and
retain the latest 20 rows. The supervisor does not create a missing AIRC file.

Structured parser failures include source file, line and column. The control
tool can translate a generated runtime location to its containing source
function. The subsequent [source metadata implementation](NATIVE-FRONT-METADATA.md)
adds the 19-entry catalog, store defaults/registry and real contract readiness.
Live runtime log translation, expression-level precision, remaining static
diagnostics and headless `/~ai/test` remain open. Explicit `NOT_RUN` fields
describe that scope; empty metadata is not evidence of parity. Browser state restoration was
validated separately in the preceding browser receipt; the metadata receipt
also tests automatic supervisor-driven reloads in Chrome. Neither adds screenshot,
pointer or complete application-flow acceptance.

## Verification

Receipt: `native-dev-supervisor-20261120`; paths and owners: <가림>.
The [evidence](evidence/native-dev-supervisor-20261120.json) records exact source
and artifact hashes, results, corrections and remaining scope. Original API
and watcher controls keep their assertions and bounds. Private integration
controls use real generated apps, file events, HTTP and SSE, without a mock
compiler. Protected tests, seeds and foreign source changes are preserved.

Commercial status remains HOLD, the overall goal is active, and production
deployment has not run.
