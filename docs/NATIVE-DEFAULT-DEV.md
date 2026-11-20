# Native default development entry

`scripts/fl-dev` delegates to `scripts/fl-dev-native`. It requires an explicitly
configured matching Native SDK and has no Node fallback. The same Native
supervisor owns source notifications, preparation, test gates, builds,
application readiness, CSS publication, HMR, diagnostics, previous-good retention
and shutdown. Existing reference JavaScript files and protected tests remain
unchanged.

Activate the verified SDK, then run:

~~~sh
source <verified-sdk>/env.sh
./scripts/fl-dev [web-root] [--artifacts existing-directory]
~~~

Without a web argument it uses the repository's `web` directory. The application
and HMR defaults remain ports 40850 and 40851. `PORT` and `FL_HMR_PORT` select
different valid ports; the bind address remains loopback. `FL_GIT_REPO` defaults
to the physical parent of the selected web directory. Configure actual Git roots
and Provider permissions for the intended workspace before using those features.
This command does not install or change a global service.

The Native supervisor and web-build host now carry canonical directory names
through NUL-terminated reads. Physical symlink resolution is retained while
apostrophes, Korean, embedded newlines and trailing newlines survive in the
installation, source, artifact and default repository paths. An original
trailing-newline failure is retained. The existing CLI, failure and stop statuses
remain meaningful; invalid configuration blocks before a service is started.

The `native-http-process-cancel-20261120` runtime also fixes an observed shutdown
failure when `shell_capture` is waiting for an external command in its own group.
An HTTP stop requests cancellation of that bounded command, preserves a truthful
failure result and lets its handler finish. The normal command budget stays
30 seconds. The unbounded legacy operation and abrupt owner death remain outside
this cancellation proof.

Qualification includes the original four-build lifecycle with failing test-only
edits, last-good application retention, repair and stale cache rejection; actual
builds through literal and symlink directory arguments; preflight rejection;
shutdown during an owned pending command; and the full actual application in
Chrome. The small lifecycle/path cohorts retain their 90-second build deadline.
The full actual input retains its separate 300-second whole-build deadline.
Application/HMR readiness and owned shutdown retain three-second bounds.

The actual local Git fixture uses a clean environment and an explicit host-tool
path without account CLIs or Node. This verifies local Git and development
behavior; authenticated Provider discovery and remote Push/Pull still require
their own qualification. Existing foreign application edits are copied verbatim
into the private actual input and excluded from the implementation commit and
SDK source. Earlier inherited-environment and private harness failures remain
recorded in the [evidence](evidence/native-default-dev-20261120.json).

The `.freelang/deploy.sh` and `web/.projectrc.json` deployment/start commands
still contain Node/PM2 paths. Deployment migration, full DSL parity, remote Git
UI, abrupt process-owner death, visual/pointer acceptance and latest whole DB95
safety/operations remain open. Goal ACTIVE; commercial HOLD; production NOT_RUN.
Private paths and owners: <가림>.
