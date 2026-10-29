#!/usr/bin/env bash
# The pictures of Bow's trusses (data/templates/truss_bow_1873.json): each graph built by the Truss template, seen from the front,
# the viewport cropped to the truss, into docs/images/templates/truss_bow_<number>.webp. Needs python3 with Pillow.
#   bash wood/tools/shoot_truss_bow.sh
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WOOD"
CMD="Element Interactions Off;Element Attributes Off;Layers All;Arctic On;Opacity 0.4;View Front;View Orthographic;Fit"
mkdir -p docs/images/templates
cmake --build build --target templates_truss_3_bow --parallel 6 > /dev/null
rm -f data/output/pb/truss_bow_*.pb
./build/templates_truss_3_bow
png="$(mktemp --suffix=.png)"
for pb in data/output/pb/truss_bow_*.pb; do
    name=$(basename "$pb" .pb)
    UI_SHOT_ZOOM=0 UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "$png" "$pb" 1600 1000 > /dev/null 2>&1
    python3 -I - "$png" "docs/images/templates/$name.webp" <<'EOF'
import sys
from PIL import Image, ImageOps
viewport = Image.open(sys.argv[1]).convert("RGB").crop((0, 0, 1062, 955))
box = ImageOps.invert(viewport.convert("L")).point(lambda p: 255 if p > 12 else 0).getbbox()
margin = 30
viewport.crop((max(0, box[0] - margin), max(0, box[1] - margin), min(viewport.width, box[2] + margin),
               min(viewport.height, box[3] + margin))).save(sys.argv[2], quality=82, method=6)
EOF
    echo "$name"
done
rm -f "$png"
