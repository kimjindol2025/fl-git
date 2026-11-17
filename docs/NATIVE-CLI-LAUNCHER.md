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

TUI selection uses FREELANG_NATIVE_FLGIT_TUI and executes that separately
verified tool directly. Without it, tui fails explicitly with exit 2. This
milestone does not establish Native TUI readiness; terminal interaction and
restoration remain NOT_RUN. No TUI source or functionality was removed.

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
and TUI interaction are NOT_RUN. This is distinct from the original 71 unit
cases; none of their tests, expectations or runners changed. New JS files
and JS line changes are zero.

Recorded verification (2026-11-18 KST):

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
