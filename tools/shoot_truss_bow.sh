#!/usr/bin/env bash
# The pictures of Bow's trusses (data/templates/truss_bow_1873.json): each graph built by the Truss template, solid in the Arctic look as every
# other picture, a screenshot of the viewer cropped to the truss, into docs/images/templates/truss_bow_<number>.webp,
# lossless, SHOTS at a time (3 by default). Needs python with Pillow and numpy.
#   bash wood/tools/shoot_truss_bow.sh
set -euo pipefail
cd "$(dirname "$0")/.."
SHOTS="${SHOTS:-3}"
mkdir -p docs/images/templates
rm -f data/output/pb/truss_bow_*.pb
if [ -x build/Release/templates_truss_3_bow.exe ]; then
    build/Release/templates_truss_3_bow.exe
else
    cmake --build build --target templates_truss_3_bow --parallel 6 > /dev/null
    ./build/templates_truss_3_bow
fi
slot=0
for pb in data/output/pb/truss_bow_*.pb; do
    name=$(basename "$pb" .pb)
    SHOT_PORT=$((9333 + slot % SHOTS)) "${PYTHON:-python3}" tools/render_scene_picture.py "$pb" "docs/images/templates/$name.webp" || echo "FAILED $name" &
    slot=$((slot + 1))
    if [ $((slot % SHOTS)) -eq 0 ]; then wait; fi
done
wait
