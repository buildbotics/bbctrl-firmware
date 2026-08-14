---
name: jgl-review-testing
description: Audit verification coverage for bbctrl-firmware — what is actually checked before a change ships, and what isn't. Produces a prioritized plan. Use when the user says "review tests", "/jgl-review-testing", "audit tests", "how are my tests?", or before a refactor.
---

# Review Testing

Audit what this project actually verifies before a change ships.

**Starting position: there is no test framework, no test file, and no
CI.** The only automated gate is `make lint`, run manually. Say that
plainly at the top of every report — it is the context for everything
else.

That makes this skill's job different from a normal test audit. Instead
of grading assertions, you're mapping the verification gap and
prioritizing the first steps out of it.

## 1. Confirm the current state

Don't assume the baseline above is still true — re-check each time:

```bash
find src -name 'test_*.py' -o -name '*_test.py' -o -name '*.test.js' -o -name 'tests' -type d
ls .github/workflows 2>/dev/null
ls .githooks 2>/dev/null
grep -n "pytest\|unittest\|mocha\|jest" package.json setup.py Makefile
```

Report what you found. If tests or CI have appeared since this file was
written, audit them properly using the criteria in section 4 and update
this skill's premise.

## 2. Map what verification exists

Build a gate inventory — for this project it's short, and that's the
point:

| Gate | Runs | Trigger | Can it fail a change? |
|---|---|---|---|
| `make lint` | pylint `-E` on `src/py/`, jshint on `src/js/*.js` | manual | only if someone runs it |
| `make html` | Pug/Stylus/browserify build | manual | only if someone runs it |
| `make -C src/avr`, `src/pwr` | firmware compile | manual | only if someone runs it |
| CI | — | — | **none exists** |
| Pre-commit / pre-push hook | — | — | **none exists** |
| Tests | — | — | **none exist** |

Note the limits of what does exist:

- `pylint -E` reports **errors only**. Unused variables, shadowed names,
  and most correctness warnings pass silently.
- `jshint` covers `src/js/*.js` only — not `src/pug/`, not
  `src/stylus/`, not `scripts/`, not the Python-adjacent JS in
  `src/resources/`.
- Nothing checks the Pug templates except a full `make html`.
- Nothing at all checks `src/avr/`, `src/pwr/`, or `src/boot/` beyond
  "it compiles."

## 3. Identify the highest-value testable surfaces

Rank by (logic density × blast radius ÷ effort to test). In this repo the
Python side is where testing starts, because it holds most of the logic
and needs no harness beyond pytest.

Strong candidates in `src/py/bbctrl/`:

- **`Config.py`** — config load/save/validate. A round-trip test
  (save → load → compare) is high value and easy.
- **`Cmd.py`** — command encoding for the AVR link. Round-trip
  encode/decode.
- **`Comm.py`** — serial protocol framing and response parsing. Test
  against recorded byte sequences; mock the port, not the parser.
- **`State.py`** — state transitions and variable updates. Pure enough
  to test directly.
- **`Planner.py`** / **`plan.py`** — planner interaction. Higher value,
  higher effort (needs the CAMotics module).
- **`Web.py` / handlers** — extract logic first, then test the extracted
  functions. Testing Tornado handlers end-to-end is a later step.

The JS side is harder: `src/js/` has no module system (browserify
assembles globals), so unit testing needs either a jsdom harness or the
logic pulled into plain functions first. Good extraction targets:
`util.js`, `unit-value.js`, `filters.js`, `cm-gcode.js`. Recommend
extraction before testing, not a harness first.

`src/avr/` and `src/pwr/` have an emulator at `src/avr/emu/` — worth
noting as an existing, unused testing affordance.

## 4. If tests do exist, audit them properly

Apply these once there's something to audit:

1. **Untested logic files** — source with real logic and no test.
2. **Weak assertions** — existence checks where a computed value should
   be asserted.
3. **Missing edge cases** — collection functions tested at only one
   size; check 0, 1, 2, N.
4. **Missing round-trips** — any encode/decode, save/load,
   serialize/parse pair not tested both ways.
5. **Tests mirroring implementation** — the assertion is the function's
   logic copied into the test; it'll be wrong the same way.
6. **Over-mocking** — mock the serial port, the filesystem, the network,
   and time. Not every function boundary.
7. **Tests that no gate runs** — a test file on disk that CI doesn't
   pick up is the same problem as a missing test, often worse.
8. **Soft-failing CI** — `|| true`, `continue-on-error: true`, `exit 0`,
   or a "0 tests collected" report that doesn't fail the build.

Grep for committed focused/muted tests: `.only`, `.skip`, `xit`,
`@pytest.mark.skip`.

## What NOT to do

- Don't write tests here. Report and plan.
- Don't propose testing Pug templates, `src/overlay/` config, vendored
  headers, or `src/py/bbctrl/v4l2.py` / `ObjGraph.py`.
- Don't chase a coverage percentage.
- Don't recommend a JS test framework as step one — the extraction work
  has to come first, and pytest delivers more per hour of effort.

## Threshold

Given the current state, "improvement recommended" always applies. It
stops applying when: the logic-heavy modules in `src/py/bbctrl/` have
real tests, a CI workflow runs lint + build + tests on every push and PR
to `master`, and that workflow can fail the build.

## The plan

Prioritized, concrete, and sized for one sitting each:

1. **Do first** — the gate, then the first tests:
   - Add `.github/workflows/ci.yml` running `npm install`, `make lint`,
     `make html`, and the AVR builds on push and PR to `master`. This
     alone catches broken templates and firmware compile errors that
     nothing catches today.
   - Add pytest + pytest-cov and one *real* round-trip test for
     `Config.py` (see `/jgl-setup-testing`).
   - Wire `.githooks/pre-push` to run `make lint` (and `pytest` once it
     exists).
2. **Do second** — widen: `Cmd.py` and `Comm.py` round-trips,
   `State.py` transitions, extract pure logic out of `Web.py` handlers
   and test the extracted pieces.
3. **Skip for now** — JS unit tests (needs extraction first), AVR
   emulator harness, end-to-end hardware tests. Real, but each is a
   project rather than a task.

For each item name the file, the behavior to test, the kind of test, and
where it lives. For CI items name the workflow file and the specific
step.

## Iterating

Plan for 2–3 passes in a session rather than one 50-item plan. On pass
2+, ask what changed since the last pass, skip re-enumerating what's
already listed, and focus on what's new. Recommend stopping when the
threshold is met — don't manufacture findings.

**Pairs with `/jgl-review-code`.** Run this first to build a safety net,
then use `/jgl-review-code` to plan structural work with confidence.
