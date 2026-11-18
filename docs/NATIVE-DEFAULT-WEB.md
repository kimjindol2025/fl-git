# Native default web command

`scripts/fl-git-web` now builds and starts the production web application
through the existing Native builder and runner. It does not invoke Node,
search for a compiler or fall back to a prebuilt application. The default
command still performs a fresh build before starting the server.

## Configuration

Use the same verified Native SDK described in NATIVE-WEB-BUILDER.md:

| Variable | Required value |
|---|---|
| FREELANG_NATIVE_COMPILER | Verified Native compiler executable |
| FREELANG_NATIVE_WEB_TOOL | Matching tools/fl-front-build.fl executable |
| FREELANG_NATIVE_FLP_TOOL | Matching FLP executable when contracts exist |
| FREELANG_NATIVE_RUNTIME_ARCHIVE | Matching Native runtime archive |
| FREELANG_NATIVE_RUNTIME_INCLUDE | Matching Native runtime header directory |
| FREELANG_NATIVE_WEB_OUTPUT | Optional existing writable output directory outside web/ |

Without an explicit output directory, the command uses TMPDIR, then the
system temporary directory. It canonicalizes the parent, rejects the source
web tree (including a symlink into it), creates a private fresh parent and
retains both successful and failed build artifacts. It prints that parent as
FL_GIT_WEB_ARTIFACTS. Paths containing quotes/spaces are supported; artifact
paths are not extracted from log text. No persistent/global SDK setting is
written. Existing dependency include/link/runtime environment remains needed.

The command checks configuration and the project's FLP requirement before
allocating artifacts. Missing/non-executable tools or invalid configuration
exit 2 with FL_GIT_WEB=BLOCKED. A failed builder's exit code propagates and
prevents server startup. A successful build must supply exactly one verified
completion receipt and executable bundle. The command then execs the Native
runner using that fresh bundle; FREELANG_NATIVE_WEB_APP does not bypass the
build. The separate fl-web-run-native entry remains available for verified
prebuilt bundles.

The process lowers an unlimited/higher address-space limit to 2 GiB,
preserves a smaller inherited limit and disables core files. It records a
process-name snapshot, then uses the shared heavy flock with descriptor
closure for the build. The server does not retain the build lock. Do not
wrap this entry in a second acquisition of that same lock. It can wait for
another job holding the lock; there is no forced unlock.

FL_HTTP_HOST must be 127.0.0.1. PORT defaults to 40850; decimal 0 through
65535 is accepted and normalized, with 0 requesting an ephemeral port.
FL_GIT_REPO and FL_GIT_ROOTS overrides are retained by the Native runner;
otherwise they use this checkout's root. No deployment is performed.

## Scoped acceptance

Private receipt: native-default-web.nok8manc. Host paths/owners: <가림>.

| Observation | Result |
|---|---|
| Shell syntax / git diff --check | Exit 0 |
| Missing/bad SDK, FLP, host, port and output configuration | 17 cases, exit 2, no artifacts or server |
| Actual default entry: build then loopback server | Build exit 0; 87.164 seconds including tracing |
| App ELF SHA-256 | 481f8ad66b861edce6892e02740fb4e019bbe3a5d57a638671a53c99765bb032 |
| App bytes versus prior accepted Native V12 | Identical |
| HTTP / runtime observations | 19, including launch plus pages, memory cart, Git reads, approval and UTC audit |
| Real local Orca browser | 8 route mounts; actual cart click from 2 to 3 |
| Real client-parsed fixture products | 3; value and name UTF-8 byte differences 0 |
| Script-like name / console | No injected script element; console entries 0 |
| Browser network capture | 32 same-origin 200 requests; external hosts 0; order POSTs 0 |
| Trace: Node-family executions / connect calls | 0 / 0 |
| Server address-space / core limit | 2 GiB / 0 |
| Owned build-lock descriptors after startup | 0 |
| Owned/traced processes after cleanup | 0 |
| Tracked source and git state during diagnostics / business fixture files | Unchanged |

The application SHA matches prior real browser validation, so this launcher
change did not alter generated application bytes. The current FX compiler
SHA is c8c6a0e691ddec4c4a9e0626d2b7c17d0d7f0450c40fb1847f4c1b13ff854f67.
Seed bootstrap/stage0.c remains
0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

The original private end-to-end diagnostic exited 1 after successful browser
verification and process cleanup: it incorrectly treated ordinary HTTP
response sendto calls as outbound connections. Its source/log/exit remain
preserved. Read-only trace reanalysis exited 0 and classified all 112 sendto
calls as having NULL destinations; connect, sendmsg and explicit-destination
sendto calls were 0. The first reanalysis missed strace's unfinished-call
format and also failed; the final parser handles that format. No product
source, repository test, expected value or protected runner was changed to
obtain the reanalysis result. This is scoped evidence, not an original
diagnostic exit-0 claim.

Additional private generic-source tests reached the shared lock while
another job held it and timed out at their unchanged 120-second bound.
The Native builder was never entered; owned processes were cleaned up.
Generic source-error propagation and this SDK's generic no-FLP execution
with a smaller inherited limit are NOT_RUN in that receipt. No lock was
forced, timeout raised or retry attempted. The earlier generic builder
receipt predates the current runtime SDK. Shellcheck is unavailable;
installation was not attempted.

## Remaining work

The development/HMR entry and .freelang/deploy.sh still invoke Node, as do
other default checker/test/CLI paths listed in NATIVE-EXIT-STATUS.md. Full
diagnostic/fix/contracts, advanced framework/TTY/provider behavior, admin
mutations, complete Git UI writes and order submission/storage are not
accepted by this scope. Existing JavaScript and tests are preserved; new
authored JS files and JS line growth are 0. No customer DB is connected.

FX source is unchanged by this launcher task. DB95 full validation and
checkpoint fault recovery remain unresolved; commercial readiness HOLD,
deployment NO. The existing private curl dependency still needs approved
distribution packaging.
