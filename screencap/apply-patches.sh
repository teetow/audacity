#!/bin/sh
# Apply the screencap fork's patches to the muse submodule.
# Run after cloning, after `git submodule update`, and after each rebase on master.
# Already-applied patches are skipped, so running it twice is harmless.
set -e
cd "$(dirname "$0")/.."
for p in screencap/patches/muse/*.patch; do
  if git -C muse apply --reverse --check "../$p" 2>/dev/null; then
    echo "already applied: $p"
  else
    git -C muse apply --3way "../$p" && echo "applied: $p"
  fi
done
