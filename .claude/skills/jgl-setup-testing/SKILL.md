---
name: jgl-setup-testing
description: Wire up pytest for src/py, write a meaningful starter test, and set up CI and a pre-push hook. Use when the user says "set up testing", "/jgl-setup-testing", "add tests", or when /jgl-check reports no test framework configured.
---

# Setup Testing

Bootstrap testing on a project that has none. Goal: a test command that
actually runs, at least one **real** test (not a smoke test), coverage
tooling, CI, and a pre-push hook.

## 0. Confirm the plan first

State the plan and get confirmation before installing anything:

> This project has no tests. I'd start with **pytest + pytest-cov over
> `src/py/`** — that's where most of the testable logic lives and the
> toolchain is already Python 3. First real test would be a config
> save/load round-trip in `src/py/bbctrl/Config.py`. JS testing is a
> separate, harder step (`src/js/` has no module system). Sound right?

Don't start with JS. `src/js/` is browserify-assembled globals with no
exports — unit testing it requires either a jsdom harness or extracting
the logic into plain functions first. Recommend the extraction, and only
after the Python side is working.

## 1. Install

```bash
pip install pytest pytest-cov
```

Add them where the project declares dev requirements. There is no
`requirements-dev.txt` today — create one rather than putting test-only
dependencies in `setup.py`'s install_requires:

```
# requirements-dev.txt
pytest>=8.0
pytest-cov>=5.0
```

## 2. Configure

Add to `setup.cfg` (which already exists) rather than creating a new
config file:

```ini
[tool:pytest]
testpaths = tests
python_files = test_*.py
addopts = --cov=bbctrl --cov-report=term-missing
```

Tests need `src/py` importable. Add a `tests/conftest.py`:

```python
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'src', 'py'))
```

Keep it minimal. Don't add options the user hasn't asked for.

## 3. Write a **real** starter test

Do not ship `assert 1 + 1 == 2`. Pick real logic and assert on specific
computed values. The starter test is the template everything else gets
copied from — make it good.

Best first target: **config round-trip** in `src/py/bbctrl/Config.py`.
Read the module first and write against what it actually does — the
sketch below shows the *shape*, not the API:

```python
# tests/test_config.py
def test_save_then_load_round_trips_motor_settings(tmp_path):
    original = {'motors': [{'axis': 'X', 'max-velocity': 5000}]}
    path = tmp_path / 'config.json'

    save_config(original, path)
    loaded = load_config(path)

    assert loaded['motors'][0]['axis'] == 'X'
    assert loaded['motors'][0]['max-velocity'] == 5000


def test_load_missing_file_returns_defaults():
    assert load_config('/nonexistent/config.json') == DEFAULT_CONFIG


def test_load_rejects_out_of_range_velocity(tmp_path):
    ...
```

Not:

```python
def test_config_exists():
    assert load_config is not None   # catches almost nothing
```

Cover sizes 0, 1, and N where a collection is involved. Other strong
early targets once the first test passes: `Cmd.py` (command
encode/decode round-trip) and `Comm.py` (protocol framing against
recorded bytes, with the serial port mocked and the parser real).

## 4. Wire the Makefile

Add a target next to the existing `lint` / `pylint` / `jshint` ones,
matching their style:

```make
test:
	pytest

.PHONY: test
```

Also add `test` to the existing `.PHONY` line if you'd rather keep it
consolidated. Don't touch the build targets.

## 5. Wire CI

There is no `.github/workflows/` directory. Create one. Getting lint and
the builds into CI is worth as much as the tests themselves — nothing
currently catches a broken Pug template or a firmware compile error
before it's pushed.

```yaml
name: CI
on:
  pull_request:
  push:
    branches: [master]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Install toolchain
        run: |
          sudo apt-get update
          sudo apt-get install -y build-essential gcc-avr avr-libc \
            pylint python3 python3-setuptools

      - uses: actions/setup-node@v4
        with:
          node-version: 20

      - run: npm install
      - run: npm audit --audit-level=moderate

      - name: Python test deps
        run: pip install -r requirements-dev.txt

      - run: make lint
      - run: make test
      - run: make html
      - run: make -C src/avr
      - run: make -C src/pwr
```

Verify the apt package list against `README.md` before committing —
it's the source of truth for the toolchain, and it may have drifted.

If a workflow file already exists, **don't overwrite it.** Show the user
the change you'd make and let them decide.

Mention branch protection: a workflow that isn't a required check on
`master` can't block a broken PR from merging. That's a GitHub settings
change the user has to make.

## 6. Add a pre-push hook

`.githooks/pre-push`, version-controlled. Run the fast gate only — lint
and tests. The AVR and image builds are too slow for a push gate.

```bash
#!/usr/bin/env bash
#
# pre-push hook — block a push when the gate CI runs would fail.
# Fast subset only: lint + unit tests. Firmware and image builds are
# left to CI; they're too slow to run on every push.
#
# Emergency bypass:  git push --no-verify
set -euo pipefail

echo "pre-push: lint + tests …  (emergency bypass: --no-verify)"

if ! make lint; then
  echo "pre-push: ✗ lint failed — push aborted." >&2
  exit 1
fi

if ! make test; then
  echo "pre-push: ✗ tests failed — push aborted." >&2
  exit 1
fi

echo "pre-push: ✓ lint + tests passed."
```

Then, once per clone:

```bash
chmod +x .githooks/pre-push
git config core.hooksPath .githooks
```

If `.githooks/pre-push` already exists, **don't overwrite it** — show the
user the change and let them decide.

## 7. Verify

Run `pytest` (or `make test`) once and show the output. If anything
fails, fix it in the same session — don't leave a half-configured setup.

If you're on a host without `make` or the toolchain, say so plainly:
write the files, run what you can, and name exactly what remains
unverified. Don't report a green run you didn't produce.

## 8. Report

```
✓ pytest + pytest-cov added to requirements-dev.txt
✓ [tool:pytest] section added to setup.cfg
✓ tests/conftest.py written (puts src/py on sys.path)
✓ tests/test_config.py written (4 tests, all passing)
✓ make test target added
✓ .github/workflows/ci.yml written (lint + test + html + avr + pwr)
✓ .githooks/pre-push written + chmod +x; core.hooksPath set

Still manual: branch protection on master — make CI a required check
in GitHub Settings → Branches.

Next: /jgl-review-testing to pick the second batch of tests.
```
