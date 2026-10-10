#!/usr/bin/env bash
# Every picture of docs/joint_library.md, drawn by the session viewer's renderer (tools/shoot_native.sh: Arctic, black outlines), two per
# design and never overlapping: <id>_unit.png, the design's male and female outlines in its unit box with the parameters the user can
# change written under it, and <id>.png, the oracle's pair drawn
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
./build/joint_tiles "$WORK/sweep" $(
    for d in 4 8 12; do for s in 0.0 0.5 1.0; do echo "ip/ss_e_ip_1/$d/$s op/ss_e_op_1/$d/$s op/ss_e_op_2/$d/$s"; done; done
    for d in 4 8 16; do for s in 0.25 0.5 0.75; do echo "ts/ts_e_p_2/$d/$s ts/ts_e_p_3/$d/$s"; done; done
    for n in 1 2 3 4 5; do for s in 0.25 0.5 0.75; do echo "cr/cr_c_ip_$n/$s"; done; done
    for d in 2 4 6; do echo "r/ss_e_r_2/$d/0.5 r/ss_e_r_3/$d/0.5 ip/ss_e_ip_2/$d ip/ss_e_ip_5/$d"; done
    for t in 0 0.25 0.5; do for c in 0 1; do echo "op/ss_e_op_4/8/$t/$c/1"; done; done
    for d in 4 8 12; do echo "op/ss_e_op_5/$d/0 op/ss_e_op_6/$d op/ss_e_op_17/$d"; done
    for d in 4 6 8; do for s in 0.5 0.95; do echo "tt/tt_e_p_2/$d/$s/8"; done; done
    for l in 30 60 90; do echo "tt/tt_e_p_3/$l/12/8 tt/tt_e_p_4/$l/12/8 tt/tt_e_p_5/$l/0.95/8"; done
)
rm -f "$OUT"/*.png
for pb in "$WORK"/tiles/*.pb; do
    png="$OUT/$(basename "$pb" .pb).png"
    bash tools/shoot_native.sh "$png" "$pb" > /dev/null
    # a unit box's legend under it: the factory call and what the user can change, with the values drawn
    legend="${pb%.pb}.txt"
    if [ -f "$legend" ]; then
        width=$(identify -format %w "$png")
        convert "$png" \( -size "$((width - 160))x" -background "$BG" -fill "rgb(40,40,40)" -font DejaVu-Sans -pointsize 64 \
            caption:"$(cat "$legend")" -bordercolor "$BG" -border 80x40 \) -gravity west -append -depth 8 "$png"
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
sweep ss_e_ip_1 "ss_e_ip_1: divisions (rows) and shift (columns)" 3 unit $(grid ip/ss_e_ip_1 "4 8 12" "0.0 0.5 1.0")
sweep ss_e_op_1 "ss_e_op_1: divisions (rows) and shift (columns)" 3 unit $(grid op/ss_e_op_1 "4 8 12" "0.0 0.5 1.0")
sweep ss_e_op_2 "ss_e_op_2: divisions (rows) and shift (columns)" 3 unit $(grid op/ss_e_op_2 "4 8 12" "0.0 0.5 1.0")
sweep ts_e_p_2 "ts_e_p_2: divisions (rows) and shift (columns)" 3 unit $(grid ts/ts_e_p_2 "4 8 16" "0.25 0.5 0.75")
sweep ts_e_p_3 "ts_e_p_3: divisions (rows) and shift (columns)" 3 unit $(grid ts/ts_e_p_3 "4 8 16" "0.25 0.5 0.75")
sweep cr_c_ip "cr_c_ip_1 to cr_c_ip_5 (rows): shift (columns)" 3 plates $(for n in 1 2 3 4 5; do for s in 0.25 0.5 0.75; do echo "cr_c_ip_$n, shift $s|cr/cr_c_ip_$n/$s"; done; done)
sweep ss_e_r "ss_e_r_2 and ss_e_r_3 (rows): divisions (columns)" 3 plates $(for n in 2 3; do for d in 2 4 6; do echo "ss_e_r_$n, divisions $d|r/ss_e_r_$n/$d/0.5"; done; done)
sweep ss_e_ip_2_5 "ss_e_ip_2 and ss_e_ip_5 (rows): divisions (columns)" 3 plates $(for n in 2 5; do for d in 2 4 6; do echo "ss_e_ip_$n, divisions $d|ip/ss_e_ip_$n/$d"; done; done)
sweep ss_e_op_4 "ss_e_op_4, 8 divisions: taper (rows) and chamfer (columns)" 2 unit $(for t in 0 0.25 0.5; do for c in 0 1; do echo "taper $t, chamfer $c|op/ss_e_op_4/8/$t/$c/1"; done; done)
sweep ss_e_op_5_6_17 "ss_e_op_5, ss_e_op_6, ss_e_op_17 (rows): divisions (columns)" 3 unit $(for d in 4 8 12; do echo "ss_e_op_5, divisions $d|op/ss_e_op_5/$d/0"; done; for n in 6 17; do for d in 4 8 12; do echo "ss_e_op_$n, divisions $d|op/ss_e_op_$n/$d"; done; done)
sweep tt_e_p_2 "tt_e_p_2: divisions (rows) and shift (columns)" 2 plates $(for d in 4 6 8; do for s in 0.5 0.95; do echo "divisions $d, shift $s|tt/tt_e_p_2/$d/$s/8"; done; done)
sweep tt_e_p_3_4_5 "tt_e_p_3, tt_e_p_4, tt_e_p_5 (rows): division length (columns)" 3 plates $(for n in 3 4; do for l in 30 60 90; do echo "tt_e_p_$n, length $l|tt/tt_e_p_$n/$l/12/8"; done; done; for l in 30 60 90; do echo "tt_e_p_5, length $l|tt/tt_e_p_5/$l/0.95/8"; done)
# only the pictures the page shows
for f in "$OUT"/*.png; do
    grep -q "joint_library/$(basename "$f")" docs/joint_library.md || rm "$f"
done
rm -rf "$WORK"
