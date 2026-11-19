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
initial Native development-service/watcher tools. The matching headers, archive
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
