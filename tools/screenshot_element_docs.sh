#!/usr/bin/env bash
# The element pages' pictures: each example under examples/elements built, run and published to the
# live viewer, then opened in a Chrome window on the GPU with the panel hidden, the elements
# alone (their interaction features and attributes off, so a joint's cut outlines and base planes are not drawn), the viewer's Arctic look with
# its soft shadows, full opacity, no grid, the orthographic isometric view, screenshotted at 4800 x 3000 and cropped by
# tools/crop_viewer_shot.py to 3:2 with the viewer's backdrop around the element, which only crops. On Windows the scene is served from this machine
# instead of published: every example is built and run first, its scene kept aside, then the shots run SHOTS at a time (3 by default),
# each Chrome on its own port.
#   bash wood/tools/screenshot_element_docs.sh element_plate element_column ...
set -euo pipefail
cd "$(dirname "$0")/../.."
SHOTS="${SHOTS:-3}"

# one example shot: the scene given, or the published one
shoot() {
    local name="$1" port="$2" pb="${3:-}"
    local view="View Isometric"
    # a column's head and cuts face the inside of the floor, away from the usual corner
    case "$name" in element_column*) view="View Isometric Back" ;; esac
    # the elements selected and fitted, so the hidden attribute rows do not widen the frame, then the selection cleared
    # the command bar and the empty canvas cut off, nothing else; a shot of a scene that had not loaded is taken again
    local try
    for try in 1 2 3; do
        SHOT_PB="$pb" SHOT_PORT="$port" SHOT_SCALE=3 SHOT_QUERY="opacity=1&nomarkers=1&thickness=0.7" node --experimental-websocket wood/tools/screenshot_viewer.mjs "wood/docs/images/elements/$name.png" \
            "Layers Off" "Element Interactions Off" "Element Attributes Off" "Arctic On" "Outline Off" "$view" "View Orthographic" "Select All" "Fit" "Escape" > /dev/null
        if SHOT_ASPECT=1.5 python wood/tools/crop_viewer_shot.py "wood/docs/images/elements/$name.png" > /dev/null; then
            echo "$name"
            return 0
        fi
    done
    echo "FAILED $name"
}

case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*)
        scenes="$(cygpath -m "$(mktemp -d)")" # a Windows path, node reads it
        for name in "$@"; do
            [ -n "${SHOT_NO_BUILD:-}" ] || cmake --build wood/build --config Release --target "$name" --parallel 6 > /dev/null
            if "wood/build/Release/$name.exe" > /dev/null; then
                cp wood/data/output/pb/live.pb "$scenes/$name.pb"
            else
                echo "FAILED $name"
            fi
        done
        slot=0
        for name in "$@"; do
            [ -f "$scenes/$name.pb" ] || continue
            shoot "$name" $((9333 + slot % SHOTS)) "$scenes/$name.pb" &
            slot=$((slot + 1))
            if [ $((slot % SHOTS)) -eq 0 ]; then wait; fi
        done
        wait
        rm -rf "$scenes" ;;
    *)
        for name in "$@"; do
            bash bash/publish-scene.sh --target "$name" > /dev/null
            sleep 2
            shoot "$name" 9333
        done ;;
esac

# the family pages' overview pictures are copies of one example's
while read -r overview example; do
    if [ -f "wood/examples/elements/$example.cpp" ]; then cp "wood/docs/images/elements/$example.png" "wood/docs/images/elements/$overview.png"; fi
done <<'OVERVIEWS'
element_cut_plane element_beam_variable_cut
element_joint element_joint_drill
element_joint_beam element_joint_beam_wedge
element_joint_beam_connectors element_joint_beam_butterfly_key
element_joint_beam_cross element_joint_beam_cross_half_lap
element_joint_beam_end_to_end element_joint_beam_jupiter_scarf
element_joint_beam_pins element_joint_beam_headed_pins
element_joint_beam_top_side element_joint_beam_top_side_milled_wedge
element_joint_plate element_joint_plate_parameters
element_joint_plate_cross element_joint_plate_cross_slot
element_joint_plate_side_side_inplane element_joint_plate_side_side_inplane_dovetails
element_joint_plate_side_side_outplane element_joint_plate_side_side_outplane_fingers
element_joint_plate_side_side_rotated element_joint_plate_side_side_rotated_hooks
element_joint_plate_top_side element_joint_plate_top_side_three_tenons
element_joint_plate_top_top element_joint_plate_top_top_circle_pins
OVERVIEWS
