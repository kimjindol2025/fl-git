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

`scripts/fl-check` is the smallest candidate: its wrapper is 13 shell lines and its checker is already FreeLang. It was **not moved** because the current tool contract needs `shell_env` and `shell_cwd`; in the pinned FX commit those names appear in the JS bootstrap implementation, with no native runtime implementation found by the static source search. Native support must be established or a safe argument interface approved before matching the existing 22 diagnostics/results. No missing-function error was observed at compile time because no compile was run.

Priority list for a follow-up FX/native instruction:

1. **P0 — Native process environment and working directory:** implement or provide native equivalents for `shell_env` and `shell_cwd`; needed by both FreeLang tools to select repository roots and targets.
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
