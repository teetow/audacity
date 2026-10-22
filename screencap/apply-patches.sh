#!/bin/sh
# Bring muse/ to "the commit master pins + every patch in screencap/patches/muse".
# Run after cloning, after `git submodule update`, and after each rebase on master.
#
# Decides by comparing whole trees, never patch by patch (with stacked patches
# a later one can change an earlier one's lines, so per-patch checks lie):
#   - muse/ isn't at the pinned commit: you're working on the patches as commits
#     on muse's own branch; nothing to apply.
#   - muse/'s files already equal "pinned + patches": nothing to do.
#   - muse/'s files equal the pinned commit: apply the series.
#   - anything else: stop, rather than mix the patches into unknown changes.
set -e
cd "$(dirname "$0")/.."

pinned=$(git ls-tree HEAD muse | awk '{print $3}')
head=$(git -C muse rev-parse HEAD)
if [ "$head" != "$pinned" ]; then
    echo "muse/ is at $head, not the pinned $pinned: patches are expected as commits there; not applying"
    exit 0
fi

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# The tree "pinned + all patches", built in a scratch index
GIT_INDEX_FILE="$tmp/expected" git -C muse read-tree "$pinned"
for p in screencap/patches/muse/*.patch; do
    GIT_INDEX_FILE="$tmp/expected" git -C muse apply --cached "../$p"
done
expected=$(GIT_INDEX_FILE="$tmp/expected" git -C muse write-tree)

# The tree of muse/'s current files (tracked and untracked)
GIT_INDEX_FILE="$tmp/current" git -C muse read-tree "$pinned"
GIT_INDEX_FILE="$tmp/current" git -C muse add -A
current=$(GIT_INDEX_FILE="$tmp/current" git -C muse write-tree)
pinned_tree=$(git -C muse rev-parse "$pinned^{tree}")

if [ "$current" = "$expected" ]; then
    echo "patches already applied"
elif [ "$current" = "$pinned_tree" ]; then
    for p in screencap/patches/muse/*.patch; do
        git -C muse apply "../$p"
        echo "applied: $p"
    done
else
    echo "muse/ has changes that aren't exactly the patches; not touching it." >&2
    echo "Commit them on muse's screencap branch and re-export, or reset muse/ with: git submodule update --force" >&2
    exit 1
fi
