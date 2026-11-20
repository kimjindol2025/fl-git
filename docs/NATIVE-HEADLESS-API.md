# Prepared Native Island execution

The explicit `scripts/fl-dev-native` and `scripts/fl-front-build-native` entries
use the matching SDK's headless tool and real JavaScript engine. The default
`scripts/fl-dev` remains on its previous path. This checkpoint does not establish
complete DSL compatibility, application/Git acceptance or commercial readiness.

Activate a matching SDK with both engine libraries, then run the repository's
Native entry. The inspection service exposes, for example:

```text
GET /~ai/test?file=islands/counter.flx&island=counter&click=button
```

`file` is relative to the selected project web directory. `island`, `click`,
`dispatch` and JSON `store` preserve their complete query values. Invalid store
JSON produces an error result. Missing `file` retains the parameter-help result.
As in the existing API, result objects use HTTP 200 with the service's CORS
headers; `ok: false` describes an execution, preparation or validation failure.

Compilation happens during a build. The preparation command copies each source
to an owned artifact directory, checks source hashes before and after the copy
and compilation, and records compiled modules or compilation errors. Sources
with `.flx` and `.fl` suffixes are discovered recursively, with canonical
containment and visited-directory checks. Outside symlinks are excluded. Each
Island name group is compiled independently, so a malformed unselected Island
does not block a valid selected group. Default selection still reports the
whole-source error. Unknown selection returns the existing empty-results shape.
Missing matching engine/tool inputs record preparation as `NOT_RUN` with its
configuration error. Builds with no headless tests can still generate ordinary
browser/server bundles; declared headless tests fail explicitly until their
required preparation is available.

The process/file host publishes the complete immutable index by an atomic
rename. Publication happens before ordinary page diagnostics, so valid prepared
Islands remain inspectable when another build check fails. Initial, missing and
newly added sources return `E_HEADLESS_NOT_READY` until their preparation is
published. Source edits immediately invalidate old preparation. An unsuccessful
preparation preserves the previous index, whose changed sources still fail the
staleness check.

Every request checks the project-relative path and canonical containment before
reading or hashing its selected source. Absolute paths, traversal, NUL bytes,
outside symlinks and non-files are rejected. The index must identify that project;
selected manifest paths must stay in the artifact tree and retain their digest.
The manifest must identify the selected source. Source, compiler, headless tool,
runtime support, engine and both engine-library hashes must still match. A
selected generated module must remain within its artifact directory and retain
its digest. These checks add no exception to the existing Native file policy.

HTTP requests execute the prepared module with a 2000ms engine deadline inside
the unchanged 3-second request budget. They do not launch a compiler. Preparation
retains the 15-second parser and browser-generation bounds. All Native processes
retain the 2GiB address-space cap and the existing Native 64MiB file/output bound.
Build and qualification work uses the shared heavy-build lock.

The native process boundary accepts an absolute executable and a literal string
argv vector, without shell parsing. It captures stdout/stderr separately, gives
stdin EOF, closes unrelated inherited descriptors, and reports exit, signal,
spawn, IO, timeout and overflow outcomes. It cancels only its own process group
and reaps its direct child on normal completion, timeout and output failure.
Abrupt owner death/cancellation is not qualified by this checkpoint.

The build also reads direct `tests/*.test.flx` declarations as AST data. It never
evaluates test files as application code. `deftest` options select a source,
Island, click, dispatch and initial store. Expectations cover `html-contains`,
`html-not-contains` and typed `store-key`/`store-val`; false, zero, nil and missing
keys stay distinct. Failed expectations, parse errors and nonliteral option
expressions produce `island-test` errors and prevent application generation.
Individual results and diagnostics are retained in `headless/island-tests.json`
and incorporated into build/development state. Test-only edits trigger a rebuild;
a failing rebuild retains the last ready application.

Test status is `PASS`, `FAIL`, `NOT_REQUIRED` when no test file exists, `NO_CASES`
when files contain no declarations, and `NOT_RUN` when a metadata query has no
execution receipt. Zero declared cases are never reported as executed assertions.

The original headless comparison remains PARTIAL (27/28 complete typed results),
including the recorded direct `nil?` reference-emitter difference. The standalone
cold CLI also selects raw blocks and row labels before compilation. Its behavior
with an invalid unselected block or an empty selection has separate verification.
Full DSL behavior, function
introspection, engine error wording, default development/deployment migration,
real browser/Git flows and the latest whole DB safety/operations gate remain in
the active objective. Commercial status is HOLD; production deployment has not run.
