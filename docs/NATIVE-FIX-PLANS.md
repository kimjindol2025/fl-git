# Native source-edit previews

Native `--fix-dry` produces a reviewable edit plan for the five catalog families
marked machine-applicable. Source application and transaction recovery are not
implemented yet: `--fix` returns an explicit unsupported status without changes.
This checkpoint does not complete the fix workflow or switch the default
development entry.

After activating a matching Native SDK, use either entry:

```bash
scripts/fl-front-fix-native /path/to/project/web --fix-dry
scripts/fl-front-build-native /path/to/project/web /existing/output --fix-dry
```

The dedicated entry accepts `--artifacts /existing/directory` to retain the
snapshot, staged sources and parser results. Otherwise its temporary files are
removed normally. Artifacts must be outside the source web tree. Both entries
return JSON on stdout; retained artifact locations and failures go to stderr.
The SDK exports `FREELANG_NATIVE_FIX_TOOL`. There is no Node fallback.

## Rules and scope

The source set includes `.flx` and `.fl` files in `pages` and `islands`, followed
by their literal server-load dependencies within the selected project. Quoted
data, style declarations, store initializers and island bodies do not introduce
server dependencies. Neither planning nor Native parsing executes application
bodies. Source links and excluded hidden directories are reported for review.
The tool does not rewrite generated FLP output or edit FLP contracts.

| Family | Preview behavior |
| --- | --- |
| `reserved-word` | Rename `act` definitions, simple local bindings and parameters, together with their resolved references. |
| `json-parse-hyphen` | Rewrite an unshadowed `(json-parse …)` call to `(json_parse …)`. Source-defined functions and local aliases keep their spelling. |
| `let-shadow` | Rename `s`, `h` and `h!` in simple `let`/`loop` bindings and their references. Initializers retain outer and preceding binding scopes. Parameter shadow warnings are not silently machine-fixed. |
| `helper-load-order` | Rename a top-level `_foo-child.flx` when `_foo.flx` exists, and update resolved literal loads in the selected graph. Occupied destinations require review. |
| `req-key-collision` | Migrate a declared form field named `text` and its recognized literal readers to an unused `content`/`contentN` key. Preserve input type values and other fields. |

Fresh names avoid identifiers, normalized hyphen/underscore aliases and the
matching SDK inventory. Nested bindings preserve their own scopes. The concrete
reader tracks UTF-16 spans, lines and columns while preserving untouched source
bytes, comments, strings and formatting. Only explicit field/load string edits
change string literals.

Raw-body/API readers without a matching field declaration, possible JSON payload
writers and custom readers require an explicit data-contract migration. Namespace
renames, unsupported binding forms and references in deferred store initializers
also remain review cases. External consumers, dynamic imports and references
outside the selected graph are not certified by this preview.

The older builder references `web/fl-front-fix.js`, but that file is absent from
the checkout and available history. Its isolated `--fix-dry` fails before edits.
The catalog supplies the stated rule families; parity with an unavailable legacy
fix implementation is not claimed.

## Receipt and validation

The `fl-fix/1` report has `dry: true`, `filesApplied: 0`, `filesPlanned`, `safe`,
blockers and per-file results. For compatibility with the existing caller's
minimal report shape, `filesChanged` counts proposed files and `results.applied`
counts proposed rule edits; neither means that dry-run source changes occurred.
Results include destinations, original/staged SHA-256 values and exact edit
spans with before/after text. Review blockers make `safe` false.

FreeLang owns the rules and edit plan. Bash supplies literal file arguments,
enumeration, temporary directories and Native parser processes. Finalization
checks source and staged hashes, source inventory, parser exit status and the
Native AST schema/status. It preserves parser diagnostics with the original
source filename and writes `verified-plan.json` in the private run directory.
Validation covers changed files' Native parsing. It does not certify a full
application build, linking, runtime behavior or transaction isolation.

The reader can preserve a nested block-comment span, but the current Native
parser rejects `#| … |#` input. A changed file containing it remains a parser
failure; the tool does not strip unsupported syntax to manufacture a pass.

Evidence: [native-fix-plan-20261120](evidence/native-fix-plan-20261120.json),
paths/owners: <가림>. The receipt distinguishes source previews from private
diagnostic reconstruction used to run their results. Such reconstruction is
not a product `--fix` implementation or a rollback/transaction qualification.
It records preserved failures and resource limits separately from successful
controls. Compiler/runtime inputs and protected tests remain unchanged.

The preview's browser verification also exposed a development watcher framing
defect: a timed line read could consume its delimiter while returning failure,
then concatenate two valid `CHANGE` records. The supervisor now reads one
character with `read -N 1`, keeping LF as data even when the read is interrupted.
It still requires an LF-terminated `CHANGE` record, emits no event for unfinished
prefixes and rejects malformed or CR-terminated input. The polling and debounce intervals, Native
notification producer and readiness/stop bounds are unchanged. Deterministic
completed-read interruption and original fragmented-input controls exercise
both the failing old reader and the corrected reader.

Source application, recovery, complete fix-site/dataflow qualification, advanced
module behavior and the wider Native migration remain open. Headless tests,
default development/deployment migration, actual Git workflows and the latest
full DB safety/recovery/concurrency/load/operations gates are still required.
The full goal remains active, commercial readiness is HOLD, and production
deployment has not run.
