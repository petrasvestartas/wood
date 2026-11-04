#!/usr/bin/env bash
# The pictures of the truss library (templates_truss_2_library) and the Pratt truss: each scene solid in the Arctic look as every other picture, a
# screenshot of the viewer cropped to the truss, into docs/images/templates/<name>.png, SHOTS at a time (3 by default). Needs python with Pillow and numpy.
#   bash wood/tools/shoot_truss_library.sh
set -euo pipefail
cd "$(dirname "$0")/.."
SHOTS="${SHOTS:-3}"
BUILD=build
[ -d build/Release ] && BUILD=build/Release
rm -f data/output/pb/truss_*.pb
"$BUILD"/templates_truss_1_pratt > /dev/null
cp data/output/pb/live.pb data/output/pb/truss_templates_truss_1_pratt.pb
"$BUILD"/templates_truss_2_library > /dev/null
slot=0
for pb in data/output/pb/truss_*.pb; do
    name=$(basename "$pb" .pb)
    name=${name#truss_templates_}
    case "$name" in truss_1_pratt) name=templates_truss_1_pratt ;; esac
    SHOT_PORT=$((9333 + slot % SHOTS)) "${PYTHON:-python3}" tools/render_scene_picture.py "$pb" "docs/images/templates/$name.png" || echo "FAILED $name" &
    slot=$((slot + 1))
    if [ $((slot % SHOTS)) -eq 0 ]; then wait; fi
done
wait
