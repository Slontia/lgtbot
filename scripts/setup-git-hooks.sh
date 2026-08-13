#!/bin/sh
set -eu

root=$(git rev-parse --show-toplevel)
cd "$root"

chmod +x githooks/commit-msg
git config --local core.hooksPath githooks

echo "Installed git hooks from ${root}/githooks (core.hooksPath=githooks)"
