# screencap branch

Audacity 4 built for capturing tutorial frames: upstream `master` plus fixes that let a script drive the app through UI Automation (Windows accessibility) instead of clicking blind.

Branch `screencap`, worktree `D:\projects\code\audacity.worktrees\screencap`. Not upstreamed for now; rebased on `origin/master` every morning. Commits follow Conventional Commits so any of them can become a PR later.

## Screencap mode

Run `Audacity4.exe -F` with the environment variable `MUSE_SCREENCAP` set (any value). `-F` starts from factory settings, so every run looks the same. `MUSE_SCREENCAP`:

- doesn't activate the app's windows (Windows): launching, dialogs and popups don't take the focus. Clicking a window still activates it; Alt-Tab and the taskbar don't.
- keeps popups (menus, flyouts) open while the app has no focus. A press outside still closes them; a script closes them by sending the popup window WM_CLOSE (a posted Escape doesn't arrive, keys go to the focused window).
- shows the release UI on development builds (dev mode off: no DevTools tab, no Diagnostics menu).
- doesn't check for updates, so repeated runs send no update-check requests.

Without the variable the build behaves like upstream, plus the accessibility fixes.

The driver that uses all this lives in the Audacity-Plan repo (`tools/tutorials/au4/`).

## Patches to the framework

Fixes to the MuseScore framework live in `muse/`, a submodule. They are commits on a local `screencap` branch inside `muse/` (the PR-able form) and are carried here as `patches/muse/*.patch`, exported with `git format-patch`, so the commit messages travel with them. Rebasing this branch then never conflicts on the submodule pointer.

`apply-patches.sh` applies them to whatever commit master pins `muse/` to, and skips any already applied. To add or change one: commit in `muse/` on its `screencap` branch, then re-export:

```
git -C muse format-patch -N --no-signature --zero-commit -o ../screencap/patches/muse <pinned-commit>..screencap
```

## Daily routine

```
git fetch origin
git rebase origin/master
git submodule update            (resets muse/ to master's pinned commit)
screencap\rebuild.cmd           (re-applies the patches, then builds)
```

If a patch no longer applies because upstream changed the same lines, `apply-patches.sh` stops: rebase the `muse/` `screencap` branch onto the new pinned commit, fix the conflict there, and re-export.

The build uses the `default` preset from `CMakeUserPresets.json`, which is gitignored; copy it from the main checkout if it is missing. Output: `build\default\install\bin\Audacity4.exe`.
