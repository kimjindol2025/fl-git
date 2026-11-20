# Native TCP helper packaging and release invocation

SCOPED_PACKAGING=PASS; COMMERCIAL_READINESS=HOLD; ACTIVATION=NO.

The SDK packager builds `fl-tcp-request-native` from the matching FX runtime
source and archive. Configure `FREELANG_NATIVE_TCP_REQUEST_SOURCE` explicitly
when its source is outside the selected runtime source directory. The helper,
its retained C source and activation paths are included in `SDK.SHA256`.
Activation exports both the executable path and the retained source path;
source `env.sh` again after moving the SDK. The package now contains 18 ELFs.

Release staging copies an available SDK helper into `code/bin`, marks it
executable before sealing and includes it in the normal release inventory.
Foreground and service plans select that copy through literal argv. Older
SDKs/releases without a helper retain their previous plan. The foreground host
accepts exactly the original 17 NUL records or 18 records with one helper
assignment matching the same sealed application's path. It never evaluates
these records as shell text. FreeLang retains plan and content policy; Bash
handles fixed process, file and permission operations.

The corrected package reuses the unchanged qualified runtime archive,
bootstrap seed and fixed-point compiler C. No runtime or seed regeneration is
performed to repair frontend argv composition. Its runtime and helper ELF are
byte-identical to the first R35 package. The subsequent foreground host fix
changes an unpackaged Bash launcher; SDK binaries remain unchanged.

All 45 original release controls pass at the existing 90-second small-build,
three-second readiness and three-second stop bounds. The successful first 36
checks are reused from the same SDK and unchanged preparation sources; the
remaining nine original assertions run against the corrected launcher. The
original controller fragments, fixtures and bounds stay unchanged. Earlier
missing-program and record-count failures remain separate retained history.

The checks cover fresh preparation, complete NUL locators, sealed inventories,
relocation, content/alias rejection, rollback plans, installed systemd syntax
parsing, entry guards, failed builds, literal runtime configuration, readiness
identity and normal foreground shutdown. `systemd-analyze` only parses an
owned prepared unit; no service manager is changed or activated.

Five additional controls verify that the copied helper is executable and
sealed, transports exact UTF-8/NUL bytes after relocation without SDK/library
environment, fails verification after tampering, and preserves an older
17-record release's three-second readiness and normal stop. All product
execution traces in these cohorts contain zero Node-family/Python runs.
Private Python controllers remain outside those product traces.

The four existing foreign frontend files remain untouched and uncommitted.
These scoped release checks do not establish full DSL compatibility, a new
browser/UI qualification, remote-provider authentication, DB operational
readiness or production deployment. Detailed identities and audits are in
[`evidence/native-tcp-helper-packaging-20261121.json`](evidence/native-tcp-helper-packaging-20261121.json).
