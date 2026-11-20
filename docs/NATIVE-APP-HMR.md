# Actual Native application and development CSP

The actual Git application supplied `connect-src 'self'`. Its Native development
event stream uses a separate loopback port, so the browser correctly blocked
HMR. Generated development responses now add the build plan's exact
`http://127.0.0.1:<hmrPort>` origin to the effective connection policy. The
runtime environment cannot substitute a different port into that response.

`fl-front-server-dev-assets-at` handles explicit `connect-src`, its `default-src`
fallback, `'none'`, the first duplicate directive, comma-separated policies,
header arrays, case variants and Report-Only headers. Other directives and
headers retain their values. Policies without either connection restriction
remain unchanged. The original two-argument helper remains available; production
generation and non-HTML responses retain their previous behavior. Invalid ports
fail explicitly. Invalid header values reach the original wire validator without
being normalized into acceptable values.

These rules follow [Content Security Policy Level 3](https://www.w3.org/TR/CSP3/),
including policy lists, first-directive parsing and connection fallback. The
change is implemented in FreeLang; browser JavaScript remains generated.

An independent runtime issue appeared after the CSP correction: completed idle
HTTP connections held all workers while browser asset requests queued. The FX
runtime pressure fix releases only completed idle plain-HTTP connections before
any next-request byte. Both failures and their separate reproductions are
retained in receipts `native-actual-app-20261120` and
`native-http-pressure-20261120`.

The final coherent SDK and full actual application are verified in Chrome using
an owned temporary Git repository: repository discovery/selection, literal
apostrophe/Korean/newline filenames, selection, real Stage and Commit, preservation
of an unselected file, reload, computed CSS changes through SSE without navigation,
and SSE-triggered navigation preserving the Git result. The test deliberately
sets the application's runtime HMR environment port to 1 while the build plan
uses its real port. The exact response policy and successful HMR prove which
port is used. Both children must become ready and stop within three seconds.

The [evidence](evidence/native-actual-app-hmr-20261120.json) distinguishes the
final SDK run from earlier candidate runs and records source/SDK hashes, original
failures and retained limitations. DOM clicks use real controls after checking
their layout boxes. Screenshot and physical-pointer acceptance remain NOT_RUN
after earlier surface failures; those operations were not retried. Browser
connections are limited to the owned loopback app and HMR origins. The repository
has no remote or account credentials, so this receipt covers local Git behavior;
remote Provider Push/Pull is not qualified by it.

Existing foreign application edits were included verbatim in the private actual
app snapshot for verification and remain outside the implementation commit.
Protected tests, expected values, limits and the seed remain unchanged.
Full DSL parity, default development/deployment migration and latest whole DB95
acceptance remain open. Commercial HOLD; production deployment NOT_RUN.
Private paths and owners: <가림>.
