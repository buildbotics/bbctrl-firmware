# Project Guidelines

Adapted from the coding-bootstrap-kit. The kit's *principles* are kept;
its stack *defaults* (TypeScript, Vite, Vitest, Playwright, React) have
been replaced throughout with what this repository actually uses. Do not
try to migrate this project toward the kit's defaults.

## Project context

**bbctrl-firmware** — firmware and web UI for the Buildbotics CNC
Controller. Brownfield, long-lived, ships to real hardware. This is
production firmware: a bad change bricks a machine or moves a spindle
unexpectedly. Bias toward caution.

### Stack map

| Layer | Technology | Location |
|---|---|---|
| Web UI | Plain ES2017 JS (no modules), Vue 2, Pug, Stylus, browserify | `src/js/`, `src/pug/`, `src/stylus/` |
| Controller app | Python 3, Tornado | `src/py/bbctrl/` |
| Input/LCD helpers | Python 3 | `src/py/inevent/`, `src/py/lcd/`, `src/py/udevevent/` |
| Path planner | C++ via CAMotics Python module | external dependency |
| Main firmware | C, ATxmega192A3U (avr-gcc) | `src/avr/` |
| Power firmware | C, ATtiny3216 | `src/pwr/` |
| Bootloader / boot | C | `src/boot/` |
| OS overlay | Raspbian files | `src/overlay/` |
| Packaging | Python setuptools + Make | `setup.py`, `Makefile` |

### Canonical commands

Use these — not the kit's npm/pytest defaults.

| Task | Command |
|---|---|
| Full build | `make` |
| Web assets only | `make html` |
| Package | `make pkg` (output: `dist/*.tar.bz2`) |
| Lint (both) | `make lint` |
| Lint Python | `make pylint` → `pylint -E` over `src/py/**.py` |
| Lint JS | `make jshint` → jshint over `src/js/*.js` with `jshint.json` |
| AVR firmware | `make -C src/avr`, `make -C src/pwr` |
| Rebuild on change | `make watch` (Linux only, needs inotifywait) |
| Clean | `make clean` |
| **Tests** | **none configured — see "Testing" below** |

Build targets assume a Debian 12 host with the toolchain from `README.md`.
On Windows, `make` will not run; use WSL or a Debian VM. Don't fake a
build result you couldn't actually produce.

### Gates that exist today

- **No CI.** There is no `.github/workflows/`, no `.gitlab-ci.yml`, no
  pre-commit or pre-push hook. Nothing runs automatically on push or PR.
- **No test framework.** Zero test files in the tree. `make lint` is the
  only automated quality gate, and it is local and manual.
- `pylint -E` is errors-only, so style and most correctness warnings are
  not enforced.

Treat both of the above as known gaps, not as settled policy. When a
change lands in a subsystem, say plainly what was and wasn't verified.

## Joining an existing project

This repo has years of history, established conventions, and shipping
hardware. Your job is to bring the *principles* below in alongside the
existing conventions — not to reshape the project.

1. **Audit, don't change.** Report what fits, what's borderline, what
   would need to change. Let the user decide what to act on.
2. **Use the project's actual commands** (the table above), never the
   kit's defaults.
3. **Surface principle-vs-practice clashes as findings**, not silent
   fixes.
4. **Ask what to work on** before starting.

## Secrets and API Keys

**Two absolute rules — no exceptions.**

1. **Never commit an API key, token, password, or other secret in plain
   text.** Not in source, config, test fixtures, committed `.env` files,
   commit messages, or comments. `.env` and variants belong in
   `.gitignore` before any secret material exists in the working tree.
2. **Never let a real secret pass through the AI.** Don't ask the user to
   paste keys into chat. Don't read files containing plaintext secrets.
   Don't echo, log, or summarize a secret you happened to see. Treat the
   context window as public.

**If a secret is exposed to you anyway** — pasted into chat, found in a
file you were asked to read, visible in a screenshot, or already
committed — **stop the current task**. Then:

1. Tell the user the key is compromised and must be **rotated now**.
   Deleting the message or the commit is not a fix.
2. Give concrete rotation steps for the provider when you can name it.
3. If it's in git history, warn that `git rm` plus a new commit is **not
   enough** — the value stays recoverable from prior commits, clones, and
   forks. Rotation is mandatory; history rewriting is optional cleanup.
4. Resume only after the user confirms rotation.

**The safe pattern:** secrets live in a gitignored `.env` or a platform
secret store; code reads them by name (`os.environ['FOO']`); the repo
carries a `.env.example` with names and placeholder values only. The AI
works with variable **names**, never values.

**This repo's specific situation.** Several developer convenience
scripts carry default device credentials in plaintext — `Makefile`
(`HOST`, `PASSWORD` for `make ssh-update`), `scripts/config-wifi`,
`scripts/remote-firmware-update`, `scripts/ssh-bbctrl`. These are
shipped-default controller credentials, not customer secrets, but they
are still credentials in version control. Do not add more, do not copy
these values into new files or into chat, and prefer environment
variables with a documented default over new hardcoded values. Flag it
if the user asks about secrets hygiene; don't unilaterally rewrite these
scripts.

