#!/usr/bin/env bash
# The element pages' pictures on a machine with no GPU and no browser: each example under examples/elements built and run, its live.pb shot
# by tools/shoot_ui.sh with the viewer's interface, the layer panel open with every row, the element interactions and attributes on, Arctic,
# the orthographic isometric view (the back view for a column, whose head and cuts face the inside of the floor), 3200 x 2000.
#   bash wood/tools/shoot_elements_ui.sh                       every example
#   bash wood/tools/shoot_elements_ui.sh element_plate ...     the ones named
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WOOD"
if [ "$#" -eq 0 ]; then
    set -- $(ls examples/elements/*.cpp | xargs -n1 basename | sed 's/\.cpp$//')
fi
cmake --build build --target "$@" --parallel 6 > /dev/null
for name in "$@"; do
    view=iso
    case "$name" in element_column*) view=iso_back ;; esac
    if ! ./build/"$name" > /dev/null 2>&1; then
        echo "$name: the example failed"
        continue
    fi
    UI_SHOT_VIEW="$view" bash tools/shoot_ui.sh "docs/images/elements/$name.png" data/output/pb/live.pb > /dev/null 2>&1 || echo "$name: the shot failed"
    echo "$name"
done
