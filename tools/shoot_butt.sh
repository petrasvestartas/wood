#!/usr/bin/env bash
# The butt pictures: the solids at 40 percent opacity over the user's axes and mid-plane polygons, drawn red by the examples, so the drawn
# input and the computed members read together. Element examples go into docs/images/elements and truss template examples into docs/images/templates.
#   bash wood/tools/shoot_butt.sh
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WOOD"
CMD="Element Interactions Off;Element Attributes Off;Layers All;Arctic On;Opacity 0.4;View Isometric;View Orthographic;Fit"
mkdir -p docs/images/elements docs/images/templates
# the viewport only (no outliner, no command bar), its background whitened, the subject framed on 2000 x 1250 as the
# committed pictures are: python3 with Pillow
frame() {
    python3 -I - "$1" <<'EOF'
import sys
from PIL import Image, ImageOps
path = sys.argv[1]
shot = Image.open(path).convert("RGB")
w, h = shot.size
view = shot.crop((0, 0, int(w * 0.775), int(h * 0.963))).point(lambda v: min(255, round(v * 255 / 248)))
box = ImageOps.invert(view.convert("L")).point(lambda p: 255 if p > 8 else 0).getbbox()
subject = view.crop(box)
scale = min(1620 / subject.width, 990 / subject.height)
subject = subject.resize((round(subject.width * scale), round(subject.height * scale)), Image.LANCZOS)
out = Image.new("RGB", (2000, 1250), "white")
out.paste(subject, ((2000 - subject.width) // 2, (1250 - subject.height) // 2))
out.save(path)
EOF
}
cmake --build build --target element_joint_beam_butt element_joint_beam_butt_plates templates_truss_1_pratt templates_truss_2_library --parallel 6 > /dev/null
for name in element_joint_beam_butt element_joint_beam_butt_plates; do
    ./build/"$name" > /dev/null
    UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "docs/images/elements/$name.png" data/output/pb/live.pb > /dev/null 2>&1
    frame "docs/images/elements/$name.png"
    echo "$name"
done
./build/templates_truss_1_pratt > /dev/null
UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh docs/images/templates/templates_truss_1_pratt.png data/output/pb/live.pb > /dev/null 2>&1
frame docs/images/templates/templates_truss_1_pratt.png
echo templates_truss_1_pratt
rm -f data/output/pb/truss_*.pb
./build/templates_truss_2_library > /dev/null
for pb in data/output/pb/truss_*.pb; do
    case "$pb" in *truss_bow_*) continue ;; esac
    name=$(basename "$pb" .pb)
    UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "docs/images/templates/$name.png" "$pb" > /dev/null 2>&1
    frame "docs/images/templates/$name.png"
    echo "$name"
done
