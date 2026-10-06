#!/usr/bin/env bash
# The element pages' pictures: each example under examples/elements built, run and published to the
# live viewer, then opened in a Chrome window on the GPU with the layers expanded, the element
# features, the Arctic look and the orthographic isometric view, and screenshotted.
#   bash wood/tools/screenshot_element_docs.sh element_plate element_column ...
set -euo pipefail
cd "$(dirname "$0")/../.."
for name in "$@"; do
    bash bash/publish-scene.sh --target "$name" > /dev/null
    sleep 2
    node --experimental-websocket wood/tools/screenshot_viewer.mjs "wood/docs/images/elements/$name.png" \
        "Layers All" "Element Features On" "Arctic On" "View Isometric" "View Orthographic" "Fit" > /dev/null
    echo "$name"
done
