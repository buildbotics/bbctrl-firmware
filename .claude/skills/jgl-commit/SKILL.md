---
name: jgl-commit
description: Stage and commit changes locally with a meaningful message. Does NOT push. Use when the user says "commit", "/jgl-commit", "save my work", or "checkpoint".
---

# Commit

Stage relevant changes and write a meaningful commit message. **This
skill does not push.** Pushing is `/jgl-push` — a separate, explicit
step, because this repo ships firmware to real machines.

## 1. Inspect what changed

`git status` for modified, staged, and untracked files. `git diff` for
unstaged changes so you understand what's actually being committed.

If nothing changed, stop and say so. Don't create an empty commit.

Watch for build output that shouldn't be committed: `build/`, `dist/`,
`node_modules/`, `*.hex`, `*.elf`, `*.pyc`, `__pycache__/`. Most are
gitignored, but check anyway — `make demo` creates a symlink at
`src/py/bbctrl/http` that should never be staged.

## 2. Stage relevant files

Pass explicit paths to `git add`. Not `git add .`, not `git add -A`.

**Never stage files that look like secrets:** `.env` and variants,
`credentials.json`, `secrets.json`, `token.json`, `*.pem`, `*.key`,
`id_rsa*`.

If any exist in the working tree, **warn before committing** and don't
stage them, even if the user said "commit everything." If the user
insists, warn that the file will be in git history forever and that any
key in it must be rotated once the commit lands.

This repo already carries default device credentials in `Makefile`,
`scripts/config-wifi`, `scripts/remote-firmware-update`, and
`scripts/ssh-bbctrl`. Don't add new ones, and flag it if a diff
introduces another hardcoded credential.

## 3. Write the message

Short, specific, says **what changed and why**. Match the repo's existing
style: terse imperative subject, optional bulleted body for multi-part
changes. Under 72 characters in the subject.

Good, in this repo's voice:

- `Added support for Huanyang GT-series VFD`
- `Fix off-by-one in probe retract distance`
- `Disable jog axes when not idle`

**No AI attribution.** No `Co-Authored-By` trailer naming an AI tool or
model, no "Generated with …" line, no "🤖" marker, no session URL, no
note that the change was AI-written. Strip any such line your tooling
appends by default. The message should read exactly as if a person typed
it by hand.

If the change is user-facing, ask whether it belongs in `CHANGELOG.md`
too — but don't edit `CHANGELOG.md` or bump the version in
`package.json` / `CODE_TAG` unless asked.

## 4. Commit

```bash
git commit -m "$(cat <<'EOF'
<subject>

<optional body>
EOF
)"
```

## 5. Report

```
Committed locally: "Fix off-by-one in probe retract distance"
2 files changed.

Push when you're ready: /jgl-push
```

Don't push. Even if the user said "commit and push" — tell them push is
a separate command and run it only if they confirm.

## If a pre-commit hook fails

The commit didn't happen. Fix the underlying issue and try again — do
**not** use `--no-verify`. If the hook itself is broken, fix the hook.
