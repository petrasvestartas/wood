#!/usr/bin/env bash
# The butt pictures: the solids at 40 percent opacity over the user's axes and mid-plane polygons, drawn red by the examples, so the drawn
# input and the computed members read together. The element examples into docs/images/elements, the frame, the house and every case of
# 22_butt_gallery and 23_butt_zigzag into docs/images/examples.
#   bash wood/tools/shoot_butt.sh
set -euo pipefail
WOOD="$(cd "$(dirname "$0")/.." && pwd)"
cd "$WOOD"
CMD="Element Interactions Off;Element Attributes Off;Layers All;Arctic On;Opacity 0.4;View Isometric;View Orthographic;Fit"
mkdir -p docs/images/examples
cmake --build build --target element_joint_beam_butt element_joint_beam_butt_plates 20_butt_frame 21_butt_house 22_butt_gallery 23_butt_zigzag --parallel 6 > /dev/null
for name in element_joint_beam_butt element_joint_beam_butt_plates 20_butt_frame 21_butt_house; do
    ./build/"$name" > /dev/null
    case "$name" in element_*) out=docs/images/elements/$name.png ;; *) out=docs/images/examples/$name.png ;; esac
    UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "$out" data/output/pb/live.pb > /dev/null 2>&1
    echo "$name"
done
./build/22_butt_gallery > /dev/null
./build/23_butt_zigzag > /dev/null
for pb in data/output/pb/butt_*.pb; do
    name=$(basename "$pb" .pb)
    UI_SHOT_CMD="$CMD" bash tools/shoot_ui.sh "docs/images/examples/$name.png" "$pb" > /dev/null 2>&1
    echo "$name"
done
