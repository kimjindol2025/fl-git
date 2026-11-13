# FE-11 Native migration — phase 1

## Result

**FE11=보류.** The FE-10 base was clean at `6e8257f63fad1cdbda336bbb86b6f82392a21ddd`; this work is on `feat/fe-11-native`. No product or test source was changed. A safe Native run was not available, so this document does not claim a migration or a current 22/22 check.

The requested FX source commit is present locally as `aa6612d532c678c1412fed496000962b6520aee7` (`fix/b13-utf16`). The FX working copy itself is dirty and checked out on a different review branch; it was not changed or used as a source tree. The pinned commit does not contain a runnable compiler binary. Other local `stage0-bin`/`stageN-bin` artifacts were found, but no matching binary SHA receipt was recorded in that commit, so they were not treated as a compiler proven to come from `aa6612d5`.

The shared heavy-work lock was held by B-14 when checked. FE-11 therefore ran no compiler, checker, test suite, or server. This avoids overlapping its required 2 GiB-limited build with B-14.

## Node and JavaScript execution inventory

“Place” below means one execution/build responsibility, not one file. Line counts are repository-tracked source lines at the FE-10 base. The immutable FE-10 JavaScript baseline is 49,618 lines; current tracked `*.js` totals 49,617. The `.mjs` TUI host is counted in the dependency inventory but not in the `.js` line baseline. The external AFJ runner is not included in the fl-git total.

| Place | What runs there | JS/host lines | Migration difficulty |
|---|---|---:|---|
| CLI and TUI host | `scripts/fl-git` invokes AFJ; `scripts/tui-runner.mjs` hosts terminal interaction; external AFJ `bootstrap.js` is 49,372 lines | 32 shell + 14 MJS; external 49,372 | Hard |
| fl-check | `scripts/fl-check` starts `tools/fl-check.fl` through AFJ Node | 13 shell; checker itself is FreeLang | Easy after native process env/cwd support |
| fl-test | `scripts/fl-test` invokes AFJ check/run for the existing `.test.fl` suite (7 test files) | 49 shell | Medium; needs native check/run and stable exit-status mapping |
| flp-compile | `scripts/flp-compile` invokes `tools/flp-compile.fl`; that tool reads and writes generated page files | 16 shell; compiler itself is FreeLang | Medium; needs native file/process contract and isolated outputs |
| Validation helpers | `scripts/provider-gate`, `scripts/check.sh`, and `scripts/fl-tools` invoke Node/AFJ paths | 73 + 26 + 48 shell | Medium |
| Front build | `web/fl-front-build.js` scans pages, emits `_app.fl`, and calls the style compiler and diagnostics | 2,937 + 312 + 220 JS | Hard; broad build and generated-file contract |
| Development server | `web/dev-server.js`, `scripts/fl-dev`, and `scripts/fl-git-web` watch, build, and start the app | 665 JS + 14 + 10 shell | Hard; watcher, HMR, HTTP and child-process lifecycle |
| AFJ web runtime | `web/runtime/bootstrap.js`, `web/bootstrap-wrapper.js`, and `web/ecosystem.config.js` provide app execution and service startup | 44,582 + 14 + 12 JS | Hard; largest shared runtime surface |
| Island client/tooling | `web/island-compiler.js` emits browser JavaScript; `web/island-runner.js` executes headless UI checks | 582 + 293 JS | Hard; browser DOM/fetch behavior is a separate target from native server execution |

Current tracked JavaScript files: 9 `.js` files, **49,617 lines**. `scripts/tui-runner.mjs` adds 14 Node-host lines but is not a `.js` file and is outside the baseline counter. No file was added or changed in either language by this task: `NEW_JS=0`.

## Native input matrix

Requested input count is 26: 2 tool programs, 9 top-level `src/*.fl`, and 15 `web/pages/**/*.flx`. **0/26 were run**; each is NOT RUN, not a compiler failure. The reason is the combination of no compiler binary tied to the pinned FX commit and the occupied heavy-work lock.

| Input | Native result | Reason |
|---|---|---|
| `tools/fl-check.fl` | NOT RUN | No proven `aa6612d5` native compiler; B-14 lock held |
| `tools/flp-compile.fl` | NOT RUN | Same; its execution writes generated page files, so a separate temporary source/output copy is also required |
| `src/main.fl` | NOT RUN | No proven compiler; lock held |
| `src/git.fl` | NOT RUN | No proven compiler; lock held |
| `src/tui.fl` | NOT RUN | No proven compiler; lock held |
| `src/tui-mouse.fl` | NOT RUN | No proven compiler; lock held |
| `src/provider.fl` | NOT RUN | No proven compiler; lock held |
| `src/provider-adapter.fl` | NOT RUN | No proven compiler; lock held |
| `src/provider-context.fl` | NOT RUN | No proven compiler; lock held |
| `src/provider-gate.fl` | NOT RUN | No proven compiler; lock held |
| `src/provider-write.fl` | NOT RUN | No proven compiler; lock held |
| `web/pages/_admin-api.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/_cart-api.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/_layout.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/_order-api.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/_orders-store.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/_products-store.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/admin/orders.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/admin/products.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/api.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/cart.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/checkout.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/index.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/order/track.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/products.flx` | NOT RUN | No proven compiler; lock held |
| `web/pages/products/[id].flx` | NOT RUN | No proven compiler; lock held |

