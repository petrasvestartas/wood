# compas_tf reference dumps of the square floor

The parity records of `wood_floor` (`src/templates/floor/`) against compas_tf's floor examples
(`compas_tf/examples/example_model_*.py`, consumed, never edited), all for the example-1
configuration: half spans 3000 / 3000, column head 220 / 120, outer and inner ribs 100 / 60,
beams 60, wedge 240, t-sections 27, height 650, rise 453, oculus 1000. Generated on 2026-10-01 by
the scripts beside them and reproduced byte for byte on 2026-10-02; `docs/floor_parametric_model.md`
section 9 lists the gates that read them.

| file | records | compas_tf source | generator | wood dump it is compared with |
|---|---|---|---|---|
| `reference_floorguide.txt` | 193 | the `FloorGuide` of example 1: plan polygons, construction planes (origin + normal), quads, the four parabolas with their +t / +2t offsets, block levels, bed planes, every member outline | `dump_floorguide.py` | `templates_floor_1_floorguide` -> `data/output/pb/floor_1_floorguide.txt` |
| `reference_models.txt` | 145 | examples 4 + 5: every quarter and oculus element as `name volume cx cy cz` (mm3, box centre in world mm) | `reference_models.py` | `templates_floor_4_quarters` + `templates_floor_5_oculus` -> `floor_4_quarters.txt` + `floor_5_oculus.txt` |
| `reference_wedges.txt` | 65 | example 6: the 8 ring contacts (pair, wedge thickness, length, dowels, area), then the carved ring beams, the wedges and their dowels | `reference_wedges.py` | `templates_floor_6_contacts_floor` -> `floor_6_contacts_floor.txt` |
| `reference_rectangle.txt` | 21 | example 8, first half: the 8 column x outer rib contacts (area, rib thickness, connector origin and x axis), the carved columns and outer ribs | `reference_rectangle.py` | `templates_floor_8_contacts_cantilevers` -> `floor_8_contacts_cantilevers.txt` |
| `reference_tie.txt` | 17 | example 8, second half: the 4 rib seam contacts (area, tie origin and y axis), the tie bodies, the outer ribs carved by both connectors | `reference_tie.py` | the same example -> `floor_8_ties.txt` (its own file, since both halves carry a `contacts` count record) |

## Regenerating

The scripts read each other by relative path (`reference_tie.py` runs the whole chain), so run them
from this directory, with compas_tf on the path, in a Python that has compas_tf's dependencies
(compas, compas_model, compas_manifold), and under the guard:

    cd data/reference/floor
    export PYTHONPATH=../../../../compas_tf/src
    ../../../tools/run_guarded.sh -t 10 -m 4 -- python dump_floorguide.py reference_floorguide.txt
    ../../../tools/run_guarded.sh -t 10 -m 4 -- python reference_models.py reference_models.txt
    ../../../tools/run_guarded.sh -t 10 -m 4 -- python reference_wedges.py reference_wedges.txt
    ../../../tools/run_guarded.sh -t 10 -m 4 -- python reference_rectangle.py reference_rectangle.txt
    ../../../tools/run_guarded.sh -t 10 -m 4 -- python reference_tie.py reference_tie.txt

Each takes under five seconds.

## Comparing

`tools/compare_dumps.py <reference> <candidate> [tolerance] [--relative <ratio>]` matches the records
by name, words exactly and numbers within the tolerance, and prints
`<N> records, <F> failing, worst deviation <D>`:

    python3 tools/compare_dumps.py data/reference/floor/reference_floorguide.txt data/output/pb/floor_1_floorguide.txt 1e-6
    cat data/output/pb/floor_4_quarters.txt data/output/pb/floor_5_oculus.txt > data/output/pb/floor_4_5_models.txt
    python3 tools/compare_dumps.py data/reference/floor/reference_models.txt data/output/pb/floor_4_5_models.txt 1e-6 --relative 1e-9
    python3 tools/compare_dumps.py data/reference/floor/reference_wedges.txt data/output/pb/floor_6_contacts_floor.txt 1e-6 --relative 1e-9
    python3 tools/compare_dumps.py data/reference/floor/reference_rectangle.txt data/output/pb/floor_8_contacts_cantilevers.txt 1e-6 --relative 1e-9
    python3 tools/compare_dumps.py data/reference/floor/reference_tie.txt data/output/pb/floor_8_ties.txt 1e-6 --relative 1e-9

The volume records differ from compas_tf in the last printed digits of the volume only (2e-12
relative for the plain members, 1.5e-11 for the carved ring beams of example 6, coordinates exact
at 1e-6), hence `--relative 1e-9`. Two differences are by design and no gate: compas_tf's wedge
dowels are 8-sided prisms while wood's drill meshes are finer, so the `connector_wedge_k_cylinder_j`
volumes differ by about 1.1e4 mm3 (G4 reads the wedge bodies, the carved beams and the contact
lines); and the example-8 columns and ribs carry the support joint, the 12 mm foot recess and the
assembly dowels compas_tf does not have, so their volumes are compared with the wood baseline
instead (G5). The contact lines (`contacts`, `wedge k`, `connector k`, `tie k`) are written by the
examples from the floor's relationship table (plan step 5): the pair in the order compas_tf's
pairwise search listed it (the member earlier in the scene first), the thicker member, the wedge
length and dowel count, the contact area, the plate origin on the contact's top edge with its x
axis toward the rib, and for a tie the contact normal toward the first member; the connectors are
numbered in that search order, so `connector_wedge_2` is the seam between quarters 0 and 3.

