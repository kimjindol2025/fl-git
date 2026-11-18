# Native browser verification

STATUS=PASS for the bounded eight-view/cart/data checks below. Node removal
and DB commercial readiness remain unfinished. Final receipts are
native-web-build-v12.kttzqx4l, native-web-runtime-v12.qfbma2qq and
native-orca-web-v4.u5o8hbnf. The application ELF SHA-256 is
481f8ad66b861edce6892e02740fb4e019bbe3a5d57a638671a53c99765bb032.
The matching FX checkpoint is c2752e1dfb2aa486b766bbef34bfd91c59984b20;
its fresh bootstrap/core evidence is in FX NATIVE-JSON-STATUS.md.

| Real browser observation | Result |
|---|---|
| Client parses three dangerous-text product fixtures | Value differences 0; UTF-8 name-byte differences 0 |
| Injected UNTRUSTED script element | Absent |
| Actual cart button clicks | Visible count 1 then 2 |
| Server cart after clicks | Count 2; total 2000 |
| Eight route islands | Mounted, including Git root with fixture data |
| Root favicon under original CSP | Same-origin SVG, 200; policy unchanged |
| Navigation console errors / captured console entries | 0 / 0 |
| Browser network capture | 78 entries, all 200; external hosts 0 |
| Mobile cart width / horizontal overflow | 390 px / 0 |
| Order creation POSTs | 0 |
| Current unchanged unit assertions through Native entry | 71/71; Node execution 0 |
| Native build / HTTP probe | Both exit 0; 19 HTTP rows |
| Final owned server | Stop requested within unchanged ten-minute lease; exit 0 |
| Owned processes after cleanup | 0 |
| Source and business fixture files before/after | Identical |
| Server Node executions / outbound connects | 0 / 0 |

The browser receipt has 14 observation rows, including the eight views; it
is not the protected API suite or a complete feature-count claim. Routes:
/, /products, /products/1, /cart, /checkout, /order/track, /admin/products and
/admin/orders. Admin views are mounted without login or mutations.

## Data and event evidence

The actual Native product page and generated ES module consume three fixture
products containing quotes, backslashes, <, >, &, Korean, emoji and a
script-like name. The live client renders 3개. A temporary in-memory
JSON.parse observer records the actual client parse during reimport of the
unmodified, same-origin module. It returns the original parse value and is
restored in finally. All values equal the server fixture and UTF-8 name bytes
have zero differences. The UNTRUSTED script element is absent. This is real
browser execution, separate from earlier HTML-parser and Native host-model
evidence. The observer is diagnostic tooling, not application JS source.

Two actual clicks then display 1 and 2 and match the independently read
synthetic server cart. The cart route displays quantity 2 and total 2000.
The Git root receives the fixture repository data. Desktop/mobile cart
screenshots were visually inspected; the mobile view has no horizontal
scroll overflow. SSR header counts refresh on navigation; a general shared
header/store update contract is not established by this widget check.

## Product corrections and preserved failures

The first real cart click updated the server but left an empty widget count.
The callback unwrapped res.data although fetch-post! delivers the parsed
body directly. The existing Node reference uses the same raw-body convention.
The application now reads count from that body; no framework request API was
changed to accommodate the incorrect caller.

Editing only the paired .flx source did not affect the executed module,
because FLP generation overwrote it with the same incorrect stencil.
Both detail stencils in tools/flp-compile.fl and the paired client are now
corrected. Before browser acceptance, private checks require generated and
intended client bytes to match and the module hash to differ from the failed
artifact. The checked-in source and actual generated client now agree.

The Git root's data: favicon caused a CSP console error. Its source now uses
the existing /public/product-default.svg resource. The security policy was
not relaxed. Final root navigation and the SVG request pass without errors.

Retained earlier receipts:

- native-orca-web.qhxpd7lh: product/data checks passed; cart count failed.
- native-orca-web-v2.vwynkdvm: raw-source edit was overwritten by FLP generation.
- native-orca-web-v3.lm9_rb4p: corrected cart and eight mounts; root CSP error.
  Its unchanged server lease expired before the stop request, exit 1, with
  zero owned processes and unchanged source/business files after cleanup.
- native-orca-web-v4.u5o8hbnf: final bounded checks pass; requested cleanup exits 0.

The expired lease remains a failed receipt; its limit was not increased and
it is not merged into the final successful run. The old widget screenshot
visibly records the empty count.

## Artifacts and isolation

Screenshot SHA-256s:

- before-cart.png: 23e4145fb500e382207ef34c3ae7cb685c49bf15496e4419ae257503c72b6a0a
- final-desktop.png (1479 x 947): 9282318fd6c5cac4668ae6c6343b8f5b1219256b4505da7a2da5dfc791bafd4f
- final-mobile.png (390 x 844): f87b80f6a919e75ffc6c753ad2ba8a12954df4a5eb8c5f8213120d1b75933df7

The existing local Orca embedded browser was used after registering the
correct isolated frontend checkout. Registration changed Orca metadata only;
source state was checked identical immediately before/after. Earlier
standalone Chrome/Firefox failures remain recorded. No browser download,
security-flag change or memory-limit increase was used. Compiler and owned
Native server commands retain the two-GiB cap; the existing IDE browser's
configuration was not changed. Orca's CLI/browser reference guides were read.

Git/product data is synthetic. No customer database, external provider,
external AI/mail, order submission or order storage was exercised. Existing
repository tests, runners, expected values, timeouts and seed were unchanged.
New authored JS files and JS source-line changes are zero. Build traces have
zero Node execution/network syscalls; owned-server traces have zero Node
execution/outbound connects. The final server is stopped and the owned
browser tab is blank.

## Remaining acceptance

Admin login/mutations, remote provider operations, the complete Git UI write
workflow and browser corner cases remain NOT_RUN. Eight mounted views do not
prove every application feature. Default Node command migration, dev/HMR,
advanced framework/store behavior, TTY Unicode and production packaging remain
pending. Full DB95 acceptance and checkpoint fault recovery remain unfinished;
commercial readiness HOLD, deployment NO. Host paths and owners: <가림>.
