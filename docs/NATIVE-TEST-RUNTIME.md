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
and executed with the original runner's ten-second bound. The validation
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

Reproduction records are retained privately under evidence identifier
native-test-forms.idxwvm; host paths and owners are <가림>. Compiler hashes,
fixed-point comparisons and release limitations are also recorded in the FX
docs/COMMERCIAL-NATIVE-WORKLOG.md.

Final artifact receipt (2026-11-18 KST):

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
