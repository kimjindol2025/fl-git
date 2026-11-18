# Native island compilation (developing)

scripts/fl-island-compile-native is a separate Node-free generation path.
The complete web builder, server entry and dev server remain pending. Default
launchers and existing JavaScript are unchanged. This entry has not completed
the physical browser/application acceptance gate.

## Use

Set FREELANG_NATIVE_COMPILER to the verified compiler with --parse-json and
the island browser primitives, and FREELANG_NATIVE_ISLAND_TOOL to the matching
Native executable built from tools/fl-island-compile.fl. Supply the approved
runtime/dependency environment and an existing output directory.

~~~sh
scripts/fl-island-compile-native web/islands/products.flx output-directory
~~~

A fresh artifact child contains ast.json, program.fl, manifest.json, module.js
and diagnostics. The command parses source without executing it or following
its load directives, emits FreeLang plus a manifest, and invokes --browser-js.
The only generated load is the known FreeLang support module. module.js is a
generated artifact. Missing dependencies fail explicitly; no binary search or
Node fallback occurs. Failed generation retains logs without publishing a
successful module.

## Source and host contracts

The normalizer consumes freelang.ast.v1 and renames local bindings/parameters.
Per-scope cells support forward closures and the tested outer shadowing cases.
Top-level atom bindings persist across render calls. Local load functions
remain lexical calls; unbound server load/eval are rejected. Core if/do remain
syntax. Malformed if arity now fails in the normalizer.

DOM construction uses the FreeLang island-h/island-mount boundary. Nested
children and island-str/join preserve element references while ordinary text
becomes text nodes. Joining element arrays retains the separator. Form reset
uses HTMLFormElement.prototype.reset.call to avoid a field named reset
clobbering the method. Value access supports input, textarea and select;
attribute access uses the island allowlist.

Inline RegExp tests preserve pattern/flags/text evaluation order. Only the
supported inline constructor shape is accepted. document.getElementById,
JSON.parse and window.location.assign interop require their unshadowed global
receivers. Navigation permits credential-free HTTP(S) URLs. Number and confirm
have typed entry points. The product-photo helper retains the server's public
path validation and default image.

String helpers cover trim, literal inclusion/count/split and regex split.
assoc/merge/dissoc/keys construct new maps and set own data properties;
__proto__ is data rather than a prototype setter. json_merge additionally
parses string inputs. This is the supported island subset, not a guarantee of
every legacy helper coercion or grammar form.

JSON requests support uppercase GET/POST/PUT/PATCH/DELETE/HEAD/OPTIONS,
HTTP(S) URLs without embedded credentials, string-valued header maps and
acyclic JSON bodies within the existing depth-512 boundary. Shared subtrees
are allowed; functions, atoms, task/listener handles and opaque host objects
are refused as body data. These checks precede JSON.stringify.

fetch! accepts URL/callback or method/URL/body/headers/callback;
fetch-post! accepts URL/body/callback; fetch-delete! accepts URL/callback;
fetch-result! accepts method/URL/body/callback. The short forms deliver data
only on success and report failure to the console. The longer fetch! and
fetch-result! forms deliver {ok,status,data,error}. Requests use the existing
owned cancellation registry, settle before callback delivery, and do not
deliver a second result when a callback throws. No new timeout is imposed.
The older restricted browser-http! contract is unchanged.

Invalid JSON responses produce an unsuccessful result. Empty 204/205/HEAD
responses yield an empty object. This deliberately differs from the legacy
helper's treatment of a successful HTTP response containing malformed JSON;
compatibility for that legacy behavior is not claimed.

Source strings retain quotes, controls, NUL, Korean, emoji and script-like
text. Isolated UTF-16 surrogate units use a JSON data expression because UTF-8
file output intentionally replaces those units. Private compiler names and
aliases are rejected before renaming. Unsupported watch options, loop/recur,
definitions/assignment and variadic parameters fail explicitly.

## Current verification

