---
name: jgl-status
description: Session checkpoint — what changed, what's still open, anything risky uncommitted. Use when the user says "status", "/jgl-status", "where are we", or before /jgl-commit on a long session.
---

# Status

A short, scannable session checkpoint: *what have I done, what's left, is
anything at risk?* Aim for under 20 lines.

This skill is read-only. **Do not change any files.** If you spot
something that needs fixing, mention it as a finding — don't act.

## 1. What changed

`git status` and `git diff --stat`. Group into three buckets:

- **Committed locally, not pushed** — `git log @{u}..HEAD --oneline`
  (or against `origin/master` if there's no upstream).
- **Staged, ready to commit**
- **Unstaged work in progress**

Show file counts per bucket. Name the files when there are a few;
summarize when there are many ("9 files across `src/py/bbctrl/` and
`src/js/`").

Group by subsystem when that's clearer than a flat list — `src/js/` (web
UI), `src/py/` (controller app), `src/avr/` or `src/pwr/` (firmware),
`src/pug/` / `src/stylus/` (templates and styles), `scripts/` (dev
tooling). A change that spans firmware and UI is worth calling out on its
own; those have to ship together.

## 2. What's still open

The most useful part of the checkpoint. Scan for:

- **Lint failures** — run `make lint` if `src/py/` or `src/js/` changed
  and the host can run it. If it's red, flag it. If `make` isn't
  available on this host, say that instead of skipping silently.
- **Unbuilt web assets** — if `src/pug/`, `src/stylus/`, or `src/js/`
  changed, note whether `make html` has been run since.
- **Unbuilt firmware** — if `src/avr/` or `src/pwr/` changed, note
  whether it's been compiled.
- **New TODO / FIXME / XXX markers** introduced in this session
  (`git diff` for them).
- **Untested behavior change** — this project has no test suite, so
  every behavioral change is unverified by definition. One line naming
  what changed and how it was (or wasn't) checked is more useful here
  than a coverage number.
- **Feature branch with no PR** — if on a feature branch, note that no
  PR was opened from this session.

Don't invent items. If nothing's open, say "Nothing flagged."

## 3. Anything risky uncommitted

- **Uncommitted credentials** — new or modified files matching `.env`,
  `*.key`, `*.pem`, `credentials*`, or a diff that adds a hardcoded
  password/host to `Makefile` or `scripts/`.
- **Stray build artifacts** — a `src/py/bbctrl/http` symlink from
  `make demo`, loose `*.hex` / `*.elf`, `build/`, `dist/`.
- **Uncommitted `package.json` changes** with no `npm install` /
  `npm audit` run since.
- **Version or changelog drift** — `package.json` version or `CODE_TAG`
  modified without a matching `CHANGELOG.md` entry, or vice versa.
- **Uncommitted work older than 2 hours** — one line, not a lecture.

## 4. Report

```
Status — master (⚠ working directly on the production branch)

Committed, not pushed:  2 commits
  • Add Huanyang GT-series VFD register map
  • Wire GT-series into modbus settings UI

Staged:                 0 files
Unstaged:               3 files
  • src/py/bbctrl/Modbus.py (~60 lines added)
  • src/js/modbus-reg.js (~20 lines added)
  • src/pug/settings-tool.pug (~8 lines added)

Open:
  • make lint not run since the Python change
  • make html not run — pug + js changed
  • Firmware/UI change pair: both sides must ship together

Risky:
  • Nothing flagged

Next steps you might consider:
  • Run make lint and make html (/jgl-check)
  • Commit the working tree (/jgl-commit)
  • Move to a feature branch before pushing (/jgl-push warns on master)
```

End with 2–3 suggested next steps. Suggestions, not commands.
