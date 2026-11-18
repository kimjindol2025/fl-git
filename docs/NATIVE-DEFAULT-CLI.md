# Native default CLI and TUI

scripts/fl-git now delegates to scripts/fl-git-native. The default CLI and its
tui subcommand use explicitly configured, verified Native executables; missing
tools fail with exit 2. There is no binary search, implicit rebuild or Node
fallback. Current working directory, arguments and child/Git failures are
preserved. The existing FL_GIT_EXIT protocol remains in the Native launcher.

## Configuration

Provide the matching SDK/library environment and set:

```sh
export FREELANG_NATIVE_FLGIT=<verified-native-cli-executable>
export FREELANG_NATIVE_FLGIT_TUI=<matching-native-tui-executable>
./scripts/fl-git --help
./scripts/fl-git status
./scripts/fl-git tui
```

The CLI is built from src/main.fl; TUI from src/tui.fl with the same compiler
and runtime. No private include/library path is embedded in product scripts.
The already-approved temporary curl SDK remains a packaging limitation.

## Text and Git paths

src/tui-input.fl decodes complete UTF-8 sequences before adding text to a
commit message. Invalid sequences become replacement characters without
dropping the following key. Backspace removes one Unicode scalar and redraws
the prompt instead of guessing terminal cell widths. Escape and Ctrl-C cancel
the prompt without committing. NUL and unexpected input failure report an
error; TUI cleanup runs before returning nonzero.

Git C-quoted paths are decoded as bytes, including three-digit octal UTF-8,
quotes, backslashes and control escapes. Rename separators are recognized only
outside quotes and only for rename/copy status. Ordinary filenames containing
an arrow retain their full name. Blank-line filtering preserves status columns.
Undecodable non-UTF-8 byte paths and malformed quotations are rejected explicitly,
rather than selecting a different filename. Supporting arbitrary binary Unix
filenames would require a byte-preserving process/path interface in FX.

A failed stage/unstage reports failure, including the actual Git status; it no
longer reports success merely because the operation was attempted. Filename
labels and status rows escape C0/C1 controls while preserving ordinary Unicode.
The status command disables color and forces Git quoting only for that command,
so project configuration cannot invalidate its columns or hide invalid UTF-8
path bytes before validation. A real pre-change OSC52 filename probe emitted a
raw terminal sequence in the success label; that failing receipt is retained.
The final two control-filename probes stage the exact filenames without emitting
their terminal controls. This is not a claim that all external Git output, such
as intentionally colored diff/log output, is globally sanitized. Keyboard,
mouse, diff/log/branch/stash and two-key remote confirmation routes are retained.

## Evidence

Paths/owners: <가림>. Receipt: native-tui-utf8.y9eukobu.

| Check | Before / current result |
|---|---|
| Original actual TUI counterexamples | ASCII 1 pass; Unicode commit and C-quoted stage 2 failures → all three pass |
| Real Git status paths, quotePath true/false | 9 pass / 19 fail → 28/28 pass |
| Malformed/non-UTF-8 path rejection | 6/6 |
| UTF-8 deletion contracts | 8/8 |
| Default-entry text/path/byte-split/invalid-input workflows | 9/9; exact committed/staged bytes |
| Prompt cancellation / NUL / signal during partial UTF-8 | 4/4; no unintended commit; terminal restored |
| Real Git index-lock stage failure through default entry | 1/1; index unchanged and failure displayed |
| Terminal-control filenames staged through the default entry | 2/2; exact filenames; no raw OSC52/C1 filename controls emitted |
| C0/C1 text display contracts | 6/6 |
| Default-entry CLI integration | Existing 22/22; same cases, expectations and 10-second bounds |
| Existing unit regression | Seven unchanged tracked suites, 71/71 |
| Bash syntax and git diff whitespace | PASS |
| ShellCheck | NOT_RUN; unavailable; no installation |

Before the safety correction, Escape committed the partially entered message,
Ctrl-C became a byte in the committed message, and a caught NUL error exited 0.
The pre-change probe records three failures and one successful partial-UTF-8
signal cleanup. Current probes verify cancellation leaves the fixture commit
unchanged, NUL returns 1, and TERM returns 143 with terminal restoration.

The private path-fixture encoder initially used JSON escapes as raw language
literal escapes. That altered two control-character expected strings. Both the
original failing artifacts and encoding diagnosis are retained. The unchanged
real Git fixture JSON (SHA-256
714518b21d11da3a1d68bb0d2522dfe240382ae520b1b0506cad73e80205b2a2)
is now interpreted with json-parse; expected values and all 28 cases are retained.
The original baseline outputs independently compare as 9 pass / 19 fail.

The original seven tracked test files were copied byte-for-byte into a private
test project with source/docs links. The existing fl-test-native launcher runs
those unchanged contents. The unrelated untracked Git-status test and concurrent
web changes were preserved and are outside this receipt. No protected test,
expectation, timeout or runner was edited. The private 22-case CLI adapter changes
only its target from the explicit Native entry to the default entry.

- FX source: 48988a3756f2c0520be7121021c54083aaafb62a.
- Compiler ELF SHA-256:
  e966e6342053c48fdd3af001a029e0d4a4923832f5122f0f300696ce8d1b2c94.
- Matching runtime archive SHA-256:
  09be8c24c8c2bff543d6d9a7c5942595529473759c60b52be18ff061951624f1.
- CLI ELF SHA-256:
  c767a3f5cb65a9d04538adf77beb19e14a65ec65a1419866c913b9b9925f9086.
- TUI ELF SHA-256:
  ef57db29d0a6c9ed47863f1d315dbf3185074c180a0a9f5cd0507d1fe4e71645.
- Seed unchanged:
  0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

Collected execve traces show zero Node-family executions across 65 trace files. Collected network traces contain 0 connect calls. Scope is the described local probes, not all application flows.

No authored JS or JS line growth is added. CLI tests use only synthetic local
repositories and no remotes. Authenticated provider writes and every interactive
menu flow are not claimed as verified by these cases. Default checker/test/dev
and common deployment Node paths, SDK packaging, complete app flows and DB
commercial qualification remain open. No production service or customer DB was
accessed or deployed.