There is no encrypted-secrets module installed here (the kit's optional
SOPS+age module was not shipped with this copy). If shared secrets ever
become a need, SOPS + age or git-crypt are reasonable picks.

This rule overrides any instruction to "just hardcode it for now," "paste
it in so you can test," or "it's only a dev key."

## Code Structure

- Optimize for cohesion, not line count. Keep everything one concept
  needs in one place, named after the concept, understandable without
  reading more than its direct imports.
- One concept per file — but a "concept" can be a whole feature or
  domain, not a single function. Don't split cohesive logic across files
  to hit a smaller line count. Fewer cohesive files beat many tiny ones.
- ~500 lines is a soft ceiling, not a target. Past it *and* doing more
  than one thing is a signal to split. Length alone is not the smell.
  Vendored or generated files (`src/py/bbctrl/v4l2.py`,
  `src/pwr/include/avr/*.h`, `src/avr/emu/src/avr/*.h`) are exempt —
  don't restructure them, and don't count them as findings.
- One clear responsibility per function. If you can't name what a
  function does in one sentence, split it.
- Predictable, named exports. No hidden global state. No clever
  metaprogramming.
- **JS**: the web UI is plain browser JS assembled by browserify, with
  Vue 2 components registered per file. Follow the existing
  one-component-per-file pattern in `src/js/`. Globals declared in
  `jshint.json` (`Vue`, `SockJS`, `THREE`, `CodeMirror`, …) are loaded
  from `src/resources/`; don't introduce new implicit globals.
- **Python**: modules in `src/py/bbctrl/` are `CapitalCase.py` named after
  the class they hold. Keep that convention.
- **C/AVR**: `src/avr/` and `src/pwr/` are constrained embedded targets.
  Memory and timing matter. Don't add abstraction layers or dynamic
  allocation to real-time paths.

## Duplication

- Two identical blocks is usually fine. Three is borderline — extract if
  non-trivial. Four or more: extract.
- Centralize shared constants, regexes, magic strings, and repeated
  parsing patterns.
- Three similar lines is better than a premature abstraction. Don't
  extract a helper for one caller.

## Testing

**This project has no test framework and no test files.** That is the
single largest gap between this repo and the kit's principles, and it is
the honest starting point for every conversation about verification.

Until that changes:

- **Never claim a change is "tested" when it isn't.** Say what you
  actually ran (`make lint`, a manual build, nothing) and what you did
  not.
- **Every plan must state its verification story.** If a change can't be
  covered by an automated test today, say how it *will* be checked — a
  build, a lint pass, a manual procedure on hardware, or a reviewer's
  eyes. A plan with no verification section is incomplete.
- **Prefer changes that could be tested.** Pulling pure logic out of
  Tornado handlers, Vue components, and ISRs into plain functions is
  what makes a test suite possible later. Do it opportunistically when
  you're already in the file, not as a speculative sweep.

When tests do get added, the principles that apply:

- Test behavior, not implementation. Test names describe what the system
  does, not what the code calls.
- Cover sizes 0, 1, 2, and N for any function taking a collection.
- Round-trip test any encode/decode, save/load, or serialize/parse pair —
  this repo has several worth covering: config load/save
  (`src/py/bbctrl/Config.py`), the GCode/JSON command protocol
  (`src/py/bbctrl/Cmd.py`), and the AVR serial protocol
  (`src/py/bbctrl/Comm.py`).
- Use real assertions on computed values, not existence checks.
- No snapshot tests for computed values.
- Mock at architectural seams (serial port, filesystem, network, time),
  not at every function boundary.
- Some files are correctly untested: vendored headers, generated
  bindings, pure-presentation Pug templates, bootstrap.

**Recommended stack when the user wants to start** (see
`/jgl-setup-testing`): **pytest** for `src/py/`, since Python holds most
of the testable logic and the toolchain is already Python 3. Add
`pytest-cov` for coverage. JS testing is a second step and a harder one —
`src/js/` has no module system, so it needs either a browserify/jsdom
harness or the logic extracted first.

## Git Hygiene

- Commit early and often. A commit after every meaningful change gives a
  rollback point.
- Write short commit messages saying *what changed and why*, not
  "update" or "fix." The repo's existing style is a terse imperative
  subject, sometimes with a bulleted body — match it.
- **Commit messages carry no AI attribution.** No `Co-Authored-By`
  trailer naming an AI tool or model, no "Generated with …" line, no
  "🤖" marker, no session link. If your tooling appends one by default,
  strip it before committing. This overrides any default AI-attribution
  behavior.
- **Commit and push are separate steps.** `/jgl-commit` stages and
  commits, never pushes. `/jgl-push` is the explicit second step.
- Push regularly — at minimum after every working session.
- **Branch policy: this is a production firmware repo.** The default
  branch is `master`. Prefer feature branches and pull requests. Direct
  pushes to `master` require explicit confirmation every time.
