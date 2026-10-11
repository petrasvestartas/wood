#!/usr/bin/env bash
# Every joint-unit picture of the JointPlate family pages, drawn by the session viewer's renderer (tools/shoot_native.sh: Arctic, black outlines), two per
# design and never overlapping: <id>_unit.png, the design's male and female outlines in its unit box at half scale, its axes, sides and parameters
# labelled, and <id>.png, the oracle's pair drawn
# apart, the plates with the merged outlines and the joint's solids (keys, drills). The parameter sweeps are grids of either, labelled.
#   bash tools/joint_library_figures/make.sh
set -euo pipefail
cd "$(dirname "$0")/../.."
WORK=$(mktemp -d)
OUT=docs/images/joint_library
BG="rgb(248,248,248)"
mkdir -p "$WORK/tiles" "$WORK/sweep" "$OUT"
cmake --build build --target joint_tiles --parallel 6 > /dev/null

# every design the oracle builds, each scene shot as it is
./build/joint_tiles "$WORK/tiles"
# the rotated designs on a pair folded 120 degrees, and the snap fit, which the oracle does not list
./build/joint_tiles "$WORK/tiles" "r@120/rotated_mitre" "r@120/rotated_hooks/4/0.5" "r@120/rotated_keys/4/0.5" "ts/tee_snap_fit"
# the trait de Jupiter, which the oracle joins in plane, and the designs the oracle does not list
./build/joint_tiles "$WORK/tiles" "ip/jupiter_scarf" "ip/in_plane_cross_splayed_scarf" "op/corner_mitre_tenon" "op/corner_linked_tenons_wall/8"
# the designs that belong to beams: the cross joints on two crossing beams, the wedge on a tee, square and at 60 degrees
./build/joint_tiles "$WORK/tiles" $(for d in cross_slot cross_sliced_half_lap cross_half_lap cross_half_lap_diagonal_drills cross_half_lap_drill cross_brussels_half_lap tee_milled_wedge; do echo "beam/$d/90 beam/$d/60"; done)
./build/joint_tiles "$WORK/sweep" $(
    for d in 4 8 12; do for s in 0.0 0.5 1.0; do echo "ip/in_plane_dovetails/$d/$s op/corner_fingers/$d/$s op/corner_graded_fingers/$d/$s"; done; done
    for d in 4 8 16; do for s in 0.25 0.5 0.75; do echo "ts/tee_dense_tenons/$d/$s ts/tee_tenons/$d/$s"; done; done
    for n in 1 2 3 4 5; do for s in 0.25 0.5 0.75; do echo "cr/cross_$n/$s"; done; done
    for d in 2 4 6; do echo "r/rotated_hooks/$d/0.5 r/rotated_keys/$d/0.5 ip/in_plane_butterfly_keys/$d ip/in_plane_tooth_keys/$d"; done
    for t in 0 0.25 0.5; do for c in 0 1; do echo "op/corner_tenons/8/$t/$c/1"; done; done
    for d in 4 8 12; do echo "op/corner_linked_tenons/$d/0 op/corner_linked_tenons_wall/$d op/corner_capped_fingers/$d"; done
    for d in 4 6 8; do for s in 0.5 0.95; do echo "tt/stacked_circle_pins/$d/$s/8"; done; done
    for l in 30 60 90; do echo "tt/stacked_ring_pins/$l/12/8 tt/stacked_grid_pins/$l/12/8 tt/stacked_rectangle_pins/$l/0.95/8"; done
    for f in op op@120 op@150; do for d in 4 8 12; do echo "$f/corner_fingers/$d/0.5"; done; done
    for f in ts ts@skew75 ts@lean80; do for d in 8 16 24; do echo "$f/tee_tenons/$d/0.5"; done; done
    for f in ip ip@trapezoid ip@short; do for d in 4 8 12; do echo "$f/in_plane_dovetails/$d/0.5"; done; done
    for s in 0.5 0.64 0.9; do for d in 4 8 12; do echo "tpl/chevron/corner_fingers/$d/$s"; done; done
)
rm -f "$OUT"/*.png
for pb in "$WORK"/tiles/*.pb; do
    png="$OUT/$(basename "$pb" .pb).png"
    labels="${pb%.pb}.labels"
    if [ -f "$labels" ]; then
        LABELS="$labels" bash tools/shoot_native.sh "$png" "$pb" > /dev/null
    else
        bash tools/shoot_native.sh "$png" "$pb" > /dev/null
    fi
done
for pb in "$WORK"/sweep/*.pb; do
    bash tools/shoot_native.sh "$WORK/sweep/$(basename "$pb" .pb).png" "$pb" > /dev/null
done

# a sweep: its name, title (for the reader of this script), columns, the picture kind (unit or plates) and label|id pairs
sweep() {
    local name=$1 columns=$3 kind=$4
    shift 4
    local args=()
    for pair in "$@"; do
        local file="${pair#*|}"
        file="$WORK/sweep/${file//\//_}"
        [ "$kind" = unit ] && file+="_unit"
        args+=(-label "${pair%%|*}" "$file.png")
    done
    montage "${args[@]}" -tile "${columns}x" -geometry 960x720+24+24 -pointsize 40 -background "$BG" -depth 8 -colors 256 "$OUT/sweep_$name.png"
}
grid() { local IFS=" "; for d in $2; do for s in $3; do echo "divisions $d, shift $s|$1/$d/$s"; done; done; }
IFS=$'\n'
sweep in_plane_dovetails "in_plane_dovetails: divisions (rows) and shift (columns)" 3 unit $(grid ip/in_plane_dovetails "4 8 12" "0.0 0.5 1.0")
sweep corner_fingers "corner_fingers: divisions (rows) and shift (columns)" 3 unit $(grid op/corner_fingers "4 8 12" "0.0 0.5 1.0")
sweep corner_graded_fingers "corner_graded_fingers: divisions (rows) and shift (columns)" 3 unit $(grid op/corner_graded_fingers "4 8 12" "0.0 0.5 1.0")
sweep tee_dense_tenons "tee_dense_tenons: divisions (rows) and shift (columns)" 3 unit $(grid ts/tee_dense_tenons "4 8 16" "0.25 0.5 0.75")
sweep tee_tenons "tee_tenons: divisions (rows) and shift (columns)" 3 unit $(grid ts/tee_tenons "4 8 16" "0.25 0.5 0.75")
sweep cross "cross_sliced_half_lap to cross_brussels_half_lap (rows): shift (columns)" 3 plates $(for n in 1 2 3 4 5; do for s in 0.25 0.5 0.75; do echo "cross_$n, shift $s|cr/cross_$n/$s"; done; done)
sweep rotated "rotated_hooks and rotated_keys (rows): divisions (columns)" 3 plates $(for n in 2 3; do for d in 2 4 6; do echo "rotated_$n, divisions $d|r/rotated_$n/$d/0.5"; done; done)
sweep in_plane_butterfly_keys_5 "in_plane_butterfly_keys and in_plane_tooth_keys (rows): divisions (columns)" 3 plates $(for n in 2 5; do for d in 2 4 6; do echo "in_plane_$n, divisions $d|ip/in_plane_$n/$d"; done; done)
sweep corner_tenons "corner_tenons, 8 divisions: taper (rows) and chamfer (columns)" 2 unit $(for t in 0 0.25 0.5; do for c in 0 1; do echo "taper $t, chamfer $c|op/corner_tenons/8/$t/$c/1"; done; done)
sweep corner_linked_tenons_6_17 "corner_linked_tenons, corner_linked_tenons_wall, corner_capped_fingers (rows): divisions (columns)" 3 unit $(for d in 4 8 12; do echo "corner_linked_tenons, divisions $d|op/corner_linked_tenons/$d/0"; done; for n in 6 17; do for d in 4 8 12; do echo "corner_$n, divisions $d|op/corner_$n/$d"; done; done)
# a fixture's rows: label|fixture, then divisions across
rows() { local IFS=" " design=$1 divisions=$2; shift 2; for row in "$@"; do for d in $divisions; do echo "${row%%|*}, divisions $d|${row#*|}/$design/$d/0.5"; done; done; }
sweep corner_fingers_angles "corner_fingers at 90, 120 and 150 degrees (rows): divisions (columns)" 3 plates $(rows corner_fingers "4 8 12" "90 degrees|op" "120 degrees|op@120" "150 degrees|op@150")
sweep tee_tenons_angles "tee_tenons square, skewed 75, leaning 80 (rows): divisions (columns)" 3 plates $(rows tee_tenons "8 16 24" "square|ts" "skewed 75 degrees|ts@skew75" "leaning 80 degrees|ts@lean80")
sweep in_plane_dovetails_seams "in_plane_dovetails on a straight, slanted and short seam (rows): divisions (columns)" 3 plates $(rows in_plane_dovetails "4 8 12" "straight seam|ip" "slanted seam|ip@trapezoid" "short seam|ip@short")
sweep corner_fingers_chevron "corner_fingers on a pair of the Chevron template: shift (rows) and divisions (columns)" 3 plates $(for s in 0.5 0.64 0.9; do for d in 4 8 12; do echo "shift $s, divisions $d|tpl/chevron/corner_fingers/$d/$s"; done; done)
sweep stacked_circle_pins "stacked_circle_pins: divisions (rows) and shift (columns)" 2 plates $(for d in 4 6 8; do for s in 0.5 0.95; do echo "divisions $d, shift $s|tt/stacked_circle_pins/$d/$s/8"; done; done)
sweep stacked_ring_pins_4_5 "stacked_ring_pins, stacked_grid_pins, stacked_rectangle_pins (rows): division length (columns)" 3 plates $(for n in 3 4; do for l in 30 60 90; do echo "stacked_$n, length $l|tt/stacked_$n/$l/12/8"; done; done; for l in 30 60 90; do echo "stacked_rectangle_pins, length $l|tt/stacked_rectangle_pins/$l/0.95/8"; done)
# only the pictures the page shows
for f in "$OUT"/*.png; do
    grep -qr "joint_library/$(basename "$f")" docs/elements || rm "$f"
done
rm -rf "$WORK"
