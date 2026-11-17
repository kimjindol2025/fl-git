# Native CSS compiler

The separate scripts/fl-style-compile-native entry runs a Native FreeLang
CSS compiler. It is a component for migrating the web build; the default
web builder and development server still require Node. No existing feature,
JS source, test, expected value, timeout or protected entry was changed.

## Use

Set FREELANG_NATIVE_STYLE_TOOL to the verified executable compiled from
tools/fl-style-compile.fl, with the matching approved runtime environment.

~~~sh
scripts/fl-style-compile-native --page page.flx
scripts/fl-style-compile-native --compile pages-directory output-directory
~~~

--page returns the legacy compilePageCSS shape: css, bindings and rawCss.
--compile writes app.css with the existing generated header and returns
cssPath and bindings. File traversal is recursive and sorted by name. The
first encountered declaration of a style name wins. An explicit ordered
name vector preserves numeric-name order, independently of object-key order.

The launcher forwards arguments and exit status without a Node fallback.
Missing/non-executable Native tools fail with BLOCKED and exit 2. Invalid
arguments show usage and exit 2. Source/IO failures return an error record
and exit 1. All sources are collected before writing CSS, so a malformed
later input leaves an earlier output intact. The final artifact write uses
the existing file API; it is not an atomic crash-safe publication protocol.

The existing filesystem module supplies its unchanged 64 MiB file cap,
configured-base confinement and no-follow path checks. Source and output
symlinks fail explicitly. That is stricter than the legacy Node file read.
The source scanner respects comments, escaped strings and triple strings,
and rejects unterminated strings/style forms. Text inside comments/strings
does not become an active style, unlike the old substring-based extractor.

## Native implementation

tools/front-style.fl implements parsing, declaration rendering, pseudo and
responsive grouping, class bindings and the 32-bit djb2/XOR hash. It retains
54 declarative property mappings and ten property transforms from the
existing source. JSON property order and UTF-16 code units, including Korean,
emoji, NUL and isolated surrogates, determine the class hash. The byte-based
legacy char-code-at API is decoded explicitly rather than mistaken for a
UTF-16 accessor.

Quoted-pair order, numeric duplicate handling, unknown-property skipping,
shadow aliases, responsive order xl/lg/md/sm and current pseudo-prefix routing
follow the inspected reference. In particular, longer focus-prefixed keys
retain that reference routing behavior. This component does not claim full
DSL validation or universal diagnostic parity.

## Verification receipt

| Evidence | Result |
|---|---|
| Independent declaration/hash/parser/scanner contracts | 88/88 |
| End-to-end Native entry controls | 12/12 |
| Directory artifact | exact CSS bytes, ordered bindings and duplicate handling |
| Bad later input | exit 1; prior CSS bytes unchanged |
| Empty directory | generated header only, no false style bindings |
| Missing input, unwritable output, source/output symlinks | fail |
| Missing tool / invalid arguments | exit 2 |
| Existing real page sources | read successfully; CSS written only to a private directory |
| Inspected source body | not executed; file-write marker absent |
| Collected Native traces | 0 Node-family executions, 0 network request syscalls |
| Syntax/static checks | bash -n and staged git diff --check, exit 0 |

The 88 expectations are private diagnostics with an independent Python
UTF-16/hash/JSON and declaration oracle; static mapping values are extracted
from the existing reference source. No Node CSS build stage was executed.
Dynamic Node CSS comparison is NOT_RUN. The real current pages contain no
style blocks; nonempty CSS functionality is established by the synthetic
contracts, not by claiming an empty app build proves it.

The initial entry receipt stopped on source-symlink handling. The result-style
IO API follows symlinks when no base is configured; the entry was changed to
the existing no-follow file module. The next receipt stopped on an invalid
argument branch missing an explicit do. Both raw failed receipts remain;
the original expectations were kept and the corrected entry passed 12/12.

Native CSS executable SHA-256:
4f50af462c461c523e47d888cf6736bc153b0734909fe9c751fd95caeab67454.
Matching FX source commit:
2e402673dc0fc5b30450c316934e8bd4d7ae492c.
Verified compiler ELF SHA-256:
24355a87f854de32775c65773663f32050f4cc5c0b2e156b964966ea356f7fa3.
Stage 2/3/4 exact C SHA-256:
4e589203e96b1cbfa8e4983439df33ab9a471daa3c95f5d4d89f99573c139c7e.
Seed before/after:
0101cb63a86f20b93451f70b88a0b577f9548ded0a8b328ebdeedb7e8e4fd767.

Receipts: native-style-component.9jljuell, native-style-cli.by1z4jeg,
native-style-cli-v2.k6vvszkz and native-style-cli-v3.17miehwq.
All source/reference/seed manifests matched before and after successful runs.
Host paths and owners: <가림>. No installation, seed regeneration or global
configuration change occurred. New JS files: 0; JS line changes: 0.

Island compilation/runtime, the complete server-app builder, diagnostic
catalogue/fix paths, dev server, browser interaction and default migration
remain pending. This CSS receipt establishes none of the DB commercial
safety requirements; DB95 checkpoint fault recovery remains HOLD.
