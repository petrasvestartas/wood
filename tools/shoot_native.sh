#!/usr/bin/env bash
# Pictures from the session viewer's renderer without a browser or a GPU: each session file drawn by tools/wood_shot.rs on the native
# headless GPU (Mesa's lavapipe when there is no GPU), Arctic shading and black outlines, the orthographic view fitted, trimmed to a PNG.
#   bash wood/tools/shoot_native.sh <out.png> <scene.pb> [iso|iso_back|top|front] [width height]
#   bash wood/tools/shoot_native.sh --example <name> [iso|iso_back|top|front]     builds and runs examples/elements/<name>, shoots its live.pb
#                                                                                into docs/images/elements/<name>.png
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
VIEWER="${SESSION_VIEWER:-$WOOD/../session/session_viewer}"
HOST="$(rustc -vV | sed -n 's/host: //p')"
SHOT="$VIEWER/target/$HOST/release/examples/wood_shot"
[ -f /usr/share/vulkan/icd.d/lvp_icd.json ] && [ -z "${VK_ICD_FILENAMES:-}" ] && export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json

# the renderer, built once in the viewer's crate
if [ ! -x "$SHOT" ] || [ "$WOOD/tools/wood_shot.rs" -nt "$SHOT" ]; then
    cp "$WOOD/tools/wood_shot.rs" "$VIEWER/examples/wood_shot.rs"
    (cd "$VIEWER" && REGEN_PROTO=0 cargo build --release --example wood_shot --target "$HOST") >&2
fi

if [ "${1:-}" = "--example" ]; then
    name="$2"
    cmake --build "$WOOD/build" --target "$name" --parallel 6 > /dev/null
    (cd "$WOOD" && "./build/$name" > /dev/null)
    out="$WOOD/docs/images/elements/$name.png"
    pb="$WOOD/data/output/pb/live.pb"
    view="${3:-iso}"
    width=1600
    height=1000
else
    out="$1"
    pb="$2"
    view="${3:-iso}"
    width="${4:-1600}"
    height="${5:-1000}"
fi

ppm="$(mktemp --suffix=.ppm)"
"$SHOT" "$pb" "$ppm" "$width" "$height" "$view"
convert "$ppm" -fuzz 2% -trim +repage -trim +repage -bordercolor "rgb(248,248,248)" -border 40 "$out"
rm -f "$ppm"
echo "$out"
