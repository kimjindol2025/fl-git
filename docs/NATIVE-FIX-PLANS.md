# Native source fixes and recovery

Native `--fix-dry` produces a reviewable edit plan for the five catalog families
marked machine-applicable. Explicit `--fix` applies a verified plan through a
Native file host, retaining backups and an interruption recovery journal.
The default remains a read-only preview. This does not switch the development
entry or qualify every possible source/dataflow case.

After activating a matching Native SDK, use either entry:

```bash
scripts/fl-front-fix-native /path/to/project/web --fix-dry
scripts/fl-front-build-native /path/to/project/web /existing/output --fix-dry
scripts/fl-front-fix-native /path/to/project/web --fix --artifacts /existing/output
scripts/fl-front-build-native /path/to/project/web /existing/output --fix
scripts/fl-front-fix-native --recover /retained/private-run
```

The dedicated entry accepts `--artifacts /existing/directory` to retain the
snapshot, staged sources and parser results. Unretained preview files are removed
normally; application always retains its private run. Artifacts must be outside
the source web tree. The dedicated entry returns JSON on stdout; retained run
locations and failures go to stderr. Builder `--fix-dry` returns the preview;
builder `--fix` prints the application report and then ordinary build output.
If that subsequent build fails, the source transaction remains committed and
the build still fails. Source application does not certify a successful build.
The SDK exports `FREELANG_NATIVE_FIX_TOOL` and `FREELANG_NATIVE_FIX_HOST`.
There is no Node fallback.

## Rules and scope

The source set includes `.flx` and `.fl` files in `pages` and `islands`, followed
by their literal server-load dependencies within the selected project. Quoted
data, style declarations, store initializers and island bodies do not introduce
server dependencies. Neither planning nor Native parsing executes application
bodies. Source links and excluded hidden directories are reported for review.
The tool does not rewrite generated FLP output or edit FLP contracts.

| Family | Planned edit |
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
outside the selected graph are not certified by the tool.

The older builder references `web/fl-front-fix.js`, but that file is absent from
the checkout and available history. Its isolated `--fix-dry` fails before edits.
The catalog supplies the stated rule families; parity with an unavailable legacy
fix implementation is not claimed.

## Receipt and validation

The `fl-fix/1` report has `dry: true`, `filesApplied: 0`, `filesPlanned`, `safe`,
blockers and per-file results, with `application: NOT_REQUESTED`. For compatibility with the existing caller's
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
application build, linking or runtime behavior.

## Applying and recovering files

FreeLang revalidates a safe, finalized plan and emits literal NUL-delimited file
operations. The C host supplies filesystem IO only. It takes a cooperative lock
on the selected project, verifies all source hashes and the initial inventory,
rejects source symlinks/hardlinks and occupied move destinations, and opens path
components without following symlinks. An unfinished `.fl-fix.active` marker
prevents another application until recovery or manual review.

Before publication, the host persists the journal, original byte backups and
replacement inode identities, prepares every replacement and preserves file
mode, owner/group and accessible extended attributes. An attribute-copy failure
aborts application. Journal files, replacement files and their directories are
synchronized. Each replacement uses Linux `renameat2(RENAME_EXCHANGE)`; file
renames publish without replacing an occupied destination and retain the old
name's inode until commit. These operations are individually atomic. Readers
can observe an intermediate combination of files during a multi-file change.

The host checks source and parent identities again while publishing and checks
unchanged dependencies, published hashes and the resulting inventory before
recording commit. On failure or INT/TERM it restores only while the visible
replacement still has the transaction's inode and bytes. A replacement by
another writer captured during the exchange is restored too. A later conflicting
edit is preserved, with the old inode and backups retained for review. These
checks exercise specific concurrent-write boundaries; they do not provide
isolation from arbitrary writers that ignore locks.

`--recover` uses the same private run after an interrupted process. It restores
an uncommitted transaction or finishes cleanup after a recorded commit; it does
not undo a completed commit. Recovery never forces an overwrite. Corrupt or
incomplete ownership records and unresolved conflicts remain nonzero failures
requiring review. Backups contain original bytes, and the manifest records
original file identities and mode/owner/group. Keep the run until the result
has been reviewed. Disk power-loss behavior has not been qualified.

| Status | Exit and applied count |
| --- | --- |
| `applied` | Success; `filesApplied`/`filesChanged` count committed files in this transaction. |
| `rolled-back` | Application fails; successful explicit recovery exits zero. Both counts are zero. External edits can remain. |
| `rejected` | Application fails before publication, with zero applied files. A rejected recovery has unknown counts. |
| `rollback-incomplete` | Failure; applied/changed counts are `null`, and recovery or review is required. |
| `applied-cleanup-incomplete` | Failure after recorded commit; committed counts are retained and cleanup recovery is required. |

The final `transaction` object records intent, status, success, signal, observed
publication/restoration counts, conflicts and `needsRecovery`. Recovery reports
the original transaction's committed count, not a new application. Per-file
`results`, including the legacy field name `results.applied`, always describe
the proposed plan; use the top-level status/counts for the actual outcome.

The reader can preserve a nested block-comment span, but the current Native
parser rejects `#| … |#` input. A changed file containing it remains a parser
failure; the tool does not strip unsupported syntax to manufacture a pass.

Earlier preview evidence: [native-fix-plan-20261120](evidence/native-fix-plan-20261120.json),
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

Product application/recovery qualification is recorded separately in
[native-fix-apply-20261120](evidence/native-fix-apply-20261120.json). The older
preview receipt's private source reconstruction remains distinct from the new
public `--fix` checks. Complete fix-site/dataflow qualification, advanced module
behavior and the wider Native migration remain open. Headless tests,
default development/deployment migration, actual Git workflows and the latest
full DB safety/recovery/concurrency/load/operations gates are still required.
The full goal remains active, commercial readiness is HOLD, and production
deployment has not run.