## Smallest migration candidate and blockers

`scripts/fl-check` is the smallest candidate: its wrapper is 13 shell lines and its checker is already FreeLang. It was **not moved** because no native compiler could be produced in the available environment. Static inspection shows `shell_env` and `shell_cwd` are JS-bootstrap names; the FX native symbol table has `env-get` and `process-getcwd` equivalents. Their direct runtime behavior and the compiler's missing-symbol diagnostics were not tested.

Priority list for a follow-up FX/native instruction:

1. **P0 — Native process environment and working directory:** test and adapt `shell_env`/`shell_cwd` callers to the native `env-get`/`process-getcwd` names; needed by both FreeLang tools to select repository roots and targets.
2. **P0 — Proven runnable compiler for the pinned source:** provide a compiler artifact and source/build receipt tied to `aa6612d5`; do not substitute an unproven local binary or rebuild/replace the FX seed under this task.
3. **P1 — Native AFJ-compatible HTTP/SSR runtime contract:** FX has native HTTP/runtime source, but parity with AFJ routes, request/response helpers, Island output and startup behavior has not been demonstrated. A compatibility map and isolated proof are needed before replacing the web runtime.

Additional static observations: FX commit `aa6612d5` contains native source for basic file read/write, file existence, directory listing, regex and JSON operations. Their compiler registration and behavior on these 26 inputs are unverified. The browser Island compiler emits JavaScript for the browser; replacing Node server execution does not by itself remove that client-side JavaScript target.

## Checks and change boundary

| Check | Result |
|---|---|
| FE-10 base and clean status before branch | PASS; expected commit |
| New branch | `feat/fe-11-native` created from FE-10 base |
| FX source | Read-only; target commit object present; FX worktree left untouched |
| Native matrix | 0/26 run; no pass/fail claimed |
| fl-check | Not rerun; prior FE-10 report was 22/22, not revalidated here |
| fl-test | Not rerun; prior FE-10 report was 71/71, not revalidated here |
| FE feature regression | Not run; no product files changed |
| Source/test/JS changes | 0 |

**Next:** after B-14 releases `/tmp/fl-heavy.lock`, obtain a provenance-verified native compiler for `aa6612d5`; run only under the lock and a 2 GiB cap. First test whether the two process-context functions are available, then compile and execute `fl-check` against an isolated fixture and compare all 22 results with the existing checker. Move the launcher only after parity is demonstrated. Recheck the FE test suites and localhost-only FE behavior before claiming regression-free migration.

`NATIVE_FLCHECK=NOT_RUN/22` · `NATIVE_MATRIX=0/26` · `FE_REGRESS=UNVERIFIED` · `NEW_JS=0` · `RELEASE=UNCHANGED`

## FE-11R follow-up

**FE11R=계속.** Branch `feat/fe-11r-native` is based on `bb93ed1fa23a5409efd12c7b1a1737d73d2c94c1`. The read-only FX source is `fix/b14-big-c` at `6e21df51a09453d7be7b45d10e7551c90b75b555`. The pinned FX bootstrap reproduced B-14's C fixed point and built an x86_64 compiler ELF from Stage3 C.

| Attempt | Result |
|---|---|
| First isolated build invocation | Failed before compiler execution because the systemd unit did not receive its temporary snapshot variables. No C compiler ran and no seed was changed. |
| Corrected isolated build invocation | Reached the Stage 0 C compile, then stopped because `curl/curl.h` was not in the stripped build environment. No stub or alternate compiler was used. |
| Corrected isolated build with existing curl development paths | PASS: FX's committed GCC recipe generated and linked Stage0–3 compiler binaries. Stage2/3/4 C hashes all equal the B-14 fixed-point hash. No package or system configuration was changed. |

The pinned FX archive seed was hashed before and after the successful bootstrap: `SEED_SHA_START=SEED_SHA_END=f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c`. FX worktrees were not modified. The build and Native test jobs ran under `/tmp/fl-heavy.lock` with a 2 GiB cap; bootstrap peak memory was 670.7 MiB.

**Pinned build recipe** (FX `scripts/verify-native-bootstrap.sh`): `gcc -O2 -w -I runtime -o <stageN-bin> <stageN.c> <runtime C sources> -lm -lpthread -ldl -lsqlite3 -lssl -lcrypto -lcurl`; each Native compiler then compiles `self/cgc-main.fl` to the next stage C. Results: `STAGE2_SHA=STAGE3_SHA=STAGE4_SHA=f573f59076c35f90a597b6fd2c7bb6255bbeff1f306b05ade0e4248161b5aff4`; fixed-point comparison PASS. Stage3 x86_64 compiler ELF SHA-256: `124a9da3db2e6e6f4b9988650b0ae6e5762a9097870d6ad3c44373922448e429`. The bootstrap emitted seven `nil`-propagation warnings; they remain in the build log.

