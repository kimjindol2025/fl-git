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
frames, and leaves final descriptor ownership to HTTP. The matching fresh SDK
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


## Verification and deployment boundary

Keep the existing tests, runners, expected values, seed and time/memory limits.
Use the shared build lock for heavy work. Preserve unrelated frontend changes.
Record pre-change failures and exact current outcomes, audit the product path
for Node execution, and distinguish scoped tests from full app acceptance.
Commercial DB readiness remains HOLD; production deployment has not been run.
