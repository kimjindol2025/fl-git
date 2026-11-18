# Native default page checker

scripts/fl-check now delegates to scripts/fl-check-native. It runs the existing
tools/fl-check.fl program as a verified Native executable. It does not build a
compiler or tool, search for a binary, or fall back to Node.

Set FREELANG_NATIVE_FLCHECK to the matching prebuilt page checker and provide
its approved library environment.

~~~sh
export FREELANG_NATIVE_FLCHECK=<verified-native-page-checker>
./scripts/fl-check
./scripts/fl-check web/pages/products.flp
~~~

The first argument remains the target, defaulting to web/pages in the
repository containing the entry. Further arguments retain their prior ignored
behavior. The caller's working directory is preserved; relative targets resolve
there. FL_CHECK_ROOT and FL_CHECK_TARGET are passed to the tool, and an existing
FL_CHECK_SOURCE_TARGET override is preserved. Paths containing whitespace,
apostrophes, quotes, Unicode or newlines are passed as data. Symlinked entries
resolve back to the repository. The former FREELANG_AFJ_RUNNER override is not
used by the default entry.

This is the page/FLX/FLP structure and boundary checker. It is separate from the
FX compiler's --check language command. A diagnostic's fix field contains advice;
there is no automatic file-rewriting --fix option in this program.

## Process and diagnostic status

A successful process must return exactly one complete FL_CHECK_EXIT=0 or
FL_CHECK_EXIT=1 line. Missing, duplicate, conflicting or malformed markers block
the command with exit 2. Missing, non-executable or directory-valued tool
configuration also blocks with exit 2. A valid marker maps to the corresponding
0 or 1 exit, and protocol marker lines are removed from stdout.

A failed process keeps its actual exit status even if it printed a checker
marker. Previously, exit 9 plus FL_CHECK_EXIT=1 became exit 1, and termination
status 143 plus that marker also became 1. Both are now preserved. Non-protocol
stdout and stderr remain available for ordinary process failures. This status
handling changes the launcher only; checker rules, diagnostic fields and
protected fixtures remain unchanged.

## Evidence

Paths and owners: <가림>. Receipt: native-check-default.c9k7107o.

| Scope | Result |
|---|---|
| Before change, declared launcher contracts | 18/24; 6 violations reproduced |
| Updated explicit Native entry | 24/24 launcher contracts |
| Updated default entry | 24/24 launcher contracts |
| Existing protected fixtures | 32/32 expected exits and diagnostic classes, through both updated entries |
| Default pages, explicit pages directory and ignored extra argument | 3/3; exit 0 |
| Reference comparison for these 35 real checker inputs | Exit, ordered full JSON diagnostics and normalized stdout bytes all equal |
| Additional real checker inputs | 3/3, same status and stdout bytes as reference |
| Product traces | 121 files; Node executions 0, connect calls 0 |
| Bash syntax and diff whitespace | PASS |
| ShellCheck | NOT_RUN; unavailable; no installation |
| New JS / JS line growth | 0 / 0 |

Launcher contracts use private protocol sentinels to exercise missing tools,
status propagation, malformed markers, stderr, literal argument transport and
symlink behavior. They are distinct from actual checker fixture executions.

The three additional real inputs exercise a relative file with quotes, an
apostrophe, Korean, an emoji and a newline in its filename from outside the
repository; quoted dangerous strings in otherwise valid props; and all seven
excluded source directories. The invalid DOM input remains rejected, quoted
content remains data, and excluded files remain excluded. Original fixture
bytes and expected outcomes were preserved.

The comparison oracle interprets the exact same tools/fl-check.fl with the
verified FX repository's bootstrap.js. It uses direct Node CLI memory flags,
then applies the established marker-to-exit and stdout-filtering contract.
No Node compiler/build stage was used. These 38 reference executions are
diagnostic-only and are separated from the Node-free product traces.

An initial private oracle attempt put --no-expose-wasm in NODE_OPTIONS, where
the installed Node rejected it. That batch recorded 35 option failures before
any checker execution; the harness should have stopped after the first setup
failure. Those receipts are retained and excluded from parity claims. The
corrected CLI-argument oracle stops on the first unexpected process/protocol
failure or diagnostic difference; all 38 valid comparisons passed. No product
rule, fixture expectation, public timeout or protected regression runner was
changed to obtain the result.

- FX HEAD: 48988a3756f2c0520be7121021c54083aaafb62a.
- Compiler SHA-256:
  e966e6342053c48fdd3af001a029e0d4a4923832f5122f0f300696ce8d1b2c94.
- Runtime archive SHA-256:
  09be8c24c8c2bff543d6d9a7c5942595529473759c60b52be18ff061951624f1.
- Reused, source-matched Native checker SHA-256:
  c1ba0532c7a3e722f6cc808a70e93fd8dca27519f42493c821bf96826f6eb515.
- Seed unchanged:
  0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

This result establishes the recorded page-checker and launcher contracts, not
complete checking for every possible language input. No compiler/runtime
source changed. No fresh fixed-point rebuild, full web/browser flow or DB95
suite was required or run for this launcher change. Other concurrent source
and test work is preserved outside this commit. SDK packaging, dev/HMR, common
tools/deployment, all-app acceptance and DB commercial qualification remain
open. The overall goal remains active; production deployment is not approved.
