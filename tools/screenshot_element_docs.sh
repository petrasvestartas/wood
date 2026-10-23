#!/usr/bin/env bash
# The element pages' pictures: each example under examples/elements built, run and published to the
# live viewer, then opened in a Chrome window on the GPU with the layers expanded, the element
# features, the Arctic look and the orthographic isometric view, and screenshotted. On Windows the scene is served from this
# machine instead of published.
#   bash wood/tools/screenshot_element_docs.sh element_plate element_column ...
set -euo pipefail
cd "$(dirname "$0")/../.."
for name in "$@"; do
    view="View Isometric"
    # a column's head and cuts face the inside of the floor, away from the usual corner
    case "$name" in element_column*) view="View Isometric Back" ;; esac
    # Linux publishes to the live viewer; Windows (no systemd-run, no R2 profile) builds and runs the example and serves its live.pb
    # to the hosted viewer from this machine
    case "$(uname -s)" in
        MINGW*|MSYS*|CYGWIN*)
            cmake --build wood/build --config Release --target "$name" --parallel 6 > /dev/null
            "wood/build/Release/$name.exe" > /dev/null
            export SHOT_PB=wood/data/output/pb/live.pb ;;
        *)
            bash bash/publish-scene.sh --target "$name" > /dev/null
            sleep 2 ;;
    esac
    node --experimental-websocket wood/tools/screenshot_viewer.mjs "wood/docs/images/elements/$name.png" \
        "Layers All" "Arctic On" "$view" "View Orthographic" "Fit" > /dev/null
    echo "$name"
done
