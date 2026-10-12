#!/usr/bin/env bash
# The pictures of docs/plans/pictures_to_shoot.txt shot on this machine's GPU while the cloud session works: pull the branch, build every
# listed example, shoot it with tools/screenshot_element_docs.sh, commit the pictures and push them. With --watch it checks the branch
# every few minutes and shoots again whenever a new commit arrives.
#   bash wood/tools/shoot_pictures.sh                 once
#   bash wood/tools/shoot_pictures.sh --watch 10      every 10 minutes, until stopped
set -euo pipefail
cd "$(dirname "$0")/.."
BRANCH=joinery-library
LIST=docs/plans/pictures_to_shoot.txt
WATCH=${1:-}
MINUTES=${2:-10}

shoot() {
    git pull --rebase origin "$BRANCH"
    mapfile -t names < <(grep -v '^#' "$LIST" | grep -v '^$')
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release > /dev/null
    cmake --build build --target "${names[@]}" --parallel 6
    bash tools/screenshot_element_docs.sh "${names[@]}"
    git add docs/images/elements/*.png
    if ! git diff --cached --quiet; then
        git commit -m "wood: element pictures at $(git rev-parse --short HEAD)"
        git pull --rebase origin "$BRANCH"
        git push origin "$BRANCH"
    fi
}

shoot
while [ "$WATCH" = "--watch" ]; do
    last=$(git rev-parse HEAD)
    sleep $((MINUTES * 60))
    git fetch origin "$BRANCH"
    if [ "$(git rev-parse "origin/$BRANCH")" != "$last" ]; then
        shoot
    fi
done
