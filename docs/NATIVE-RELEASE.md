# Native release preparation

The deployment entry now builds Native code and prepares a separate user-service
plan. The rollback entry selects a previous sealed release explicitly. Successful
preparation reports `PREPARED` and `ACTIVATION=NOT_RUN`; it does not restart or
install a service. The old PM2 restart path is removed from these entries.

## Prepare and inspect

Verify and activate the matching SDK in Bash, then choose an existing artifact
parent outside the source web directory:

```sh
<sdk>/verify.sh
source <sdk>/env.sh
./scripts/fl-release-prepare-native ./web --artifacts <existing-directory> \
  --repo <repository-directory> --roots <allowed-roots> --name fl-git --port 40850
```

The SDK exports `FREELANG_NATIVE_SDK_ROOT` and `FREELANG_NATIVE_RELEASE_TOOL`.
Every configured compiler, web/headless/FLP/style/island tool, runtime archive and
header directory must belong to that SDK. There is no automatic tool search or
Node fallback. The public entry holds `/tmp/fl-heavy.lock` while building;
the internal build host expects its caller to hold that lock. Memory remains
capped at 2 GiB and linking retains the 180-second bound.

The private preparation directory retains build diagnostics, `code/`,
`service/plan.json`, the generated `.service` file, and a NUL-terminated
`release-location.nul`. A failed build returns nonzero and cannot produce a
completed release or service-plan receipt. Production preparation clears the
development headless-index publication target and selects release mode explicitly.

`code/release.json` records relative file paths, byte sizes and SHA-256 digests,
the directory inventory, SDK manifest digest and runtime archive digest. The
Native verifier rejects changed/missing/extra files, extra directories and
symlink aliases. The service plan pins the selected release-manifest digest.
This detects drift in an owned release; it is not a signature or isolation
against an arbitrary writer able to replace both code and its trust anchor.

The code directory contains the Native app, public assets, its copied curl
library, verifier, monotonic clock/readiness helpers and build/ELF provenance.
The app is relinked with `$ORIGIN/../lib` RUNPATH. Launch plans unset
`LD_LIBRARY_PATH`, so an SDK library path is not inherited by host Git/curl.
The recorded platform's libc, OpenSSL, SQLite and other system dependencies
remain required. This is not a universal binary distribution.

Code can be copied to another directory and verified there. Prepare a new service
plan after relocation; the old plan intentionally contains the old absolute
location. Service configuration is kept outside the code inventory.

```sh
"$FREELANG_NATIVE_RELEASE_TOOL" --verify <selected-code-directory>
./scripts/fl-release-prepare-native --from-release <selected-code-directory> \
  --artifacts <existing-directory> --repo <repository-directory>
```

## Foreground execution and rollback selection

```sh
./scripts/fl-release-run-native <selected-code-directory>
./.freelang/rollback.sh <previous-code-directory> --artifacts <existing-directory>
```

Foreground execution also accepts `FREELANG_NATIVE_RELEASE`. It requires an
explicit trusted `FREELANG_NATIVE_RELEASE_TOOL`, verifies the release, obtains a
complete NUL argv plan and execs it literally. The default role is `developer`;
`FL_GIT_REPO`, `FL_GIT_ROOTS`, `FL_GIT_ROLE`, `FL_GIT_PORT`/`PORT` and
`FL_RELEASE_HOST_PATH` select the runtime configuration. The default host PATH is
`/usr/bin:/bin`. Binding remains `127.0.0.1`.

Rollback prepares a plan pointing to the selected previous code. It neither
undoes repository/data writes nor restarts the current artifact under a rollback
label. No service installation, enable/start/restart/reload or production change
is part of these commands. Generic deployment tooling must inspect the explicit
`ACTIVATION=NOT_RUN` result; preparation success is not deployment success.

## Readiness and supervision plan

Release builds reserve `/~native/ready`, return JSON `ok: true` and identify the
current invocation in `x-fl-native-instance`. Foreground runs create a fresh
128-bit identifier. A systemd activation uses its own `INVOCATION_ID`, which is
unique to that activation. A previous listener with another identifier cannot
satisfy the readiness check. See [systemd execution environment](https://www.freedesktop.org/software/systemd/man/latest/systemd.exec.html).

The copied readiness helper observes a 3,000-ms aggregate monotonic deadline,
checks HTTP 200, typed JSON and the exact invocation identifier, and checks
`MAINPID` when supplied. Failed observation and its cleanup return nonzero;
cleanup time is measured separately from the observation budget. Raw HTTP
headers/body are removed; only a constant failure report remains.

The prepared user unit has `Type=exec`, automatic restart, the former 8-second
startup upper bound, 3-second stop bound, a 2-GiB address-space cap, disabled core dumps and
`KillMode=mixed`. The latter sends TERM to the main process and later kills
remaining processes in its cgroup. These are prepared manager semantics, not a
claim of tested service activation. See [service options](https://www.freedesktop.org/software/systemd/man/latest/systemd.service.html)
and [kill options](https://www.freedesktop.org/software/systemd/man/latest/systemd.kill.html).

Unit commands use C string quoting, double percent specifiers and disable dollar
substitution with the command prefix. Literal paths are argv to `env -C`;
nonprintable paths are not put in `Environment=` assignments. See
[unit syntax](https://www.freedesktop.org/software/systemd/man/latest/systemd.syntax.html).
The installed systemd 257 parser checks generated user units with
`systemd-analyze --user --man=no --generators=no verify`, an owned runtime
directory and a clean environment. This verifies configuration, not startup;
the verifier may contact a system bus. See [the installed-version verifier](https://www.freedesktop.org/software/systemd/man/257/systemd-analyze.html).

## Qualification scope

The [machine-readable evidence](evidence/native-release-prepare-20261120.json)
pins the 17-ELF SDK and final actual release. Final public release controls pass
45/45; foreground/default-path controls pass 7/7; actual relocated production
Chrome/local Git passes 20/20. Candidate release-mode 8/8 and original development
13/13, 16/16 and 17/17 are carried by exact final/candidate executable, helper and
used-host identity. The final actual app is freshly built with the final SDK.
The complete runtime and 15 unchanged executables retain their scoped R24
qualification; no new compiler bootstrap or whole DB pass is claimed.

All 66 collected product traces are decoded: 11,957 exec entries, zero Node
family and zero service-manager CLI executions. The final browser runtime has
zero account CLI executions. Existing credentials and remote providers are not
used by these local fixtures. One private 90-second observation included a
queued build and timed out; that failed attempt and its later, outside-window
preparation remain retained. Running the final cohort sequentially preserves
the same 90-second and 3-second limits and passes.

The evidence separates Native release preparation, actual relocated production
app/browser/local Git, retained development HMR tests and still-unrun service
activation. Literal Native argv, artifact/source/SDK paths include spaces,
apostrophes, Korean, percent, dollar, embedded LF and trailing LF.

The existing foreign Git API has two additional failing cases: it trims a
repository basename ending in LF, and repository discovery splits a basename
containing LF. Those failures are retained and unresolved; the four foreign
files remain byte-for-byte unchanged. The passing production Chrome cohort uses
the original R24 domain: a special-character/LF parent, basename `sample`, and
an unselected filename containing apostrophe/Korean/LF. It does not qualify
those extended repository-basename cases.

Full DSL compatibility, authenticated remote Git UI, screenshot/pointer
acceptance, actual user-service activation and the latest whole DB95 safety,
recovery, concurrency, load and operational qualification remain open.
Goal ACTIVE; commercial HOLD; production NOT_RUN.
