# FE-12 Native second tool and failure reduction

## Scope and fixed inputs

- Branch: `feat/fe-12-native2`, created from `9e6a943537540bb9ad4fd8a783a1a24ac375fd44`.
- FX compiler source remains read-only at `6e21df51a09453d7be7b45d10e7551c90b75b555`.
- Existing compiler artifact SHA-256: `124a9da3db2e6e6f4b9988650b0ae6e5762a9097870d6ad3c44373922448e429`.
- Fixed C seed start SHA-256: `f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c`.
- The seed is read-only. End hash and equality are recorded in the final section after checks.

## FE-11R failure map (11 inputs)

| Input | Failure group | Native blocker | Ownership / FE-12 action | Current FE-12 result |
|---|---|---|---|
| `tools/flp-compile.fl` | Runtime API names | `shell_env`; also used the non-Native `str-count` | fl-git controlled; changed to `env-get` and `str-index-of` | PASS; Native and Node outputs equal |
| `src/tui.fl` | Runtime API | `tty-raw-leave!` is absent from the pinned Native symbols | FX capability required; unchanged | FAIL, unchanged |
| `web/pages/_order-api.flx` | Generated C/shared state | Direct global `fl-cart-state` reference emitted as undeclared C state when compiled alone | fl-git controlled; delegated cart clearing to the actual cart API function | PASS; compile/link/run exit 0 |
| `web/pages/admin/orders.flx` | Browser DOM / page binding | `document` and `AdminOrders` are browser/page aggregation inputs | Browser-only Island is excluded from the server bundle | Standalone FAIL; bundled server path UNVERIFIED |
| `web/pages/admin/products.flx` | Browser DOM / page binding | `document` and `AdminProductForm` are browser/page aggregation inputs | Browser-only Island is excluded from the server bundle | Standalone FAIL; bundled server path UNVERIFIED |
| `web/pages/api.flx` | Runtime API name | `fl-env-get` is not in Native; FX provides `env-get` | fl-git controlled; changed to `env-get` | PASS; compile/link/run exit 0 |
| `web/pages/cart.flx` | Browser DOM / page binding | `CartPage` is an Island component, not a standalone server symbol | Browser-only Island is excluded from the server bundle | Standalone FAIL; bundled server path UNVERIFIED |
| `web/pages/checkout.flx` | Browser DOM | `window` is client-only | Browser-only Island is excluded from the server bundle | Standalone FAIL; bundled server path UNVERIFIED |
| `web/pages/index.flx` | Generated C / browser aggregation | After the CSS quote was made C-safe, standalone link fails because `(island FlGitApp ...)` belongs to the combined build | CSS string issue corrected | FAIL at standalone `FlGitApp`; app bundle FAILS Native allocation |
| `web/pages/order/track.flx` | Browser DOM | `document` is client-only | Browser-only Island is excluded from the server bundle | Standalone FAIL; Native HTTP flow NOT RUN |
| `web/pages/products/[id].flx` | Browser DOM | `document` is client-only | Browser-only Island is excluded from the server bundle | Standalone FAIL; bundled server path UNVERIFIED |

Group totals: runtime/API names 3, browser DOM or aggregation 6, generated C/shared-state 2 (11 total). The TUI runtime capability remains the only identified blocker with no fl-git-only replacement.

## Changes and checks completed so far

- `tools/flp-compile.fl` now uses Native-supported `env-get` and `str-index-of` while remaining valid under the existing Node launcher.
- Native and Node executions both returned `FLP_COMPILE_EXIT=0`; the four generated page outputs compared byte-for-byte equal. The compiler intentionally deletes its optional `FLP_OUTPUT` marker file on success; it is not part of the output comparison.
- Added `scripts/flp-compile-native`, which runs a caller-supplied, already verified Native tool binary and leaves the Node comparison launcher intact. The wrapper ran successfully; selected generated outputs compare byte-for-byte with the Node baseline.
- `web/pages/api.flx` uses the FX-compatible `env-get` spelling; Native compile, link, and execution exited 0.
- `_orders-store.flx`, `_products-store.flx`, and `_admin-api.flx` use the same cross-runtime environment API for paths and the configured admin secret. Secret values were not read or logged.
- `_order-api.flx` calls `fl-cart-clear!`; `_cart-api.flx` defines that real state-reset operation. Order success still clears the cart.
- Native compile, link, and execution of `_order-api.flx` exited 0 and registered its existing route handlers.
- `web/pages/index.flx` uses single quotes around the CSS font name so the Native C string generator does not emit `Segoe` as an identifier.
- Direct Native link of `index.flx` now reaches the standalone Island symbol `FlGitApp` and fails there; the server bundle must remove that browser-only block as the existing frontend build does.
- FE-11R's 15/26 baseline plus verified `flp-compile`, `api.flx`, and `_order-api.flx` executions gives **18/26 direct standalone Native passes**. The browser Islands still need the route-bundle check.

