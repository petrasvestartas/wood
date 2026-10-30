#!/usr/bin/env bash
# The pictures of Bow's trusses (data/templates/truss_bow_1873.json): each graph built by the Truss template, in the style of
# the other truss pictures (tools/shoot_butt.sh): the solids at 40 percent opacity over the red guide lines, isometric, the
# viewport framed on white, into docs/images/templates/truss_bow_<number>.webp at 1200 x 750. Needs python3 with Pillow.
#   bash wood/tools/shoot_truss_bow.sh
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WOOD"
CMD="Element Interactions Off;Element Attributes Off;Layers All;Arctic On;Opacity 0.4;View Isometric;View Orthographic;Fit"
mkdir -p docs/images/templates
cmake --build build --target templates_truss_3_bow --parallel 6 > /dev/null
rm -f data/output/pb/truss_bow_*.pb
./build/templates_truss_3_bow
png="$(mktemp --suffix=.png)"
for pb in data/output/pb/truss_bow_*.pb; do
    name=$(basename "$pb" .pb)
    UI_SHOT_ZOOM=0 UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "$png" "$pb" > /dev/null 2>&1
    python3 -I - "$png" "docs/images/templates/$name.webp" <<'EOF'
import sys
from PIL import Image, ImageOps
shot = Image.open(sys.argv[1]).convert("RGB")
w, h = shot.size
view = shot.crop((0, 0, int(w * 0.775), int(h * 0.963))).point(lambda v: min(255, round(v * 255 / 248)))
box = ImageOps.invert(view.convert("L")).point(lambda p: 255 if p > 8 else 0).getbbox()
subject = view.crop(box)
scale = min(1620 / subject.width, 990 / subject.height)
subject = subject.resize((round(subject.width * scale), round(subject.height * scale)), Image.LANCZOS)
out = Image.new("RGB", (2000, 1250), "white")
out.paste(subject, ((2000 - subject.width) // 2, (1250 - subject.height) // 2))
out.resize((1200, 750), Image.LANCZOS).save(sys.argv[2], quality=82, method=6)
EOF
    echo "$name"
done
rm -f "$png"
