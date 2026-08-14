---
name: jgl-check
description: Run the full verification suite for bbctrl-firmware — lint, build, and dependency audit — and report honestly, including what was skipped. Use when the user says "check", "/jgl-check", "verify", "is everything passing", or after any significant change.
---

# Check

Run every verification this project actually has and report results
honestly: what passed, what failed, and what was **skipped because the
tool or platform wasn't available**.

A silent skip is worse than a failure. If `make` can't run because you're
on Windows, saying nothing makes the user think the build passed. Report
every skip with a one-line reason and treat it as a finding.

## 0. Check the platform first

This project builds on Debian Linux with avr-gcc, pug-cli, browserify and
friends. On Windows or macOS most targets will not run.

```bash
uname -s
command -v make
```

If `make` is unavailable, **stop and say so up front** — then run only
what you can (file inspection, `npm audit` if node is present) and mark
everything else `⊘ SKIPPED — no make on this host`. Do not guess at
results. Do not report a build you didn't run.

## 1. Run the checks

Run all of these in order, even if an earlier one fails. The user wants
the full picture, not stop-at-first-failure.

### Lint

```bash
make lint          # runs pylint -E over src/py, then jshint over src/js
```

If you only touched one side, the narrower targets are faster:

```bash
make pylint        # pylint -E $(find src/py -name '*.py')
make jshint        # ./node_modules/jshint/bin/jshint --config jshint.json src/js/*.js
```

`make jshint` needs `node_modules` — run `npm install` first if it's
missing. `make pylint` needs `pylint` on PATH (`apt-get install pylint`).

Note in the report that `pylint -E` is **errors-only**: a clean pylint
line does not mean the Python is warning-free.

### Web asset build

```bash
make html
```

This exercises the Pug → HTML, Stylus → CSS, and browserify → JS
pipeline. It is the only thing that catches a broken template or a
syntax error in a file jshint didn't cover.

### Firmware build

```bash
make -C src/avr
make -C src/pwr
```

Needs `gcc-avr` and `avr-libc`. Skip with a reason if the toolchain is
absent. Only run these when the change touched `src/avr/`, `src/pwr/`, or
`src/boot/` — they're slow.

### Tests

**There is no test framework in this project.** Report it every time:

```
✗ Tests — NO TEST FRAMEWORK CONFIGURED (0 test files in tree)
```

This is a finding, not a skip. Offer `/jgl-setup-testing` (pytest over
`src/py/` is the recommended starting point) — but don't run it unasked.

### Dependency audit

```bash
npm audit
```

Note in the report that `/package-lock.json` is gitignored, so the audit
reflects whatever versions resolved at install time, not a pinned set.
Also flag that `browserify` and `jstransformer-escape-html` in
`package.json` have **empty version specifiers** — they float to latest.

For Python:

```bash
pip-audit
```

If it isn't installed, report `⊘ SKIPPED — pip-audit not installed
(install: pip install pip-audit)`. Don't pretend it passed.

### Type check

`⊘ N/A — no type checker configured.` The Python is largely unannotated
and the JS is plain ES2017. That's neutral for a report, but worth one
line: adding Python type hints incrementally is a real win here.

### CI

Check whether `.github/workflows/` exists. If it doesn't:

```
✗ CI — NO PIPELINE CONFIGURED (nothing runs on push or PR)
```

That's a finding, on equal footing with the missing tests.

## 2. Report

Compact summary with explicit markers:

```
✓ Lint — pylint -E clean, jshint clean (note: pylint is errors-only)
✓ Web build — make html succeeded
⊘ Firmware build — SKIPPED (change didn't touch src/avr or src/pwr)
✗ Tests — NO TEST FRAMEWORK CONFIGURED
✓ npm audit — 0 vulnerabilities (no lockfile committed)
⊘ pip-audit — SKIPPED (not installed: pip install pip-audit)
⊘ Type check — N/A (no type checker configured)
✗ CI — NO PIPELINE CONFIGURED
```

Markers: `✓` passed, `✗` failed, `⊘` skipped or N/A (always with a
reason).

Show relevant error output below the summary for anything that failed.
Every line gets a marker — no line goes unreported.
