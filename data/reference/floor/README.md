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
