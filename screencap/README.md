# screencap branch

Audacity 4 built for capturing tutorial frames: upstream `master` plus a screencap mode in which a script drives the app over a local remote-control channel and captures its windows, while the app stays in the background.

Branch `screencap`, worktree `D:\projects\code\audacity.worktrees\screencap`. Not upstreamed for now; rebased on `origin/master` every morning. Commits follow Conventional Commits so any of them can become a PR later.

## Screencap mode

Run `Audacity4.exe` with the environment variable `MUSE_SCREENCAP` set (any value). It:

- doesn't activate the app's windows (Windows): launching, dialogs and popups don't take the focus. Clicking a window still activates it; Alt-Tab and the taskbar don't.
- ignores the user's own mouse, keyboard, pen and touch from startup (Windows), so a pointer passing over the window leaves no hover or ruler marks in captures. The `user-input` command turns it back on.
- keeps popups (menus, flyouts) open while the app has no focus.
- starts unattended: no welcome dialogs, no "quit unexpectedly" prompt, no plugin validation.
- shows the release UI on development builds (dev mode off: no DevTools tab, no Diagnostics menu) and the release window title ("Audacity 4.0", not "Audacity 4.0 dev").
- doesn't check for updates, so runs send no update-check requests.
- opens the remote-control channel once startup is done, and adds the screencap commands.

Without the variable the build behaves like upstream, plus the accessibility fixes.

`MUSE_PROFILE_DIR` (optional, any mode) keeps all of a run's per-user files in one folder: settings and AU3 user data in `roaming/`, app data and logs in `local/`, documents in `documents/`. Point it at a fresh folder for every run and runs never touch your own profile or each other.

## Remote control

An MCP server (muse `rcontrol`) on `localhost`, port 2212 or `MUSE_RCONTROL_PORT`: newline-delimited JSON-RPC 2.0 over TCP, one client at a time. Every registered command is a tool, named after its URI (`command://project/new` is `project_new`). A call answers when the command has finished, with its result in `structuredContent`; a command that is disabled right now is refused.

The screencap commands (`command://screencap/...`, tools `screencap_...`):

| Command | Does |
|---|---|
| `looks` | Theme codes, and the accent colors of the current theme |
| `look` | Set the theme and, optionally, an accent of that theme |
| `controls` | Navigation sections; their panels; a panel's controls |
| `trigger` | Trigger a control by section, panel and control name, as Enter would |
| `rect` | A control's rectangle in capture coordinates |
| `popups` | Open popups, menus and dialogs with their rectangles; `wait` answers once one has drawn |
| `close-popups` | Close them all (a dialog closed this way is cancelled) |
| `capture` | Save the main window with its open popups to a PNG, optionally cropped |
| `dispatch` | Dispatch any `command://` or `action://` URI |
| `user-input` | Accept or ignore the user's own input |

Capture coordinates are device pixels from the main window's top-left corner. Controls are found by the names in the app's keyboard navigation; a control without a stable name is fixed here, in the QML.

The client that uses all this lives in the Audacity-Plan repo (`tools/tutorials/au4/`).

## Patches to the framework

Fixes to the MuseScore framework live in `muse/`, a submodule. They are commits on a local `screencap` branch inside `muse/` (the PR-able form) and are carried here as `patches/muse/*.patch`, exported with `git format-patch`, so the commit messages travel with them. Rebasing this branch then never conflicts on the submodule pointer.

`apply-patches.sh` brings `muse/` to "the pinned commit plus every patch", comparing whole trees: if that's already what `muse/` holds it does nothing, if `muse/` is clean at the pinned commit it applies the series, and anything else stops it. When `muse/` is checked out on its `screencap` branch it leaves it alone. To add or change a patch: commit in `muse/` on its `screencap` branch, then re-export:

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