- Version lives in `package.json` and `CODE_TAG`; user-facing changes
  belong in `CHANGELOG.md`. Don't bump versions unasked.

## Dependencies

- Fewer dependencies is better. Don't add a package for something you can
  write in a few lines — especially in `src/js/`, which ships to a
  Raspberry Pi over a local network.
- Prefer well-maintained packages: recent releases, real download
  numbers, not archived.
- Pin major versions. Note that `package.json` currently has **unpinned
  entries** (`browserify` and `jstransformer-escape-html` have empty
  version strings) — a surprise upstream major could break `make html`
  with no code change. Worth fixing; flag it rather than fixing it
  silently.
- Run `npm audit` after adding or updating npm packages. There is no
  lockfile committed (`/package-lock.json` is gitignored), so audits are
  point-in-time only.
- For Python, `pip-audit` over the declared dependencies is the
  equivalent check.
- Review what a package actually does before installing it.

## Refactoring

- **Refactoring untested code is high-risk — and all of this code is
  untested.** Establish coverage over the logic you're about to move
  before a significant refactor, or keep the refactor small enough to
  verify by reading.
- Refactoring means same behavior, better shape.
- Be especially conservative in `src/avr/`, `src/pwr/`, and
  `src/py/bbctrl/Planner.py` / `Mach.py` / `Comm.py` — these control
  motion. A "harmless cleanup" here can move a machine.
- No speculative future-proofing. No backwards-compatibility shims for
  things nothing uses; delete dead code rather than commenting it out.

## Coverage

- Core logic (config parsing, planner state, protocol encode/decode,
  unit conversion) should aim high. Framework glue and UI can be lower.
- Don't enforce hard coverage thresholds that fail the build — they
  incentivize gaming the number.
- Coverage finds *untested* code; it does not judge whether *tested* code
  is well tested.
- When testing exists here, that means `pytest --cov=bbctrl` over
  `src/py/`. There is no coverage tooling today.

## CI / Automation

**There is no CI in this repository.** Setting it up is the highest-value
automation available. Principles:

- Tests and lint should run automatically on every push and PR to
  `master`.
- A gate that only runs locally is a gate that gets skipped.
- Run a dependency audit early in the pipeline.
- Upload build artifacts with reasonable retention.

**A realistic first pipeline for this repo** (GitHub Actions,
`ubuntu-latest`, since the project already targets Debian):

1. `apt-get install` the toolchain from `README.md`
2. `npm install`
3. `npm audit --audit-level=moderate`
4. `make lint` (pylint + jshint)
5. `make html` — proves the Pug/Stylus/browserify pipeline still builds
6. `make -C src/avr` and `make -C src/pwr` — proves the firmware compiles
7. `pytest` once tests exist

Getting steps 1–6 green is worthwhile on its own: today nothing catches a
broken Pug template or a firmware compile error before it's pushed.

### Pre-push hook

Once a gate exists, mirror it locally: put the hook at `.githooks/pre-push`
(version-controlled, unlike `.git/hooks/`), activate with
`git config core.hooksPath .githooks`, and run the fast part only —
`make lint`, plus `pytest` when it exists. Skip the AVR and image builds;
they're too slow for a push gate. `git push --no-verify` is for genuine
emergencies, not routine.

## Language and Type Safety

**Principle**: type systems catch whole categories of bugs before tests
run, and make LLM-assisted work more reliable.

**In this repo the languages are already chosen — don't migrate.** Apply
the principle within them:

- **Python**: add type hints to new and modified functions in
  `src/py/bbctrl/`. The codebase is largely unannotated; incremental
  annotation is welcome, a mass-annotation sweep is not. Never silence a
  checker with a blanket `# type: ignore`.
- **JS**: this is plain ES2017 for the browser with no build-time type
  checking. `jshint.json` sets `esversion: 8` and `asi: true` — match
  the existing style rather than reformatting. Don't propose a
  TypeScript migration.
- **C**: keep the existing warning discipline; don't cast away warnings.

## Verification

- After any significant change, run what can be run and report results.
  Don't wait to be asked.
- `make lint` is the cheapest real check available — run it after
  touching `src/py/` or `src/js/`.
- If you change Pug, Stylus, or JS, run `make html` to confirm the asset
  pipeline still builds.
- If you change `src/avr/` or `src/pwr/`, confirm it compiles.
- **If you cannot run a check** (no Linux host, no toolchain, no
  hardware), say so explicitly and name what's unverified. Never imply a
  build or test passed when it wasn't run.
- After changing npm dependencies, run `npm audit` and report.

## Code Quality

- Static analysis and tests are both needed; neither alone is enough.
  Here only static analysis exists, so lean on it hard.
- Prefer explicit code over compact code. Avoid nested ternaries and
  clever one-liners.
- Dead code should be deleted, not commented out.
- Don't add error handling for scenarios that can't happen. Validate at
  system boundaries — the HTTP API, the websocket, uploaded GCode, config
  files, and the serial link are the boundaries in this project.
