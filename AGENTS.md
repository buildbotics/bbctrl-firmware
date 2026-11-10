# Agent Guidelines

The project guidelines for this repository live in **[CLAUDE.md](CLAUDE.md)**.

They apply to any AI coding agent working here — Claude Code, Codex,
Gemini CLI, Cursor, Windsurf, Copilot — not just Claude. Read that file
before making changes.

Short version, so nothing gets missed if you only read this file:

- **Brownfield production firmware.** This code ships to CNC machines
  that move. Audit before changing; be conservative in `src/avr/`,
  `src/pwr/`, and the motion path in `src/py/bbctrl/`.
- **Old hardware is supported.** Pi 3 and Pi 4, main board Rev 13 and
  earlier through Rev 17+, two AVR chips. Don't break older boards; see
  "Hardware compatibility" in CLAUDE.md.
- **Stack is settled**: Python 3 + Tornado, plain ES2017 JS + Vue 2 +
  Pug + Stylus, C for AVR, Make for builds. Don't propose migrations.
- **Real commands**: `make`, `make html`, `make lint`, `make test`,
  `make pkg`. The pytest suite is narrow and there is no CI — never claim
  a change was tested when it wasn't.
- **Never commit or echo secrets.** If one reaches you, stop and tell the
  user to rotate it.
- **Commit messages carry no AI attribution** — no `Co-Authored-By` for a
  model, no "generated with" line, no session link.
- **Commit and push are separate steps.** `master` is the production
  branch; pushing to it directly needs explicit confirmation.

Task playbooks live in `.claude/skills/jgl-*/SKILL.md`. They're written
as plain markdown procedures — if your tool doesn't expose them as
`/jgl-*` slash commands, read the relevant `SKILL.md` and follow it
directly.
