# Native common validation and SDK bundle

scripts/check.sh and scripts/provider-gate now delegate to Native entries.
The common check retains its original sequence: language-check git/main/tui,
CLI help, language-check/run smoke, the existing test launcher, then Provider
fixtures and CLI gate. Each formerly bounded common-check command keeps its
15-second limit. The existing test launcher keeps its own limits. Provider
PASS/DENY markers and fixture assertions are unchanged. There is no Node
fallback or automatic compiler/tool search.

scripts/fl-run-native compiles and links a FreeLang source with the explicitly
selected matching compiler, runtime archive and headers, then runs it with the
original arguments and working directory. It retains a caller-selected artifact
directory or cleans its own temporary directory. Failed generation/link/run
returns nonzero. It does not replace or modify the existing test launcher.

## Packaging

Configure the accepted compiler, its fixed-point generated C, matching runtime
archive/complete headers and curl library. Then run:

~~~sh
export FREELANG_NATIVE_COMPILER=<verified-compiler>
export FREELANG_NATIVE_COMPILER_C=<matching-fixedpoint-stage3.c>
export FREELANG_NATIVE_RUNTIME_ARCHIVE=<matching-runtime.a>
export FREELANG_NATIVE_RUNTIME_INCLUDE=<matching-runtime-directory>
export FREELANG_NATIVE_CURL_LIBRARY=<matching-libcurl.so>
flock -o /tmp/fl-heavy.lock ./scripts/fl-sdk-package-native <existing-output-directory>
~~~

The command builds a new fl-sdk directory without replacing an existing one.
It bundles the compiler, CLI/TUI, page checker, FLP/web/style/island tools and
Native development-service/watcher/control tools. The matching headers, archive
and curl shared library accompany them. Build/generated-C logs and input/source
hashes remain available. Memory is capped at 2 GiB. No package is downloaded or
installed and no global configuration is written.

The compiler is relinked from the exact accepted generated C. Compiler and
language tools use a relative $ORIGIN/../lib RUNPATH. The compiler C is preserved
in provenance/compiler.c. SDK.SHA256 records all distributed executables, headers,
libraries, activation code and compiler C. verify.sh checks that manifest.

~~~sh
<new-sdk>/verify.sh
source <new-sdk>/env.sh
./scripts/check.sh
./scripts/provider-gate
./scripts/fl-check
~~~

Activation resolves paths from the current SDK directory. Source env.sh again
after moving it. The SDK is specific to the recorded Linux host/toolchain/ABI;
it is not a universal binary distribution. The host C toolchain, libc, OpenSSL,
SQLite and curl's system dependencies still need to be available. The bundled
curl resolves from the moved SDK rather than the private build prefix.

## Evidence

Paths/owners: <가림>. Receipt: native-sse-firstclass-20261120.

| Scope | Result |
|---|---|
| Fresh seed-to-Stage4 bootstrap and core SDK regressions | PASS; see FX NATIVE-SSE.md |
| Native common check before default switch | Exit 0, original ordered stages and bounds |
| SDK package and all distributed hashes | PASS |
| SDK moved to a path containing whitespace, apostrophe, Korean and LF | Manifest, configuration and real tools pass |
| Dynamic loader library selection after move | Included curl selected from the relocated SDK |
| Relinked SDK compiler self-generation | Exact canonical generated C; 27.82 s, RSS 1539444 KiB, cap 2 GiB |
| Common check through the changed default entry after move | Exit 0 |
| Working-tree unit cohort in that common check | 8 suites, 80/80; includes the preserved separate Git-status suite |
| Original tracked unit cohort in canonical core validation | 7 suites, 71/71; unchanged assertions |
| Default Provider entry after move | Exit 0; all required PASS/DENY markers |
| Default page checker after move | Exit 0 |
| Manifest control | Owned header edit detected; restored bytes verify again |
| Node-family execution in package/common/Provider/page traces | 0 |
| New handwritten JS, protected test/runner/expectation/time-bound edits | 0 |

The first package copied the compiler executable unchanged. Its DT_RPATH chose
the old private curl directory ahead of LD_LIBRARY_PATH, so the relocation
assertion correctly failed. The failed artifact and loader output are retained.
Relinking the exact canonical C with an explicit relative RUNPATH fixes the
cause; the original relocated-library assertion was retained. New tool binaries
also use the relative RUNPATH. Compiler seed and generated C were not rewritten.

SDK compiler ELF SHA-256:
170aa919713a8cbdfa8bfea1fdad47ae839ce40162643120fb291c3f480cbcbd.
Canonical generated C SHA-256:
d1041a745a1bffbd2d72a2787da14063b4fdb976f61e09a1ddca7176fad866b9.
The package's manifest hash is
 aee83d2e16f229eee30304048bf5dbc90faf1da4c53e8e02bfb3b57f41cf0acb.

