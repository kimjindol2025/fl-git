# FE-13 — Native failure reduction and compiler memory measurement

## Scope and preservation

- FE-12 commit: `d6e9817a98223105d395b1320bb9f67cb3cd087b`.
- GitHub branch `feat/fe-12-native2` was verified at that exact commit and clean before starting FE-13.
- FE-13 branch: `feat/fe-13-native3`, created from the FE-12 commit.
- Requested FX source commit: `7f6e830456533226b3d6d67aabbf3f063ee78384`; inspected by commit object only. The B-17 branch was not checked out or used, and no FX worktree was changed.
- That FX commit contains no compiler ELF. Measurements therefore used the already verified, read-only FE-11R Stage3 compiler ELF (SHA-256 `124a9da3db2e6e6f4b9988650b0ae6e5762a9097870d6ad3c44373922448e429`), built from the earlier pinned FX source. Results describe that compiler, not a compiler rebuilt from `7f6e8304`.
- Fixed C seed SHA-256 at start and end: `f7425bf086d99d11231ba79296e194fcc36b454379e46d44131bafb0dc74101c`.

## Remaining eight failures

| Input | Current cause | fl-git-side action | FX-side requirement |
|---|---|---|---|
| `src/tui.fl` | `tty-raw-leave!` has no equivalent in the inspected Native symbol/runtime sources | No safe rename is known; no stub or behavior removal | Implement and verify the terminal raw-mode exit capability |
| `web/pages/admin/orders.flx` | Standalone Native input sees browser `document` and the page Island binding | Emit/compile the SSR route separately from its Island | No specific missing FX API identified; the current full generated bundle hits compiler allocation failure |
| `web/pages/admin/products.flx` | Standalone Native input sees browser `document` and the page Island binding | Emit/compile the SSR route separately from its Island | Same full-bundle compiler allocation limit |
| `web/pages/cart.flx` | Standalone Native input sees cross-page `CartPage` Island binding | Emit/compile the SSR route separately from its Island | Same full-bundle compiler allocation limit |
| `web/pages/checkout.flx` | Standalone Native input sees browser `window` | Emit/compile the SSR route separately from its Island | Same full-bundle compiler allocation limit |
| `web/pages/index.flx` | CSS string issue is fixed; standalone link still needs the bundled `FlGitApp` Island | Split the SSR entry from the Island bundle | Same full-bundle compiler allocation limit |
| `web/pages/order/track.flx` | Standalone Native input sees browser `document` | Emit/compile the SSR route separately from its Island | Same full-bundle compiler allocation limit |
| `web/pages/products/[id].flx` | Standalone Native input sees browser `document` | Emit/compile the SSR route separately from its Island | Same full-bundle compiler allocation limit |

The six DOM/page-binding cases already express SSR and Island code as distinct forms, and the existing frontend build extracts Islands. The Native path still receives one aggregate server source. No further source patch was made: the current evidence does not establish a safe per-route output/link contract, and the full bundle fails before route execution. `tty-raw-leave!` was not renamed because no matching Native API was found. The Native/API spellings changed in FE-12 remain valid under the inspected FX source (`env-get`, `process-cwd`).

## App compiler measurements

All Native compiler runs were inside the shared heavy-work lock with a 2 GiB virtual-memory ceiling. The quarter and half inputs are complete prefixes ending at top-level form boundaries; they measure input-size scaling, not independently complete storefronts. No tests or app runtime were run on those prefixes.

| App source share | Input bytes | Native C generation | Elapsed seconds | Peak RSS KiB | Result |
|---|---:|---|---:|---:|---|
| Approximately 1/4 | 77,493 (24.6%) | Completed | 4.91 | 493,168 | Exit 0 |
| Approximately 1/2 | 154,806 (49.1%) | Completed | 10.30 | 852,912 | Exit 0 |
| Full app | 315,480 (100%) | Failed before C output | 18.76 | 1,945,364 | Exit 1: lexer/string text allocation failure |

The full-app failure is the second observed failure of this same allocation cause across FE-12/FE-13. It was not retried again. No GCC full-app compile/link or app server startup was possible because the compiler emitted no full C file.

Partial C link measurements, also under the same resource cap:

| Input share | Generated C bytes | GCC compile/link seconds | Peak RSS KiB | Result |
|---|---:|---:|---:|---|
| Approximately 1/4 | 1,023,789 | 4.70 | 128,100 | Exit 0 |
| Approximately 1/2 | 418,025 | 13.12 | 314,232 | Exit 0 |

### Highest observed memory steps

1. Full Native source compile / lexer string allocation: **1,945,364 KiB**, failure before C output.
2. Half-source Native compile: **852,912 KiB**, success.
3. Quarter-source Native compile: **493,168 KiB**, success.

GCC link of the half input peaked at 314,232 KiB and the quarter input at 128,100 KiB. Full-app link and runtime memory remain unmeasured; do not infer them from the partial inputs.

## Native and regression results

| Check | Result |
|---|---|
| `NATIVE_OK` | 18/26; unchanged from FE-12 |
| `NATIVE_FLCHECK` | 22/22 receipt retained; current Native/Node raw output equality and exit 0 verified |
| `flp-compile` | Native launcher exit 0; generated output difference 0 |
| `CHECK` | 22/22 receipt retained; current checker exits 0 without diagnostics |
| `FL_TEST` | 71/71 in the current FE-12 verification run |
| FE route/visual regression | UNVERIFIED; the Native full app did not start |
| `NEW_JS` / tracked JS lines | 0 / 49,617 |
| Screenshots | None; no Native HTTP server was available |
| Ports 40850/40851 | Closed |
| FX source / fixed seed changes | 0 |

## Result

`FE13=보류`. FE-12 is preserved on GitHub, checker and Native `flp-compile` parity remain intact, and the requested memory scaling has been measured. `NATIVE_OK=18/26` is below 22/26. The remaining page inputs require a safe Native SSR/Island output boundary; the full generated app currently exceeds the compiler's string-allocation capacity under the required 2 GiB ceiling. The TUI terminal exit function remains an FX capability gap.

Next: implement a verifiable per-route SSR/Island artifact contract without changing the browser behavior, then run the Native route and visual checks under the same resource cap. Handle `tty-raw-leave!` only after FX provides a real compatible Native API. Keep the release blocked until regressions are verified.
