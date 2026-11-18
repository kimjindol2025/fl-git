# Native island compilation (developing)

scripts/fl-island-compile-native is a separate Node-free generation path.
The complete web builder, server entry and dev server remain pending. The
default launchers and existing JavaScript remain unchanged. This entry is
not accepted as a replacement for the complete browser/application workflow.

## Use

Set FREELANG_NATIVE_COMPILER to the verified compiler with --parse-json and
the island browser primitives, and FREELANG_NATIVE_ISLAND_TOOL to the matching
Native executable built from tools/fl-island-compile.fl. Supply the approved
runtime/dependency environment and an existing output directory.

~~~sh
scripts/fl-island-compile-native web/islands/products.flx output-directory
~~~

The entry creates a fresh private artifact child directory, parses the client
through the Native compiler, emits FreeLang browser source and a manifest,
then invokes --browser-js. The generated module.js is a build artifact, not
an authored JS source. The path is printed for inspecting logs and manifest.
Failure preserves diagnostic artifacts and returns the failing status.
Missing tools/inputs fail explicitly; no binary search or Node fallback occurs.

The client file must contain only island forms. The normalizer consumes
freelang.ast.v1; it does not execute the client or follow its load directives.
The only generated load is the known FreeLang support module
web/runtime/island-language.fl. Existing tests, expected values, time limits,
protected entries and JS files are preserved.

## Source transformation

tools/front-island.fl renames local bindings and parameters. Local functions
such as load become lexical function calls rather than compiler load forms.
Per-scope cells allow deferred closures to resolve later initialized local
functions and preserve tested outer shadowing. Top-level atom bindings retain
their state across the generated render callback. Core if/do syntax remains
syntax even when a local binding uses that spelling. This is a tested subset,
not a proof of all lexical/state corner cases.

DOM construction uses the FreeLang island-h/island-mount boundary in FX.
Text is represented as text nodes. Nested state changes requested during a
render are coalesced. The existing browser-* restrictions are unchanged.
Additional HTTP, DOM, string, store/HMR and browser host APIs remain pending.

Source string emission retains quotes, backslashes, controls, NUL, Korean and
emoji. Isolated UTF-16 surrogate units use a JSON data expression because
UTF-8 file writing intentionally replaces such units. The file API encoding
policy was not weakened to put invalid UTF-8 into generated source.

Compiler-private names and aliases are rejected before renaming. Unsupported
watch options, loop/recur, definitions/assignment and variadic parameters fail
explicitly. Unsupported operations must not become placeholder functions or
a passing empty application.

Five missing required-attribute values in admin/orders.flx and
admin/products.flx were made explicit. All eight original island files now
pass the Native source parser. This is a source correction; browser form
validation and login behavior are not claimed as tested.

## Verification

| Evidence | Result |
|---|---|
| Native AST command controls and original island files | 18/18, including all 8 real files |
| Generation/boundary/lexical controls | 16/16 |
| String value round trips | 7/7, including isolated surrogate and script-like text |
| Invalid AST records | 3/3 fail before generated program output |
| Native transformation models | 6/6 |
| Real product-list and cart clients | Native browser code generated with the fresh compiler |
| Complete real-client inventory | 2/8 generation PASS; 6 explicit feature gaps |
| Original browser-target generation/reproducibility | 7/7, exact bytes |
| Original browser compiler rejections | 10/10 |
| Native execution traces | 0 Node-family executions, 0 network request syscalls |
| Browser rendering, screenshots and interactions | NOT_RUN |

The six transformation models execute generated FreeLang as Native code
against a functional data-model host. They verify counter updates, local load,
forward closures, builtin-name shadowing, outer shadowing and keyword
shadowing. They do not run a DOM or establish browser runtime behavior.

The initial eight-file inventory found these blockers:

| Client | First blocker |
|---|---|
| admin/orders.flx | form .reset interop |
| admin/products.flx | form .reset interop; further helpers remain unaccepted |
| checkout.flx | RegExp .test interop; further request/navigation APIs remain unaccepted |
| index.flx | unsupported browser operation; exact operation diagnostic needs improvement |
| order/track.flx | unsupported browser operation, including request API |
| products/[id].flx | missing island-attribute/request support |

That matrix is an initial generation inventory, not NATIVE_OK 26/26 and not
runtime success. The two supported original files were reverified with the
latest fixed-point compiler and final frontend source; the six missing
features were not turned into passing expectations.

The first string receipt failed because an isolated surrogate was placed
directly in UTF-8 source. The corrected emitter preserves the original value
as escaped JSON data. The old failed receipt remains; its expected code unit
was not changed to a replacement character. The independent oracle evaluates
only the generated constant JSON expression when checking this representation.

FX source commit: 74a555c4802e886fcadb1d8b61b805defd99599d.
Compiler Stage 2/3/4 exact C SHA-256:
23ce0a1c2c08b0fb2283c2204ae727a4a61e0c4af190d741401da02534bf4466.
Verified compiler ELF SHA-256:
352cac840529249a5fd22e2c127e20467250f5cc55d80bedcdcf71c9189ec0f7.
Native island frontend executable SHA-256:
1de4b25c2ec2a6d8b938cc5763498f5f782b6a668f19b0a040b2c4060e8cc774.
Seed before/after:
0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

Receipts: native-island-frontend.47adq_b9,
native-island-frontend-final.nan8vcch,
native-island-frontend-final-v2.v6zsj0mh, native-island-final-guards.ixuayagr and
native-island-parser-final.cq39336f. Raw logs/models/artifacts are private.
Host paths/owners: <가림>. New authored JS files: 0; JS line changes: 0.

The Browser plugin and Python Playwright are unavailable. Chrome bridge
connections failed; capped private Chrome startup exited 133 without enough
detail to prove its cause. Page identity, DOM/nonblank/overlay checks, console
health, screenshots and physical interaction are NOT_RUN. No browser
dependency was installed and no memory limit was raised.

Full web compilation, store/HMR, diagnostics/fix contracts, dev server,
focus/selection behavior, all real client flows and default migration remain
pending. General binding/state and advanced grammar coverage also remain
incomplete. The DB95 commercial-readiness checkpoint recovery blocker is
unresolved; a full DB 71-case rerun is NOT_RUN.
