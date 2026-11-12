---
name: jgl-push
description: Push committed changes to the remote, with branch-aware safety. Use when the user says "push", "/jgl-push", "send it up", or after /jgl-commit.
---

# Push

Push committed changes to the remote. Deliberately separate from
`/jgl-commit` so the publishing decision is explicit.

**This repo's default branch is `master`, and it ships firmware to
production CNC machines.** Treat a direct push to `master` like a
production deploy.

## 1. Check what you're about to push

```bash
git status
git log @{u}..HEAD --oneline      # or origin/master..HEAD if no upstream
```

Show the user the commit list before pushing. If nothing is ahead, say so
and stop.

Scan the outgoing diff one more time for anything that shouldn't leave
the machine — credentials, `.env` files, large binaries, build output.

## 2. Check the branch

```bash
git rev-parse --abbrev-ref HEAD
```

**Feature branch** (anything but `master` / `main` / `production` /
`release`): push without ceremony. `git push -u origin <branch>` if
there's no upstream, plain `git push` otherwise.

**`master`**: stop and confirm. Show:

```
You're about to push <N> commit(s) to origin/master.

This is production firmware. The safer flow is a feature branch and a
pull request:

    git checkout -b <feature-branch>
    git push -u origin <feature-branch>
    (open a PR on github.com/alexpinson5/RONtrol-firmware)

Continue pushing directly to master? (yes/no)
```

Wait for explicit confirmation. **Do not push to `master` on silence or
ambiguity.** This is the load-bearing safety in this skill.

If the user confirmed earlier in this session that direct pushes to
`master` are fine, you may skip the prompt — but only within the same
session. Don't persist the preference.

## 3. Push

```bash
git push
# or, first time on a branch:
git push -u origin <branch>
```

If a `.githooks/pre-push` hook exists, let it run. Don't reach for
`--no-verify` to get past a red gate.

If the push is rejected as non-fast-forward, **do not `--force`**. Tell
the user the remote has commits they don't have, suggest
`git pull --rebase`, and let them decide.

## 4. Report

```
Pushed 3 commit(s) to origin/feature/vfd-gt-series.
https://github.com/alexpinson5/RONtrol-firmware/tree/feature/vfd-gt-series
```

Derive the URL from `git remote get-url origin` when you can.

## Never

- `--force` / `--force-with-lease` unless the user explicitly asks and
  confirms the consequences.
- `--no-verify` to skip hooks.
- Push a branch the user didn't name. If unsure which branch is current,
  stop and ask.
