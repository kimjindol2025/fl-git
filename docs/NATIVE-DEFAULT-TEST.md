# Native default test command

scripts/fl-test now delegates to scripts/fl-test-native. It discovers the same
*.test.fl files, checks each source, compiles and links it with the configured
Native SDK, then executes it in the caller's working directory. It does not
build an SDK, search for a Native compiler, or fall back to Node.

## Configuration

Set FREELANG_NATIVE_COMPILER, FREELANG_NATIVE_RUNTIME_ARCHIVE and
FREELANG_NATIVE_RUNTIME_INCLUDE to the same verified build. Supply its approved
library environment. CC optionally selects a single C compiler executable
(default gcc). FREELANG_TEST_ROOT optionally overrides the repository tests
directory. FREELANG_NATIVE_TEST_ARTIFACTS retains artifacts in an existing
directory; otherwise an owned temporary directory is cleaned on exit.

~~~sh
./scripts/fl-test
~~~

The former FREELANG_AFJ_RUNNER interpreter override is not used by this entry.
The npm test alias still requires npm itself; the direct command above is the
Node-free path. Public test execution adds no timeout; existing caller policy
is preserved.

## Truthful results

A summary must have the reserved form Test Results: passed/total passed.
Every line with the Test Results: prefix must be well formed, and every
reported decimal passed count must equal its total. Counts are compared as
strings after removing leading zeroes, avoiding machine integer overflow or
floating-point rounding. A valid 0/0 summary remains accepted. Missing,
malformed, partial or inconsistent summaries, FAILED output and program
failures produce failure, exit 1.

The old format-only predicate accepted 0/1, a partial summary followed by a
passing summary, and unequal large decimal counts. The new predicate rejects
them. The legacy Node launcher used the same predicate; this correction does
not preserve that false-green behavior. Synthetic count strings exercise this
protocol; they are not counted as executed unit cases.

Source generation requires successful compiler status and fresh nonempty C.
A legal filename containing [CGC-ERR] no longer turns a successful compilation
into a false failure. Compiler log text is not a machine status. The verified
compiler rejects invalid IR/codegen with a nonzero status; missing output and
C compilation/link failures also stop the entry.

Missing tools, inaccessible test roots, failed discovery and unusable artifact
or temporary directories report BLOCKED, exit 2. No matching files reports
NO_TESTS, exit 0. Successful suites report PASS, exit 0. Discovery remains
NUL-safe for filenames containing spaces, apostrophes, Unicode and newlines.
Symlink resolution preserves entry location and the caller's working directory.

## Verification

Paths/owners: <가림>. Receipt: native-test-default.woauxvlk.

| Scope | Result |
|---|---|
| Archived before implementation, real configured SDK | 26 interface cases; 7 contract violations reproduced |
| Updated explicit Native entry | 26/26 |
| Updated default entry | 26/26 |
| Seven unchanged tracked unit suites through both entries | 71/71 each, exit 0 |
| Actual repository root with no test-root override | 8 suites, 80/80, exit 0; includes an additional local untracked 9-case suite |
| Automatic temporary artifact lifecycle | Actual Native success; temporary directory empty after exit |
| Exact summary protocol, lightweight independent cases | 14/14 |
| Configuration guards, no compiler invocation | 5/5 |
| Collected product traces | 92 trace files; Node executions 0, connect calls 0 |
| Bash syntax / diff whitespace | PASS |
| ShellCheck | NOT_RUN; unavailable; no installation |

The additional local Git-status source/test and concurrent web changes were
read only and not included in this commit. The committed unit baseline remains
the original seven files/71 cases. Their bytes, assertions and expected values
were unchanged. No protected regression runner was edited. These product
launcher changes implement the requested Node removal and failure handling.

The first lock-acquisition attempts expired before any product test ran.
The job then retained its queue using the prescribed flock -o; individual
probe bounds and the 2-GiB cap did not change. A private read-only artifact
fixture initially collided with its own journal directory. That harness
failure and raw output are retained; the owned output target was separated
without changing expectations or permission conditions. The 21 completed
case receipts were reused rather than rerun.

- FX source: 48988a3756f2c0520be7121021c54083aaafb62a.
- Compiler ELF SHA-256:
  e966e6342053c48fdd3af001a029e0d4a4923832f5122f0f300696ce8d1b2c94.
- Runtime archive SHA-256:
  09be8c24c8c2bff543d6d9a7c5942595529473759c60b52be18ff061951624f1.
- Seed unchanged:
  0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

No fresh Node execution or Node build stage was used for this milestone.
Legacy shell contracts were inspected directly; prior diagnostic comparisons
are retained in NATIVE-TEST-RUNTIME.md, including the known stricter Native
pure-effect check. This receipt does not establish equivalence for every
language/type diagnostic or all possible test programs. New JS files and JS
line growth are zero. SDK packaging, default page-check/dev/deploy migration,
complete app flows and DB commercial qualification remain open; no customer
DB or production deployment was accessed.
