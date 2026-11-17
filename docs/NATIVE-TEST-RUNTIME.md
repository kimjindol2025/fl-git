# Native unit execution evidence

The seven existing tests/*.test.fl files can now compile and execute as
Native programs with the FX testing forms and shell API implemented on
codex/native-commercial-start. The sources, assertions and protected
scripts/fl-test runner were preserved.

| Original file | Native passing cases |
|---|---:|
| fl-git.test.fl | 3 |
| provider-adapter.test.fl | 16 |
| provider-context.test.fl | 8 |
| provider-gate.test.fl | 9 |
| provider-model.test.fl | 15 |
| provider-write.test.fl | 9 |
| tui-mouse.test.fl | 11 |
| Total | 71 |

Each file is compiled separately to C by the verified Native FX compiler,
syntax checked during GCC compilation, linked with that compiler's runtime
and executed with the Native evidence harness's ten-second bound. The
original scripts/fl-test performs Node-based check and run calls without
setting that timeout itself. The validation
checks exit status, stderr, the actual passing summary and absence of FAILED.
The process trace records zero Node/npm/npx/bun/deno executions. Compilation
and execution use the shared heavy-work lock and two-GiB virtual-memory cap.

The only frontend product correction in this milestone is in
src/provider-adapter.fl: six masking expressions now call the existing
regex_replace(text, pattern, replacement) module API. The old kebab-case
regex-replace has a different legacy argument order in Native FX. The module
API also exists in the bundled reference runtime. Existing token-masking
assertions pass without changing their expected values. No JS was added or
edited.

FX implements actual assertion failure, case isolation, accumulated totals
and reset, including summary error-array lifetime and argument-evaluation
order. Its shell-exec-result implementation captures stdout and stderr
independently, handles real failures, closes descriptors and reaps children.
Negative diagnostic cases verify truthful failure reports. The Native unit
suite does not treat an intentionally failed diagnostic as a product pass.

This evidence does not run the default Node test launcher or prove the
reference interpreter's entire static-check behavior. Current Node parity,
browser flows, web builder/dev-server migration and the supported Native test
launcher remain pending. New JS files: 0; JS line changes: 0.

The preserved runner invokes reference `check` before `run`. Source inspection
of that check command confirms a wider contract than C compilation alone:

| Check component | Required Native migration evidence |
|---|---|
| Lexing and parsing | Reject malformed delimiters and unterminated literals with a real diagnostic |
| Function metadata | Preserve missing :context/:returns diagnostics |
| Effects | Preserve declared-effect and pure-function diagnostics and the command mode's exit behavior |
| Types | Preserve type errors, warnings and failure exit status |
| JSON mode | Preserve freelang.diagnostic.v1 fields and severity/status behavior |

The current self parser's delimiter-consumption helpers tolerate missing
closing delimiters; the string reader also accepts end-of-file. These need
independent negative syntax coverage before the Native compiler is used as
the syntax-check replacement. The reference text and JSON modes differ in
their pure-effect failure exit behavior; this difference must be measured
and documented rather than silently normalized. None of these pending checks
is counted as completed by the 71 executed unit assertions.

Reproduction records are retained privately under evidence identifier
native-test-forms.idxwvm; host paths and owners are <가림>. Compiler hashes,
fixed-point comparisons and release limitations are also recorded in the FX
docs/COMMERCIAL-NATIVE-WORKLOG.md.

Earlier testing-form receipt (2026-11-18 KST):

- FX source commit:
  bdf96424b2200a15d85b131ccc9219a8b5fd9959.
- Generated compiler Stage 2 = Stage 3 = Stage 4 C SHA-256:
  65fc56c092629248471828a88859237d4a2231de344e05213df7fe0e52f96cc2.
- Verified Stage 3 compiler ELF SHA-256:
  44379c85c41071c0c7822a8710973ab0a827740b9041f4114767cdb6fbf06546.
- Native checker ELF SHA-256:
  62a4dd93f0bee2cc916a2f89761dfca4aeb0ed6a41c8ba31f47fe507b8932b83.
- Final Native unit suite: 71/71; FX regressions: 11/11; checker fixtures:
  32/32 plus default pages, with the JSON-props case at 0.07 seconds.
- Bootstrap input SHA-256 before/after:
  0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.
  The seed was not replaced or regenerated. The protected official verifier
  still pins an earlier input and is NOT_RUN; the exact Native stage
  comparisons above use a separate evidence harness with the existing recipe.

The latest terminal/value milestone regenerated the compiler and original test
programs from the current FX source: frontend 71/71, CLI 22/22, FX regressions
11/11 and checker 32/32 plus default pages all passed, exit 0. Exact Stage
2/3/4 C SHA-256 is
69ec33d69ea93079296fa5723cc84e4b5b50e4466a6bd43d379385056bc72999;
Stage 3 ELF is
54d85380a55275e5ecce727e2e491e8acfb5fe015d701f2cc21ae0a217bf1d88.
The same pass includes original DB cases 7/13/36/40 and zero Node-family
executions in collected traces. Full DB 71-case rerun remains NOT_RUN.

Native syntax, metadata/effect and best-effort type components now exist in
FreeLang, but CLI diagnostic integration is incomplete. Independent checks
exposed and corrected non-tail call values escaping as trampoline requests,
flat cond branches disappearing inside loops and quadratic character splits.
The original syntax contract including a 100k-character literal passes 5/5;
the split/Unicode/NUL contract passes 6/6. Final metadata/type verification passed 7/7 and 10/10 with the same
verified compiler and runtime, exit 0. These module checks are not the complete reference
check contract or a Node parity result. Existing Node runners remain intact.
