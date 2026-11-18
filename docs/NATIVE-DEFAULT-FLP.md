# Native default FLP compiler

scripts/flp-compile now runs the existing two page-check gates, then delegates
to scripts/flp-compile-native. It requires the verified Native page checker and
the Native executable compiled from the unchanged tools/flp-compile.fl source.

~~~sh
export FREELANG_NATIVE_FLCHECK=<verified-native-page-checker>
export FREELANG_NATIVE_FLP_TOOL=<matching-native-flp-generator>
./scripts/flp-compile
./scripts/flp-compile web/pages
~~~

Provide the matching SDK's approved library environment. There is no binary
search, SDK build, Node fallback, download or installation in these entries.
The former FREELANG_AFJ_RUNNER override is unused. The first argument remains
the source directory; extra arguments remain ignored.

The default entry checks the source directory and then products/[id].flp,
in that order. Any failure stops before generation and keeps its exit code.
The direct scripts/flp-compile-native entry retains its role as the generator
entry without adding these page-check dependencies. The full Native bundle
builder continues to call the generator inside its existing private bundle;
its interface is unchanged.

## Generation and publication

The current FreeLang compiler supports the existing shop contracts and emits
four server files and four matching island files: products, products/[id],
cart and checkout. Its existing requirement for three PRODUCT rows and other
contract conditions remains unchanged. This migration does not turn it into a
general FLP compiler or remove unsupported-contract errors.

The host launcher copies the five generator inputs into a fresh owned staging
directory under TMPDIR (default the host temporary directory). The language
generator writes there. A zero process status must include exactly one
FLP_COMPILE_EXIT=0 or FLP_COMPILE_EXIT=1 line. Missing, duplicated, conflicting
or malformed markers block with exit 2. Marker 1 returns failure 1. Actual
process failures keep their status, including 9 and termination status 143.

Success requires all eight freshly generated outputs to be regular, nonempty
files. Existing live files cannot satisfy this check. The launcher verifies
that the five contracts and output destinations did not change during
generation, prepares replacements in each destination directory and records
rollback intent before each rename. Existing file modes are retained;
new files use their generated mode. The obsolete _flp-generated.flx is removed
only after successful generation/publication. Generation rejection preserves
the last existing combined file as well as the eight destinations.

Missing/inaccessible configuration, unwritable or non-regular destinations,
and symbolic output files or output parent directories block with exit 2.
Symbolic source-directory aliases are resolved physically; newline-ending
directory names are preserved. Native generator failure, incomplete output
or an observed concurrent edit produces failure without publication.
A caught publication failure attempts rollback, preserving the actual failure
status; a rollback failure reports retained recovery snapshots explicitly.
Successful completion is printed only after cleanup.

The host shell performs filesystem coordination; the generation rules and
templates remain FreeLang. Its fixed five-input/eight-output list describes
this matching generator version and must evolve with future language support.

This is not an all-files atomic transaction for concurrent readers or a
power-loss/SIGKILL recovery protocol. Individual replacements use same-directory
renames, and the tested caught failures/signals restore the previous state.
Uncatchable termination or a second failure during restoration can leave
recovery work. Private bundle generation remains preferable for deployment.
No production deployment or commercial durability claim follows from this
compiler-launcher checkpoint.

## Verification

Paths/owners: <가림>. Receipt: native-flp-default.bf754tx6.

| Scope | Result |
|---|---|
| Fresh generator built with current Native SDK | PASS; no Node build stage |
| Before implementation, private declared contracts | 4/16; failures retained |
| Existing real generator with final output read-only | Before: exit 1 after changing six files |
| Updated explicit Native entry | 16/16 contracts; read-only output blocks with exit 2 and changes zero files |
| Updated default entry | 16/16 contracts; same read-only preservation |
| Publication faults and concurrent edits | 6/6 |
| Additional default-entry controls | 9/9 |
| Fresh server/island outputs | 8/8 nonempty, exact hashes match the pre-change real Native generator |
| Existing checked-in paired islands | 4/4 unchanged bytes |
| Generated FLX syntax | 8/8 parsed with the verified Native compiler |
| Collected build and execution traces | 98; Node executions 0, connect calls 0 |
| Bash syntax / git diff whitespace | PASS |
| ShellCheck | NOT_RUN; unavailable; no installation |
| New JS / JS line growth | 0 / 0 |

The 16 contracts exercise tool configuration, exact protocol, incomplete/stale
outputs, partial writes followed by rejection or process failure, signal
status, and temporary-directory failure. Their private protocol sentinels
are separate from real language-generator execution.

The six fault controls exercise rename failure, TERM immediately after the
third successful rename, TERM immediately after obsolete-file deletion,
new-output rollback, contract edits during generation and destination edits
during generation. Last good output and the intentionally simulated editor
values are preserved as appropriate. Modes and directory/file state are
checked in these private projects.

An intermediate publisher failed its TERM-after-third-rename control: one
output remained changed because bookkeeping followed the rename. That receipt
and candidate remain archived. Recording intent before changing the filesystem
fixes the observed interruption window; all six final controls pass.
No test expectation or protected runner was changed to obtain the result.

The nine integration controls exercise rejection by each actual page-check
gate, a default no-argument invocation in an isolated copy of the entry,
a source directory with quotes, an apostrophe, Korean, emoji and a trailing
newline, output/parent symlinks, creating missing island directories,
missing generator configuration, and valid checker input that the limited
generator rejects. Traces verify the required two checker calls on success
and zero generator calls on either checker rejection. Real generation does
not edit its contracts.

Compiler/runtime/checker sources, protected fixtures and other concurrent
source work were not changed. No fresh Node generator comparison was executed:
that would run a prohibited Node build stage. The previous Native generator
and checked-in paired source bytes are the scoped output reference. Full
application, browser, fixed-point and DB95 suites were not rerun for this
launcher-only implementation; their earlier receipts and open gaps remain.

- FX HEAD: 48988a3756f2c0520be7121021c54083aaafb62a.
- Compiler SHA-256:
  e966e6342053c48fdd3af001a029e0d4a4923832f5122f0f300696ce8d1b2c94.
- Runtime archive SHA-256:
  09be8c24c8c2bff543d6d9a7c5942595529473759c60b52be18ff061951624f1.
- Fresh FLP generator SHA-256:
  50e9cc4435f9d871b3c5f58dde0423d48cb9eedf6451a4abf9ca4e83aa370418.
- Seed unchanged:
  0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

The approved temporary curl dependency remains environment-only; official SDK
packaging is pending. Dev/HMR, shared verification/deployment, remaining app
acceptance and DB commercial qualification remain open. The full goal remains
active; commercial readiness HOLD and deployment NO.
