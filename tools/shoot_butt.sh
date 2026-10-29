#!/usr/bin/env bash
# The butt pictures: the solids at 40 percent opacity over the user's axes and mid-plane polygons, drawn red by the examples, so the drawn
# input and the computed members read together. Element examples go into docs/images/elements and truss template examples into docs/images/templates.
#   bash wood/tools/shoot_butt.sh
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WOOD"
CMD="Element Interactions Off;Element Attributes Off;Layers All;Arctic On;Opacity 0.4;View Isometric;View Orthographic;Fit"
mkdir -p docs/images/elements docs/images/templates
cmake --build build --target element_joint_beam_butt element_joint_beam_butt_plates templates_truss_1_pratt templates_truss_2_library --parallel 6 > /dev/null
for name in element_joint_beam_butt element_joint_beam_butt_plates; do
    ./build/"$name" > /dev/null
    UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "docs/images/elements/$name.png" data/output/pb/live.pb > /dev/null 2>&1
    echo "$name"
done
./build/templates_truss_1_pratt > /dev/null
UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh docs/images/templates/templates_truss_1_pratt.png data/output/pb/live.pb > /dev/null 2>&1
echo templates_truss_1_pratt
rm -f data/output/pb/truss_*.pb
./build/templates_truss_2_library > /dev/null
for pb in data/output/pb/truss_*.pb; do
    name=$(basename "$pb" .pb)
    UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "docs/images/templates/$name.png" "$pb" > /dev/null 2>&1
    echo "$name"
done