## The model's own baseline (plan step 7)

The model departs from compas_tf by decision in two places on the square: the central panel's
+t / +2t layers are offsets in the panel's own cross-section (`CentralLayers::section`, exactly
`tsections` thick on every plan) instead of compas_tf's offsets made in the outer rib's plane and
swept (`docs/floor_parametric_model.md` 4.4, step 7a); and the middle column cutter level is the
deeper of the corner's two outer rib bottoms on their fan planes (`CutterLevel::rib_bottom`,
-694.793 on the square) instead of compas_tf's -(height + 1.65 tsections) = -694.55 (R8, step 7b). The examples build the model by default; run them with
`--compas` for compas_tf's parity mode (`Floor::compas_parity`), which the gates above read.

| file | records | what | wood dump it is compared with |
|---|---|---|---|
| `model_floorguide.txt` | 193 | example 1 with the model's definitions | `templates_floor_1_floorguide` (no argument) |
| `model_models.txt` | 145 | examples 4 + 5 with the model's definitions | `templates_floor_4_quarters` + `templates_floor_5_oculus` (no argument) |
| `model_contacts_cantilevers.txt` | 21 | example 8 with the model's definitions: every column contact the full rib end face, 70238.714887 mm2, and the carved columns | `templates_floor_8_contacts_cantilevers` (no argument) |

Gate G1b: the model's dump against `reference_floorguide.txt` fails in the central row only, bed row 1
(`beds/6..11`), flanges 2b / 3a (`tsections/2`, `tsections/3`), `bed_top_planes/1` and
`wedges_inner_beams/1`, each vertex moved at most 2.666 mm on a +t edge and 5.332 mm on a +2t edge
(the design's bounds 2.897 / 5.794 are the untrimmed traces'); every other record within 1e-6;
and it matches `model_floorguide.txt` within 1e-6:

    python3 tools/compare_dumps.py data/reference/floor/model_floorguide.txt data/output/pb/floor_1_floorguide.txt 1e-6
    python3 tools/compare_dumps.py data/reference/floor/model_models.txt data/output/pb/floor_4_5_models.txt 1e-6 --relative 1e-9

In examples 4 + 5 the same members change volume (beds_1_*, tsections_2/3_*, wedges_inner_beams_1_*,
36 records); examples 6 and 8 do not read the central row.

The cutter level moves the twelve `column_cutters/*` records of example 1 by 0.243 mm at the level
(at most 0.741 mm on the narrowed bottom chamfer cutter), the eight column contacts of example 8 from
70214.105060 to the full 70238.714887 mm2 (origins and axes unchanged), the head cut from
34777378.362 to 34771221.351 mm3 (`tests/floor_elements.cpp` pins both, the carved column of example 2
176866956.468 -> 176873113.478 mm3) and nothing else.

## The rectangle (plan step 8, gate G8)

`templates_floor_9_rectangle` builds `Floor(FloorPlan::rectangle(3000, 2400), FloorSizes{})`, a
6000 x 4800 bay. compas_tf has no rectangular assembly, so parity is per quarter view (R1): run with
`--compas` (compas_tf's oculus rule and parity definitions) and every quarter is written in its
corner frame, mapped onto compas_tf's quarter 0 of `FloorGuide` with that quarter's half spans:

    ../../../tools/run_guarded.sh -t 10 -m 4 -- python dump_floorguide.py reference_floorguide_3000x2400.txt 3000 2400
    ../../../tools/run_guarded.sh -t 10 -m 4 -- python dump_floorguide.py reference_floorguide_2400x3000.txt 2400 3000

| file | records | compared with |
|---|---|---|
| `reference_floorguide_3000x2400.txt` | 193 | `floor_9_rectangle_q0.txt`, `_q2.txt` (`--compas`) |
| `reference_floorguide_2400x3000.txt` | 193 | `floor_9_rectangle_q1.txt`, `_q3.txt` (`--compas`) |
| `model_rectangle.txt` | 59 | `floor_9_rectangle.txt` (no argument): the report, every contact relationship's area, the counts; with one rib level per column (the short ribs' column plates 69605.340 mm2) |

R1 reads the 141 records compas_tf gets right on a rectangle: every polygon but `oculus_points`, every
plane and quad, the parabolas and shadows, the block levels, `bed_top_planes/0` and `/2`, the outer
ribs, the inner beams, blocks 0 / 2, flanges `tsections/0` / `/5`, bed rows 0 / 2 and the cutters (the
inner ribs and the central row are left out: compas_tf is inconsistent there, design 4.1):

    grep -v '^oculus_points\|^inner_ribs/\|^wedges_inner_beams/1/\|^tsections/[1-4]/\|^beds/\([6-9]\|1[01]\)/\|^bed_top_planes/1 \|^oculus/' \
        data/reference/floor/reference_floorguide_3000x2400.txt > r1_0.txt
    python3 tools/compare_dumps.py r1_0.txt data/output/pb/floor_9_rectangle_q0.txt 1e-6

and gives `141 records, 0 failing, worst deviation 0.000e+00` in all four views.