The frontend build source already extracts each `(island ...)` block and emits its browser script separately from the generated server route. No JavaScript file was added or changed. Native server execution must be reported separately from browser Island execution; it does not make the browser code Native.

## FE-12 execution results

| Check | Result |
|---|---|
| Direct standalone Native input matrix | **18/26** confirmed: FE-11R's 15 plus `flp-compile.fl`, `api.flx`, and `_order-api.flx` |
| Changed API/store modules | `_admin-api`, `_cart-api`, `_orders-store`, `_products-store`: all compile, link, and exit 0 under Native |
| Second tool | `flp-compile`; Native launcher exits 0; Node/Native generated page outputs compare equal |
| `/order/track` Native HTTP GET and API 200/404/429 | **NOT RUN**; full generated app compilation fails at the 2 GiB cap before producing C or opening a port |
| PII-free response | **UNVERIFIED**; no Native endpoint response was collected |
| 375px and 1280px screenshots in `docs/evidence/` | **NOT CREATED**; the Native app did not start |
| Native `fl-check` | Exit 0; raw result byte-identical to Node (`FL_CHECK_EXIT=0`, no diagnostics) |
| Existing `CHECK=22/22` | Existing FE-11R receipt retained; current `scripts/fl-check` exits 0 with no diagnostics |
| Existing `FL_TEST=71/71` | Current `scripts/fl-test` PASS: 3+16+8+9+15+9+11 = 71/71 |
| FE regression | **UNVERIFIED**; full HTTP/visual FE flow could not run |
| Tracked JavaScript line count / new JS | 49,617 / 0; changed JS files 0 |
| Seed SHA start/end | Equal: `f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c` |

### Failure-group disposition

The original eleven direct Native failures remain grouped as runtime/API names 3, browser DOM or page aggregation 6, and generated-C/shared-state 2. The API-name cases for `flp-compile` and `api.flx` are resolved by using Native-supported FreeLang names. `_order-api.flx` now compiles, links, and runs after cart reset was moved behind a real cart API function. The TUI `tty-raw-leave!` gap remains FX-owned. Browser `document`/`window` and cross-page Island symbols still need a server/browser bundling boundary. `index.flx` now passes the CSS string point but standalone compilation still fails on `FlGitApp`, which belongs to the bundle.

The full generated `_app.fl` bundle was attempted once under `flock -o /tmp/fl-heavy.lock` with a 2 GiB virtual-memory limit. The Native compiler stopped before C output with `text allocation failed` while reading the 315,480-byte generated source. No app binary was linked, no server was started, and ports 40850/40851 remained unused. The same large-bundle compile was not retried.

The Native fl-check comparison uses raw-to-raw output: both executions return the same 16-byte `FL_CHECK_EXIT=0` line. An intermediate comparison had removed the marker only on the Native side; that normalization mismatch was corrected and is not a checker difference. The existing Node test runner was used only for the requested regression checks; no JavaScript build ran.

### FX capability priorities exposed by this run

1. **Bounded-memory/streaming Native compilation for large generated source.** Needed to compile aggregated page routes under the 2 GiB test budget.
2. **A Native server/browser aggregation contract that isolates Island code.** Needed to execute order-tracking server routes without compiling browser-only `document`/`window` code as server code.
3. **AFJ-compatible Native HTTP request/response behavior for JSON POST routes.** Needed to verify lookup success, uniform 404s, rate limiting, and privacy on the actual Native server.

An initial link attempt was made before acquiring the shared lock and failed before linking because its temporary object-file directory was empty. Subsequent Native work is lock- and 2 GiB-limited. This attempt did not modify FX, the seed, or repository source.

## Final result

**Final:** `FE12=보류`. The Native second tool, checker parity, and unit regressions pass, but the 20/26 threshold and Native `/order/track` flow/screenshots are blocked by the full-bundle compiler allocation failure. Working tree remains uncommitted on `feat/fe-12-native2`; no push was requested or made.
