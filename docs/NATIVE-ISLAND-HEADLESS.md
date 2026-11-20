# Native Island headless entry

The explicit `scripts/fl-island-run-native` entry lowers extracted Island source
in FreeLang and executes the generated adapter through the SDK's real JavaScript
engine. It is an experimental standalone path. Default development, `/~ai/test`
and the build's `tests/*.test.flx` gate still need integration and qualification;
their existing `NOT_RUN` metadata remains accurate.

After activating a matching SDK with its engine pair:

```bash
"$FREELANG_NATIVE_HEADLESS_RUNNER" page.flx --island=Counter \
  '--click=#button' '--dispatch=key:a=b:c' '--store={"key":false}'
```

The CLI preserves complete option values and literal filesystem paths. Its
FreeLang option parser keeps the first occurrence and treats malformed CLI
store JSON as `{}`, matching the existing CLI's fallback. It preserves `=` in
option values; the old JavaScript CLI's `split('=')[1]` truncated those values.
The legacy programmatic `run(file, options)` is the value-comparison reference.
JSON `ok: false` exits 1, missing configuration/input exits 2, and engine or
compiler failures propagate. Each generation retains its 15-second bound,
engine execution its 3-second bound, and Native processes the 2GiB VM cap.
Qualification runs hold the existing shared heavy-build lock.

`tools/front-headless.fl` implements extraction and DSL lowering.
`web/runtime/headless-language.fl` implements the virtual document, VNode
prototype/methods/accessors, atoms, store/watchers, mock fetch events, mounting,
click/dispatch sequencing and result serialization. `tools/fl-island-headless.fl`
provides the Native command. The Bash entry only supervises literal processes
and files; no product JavaScript or Python runtime was added.

The adapter preserves details of the existing headless model: fresh context per
selected Island, index-zero IDs for each independently selected block, removal
of initial store events, raw text serialization, quote-only attribute escaping,
one-level child flattening, `value` separate from the attribute, selectors' actual
id/tag/class behavior, mock fetch callbacks absent from the returned snapshot,
click before dispatch, and `html_after: null` for dispatch-only execution.
Its DSL lowering uses real JavaScript truthiness, strict equality, coercing
arithmetic, remainder, property access and method receivers. Hoisted top-level
atoms survive render calls. Browser string/array helpers are also provided in
the context: join, concat, contains, blank, slice, trim, split and count. The
legacy headless context omitted them, which made the actual app fail its initial
render at `str_trim`. These implementations are compared with the existing
browser runtime, separately from the legacy headless result comparison.
Ordinary functions preserve omitted `undefined` arguments through compiler-only
raw function grammar. Existing ordinary browser
function defaults and user private-operation guards remain unchanged.

Compatibility is not fully qualified. The first comparison cohort matched
22/28 complete typed results. It exposed a lowering error for nested vectors
whose first item is callable; that lowering was corrected. The retained legacy
failures also include its `:nil` keyword parser failure and anonymous function
application syntax failure. A second cohort exercises the same intended
collection, missing-argument and child-flattening behaviors using forms the
reference can compile. It matches 27/28 complete typed results. The remaining
case demonstrates an existing direct `(nil? value)` emission bug: the reference
renders a function's source text where Native produces a boolean. These are
recorded differences, not an all-pass parity claim. No protected fixture,
assertion, expected output, runner or bound was edited.

Actual source testing exposed a 3-second extraction timeout on the roughly
96KB app Island. The extractor now indexes one character array rather than
repeatedly locating string slices. The retained failure and repaired execution
use the same bound. Quoted text, comments and triple-string delimiters retain
their scan rules. Parse failures now forward the compiler JSON diagnostic rather
than losing its stdout file. With the browser helpers, the actual working-tree
app completes its headless initial render; this is not browser/Git flow
acceptance. Its measured cold pipeline took about 15.9 seconds in the final SDK cohort while preserving
all per-phase bounds. The 3-second API response requirement needs compilation
ahead of requests; the standalone cold path is not wired into the service.

Broader DSL behavior (including an invalid unselected Island), function
introspection, engine-specific error messages, Native API and test-file
gate wiring, actual application and Git UI flows, default development/deployment
migration, and the latest whole DB safety/operations gate remain in the active
objective. Commercial status is HOLD; no production deployment has run.
