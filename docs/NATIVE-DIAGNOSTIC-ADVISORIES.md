# Native diagnostic advisories

The Native web builder reports CSS class mistakes, empty-vector catches,
ambiguous short uppercase definitions and duplicate function names. It also
suggests nearby known function names. These correspond to the older builder's
console guidance; they remain separate from catalog build issues. The catalog
still has 19 entries, with the island-test rule awaiting the headless runner.

`tools/front-advisories.fl` traverses the same nonexecuting AST as the static
checker. It recognizes local bindings and parameters, string or keyword class
keys in actual `h`/`h!` calls, generated underscore style aliases, and existing
global definitions. Strings, comments and quoted declarations do not become
calls. Duplicate-name guidance excludes the existing render entry names and
files declaring a namespace. That exclusion does not qualify namespace runtime
semantics. Module definitions participate through the existing dependency graph.

Name suggestions use two-row UTF-16 Levenshtein distance and stable candidate
order. CSS suggestions retain a distance limit of two and at most two results.
Function suggestions normalize hyphens to underscores, use the existing
`max(2, floor(length * 0.45))` threshold, and return at most three results.
The candidate inventory comes from the matching Native SDK and frontend names;
it is not a claim that the old Node bootstrap has the same inventory.

## Output and development state

Build metadata has `advisories` and `suggestions` arrays alongside `issues` and
catalog `diagnostics`. Each advisory includes its own code, severity, filename,
line and message. CSS/name guidance also includes the relevant names and
candidates/files. `summary.advisories` counts only advisory rows. Build warning
and error counts retain their previous meaning. `diagnosticStatus` enumerates
the four advisory checks and separately records function-suggestion support.

The host prints readable preflight diagnostics on stderr and preserves the JSON
diagnostic log. `fl-front-build --metadata <bundle>` returns JSON, while
`fl-front-build --report <bundle>` prints a readable report without writing
files. `--help` describes both modes. Neither inspection runs application bodies.
This is a Native output format, not byte-for-byte ANSI console compatibility.

Development success and failure states carry the latest attempt's guidance.
Failed attempts still preserve the active generation and application bundle;
they do not retain stale suggestions. `/~ai` exposes both arrays. After a source
repair and successful rebuild, removed advisories disappear from state and API.

Advisories do not independently fail preflight. An undefined CSS variable can
still fail the later C compiler, and an undefined function can still fail when
called. The diagnostic layer does not replace missing values to force a build
or runtime success.

## Qualification and remaining work

Receipt: `native-advisories-20261120`; paths/owners: <가림>. The
[evidence](evidence/native-advisories-20261120.json) records the verified source,
SDK, controls and retained failures. Its private Node oracle runs unchanged
reference functions solely for comparison, outside the Native product path.
AST corrections are recorded explicitly rather than presented as identical
legacy output. Protected tests, expectations, runners, bounds and seed remain
unchanged. Existing user changes are preserved.

The preserved actual frontend input builds with the existing Native command:
nine routes, six helpers, one dependency, no errors, the existing uppercase
constant warning and no advisories. Its application ELF matches the accepted
prior build. This does not qualify actual application/Git browser workflows.
An additional aggregate 40-second diagnostic control times out during sequential
browser generation with both the candidate and prior SDK. Both failed controls
remain recorded. The existing full build uses its established command without
that added aggregate deadline; its success does not turn either timed control
into a pass.

The previous builder refers to `web/fl-front-fix.js`, but that module is absent
from this checkout and its available Git history. An isolated invocation of
the original `--fix-dry` confirms `MODULE_NOT_FOUND` before any file changes.
The five catalog rules marked machine-applicable therefore remain the stated
contract for the Native fix implementation; parity with an unavailable
legacy implementation cannot be asserted. That advisory checkpoint did not
include a fix implementation. Subsequent [Native fix work](NATIVE-FIX-PLANS.md)
adds read-only edit plans, explicit application and interruption recovery within
the documented source scope.

Headless/island tests, complete fix-site/dataflow qualification, complete JSON fields and source maps,
advanced modules, default development/deployment migration, actual application
and Git workflows, and the latest full DB safety/operations gates remain open.
Commercial readiness is HOLD. Production deployment has not run; the overall
goal stays active.
