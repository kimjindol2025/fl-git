# Native fl-git CLI entry

scripts/fl-git-native runs a separately verified Native executable built from
src/main.fl. It preserves the current working directory and forwards command
arguments without interpreting them. The existing scripts/fl-git entry is
preserved while the remaining execution paths are migrated.

```sh
export FREELANG_NATIVE_FLGIT=<verified-native-cli-executable>
./scripts/fl-git-native --help
./scripts/fl-git-native status
./scripts/fl-git-native add "file with spaces.txt"
```

Provide the matching runtime library environment from the verified build.
The launcher does not inject private library paths, search for binaries or
invoke Node. Missing/non-executable tools fail with exit 2. Native process
failures are preserved. A successful process must emit the existing
FL_GIT_EXIT:<status> marker with a status in 0..255; the launcher removes the
marker and returns that status. A missing or invalid marker fails with exit 2.

TUI selection requires FREELANG_NATIVE_FLGIT_TUI and the matching verified
FREELANG_NATIVE_FLGIT CLI. The CLI's private terminal-host route supervises
the TUI with interactive terminal descriptors and restores terminal settings
when the child exits. Missing tools fail explicitly with exit 2. Dedicated
Native PTY validation covers raw input, signals, descendant reaping, actual
mouse/keyboard interaction and Enter/menu restoration. The sources retain
all TUI features. Typed Unicode input and Git-quoted paths remain pending.

The Native runtime supplies str-replace-all as the reference's literal
split/join alias of str-replace. This preserves apostrophe quoting in Git
paths and commit messages, including literal replacement text. The context
command now uses the supported doseq loop to print its seven reference cards
in order; the previous doall call had no implementation in the inspected
runtime. Card contents and the JSON context contract are preserved.

Integration verification runs only in a new private fixture repository, with
synthetic local author configuration and no remotes. It covers the CLI
protocol, success/failure status, read commands, add/commit/stage/unstage,
branch operations and their confirmation guard, push denial without --yes,
unknown commands, and text/JSON context output. Paths and messages contain
spaces, apostrophes, Korean and emoji. Actual remote writes, provider auth
are NOT_RUN. The separate terminal receipt below exercises local TUI
interaction. This is distinct from the original 71 unit
cases; none of their tests, expectations or runners changed. New JS files
and JS line changes are zero.

Earlier CLI receipt (2026-11-18 KST):

- FX source commit:
  bfc15c940c5c0fc055e303f5b00cea3339ffba72.
- Native CLI integration: 22/22, including all seven text reference cards
  and seven JSON cards/keys with the correct fixture branch.
- Latest compiler/runtime original unit suite: 71/71, each process exit 0,
  empty stderr and truthful passing summary.
- Literal replacement contract probes: 7/7.
- CLI and unit execve traces: zero Node/npm/npx/bun/deno.
- Native compiler Stage 2 = Stage 3 = Stage 4 C SHA-256:
  65fc56c092629248471828a88859237d4a2231de344e05213df7fe0e52f96cc2.
- Native compiler ELF SHA-256:
  28d6b736f3ccb8f808eccfe4afb2087329aceed2c75e7cc61683276638bf3836.
- Native CLI ELF SHA-256:
  e9dbd8355c1e97fe7b520b08bea1f0b52c77a518a01231c4792b859ad3bf457a.
- Bootstrap input before/after SHA-256:
  0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.
- Bash syntax and diff whitespace checks: PASS. ShellCheck: NOT_RUN,
  unavailable; no tool installation was performed.

Commands, return codes and artifacts are retained under private evidence
identifier native-test-forms.idxwvm (<가림>). The original protected bootstrap
verifier pins an earlier input and is NOT_RUN; actual Native stage generation
and exact byte comparisons use the existing recipe in a separate evidence
harness. No seed or comparison criterion was changed.

Latest terminal/value milestone (2026-11-18 KST):

FX source commit: 34c2e4daf8f73d44ad9da3566e7e7bbbc6caf5a7.

- Fresh Native Stage 2 = Stage 3 = Stage 4 C SHA-256:
  69ec33d69ea93079296fa5723cc84e4b5b50e4466a6bd43d379385056bc72999.
- Verified Stage 3 compiler ELF SHA-256:
  54d85380a55275e5ecce727e2e491e8acfb5fe015d701f2cc21ae0a217bf1d88.
- CLI ELF SHA-256:
  dc166efe3c5823a79f77f15b448544b2d3b7178e2266f666e4081c9be2784661.
- TUI ELF SHA-256:
  db7765749ec7abe325e8e5b8e7e77bce9a0780c73a1afcc5b342e193436752c2.
- CLI 22/22 and original unit suite 71/71 passed again. Fresh terminal
  artifacts passed raw capability/ownership/byte checks, character conversion
  4/4, 18 host restoration/reaping markers and the Enter/menu contract, exit 0.
  Signal cases include INT/HUP/QUIT forwarding, TERM escalation and child
  TERM/KILL/ABRT/SEGV; device or supervisor loss remains outside the guarantee.
- Ordinary reads block; only escape-sequence continuation reads use the
  existing 100 ms bound. Actual ESC bytes are generated from hexadecimal
  literals. Both Native print APIs flush before blocking on raw input.
- Collected CLI/core/terminal traces contain zero Node-family executions;
  source manifests match before/after. The bootstrap input is unchanged.
- Receipts: native-core-value-split.gzQgDG and
  native-terminal-final.bv22g0 (<가림>). Temporary curl packaging, default
  entry migration, browser builds and untested provider flows remain pending.