### Native input matrix

The 26 requested inputs were the two tools, nine `src/*.fl` files, and 15 `web/pages/**/*.flx` files (including existing ignored generated page files, read but not changed). A row passes when the pinned Native compiler emitted C, the C linked against the pinned FX runtime, and the program exited 0. It does not claim that uncalled handlers or browser interactions were exercised.

| Input | Native result | First failure reason, if any |
|---|---|---|
| `tools/fl-check.fl` | PASS | Native checker exit 0; output matched Node byte-for-byte |
| `tools/flp-compile.fl` | FAIL | `shell_env` is not registered in FX Native |
| `src/git.fl` | PASS | — |
| `src/main.fl` | PASS | — |
| `src/provider-adapter.fl` | PASS | — |
| `src/provider-context.fl` | PASS | — |
| `src/provider-gate.fl` | PASS | — |
| `src/provider-write.fl` | PASS | — |
| `src/provider.fl` | PASS | — |
| `src/tui-mouse.fl` | PASS | — |
| `src/tui.fl` | FAIL | `tty-raw-leave!` is not registered in FX Native |
| `web/pages/_admin-api.flx` | PASS | — |
| `web/pages/_cart-api.flx` | PASS | — |
| `web/pages/_layout.flx` | PASS | — |
| `web/pages/_order-api.flx` | FAIL | generated C references undeclared `fl_cart_state` |
| `web/pages/_orders-store.flx` | PASS | — |
| `web/pages/_products-store.flx` | PASS | — |
| `web/pages/admin/orders.flx` | FAIL | browser `document` and cross-page `AdminOrders` are undeclared |
| `web/pages/admin/products.flx` | FAIL | browser `document` and cross-page `AdminProductForm` are undeclared |
| `web/pages/api.flx` | FAIL | `fl-env-get` is not registered in FX Native; the available name is `env-get` |
| `web/pages/cart.flx` | FAIL | cross-page `CartPage` is undeclared |
| `web/pages/checkout.flx` | FAIL | browser `window` is undeclared |
| `web/pages/index.flx` | FAIL | generated C contains undeclared `Segoe` token |
| `web/pages/order/track.flx` | FAIL | browser `document` is undeclared |
| `web/pages/products.flx` | PASS | — |
| `web/pages/products/[id].flx` | FAIL | browser `document` is undeclared |

Result: **15/26** compile/link/execute successfully. Failure groups to carry to FX work: browser DOM/window plus page-component aggregation; environment/TUI runtime-name gaps; generated shared state and C-string/codegen issues.

### Native API and checker parity

Direct FX Native probes showed `shell_env` and `shell_cwd` fail with “Function not found”. `env-get` and `process-cwd` pass; the FX canonical name is `process-cwd` (not `process-getcwd`). `fl-check.fl` now uses the Native-compatible `env-get`/`process-cwd` and replaces unsupported `str-count` with shared `str-index-of`/`str-slice` logic and newline splitting. The Node wrapper accepts this source unchanged.

The compiled Native `fl-check` and Node-run checker both exited 0 and emitted the identical single `FL_CHECK_EXIT=0` line; output diff was 0 bytes. This preserves the recorded FE-10 `CHECK=22/22` result. The ordinary `scripts/fl-check` wrapper exited 0, and `scripts/fl-test` reported 71/71 (3+16+8+9+15+9+11). No test, expectation, timeout, runner, or JavaScript file changed. This is unit regression evidence; a full browser purchase/admin flow was not rerun.

### HTTP/SSR probe

A minimal Native `server_get` + `server_html` screen bound with `FL_HTTP_LOOPBACK_ONLY=1` on `127.0.0.1:40850`; local HTTP GET returned the exact `FE11R Native SSR` marker. The runtime log records graceful shutdown. The shell wrapper initially observed the listener during its shutdown race; after the unit exited there was no listener on 40850 or 40851. This proves the basic FX Native HTTP/HTML path only; full AFJ route/island compatibility remains unverified.

### Change and remaining gaps

Changed files: `tools/fl-check.fl` and this document only. `NEW_JS=0`; tracked JavaScript remains 49,617 lines. `FIRST_MOVED=fl-check`. The checker source now has a Native invocation and exact Node output parity, while `scripts/fl-check` remains the existing launcher. `flp-compile`, full AFJ page rendering, and the failed Native modules still need FX API/codegen work. No FX files, seeds, tests, FE product behavior, server 23/25, or remote repositories were modified.

`NATIVE_OK=15/26` · `SHELL_ENV_CWD=Native names env-get/process-cwd pass; shell_env/shell_cwd absent` · `AFJ_HTTP_SSR=basic FX route PASS; full AFJ compatibility UNVERIFIED` · `NATIVE_FLCHECK=22/22` · `FLCHECK_DIFF=0` · `FIRST_MOVED=fl-check` · `CHECK=22/22` · `FL_TEST=71/71` · `FE_REGRESS=0 observed` · `NEW_JS=0` · `JS_LINES=49617` · `SEED_SHA_START=SEED_SHA_END=f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c`
