# Native development server migration contract

Status: implementation in progress. tools/front-dev.fl and
tools/fl-dev-server.fl contain an initial Native HMR/inspection service;
the matching fresh Native SDK passes the 83-case API contract and the 23-case
source watcher contract. The existing
scripts/fl-dev remains the Node entry until all required behavior below
has a verified Native path.
The accepted objective is Node-free build and execution while preserving
application features, diagnostics, expectations and existing test bounds.

## Existing entry and lifecycle

The current entry checks and generates the FLP pages, then starts the development
supervisor. It builds before starting the web application, watches source files
with a 250 ms debounce, and keeps HMR on loopback port 40851. The main application
uses PORT (default 40850). HTTP/HMR startup failures must remain visible failures.
A failed rebuild must not publish incomplete output. Successful rebuilds must
notify connected browsers after the new application is ready. The existing
3-second readiness bound is not to be increased to hide a failure.

Generated CSS must not trigger its own build loop. Existing no-change detection
and the style-only update path must remain available. Source additions,
renames, deletions and nested page/island directories require observation.
Inline style changes can require soft-reload; standalone CSS changes can use
css-reload. Logic changes require rebuilding and restarting the application.
Preserve source-to-generated line translation and error-resolution bookkeeping.

## Browser and inspection interfaces

| Interface | Existing behavior to preserve |
|---|---|
| GET /~hmr | text/event-stream, connected event, then reload/css-reload/soft-reload notifications |
| POST /~errors | Parse JSON, timestamp and prepend an error, retain latest 50; bad JSON returns 400 |
| GET /~errors | Error list in newest-first order |
| DELETE /~errors | Clear error list and return an ok object |
| OPTIONS | CORS preflight: GET, POST, OPTIONS and Content-Type header |
| GET /~ai | Build summary, issues, routes, helpers, store keys, contracts, repository commit, browser state and recent errors |
| GET /~ai/routes | Route records including source file and function name |
| GET /~ai/fns | Function names, parameters and source files, including nested sources |
| GET /~ai/store | Store keys |
| GET /~ai/issues | Error and warning lists with an ok flag |
| GET /~ai/errors | Error count and up to 20 latest entries |
| GET /~ai/contracts | Repository commit, contract readiness and browser state |
| GET /~ai/browser | Browser state, including an explicit missing state |
| GET /~ai/test | Existing headless island selection, click/dispatch and initial store operations |

The Native builder currently records a smaller diagnostic/readiness scope than
the legacy builder. Empty placeholders cannot establish equivalence. The headless
island endpoint also needs a real Node-free execution path before the default
entry can claim complete migration. Browser HMR must preserve island/store state
according to its existing client contract, with the existing full-reload fallback.
No new handwritten JavaScript is authorized; browser output remains generated
from the language implementation.

## Runtime prerequisite in progress

The earlier Native SSE application answered /health with 200 and /~hmr with 404:
string callback registration only searched C symbols. The corrected FX runtime
resolves FreeLang names and closures as ordinary HTTP does, handles complete SSE
frames; that initial checkpoint left final descriptor ownership to HTTP. Its SDK
passes 74/74 normal SSE contracts, 7/7 first-class function-value checks plus
5/5 language assertions, and 19/19 legacy C/concurrent-route checks. Captured
closures, forward registration, query strings, empty/Unicode/multiline/long data,
24 ordinary parallel clients, short successful writes, EINTR and normal shutdown
with an open stream are included. TLS SSE and arbitrary noncooperative handler
shutdown are outside this receipt.

The Native inspection service passes 83/83 API controls, including real HMR
connections and all three broadcast events. File observation and Native content
snapshot/diff pass 23/23 controls, including nested additions/renames/deletions,
metadata-only saves, binary assets and generated-output exclusions. These results
do not establish rebuild supervision, browser state restoration or headless
island execution. Those features remain open; scripts/fl-dev stays unchanged.

The source compiler passes seed-to-Stage4 bootstrap, Stage2=3=4, existing core
regressions and unchanged time/memory bounds. The receipt is
native-sse-firstclass-20261120. No protected test or runner was edited.

## Subsequent implementation checks

