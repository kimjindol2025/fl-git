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

The subsequent [headless standalone entry](NATIVE-ISLAND-HEADLESS.md) packages
16 Native executables, its fixed Bash process boundary and the FreeLang headless
runtime. It requires the fresh matching runtime archive with the generic raw
function/receiver/argument/remainder primitives; the preceding archive must not
be mixed with this compiler. API/test-gate/default-dev migration remains open.
## Actual application and matching HTTP runtime

The `native-http-pressure-20261120` checkpoint rebuilds all runtime objects,
the unchanged seed through Stage4, the SDK and the full actual application.
The SDK manifest SHA-256 is
`e5e3b82e55cb298f967c09cd703ffe8bb930866266a7da80b73eb1b0e35b2468`;
runtime archive SHA-256 is
`0af1c706a52c01f7c6da4fc8855ce8b8fbaf4634978252986a6d4c3cb84bdf2d`.
All 16 ELF tools pass relocation/manifest checks. The separately relinked SDK
compiler produces the canonical C in 30.81 seconds with 1,559,604 KiB peak RSS,
within the existing 40-second/1.6-GiB gates and 2-GiB address-space cap.

The final actual app passes 25 Chrome functional controls with local real Git
Stage/Commit, computed CSS HMR and SSE reload. See [the app contract](NATIVE-APP-HMR.md)
and [exact evidence](evidence/native-actual-app-hmr-20261120.json). Existing
foreign app changes were copied verbatim into the private actual input and are
excluded from the SDK source and implementation commit. Authenticated remote
Git UI, screenshot/pointer acceptance, full DSL, default development/deployment
and latest whole DB95 acceptance remain open. Goal ACTIVE; commercial HOLD.

## Default development and bounded process shutdown

The `native-http-process-cancel-20261120` checkpoint now qualifies
`scripts/fl-dev` as the Native default entry, including actual application,
Chrome, Git, HMR, special paths and orderly shutdown of a pending bounded
command. See [the default entry](NATIVE-DEFAULT-DEV.md) and its
[evidence](evidence/native-default-dev-20261120.json).

The current 16-ELF SDK manifest SHA-256 is
`9c0050409f93e1d69411e73cb909a67d3fde875e91c737940b7d9c36155662cd`;
runtime archive SHA-256 is
`053971a3403cfdf0b1c15a0efa8e76a9eb5e5325df0fb4955279abe1bb6c6be4`.
Seed-to-Stage4 retains the canonical generated C. Relocated self-generation
takes 31.97 seconds and 1,556,608 KiB RSS within the unchanged 40-second,
1.6-GiB RSS and 2-GiB address-space gates. Existing reference files, protected
tests and four foreign source edits are preserved. Deployment/start commands
in `.freelang/deploy.sh` and `web/.projectrc.json` still await migration.
Full goal ACTIVE; commercial HOLD; production NOT_RUN.

## Native release preparation

The [Native release checkpoint](NATIVE-RELEASE.md) adds the seventeenth ELF,
`fl-front-release`, and the fixed readiness helper. Activation exports
`FREELANG_NATIVE_SDK_ROOT` and `FREELANG_NATIVE_RELEASE_TOOL`. Packaging,
activation and verification now preserve an SDK or source root ending in LF.
The final SDK manifest SHA-256 is
`e1f22e75e3a25b662411dc42d23e47049c31c0f5b80a75e4d3b4d75ee8708181`.
The runtime archive remains
`053971a3403cfdf0b1c15a0efa8e76a9eb5e5325df0fb4955279abe1bb6c6be4`;
canonical compiler C remains
`a562273ff556348581688d27f780dfb7486dcac572d671a17ace04715c8808e5`.
Fifteen R24 executables are byte-identical, the web tool gains explicit release
readiness mode, and the release policy tool is new. All 17 final executables
and env/verifier/readiness helpers are byte-identical to the tested candidate.

Final public release 45/45, foreground defaults 7/7 and actual relocated
production Chrome/local Git 20/20 pass. Existing development 13/16/17 and release
mode 8 controls pass on identical candidate tools/used hosts. The
[evidence](evidence/native-release-prepare-20261120.json) distinguishes reused
runtime evidence, real final builds and unexecuted service activation. The
foreign Git API's trailing/internal-LF repository-basename failures remain
unresolved; the original R24 browser path domain still passes. Full DSL, remote
Git UI, service activation and latest whole DB95 qualification remain open.
Goal ACTIVE; commercial HOLD; production NOT_RUN.