Native development-service APIs (83/83) and source watch contracts (23/23) are
scoped prerequisites; see NATIVE-DEV-CONTRACT.md. Development supervision,
CSS/browser HMR, state restoration and headless island execution are not
complete, so the default fl-dev remains unchanged. Deployment still has a Node
path and has not run. Whole-app/browser acceptance and DB commercial
qualification remain open. Commercial status is HOLD, overall goal active.
No production service, foreign worktree changes or customer data were changed.

The subsequent [source-fix checkpoint](NATIVE-FIX-PLANS.md) adds the Native
`fl-source-transaction` file host and exports `FREELANG_NATIVE_FIX_HOST` alongside
the FreeLang fix tool. Its SDK has 14 Native executables. Explicit source
application and interruption recovery retain private backups; they do not
establish application, DB or production acceptance. The matching
[receipt](evidence/native-fix-apply-20261120.json) records the compiler/runtime
inputs, complete package manifest and separate qualification results.

## Current coherent checkpoint

The subsequent native-dev-coherent-20261120 SDK includes the validated SSE task
handoff, ordinary HTTP idle shutdown, shared browser state/HMR, development style
handling and structural delimiter rejection. [Exact evidence](evidence/native-dev-coherent-20261120.json)
keeps its hashes and scope distinct from the older package above.

Fresh seed-to-Stage4 has exact Stage2=3=4 C. Generation takes 29.88/29.31 seconds
with RSS 1545824/1545804 KiB under the unchanged bounds. Original core, CLI,
checker, SSE/API/function/C-route regressions pass. The tracked cohort is 71/71;
the actual working-tree common check separately passes all 80 assertions across
eight suites, plus its Provider gate recheck. Foreign changes remain unchanged.

The package again passes relocation to a path containing whitespace, apostrophe,
Korean and LF, included-curl resolution, real default common/Provider/page
entries, exact compiler self-generation and manifest tamper/restore detection.
SDK self-generation takes 29.76 seconds with RSS 1547120 KiB. Collected product
build/common-check traces contain zero Node-family execution.

Current SDK compiler SHA-256:
a552d2d516df9d398845a12d7551b6e3132d73e214161609c78737e05e7951b0.
Compiler C SHA-256:
26f07fec1123c4b7bd396b68fd6272a7d9882e513e70f747d03fe206fe13d2c0.
SDK manifest SHA-256:
4765ee3e97b5cb843a3cacc23bbf02efd3c3f9e5b948b431e23a5b2930370b1e.

The packaged tools pass additional style 24/24, watcher 23/23, delimiter 65/65,
HTTP lifecycle 56/56 and SSE capacity 34/34 controls. A newly built development
bundle passes 23 actual Chrome functional controls; screenshots and pointer
hit-testing remain unverified. One open stream 180 seconds passes 41 controls,
with warm RSS growth 216 KiB and open FD growth 0; no commercial lifetime bound
is inferred. Native supervision, complete diagnostics/metadata, headless and
deployment migration and actual application acceptance remain unfinished.
The default development entry still requires its complete migration. Overall
goal active, commercial HOLD, deployment NO.

The subsequent [Native supervisor milestone](NATIVE-DEV-SUPERVISOR.md) adds
`fl-dev-control` and its activation variable to the SDK. The notification host
also exposes a monotonic clock for the unchanged readiness/stop bounds. Its
separate evidence records the rebuilt package and relocated execution. This
does not replace the earlier complete compiler/runtime qualification or imply
headless, complete metadata, deployment or application acceptance.

The next [source metadata checkpoint](NATIVE-FRONT-METADATA.md) includes catalog
data, contract readiness and store declarations in the web/control tools. Its
evidence distinguishes exact legacy-data comparisons, standalone island output
parity and actual automatic browser reload controls from the still-pending
static/headless/deployment requirements. The compiler/runtime pair is reused
from the accepted coherent checkpoint; no new whole DB result is claimed.

The next [static diagnostic checkpoint](NATIVE-STATIC-DIAGNOSTICS.md) adds Native
AST checks and SDK inventory metadata without changing the compiler/runtime.
Package provenance additionally covers the FreeLang server and island runtime
helpers. Its evidence separates declared operation/ABI counts, private legacy
reference execution, Native product traces and the remaining acceptance work.

The [Native JavaScript engine foundation](NATIVE-JS-ENGINE.md) adds an explicit
SpiderMonkey/GJS process host and optional matching library inputs. Its package
has 15 Native executables and records a fresh compiler fixed point while reusing
the unchanged accepted runtime archive. This enables generated FreeLang runtime
modules; it does not yet implement Island headless behavior or `/~ai/test`.
