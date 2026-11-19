# Native static diagnostics

The web build now checks the original catalog's 17 remaining static rules before
emitting application or browser source. Together with the earlier store-key
check, 18 catalog rules have Native implementations. The island-test rule still
requires the headless runner. This is not complete diagnostic/default-entry
migration: fix workflows and remaining metadata/source mapping work are still
open. The subsequent [Native advisory work](NATIVE-DIAGNOSTIC-ADVISORIES.md)
adds the console guidance separately from these catalog issues.

## Detection and build behavior

`tools/front-static.fl` walks the compiler's nonexecuting AST. It checks reserved
identifiers, server calls in islands, helper/builtin arity, undefined functions,
parameter/let shadowing, JSON spelling, uppercase identifiers, request keys,
helper ordering, route overlap, DB-derived row operations, nullable consumption
and string concatenation inside loops. A separate source reader recognizes
commented definitions while preserving string, triple-string and block-comment
boundaries. The existing private-HTTP configuration warning remains available.

The checker tracks local bindings, function parameters and DB-derived values by
scope. Literal strings and comments are not calls. A local function can shadow
a builtin or server helper without inheriting that helper's diagnostic. Calls
inside arguments are checked too. Diagnostics retain the existing catalog,
severity, source filename/line and message fields. Internal store deduplication
keys are removed from published issues.

`scripts/fl-front-build-native` parses the shipped server helper and static
module dependencies, then calls the tool's `--preflight <bundle>` operation.
It writes structured issues to the bundle's build state and diagnostic log.
Errors return nonzero before server/browser emission. Warnings remain warnings
and do not fail an otherwise successful build. The development control tool
retains preflight error codes and failed dependency parser positions while
preserving the previously active generation and bundle.

## Function inventory and imports

Known names come from the configured matching SDK's complete v11 operation and
Native symbol declarations, the actual parsed server helper, page/helper/module
definitions, styles and language forms. Every reported v11 operation is checked
with the running Native registry. The current SDK declares 201 distinct v11
operation names and 1109 Native ABI names; these are different inventories.
The public-name summary also includes the actual frontend declarations. It is
not the old Node bootstrap's function count. Missing or mismatched declarations
produce explicit failed-load metadata; no successful count is invented.

Literal loads are resolved within the selected project using the existing path
guard. Native parsing discovers further loads until all dependency ASTs exist;
canonical filenames prevent repeated discovery. Source/AST arguments remain
NUL-delimited, including paths containing apostrophes, Unicode and LF. Neither
application bodies nor store initializer expressions are executed. Loads inside
quoted data, style/store declarations and island bodies do not become server
dependencies. The source graph control terminates on a cycle; it does not claim
that compiling/running cyclic modules is supported. Namespace/module semantics
beyond the tested imports remain part of the framework qualification work.

The SDK provenance now covers both FreeLang framework runtime helper files.
The compiler, runtime archive and service/watcher implementations are unchanged.

## Evidence

Receipt: `native-static-diagnostics-20261120`; paths/owners: <가림>. The
[evidence](evidence/native-static-diagnostics-20261120.json) records exact hashes,
source pairs, corrected failures and scope.

Native controls pass 43/43. A private reference runs the unchanged legacy check
functions on 37 of those inputs with the same supplied SDK known-name set.
Twenty-nine results match exactly. Eight differences are retained: the legacy
reader flags literal strings and shadowed local functions/DB variables, misses
the snake-case server alias, or skips a nested builtin call. These are explicit
AST behavior corrections, not a claim of blanket legacy-output parity. Real
HTTP output and a Chrome-mounted local helper also validate the relevant name
shadowing cases. The private Node oracle is outside the Native product path.

Real build/HTTP controls pass 24/24, including recursive quoted/LF imports,
preflight errors before emission, structured failure state, warning-only builds
and unevaluated initializer loads. Boundary controls pass 6/6 for library-load
metadata, full declaration counts/hash, dependency parser locations, retained
project boundary and finite dependency discovery.

The actual frontend input includes the four preserved working-tree changes in
a separate source copy. Its Native build succeeds with nine routes, six helpers
and one dependency: zero errors and the existing uppercase constant warning.
This is a full input build, not a runtime/Git-flow acceptance claim. Synthetic
application/browser controls separately cover automatic CSS/source HMR and
restored state. Screenshots and physical pointer acceptance remain unverified.

Early Native tests caught multi-array `concat` calls dropping later issue groups;
the code now folds binary concatenations explicitly. Original failed results
remain. Private adapters also required the helper-only oracle signature, the
existing stylesheet prefix in exact HTTP output, a required island option vector,
and a bounded retry for document IDs invalidated by navigation. Assertions,
original failures and corrected scope are retained separately. Protected tests,
expectations, runners, time/memory bounds, seed and foreign files are unchanged.

CSS/dangerous-pattern/name-collision advisories and function suggestions now
have a [separate implementation and receipt](NATIVE-DIAGNOSTIC-ADVISORIES.md).
Headless/island tests, automatic fixes, complete JSON-field/source-map parity,
the default development entry and deployment migration remain open. Actual
application/Git workflows and the latest whole DB safety/qualification gates are
also required. The goal stays active, commercial readiness is HOLD, and no
production deployment has run.
