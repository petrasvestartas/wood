#!/usr/bin/env bash
# Pictures from the session viewer's renderer without a browser or a GPU: each session file drawn by tools/wood_shot.rs on the native
# headless GPU (Mesa's lavapipe when there is no GPU), Arctic shading and black outlines, the orthographic view fitted, trimmed to a PNG.
#   bash wood/tools/shoot_native.sh <out.png> <scene.pb> [iso|iso_back|top|front] [width height]
#   bash wood/tools/shoot_native.sh --example <name> [iso|iso_back|top|front]     builds and runs examples/elements/<name>, shoots its live.pb
#                                                                                into docs/images/elements/<name>.png
#   LABELS=<file.labels> bash wood/tools/shoot_native.sh <out.png> <scene.pb>     the drawing at half scale, each "x y z|text" line of the file
#                                                                                a dot on its point and its text beside it
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
    width=3200
    height=2000
else
    out="$1"
    pb="$2"
    view="${3:-iso}"
    width="${4:-3200}"
    height="${5:-2000}"
fi

ppm="$(mktemp --suffix=.ppm)"
if [ -z "${LABELS:-}" ]; then
    "$SHOT" "$pb" "$ppm" "$width" "$height" "$view"
    convert "$ppm" -fuzz 2% -trim +repage -trim +repage -bordercolor "rgb(248,248,248)" -border 80 -depth 8 "$out"
else
    # the drawing trimmed and halved; a "x y z|text" label's text beside its projected pixel (moved by the trim and the halving), a "|text" label a
    # line of the legend under the drawing
    "$SHOT" "$pb" "$ppm" "$width" "$height" "$view" "$LABELS" "$ppm.labels"
    read -r w h x y <<< "$(convert "$ppm" -fuzz 2% -format "%@" info: | tr 'x+' '  ')"
    pad=300
    draw=()
    while IFS='|' read -r xy text; do
        read -r px py <<< "$xy"
        dx=$(awk -v p="$px" -v o="$x" -v b="$pad" 'BEGIN { printf "%.0f", (p - o) * 0.5 + b }')
        dy=$(awk -v p="$py" -v o="$y" -v b="$pad" 'BEGIN { printf "%.0f", (p - o) * 0.5 + b }')
        draw+=(-draw "text $((dx + 10)),$((dy - 14)) '$text'")
    done < "$ppm.labels"
    legend=$(grep '^|' "$LABELS" | sed 's/^|//' || true)
    convert "$ppm" -crop "${w}x${h}+${x}+${y}" +repage -resize 50% -bordercolor "rgb(248,248,248)" -border "$pad" \
        -font DejaVu-Sans -pointsize 28 -fill "rgb(30,30,30)" "${draw[@]}" -trim +repage -bordercolor "rgb(248,248,248)" -border 30 "$ppm.png"
    convert -background "rgb(248,248,248)" -fill "rgb(60,60,60)" -font DejaVu-Sans -pointsize 24 -size "$(identify -format %w "$ppm.png")x" caption:"$legend" "$ppm.legend.png"
    convert "$ppm.png" "$ppm.legend.png" -background "rgb(248,248,248)" -gravity west -append +repage -bordercolor "rgb(248,248,248)" -border 30 -depth 8 "$out"
    rm -f "$ppm.png" "$ppm.legend.png"
    rm -f "$ppm.labels"
fi
rm -f "$ppm"
echo "$out"