| Evidence | Result |
|---|---|
| Original real island clients, fresh compiler | 8/8 Native browser-code generation |
| Generation/boundary/lexical/interop controls | 26/26 |
| String round trips / invalid AST records | 7/7 / 3/3 |
| Pure FreeLang kernel algorithms, functional Native host model | 38/38 |
| Fresh Native Stage 2 = Stage 3 = Stage 4 C | exact bytes, exit 0 |
| Frontend / CLI / FX regression | 71/71 / 22/22 / 11/11 |
| Page checker | 32/32 plus defaults |
| Original browser generation / byte reproducibility / rejections | 7/7 / 7/7 / 10/10 |
| Original DB95 cases 7, 13, 36, 40 | all exit 0; not the full DB gate |
| Browser DOM, HTTP, callbacks, RegExp, navigation and forms | NOT_RUN |
| Prior six Native transformation models | NOT_RERUN in this checkpoint |

All eight files now generate: admin/orders, admin/products, cart, checkout,
index, order/track, products/[id] and products. Earlier frontend receipts retain the previous six missing host feature
failures; the first connector receipt records the newly exposed malformed
index if. Index's correction changes parentheses only:
the AST node/operation/literal multiset is unchanged (7,135 nodes). No screen
section was removed. Generation is not NATIVE_OK 26/26 or browser success.

The 38-check model exercises actual extracted kernel functions, lowering only
the fixed host and/or primitives to Native short-circuit syntax. A private C
host supplies data/reference operations. It covers Unicode/text/node
composition, immutable map operations, special own keys, JSON body shapes,
shared subtrees, cycles and depth 512/513. It does not simulate DOM, network,
promises, garbage-collector weakness or browser execution.

The first ordinary-function host emulation crashed three diagnostic runs
(exit 139): eager argument evaluation caused infinite recursion where the
browser printer emits short-circuit operators. That approach was stopped.
Subsequent private fixtures exposed three extra closing parentheses; strict
parse preflight and positive CGC-error checks were added to the private
diagnostic driver. Another failed model revealed a C helper reading FL_FLOAT
indices through the FL_INT union member; fixing that helper left all 38
expectations unchanged. Product code was not changed to suppress these
diagnostic failures. All failed receipts remain preserved.

Fresh C generation took 40.42 / 40.13 seconds and 1,525,712 / 1,529,112 KiB
peak RSS. The memory cap remains 2 GiB. The existing 40-second performance
target was missed; there was no timing retry or limit change. Fixed-point C
SHA-256 is 23ce0a1c2c08b0fb2283c2204ae727a4a61e0c4af190d741401da02534bf4466.
Stage-3 ELF SHA-256 is
80b5d9e911eb71fd5d298b8516b73ae09f3838c96f7c0ddee27f0c4e8b35b9e8.
The seed remains
0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

Receipts: native-island-connectors.jb5xb3bu,
native-island-connectors-v2.7woq4di0,
native-island-http-fixedpoint.d2zl48wa,
native-island-http-core.j8s8mc2h,
native-island-http-controls.jrkjni0z,
native-island-http-strings.xukypltn,
native-island-data-kernel.1btzbvtg,
native-island-data-logical.lro5fbjs,
native-island-data-logical-v2.frytpv6y,
native-island-data-logical-v3.4yxaafim and
native-island-data-logical-v4.ccmdp6w0.
Raw paths/owners: <가림>. Existing tests, expected values, timeouts, protected
runners and JS files are unchanged. New authored JS files: 0; JS line delta: 0.

## Remaining acceptance gates

The Browser plugin and Python Playwright are unavailable. The Chrome bridge
still refuses all five sockets. Separate fresh, private startup probes under
the unchanged 2 GiB cap failed: Chrome exited 133; Firefox 140.4.0esr exited
with SIGSEGV (-11) and produced no screenshot. The exact startup causes are
unproven. No dependency was installed, browser security disabled or limit
raised. Page identity, DOM/nonblank/overlay checks, console health, screenshots
and physical interaction are NOT_RUN.

Full web compilation, stores/HMR, dev server, focus/selection, remaining
binding/state cases and default-entry migration remain pending. DB95 still
has the checkpoint fault-recovery blocker. Its full 71-case gate was not
rerun; the recorded baseline is 51 PASS / 20 FAIL. Commercial readiness HOLD.
