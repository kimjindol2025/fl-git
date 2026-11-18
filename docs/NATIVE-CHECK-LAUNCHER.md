# Native page checker launcher

The existing FreeLang checker in tools/fl-check.fl can run as a Native
executable. scripts/fl-check delegates to scripts/fl-check-native. Both use
FREELANG_NATIVE_FLCHECK to select that verified binary and pass FL_CHECK_ROOT
and FL_CHECK_TARGET to it. NATIVE-DEFAULT-CHECK.md records the current default
entry, complete marker validation and reference comparison.

Example, with a privately built binary path:

```sh
export FREELANG_NATIVE_FLCHECK=<verified-native-binary>
./scripts/fl-check-native
./scripts/fl-check-native web/pages/products.flp
```

The launcher does not search for a compiler or invoke Node. Missing or
non-executable binaries fail with exit 2. A successful binary must return the
existing FL_CHECK_EXIT=0/1 protocol marker exactly once. Missing, duplicate,
conflicting or malformed protocol output fails with exit 2; reported checker
errors return exit 1. Other Native process failures are propagated unchanged,
even if the process printed a checker marker before failing.

## Earlier verification

These results describe the earlier separate entry. Use NATIVE-DEFAULT-CHECK.md
for the current default entry and its matching SDK receipt.

- FX source: 530ab3df4490c4c7bbb35573b4826a4042fbe9c3.
- FX generated Stage 2 = Stage 3 = Stage 4:
  6325a0b92053e2d859458057a0e833f93ce42ffece74493a8e1d841f9a601f37.
- Native checker SHA-256:
  68c96570bbadc495b96245701214809d5d09e2d597ebc7d549f85833b27c7f61.
- Standalone C generation, compilation and linking: PASS.
- Default page check: exit 0.
- Existing fixture matrix: 32/32 with expected acceptance/rejection and
  expected diagnostic classes. Existing fixtures and checker source were
  not changed.
- The existing JSON-props example completes below the documented one-second
  bound.
- execve traces for all 32 final launcher runs show zero Node/npm/npx/bun/deno
  executions.
- Missing binary: exit 2. Missing checker protocol: exit 2.
- Native process failure: exit 1 preserved.
- bash syntax and git diff whitespace checks: PASS.
- New JS files: 0; existing JS edits: 0.

This is a Native checker entry point. The existing web builder, development
server, default Node launchers and test runner migration are not complete.
At that revision Node parity was not executed; the source and expected
fixture contracts were preserved. Current reference comparisons are recorded
in NATIVE-DEFAULT-CHECK.md.