The Native browser core now shares store, subscription and atom capabilities
between independently generated modules in the same document. Development
builds add a separate HMR module, including HTML pages without islands and HTML
error responses. A saved store/atom snapshot is consumed once before mounting;
restored atoms do not run their default initializer. CSS reload updates local
stylesheet links; reload events save state first. Browser errors are forwarded
to the existing inspection service.

Actual Chrome execution passed 23 functional controls: shared-state updates,
unsubscribe, cross-module dispatch, saved atom/store values, soft-reload restore,
one-time snapshot consumption, computed CSS updates without page navigation,
state retention, browser error forwarding, plain/error-page HMR modules and
unchanged non-HTML 404 responses. Controls used real DOM click methods with
layout-box checks. Pointer hit-testing and screenshot evidence remain unverified:
the background browser did not acknowledge those protocol commands. Functional
results therefore do not establish complete visual or all-app acceptance.

The first HMR execution failed because a raw host `undefined` value bypassed a
language `or` fallback and reached Object.defineProperty. Explicit nullish
handling fixes initialization, with the failing bundle and console evidence
retained. The private unsubscribe probe also initially counted a concatenated
three-field callback record as one array element. Its original failure remains;
the corrected diagnostic requires exactly one three-field record, so a second
callback would still fail. Protected test sources and expectations were unchanged.

Style/watch controls pass 24/24 plus the unchanged 23/23 watcher cases. Snapshots
optionally include a logic hash for complete .flx style forms. Strings/comments
remain source logic, malformed sources and old snapshots stay conservative, and
additions/removals remain source changes. Under FL_FRONT_NATIVE_DEV=1, style
classes stay stable across declaration edits so existing DOM can use reloaded
CSS. Production retains its original content hashes and exact tested output.

SSE task handoff passes eight streams with two ordinary HTTP workers (34/34).
The combined runtime also passes 56/56 ordinary HTTP shutdown controls without
changing header/body timeouts or cancelling the tested running/queued requests.
The earlier SSE/API/function/C-route regression controls remain passing.

Receipts: native-browser-hmr-20261120, native-browser-hmr-v2-20261120,
native-dev-style-20261120 and native-http-idle-20261120. The coherent
native-dev-coherent-20261120 checkpoint now passes seed-to-Stage4, fresh-runtime
core regressions, relocated SDK and the same additional controls, including
23 actual Chrome functional controls. Its [evidence](evidence/native-dev-coherent-20261120.json)
retains the visual/pointer limitations. Rebuild supervision, complete metadata
and diagnostics, headless execution, deployment migration and actual application
flows remain required before switching the default development entry.

The later [Native supervisor](NATIVE-DEV-SUPERVISOR.md) implements the explicit
development entry, serialized content rebuilds, CSS publication, owned readiness,
restart/rollback and failure bookkeeping. Its receipt is separate from the
earlier component/browser checks. Complete metadata and diagnostics, headless
execution, deployment migration and actual application flows remain required;
the default entry is not switched by that supervisor milestone.

The later [source metadata receipt](NATIVE-FRONT-METADATA.md) adds actual contract
readiness, store declarations and defaults, store warnings and the full catalog
data. It separates diagnostic items from coverage status. It also tests automatic
supervisor-driven CSS/source reloads in Chrome and fixes timed notification reads.
Remaining static diagnostics, stdlib metadata, live source translation and
headless execution still prevent a complete default-entry migration claim.

The [Native static diagnostic checkpoint](NATIVE-STATIC-DIAGNOSTICS.md) now
implements 17 further catalog rules, actual SDK declaration metadata, guarded
recursive load discovery and preflight/error-state integration. It preserves
the previous app on failed rebuilds through the existing supervisor contract.
Subsequent [advisories](NATIVE-DIAGNOSTIC-ADVISORIES.md) and
[explicit source fixes/recovery](NATIVE-FIX-PLANS.md) have separate qualification.
Complete fix-site/dataflow coverage, full JSON/source-map parity, headless tests
and application acceptance still keep the default development entry unchanged.


## Verification and deployment boundary

Keep the existing tests, runners, expected values, seed and time/memory limits.
Use the shared build lock for heavy work. Preserve unrelated frontend changes.
Record pre-change failures and exact current outcomes, audit the product path
for Node execution, and distinguish scoped tests from full app acceptance.
Commercial DB readiness remains HOLD; production deployment has not been run.
