#!/usr/bin/env bash
# Every picture of docs/joint_library.md: the tiles of each oracle design and the parameter sweeps, rendered with matplotlib from what
# joint_tiles writes. Needs a python with numpy and matplotlib, python3 or the one PYTHON names.
#   bash tools/joint_library_figures/make.sh
set -euo pipefail
cd "$(dirname "$0")/../.."
HERE=tools/joint_library_figures
PY=${PYTHON:-python3}
WORK=$(mktemp -d)
OUT=docs/images/joint_library
mkdir -p "$WORK/tiles" "$WORK/sweep" "$OUT"
cmake --build build --target joint_tiles --parallel 6 > /dev/null

# the tiles of every design the oracle builds
./build/joint_tiles "$WORK/tiles"
for f in "$WORK"/tiles/*.txt; do
    "$PY" "$HERE/draw_tile.py" "$WORK/tiles" "$WORK/tiles" "$(basename "$f" .txt)"
done
for f in "$OUT"/tile_*.png; do
    name=$(basename "$f" .png)
    cp "$WORK/tiles/${name#tile_}.png" "$f"
done

# the sweeps: one tile pair per parameter set
ids=""
for d in 4 8 12; do for s in 0.0 0.5 1.0; do ids+=" ip/ss_e_ip_1/$d/$s op/ss_e_op_1/$d/$s op/ss_e_op_2/$d/$s"; done; done
for d in 4 8 16; do for s in 0.25 0.5 0.75; do ids+=" ts/ts_e_p_2/$d/$s ts/ts_e_p_3/$d/$s"; done; done
for n in 1 2 3 4 5; do for s in 0.25 0.5 0.75; do ids+=" cr/cr_c_ip_$n/$s"; done; done
for d in 2 4 6; do ids+=" r/ss_e_r_2/$d/0.5 r/ss_e_r_3/$d/0.5 ip/ss_e_ip_2/$d ip/ss_e_ip_5/$d"; done
for t in 0 0.25 0.5; do for c in 0 1; do ids+=" op/ss_e_op_4/8/$t/$c/1"; done; done
for d in 4 8 12; do ids+=" op/ss_e_op_5/$d/0 op/ss_e_op_6/$d op/ss_e_op_17/$d"; done
for d in 4 6 8; do for s in 0.5 0.95; do ids+=" tt/tt_e_p_2/$d/$s/8"; done; done
for l in 30 60 90; do ids+=" tt/tt_e_p_3/$l/12/8 tt/tt_e_p_4/$l/12/8 tt/tt_e_p_5/$l/0.95/8"; done
# shellcheck disable=SC2086
./build/joint_tiles "$WORK/sweep" $ids

sweep() { "$PY" "$HERE/draw_sweep.py" "$WORK/sweep" "$OUT/sweep_$1.png" "$2" "$3" "${@:4}"; }
grid() { for d in $2; do for s in $3; do echo "divisions_${d},_shift_${s}|$1/$d/$s"; done; done; }
sweep ss_e_ip_1 "ss_e_ip_1: divisions (rows) and shift (columns)" 3 $(grid ip/ss_e_ip_1 "4 8 12" "0.0 0.5 1.0")
sweep ss_e_op_1 "ss_e_op_1: divisions (rows) and shift (columns)" 3 $(grid op/ss_e_op_1 "4 8 12" "0.0 0.5 1.0")
sweep ss_e_op_2 "ss_e_op_2: divisions (rows) and shift (columns)" 3 $(grid op/ss_e_op_2 "4 8 12" "0.0 0.5 1.0")
sweep ts_e_p_2 "ts_e_p_2: divisions (rows) and shift (columns)" 3 $(grid ts/ts_e_p_2 "4 8 16" "0.25 0.5 0.75")
sweep ts_e_p_3 "ts_e_p_3: divisions (rows) and shift (columns)" 3 $(grid ts/ts_e_p_3 "4 8 16" "0.25 0.5 0.75")
MODE=male sweep cr_c_ip "cr_c_ip_1 to cr_c_ip_5 (rows): shift (columns)" 3 $(for n in 1 2 3 4 5; do for s in 0.25 0.5 0.75; do echo "cr_c_ip_${n},_shift_${s}|cr/cr_c_ip_$n/$s"; done; done)
MODE=key sweep ss_e_r "ss_e_r_2 and ss_e_r_3 (rows): divisions (columns)" 3 $(for n in 2 3; do for d in 2 4 6; do echo "ss_e_r_${n},_divisions_${d}|r/ss_e_r_$n/$d/0.5"; done; done)
MODE=key sweep ss_e_ip_2_5 "ss_e_ip_2 and ss_e_ip_5 (rows): divisions (columns)" 3 $(for n in 2 5; do for d in 2 4 6; do echo "ss_e_ip_${n},_divisions_${d}|ip/ss_e_ip_$n/$d"; done; done)
sweep ss_e_op_4 "ss_e_op_4, 8 divisions: taper (rows) and chamfer (columns)" 2 $(for t in 0 0.25 0.5; do for c in 0 1; do echo "taper_${t},_chamfer_${c}|op/ss_e_op_4/8/$t/$c/1"; done; done)
sweep ss_e_op_5_6_17 "ss_e_op_5, ss_e_op_6, ss_e_op_17 (rows): divisions (columns)" 3 $(for d in 4 8 12; do echo "ss_e_op_5,_divisions_${d}|op/ss_e_op_5/$d/0"; done; for n in 6 17; do for d in 4 8 12; do echo "ss_e_op_${n},_divisions_${d}|op/ss_e_op_$n/$d"; done; done)
sweep tt_e_p_2 "tt_e_p_2: divisions (rows) and shift (columns)" 2 $(for d in 4 6 8; do for s in 0.5 0.95; do echo "divisions_${d},_shift_${s}|tt/tt_e_p_2/$d/$s/8"; done; done)
sweep tt_e_p_3_4_5 "tt_e_p_3, tt_e_p_4, tt_e_p_5 (rows): division length (columns)" 3 $(for n in 3 4; do for l in 30 60 90; do echo "tt_e_p_${n},_length_${l}|tt/tt_e_p_$n/$l/12/8"; done; done; for l in 30 60 90; do echo "tt_e_p_5,_length_${l}|tt/tt_e_p_5/$l/0.95/8"; done)
rm -rf "$WORK"
