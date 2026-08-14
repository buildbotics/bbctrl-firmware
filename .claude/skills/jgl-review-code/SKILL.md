---
name: jgl-review-code
description: Review the codebase for structural issues — duplication, oversized multi-responsibility files, dead code, dependency health — using LLM analysis plus the project's linters. Produces a refactor plan if warranted. Use when the user says "review code", "/jgl-review-code", "code review", "anything need refactoring?", or "look over the code".
---

# Review Code

Review the codebase for structural quality using the information sources
this project actually has:

1. **Your own analysis** — read the code and identify issues.
2. **Static analysis** — `make lint` (pylint `-E` over `src/py/`, jshint
   over `src/js/*.js`). Run the narrower `make pylint` / `make jshint`
   when the review is scoped to one side.
3. **Coverage data** — **none exists.** Report
   `⊘ Coverage — no tooling configured` and treat it as a standing
   finding, not a clean line. It also means every refactor
   recommendation below carries more risk than it would in a tested
   repo; say so.

**Check that each tool exists before running it.** If `make` isn't
available on this host, if `node_modules` is missing, or if `pylint`
isn't installed, report `⊘ SKIPPED — <reason>` with a one-line fix hint.
A silent skip looks identical to a clean run.

## Scope: what to review, what to leave alone

**In scope**: `src/js/`, `src/pug/`, `src/stylus/`, `src/py/bbctrl/`,
`src/avr/src/`, `src/pwr/src/`, `src/boot/src/`, `scripts/`.

**Out of scope — vendored, generated, or third-party.** Don't count
these as findings and don't propose restructuring them:

- `src/py/bbctrl/v4l2.py` (~2000 lines, kernel ioctl bindings)
- `src/py/bbctrl/ObjGraph.py` (~1200 lines, third-party debug utility)
- `src/pwr/include/avr/*.h` and `src/avr/emu/src/avr/*.h` (device headers)
- `src/js/helvetiker_regular.typeface.json` and anything in
  `src/resources/` (bundled libraries: Vue, THREE, CodeMirror, …)
- `src/overlay/` (OS configuration files, not application code)

## What to check

1. **Files well past ~500 lines that do more than one thing.** Length
   alone isn't the smell; multiple responsibilities is. List with line
   counts. Current in-scope candidates worth a look: `src/js/orbit.js`
   (~840), `src/js/path-viewer.js` (~745), `src/py/bbctrl/Web.py`
   (~600), `src/py/bbctrl/Camera.py` (~570), `src/js/app.js` (~400).
   Also flag the reverse — clusters of tiny files that belong to one
   concept.
2. **Duplicated logic** — the same block in three or more places. In
   this repo, look especially at the `src/js/settings-*.js` and
   `src/js/view-*.js` families and at repeated unit-conversion,
   axis-iteration, and API-error-handling patterns.
3. **Dead code** — unused functions, commented-out blocks, unreachable
   branches. Note that jshint won't catch unused globals across
   browserify modules, so this needs reading.
4. **Tangled logic** — business logic embedded in Tornado request
   handlers (`src/py/bbctrl/*Handler*.py`, `Web.py`), in Vue component
   methods, or in AVR interrupt handlers, where it can never be tested
   in isolation. This is the highest-value category in this repo,
   because extracting it is what makes a test suite possible at all.
5. **Boundary validation** — the HTTP API, the websocket, uploaded GCode,
   config files, and the AVR serial link are the trust boundaries.
   Missing or weak validation there is a real finding; defensive checks
   deep in internal call chains are not.
6. **Security concerns** — hardcoded credentials, missing authentication
   on state-changing endpoints, path traversal in file upload/download,
   command injection in `scripts/`. Flag immediately. Note the known
   plaintext defaults in `Makefile`, `scripts/config-wifi`,
   `scripts/remote-firmware-update`, `scripts/ssh-bbctrl` — mention them
   once; don't re-litigate them every review.
7. **Dependency health** — `npm audit`, and `pip-audit` if installed.
   Report `⊘ SKIPPED — <tool> not installed` rather than claiming clean.
   Flag the empty version specifiers on `browserify` and
   `jstransformer-escape-html` in `package.json`, and that no lockfile
   is committed.
8. **Real-time constraints** — in `src/avr/` and `src/pwr/`: dynamic
   allocation, floating point, or long computation on interrupt paths;
   non-volatile shared state; blocking calls in the step generator.
   These are correctness findings, not style ones.

## What NOT to do

- Don't fix anything. Report and plan.
- Don't nitpick formatting or style — `asi: true` in `jshint.json` means
  semicolon-free lines are intentional.
- Don't suggest speculative future improvements.
- Don't propose a TypeScript, React, or module-system migration. The
  stack is settled.
- **Don't propose refactors for untested code without flagging the risk.**
  In this repo that's everything — so every refactor item must name how
  the change will be verified (a build, a lint pass, a hardware check, or
  tests written first).

## Refactor threshold

- **No refactor needed** if: no oversized multi-responsibility in-scope
  files, no logic duplicated beyond 2×, no untestable tangled logic in
  handlers or components, audits clean. Say so and stop.
- **Refactor recommended** if any of:
  - 3+ oversized in-scope files each carrying more than one responsibility
  - The same logic block duplicated 3+ times
  - Business logic embedded in Tornado handlers or Vue components that
    could be pure functions
  - `npm audit` reports moderate or higher
  - A boundary with no input validation
  - Anything in the real-time constraints list above

## Refactor plan

If the threshold is met, end with a prioritized plan:

1. **Do first** — high value, low risk: extracting duplicated logic,
   fixing audit vulnerabilities, adding boundary validation, pulling pure
   logic out of handlers (this doubles as test enablement).
2. **Do second** — medium value: splitting oversized files, tightening
   internal structure.
3. **Skip for now** — real but not worth it yet, with one line of why.

For each item: file path, what's wrong, what the fix would be, and **how
it would be verified**. Keep it concrete — "extract the unit-conversion
block from `src/js/unit-value.js:40-95` into a pure function so it can be
tested independently" not "consider refactoring units."

**Be conservative about motion-critical code.** `src/avr/`, `src/pwr/`,
`src/py/bbctrl/Planner.py`, `Mach.py`, and `Comm.py` control a machine
that moves. Structural cleanup there needs a stronger justification than
"it's long," and should be flagged as requiring hardware verification.
