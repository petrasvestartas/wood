# Floor screws: report

Branch `floor-screws`, 2026-10-02, on `eff7463` (dev `578fd8f` + the marks). The rule is in
`docs/floor_parametric_model.md` section 8.1.

## Commits

| commit | what |
|---|---|
| `9598b78` | `JointBeam::screws` with `pre_drill` (proto field 23, `generated/element_joint.pb.*` regenerated with protoc 36.2, only that message changed), `WoodSession::pre_drill_lines`, the five screw relations in `relationships()`, `check_screws`, examples 8 and 9, `check_screws` in `tests/floor_elements.cpp`, the docs |
| this one | this report |

## Data model

Each screw line is stored once, on its connector (`connector_screws_<i>`). The connector names every
member the screw passes as a target and makes no cut. `WoodSession::pre_drill_lines(guid)` reads a
member's lines from the connectors that name it, so both sides of a joint read the same line. The
`pre_drill` flag, the lines and the targets are serialised in the Joint proto, and the session graph
holds an edge from the connector to every target. The viewer draws each screw as a Dowel child
`<connector>_screw_<i>`, an exact cylinder of radius 2.

## Screws per kind

| kind | rule | square | 3000 x 2400 |
|---|---|---|---|
| `screw_rib_beam` | outer rib into the seam beam, along the beam, at h/4 and h/2 (above the tie pocket) | 16 | 16 |
| `screw_beam_mitre` (red) | seam beam into the oculus beam, along the oculus beam from the seam plane | 16 | 16 |
| `screw_rib_corner` (blue) | along the inner rib axis through the beam corner into the rib end | 16 | 16 |
| `screw_ring` | ring pinwheel corners: through ring q along ring q + 1 | 8 | 8 |
| `screw_oculus` | toe screws from the ring's inner face into the quarters' oculus beams, beyond the wedge | 16 | 16 |
| total | 12 per quarter + 24 oculus | 72 in 36 connectors | 72 in 36 connectors |

Every location has 2 horizontal screws at two heights of the 197 mm joint depth. At an oculus corner,
screws that cross in plan sit at different sevenths of the depth: red and blue interleave, and the
two quarters' mitre screws at a seam differ.

## The oculus rule

The ring beams meet each other only at the pinwheel butts. These are end-on-side joints like an outer
rib on a seam beam, so they get the same rule: screws along the butting beam (`screw_ring`).

The ring meets the quarters' oculus beams side to side, along the oculus wedge. A 200 mm screw cannot
go square across 120 mm of timber, and the wedge fills the middle of the joint. So the screws are toe
screws in the free ends of the contact, driven from the ring's inner face, which can still be reached
from the oculus.

The bottom wedges and the inner plate are 27 mm layers and take no screw.

Assembly order: screw each quarter and the ring as separate assemblies, join them, then drive the
oculus screws.

## Distances (both floors)

* closest two screw axes: 28.143 mm (8 mm required)
* closest screw to a dowel bore (surfaces, each bore run on by its overshoot): 137.272 mm
* closest screw to a pocket or connector part (surfaces, each pocket checked within its own member): 5.441 mm
* every screw is held over its full 200.000 mm by the members it names
* 36 of 36 screw contacts verified by the kernel's search

## Reported misfits

None. No screw needed shortening or dropping, and every joint is deep enough for its two levels.

Worth knowing, though not a misfit: the blue screws follow the inner rib axis through the beam
corner, so each passes three members. The seam beam's end, the oculus beam (at least 24.9 mm of the
screw) and the rib are all named as targets.

## Gates

* Example 1: dump byte-identical, 193 records with 0 failing against compas_tf.
* Examples 2, 4, 5, 6 and 8 (both modes): every dump and console unchanged, apart from timing lines. Example 8's screws go to a new file, `floor_8_screws.txt`.
* Example 9 (both modes): every existing record unchanged, with screw lines added. Against `model_rectangle.txt`: 0 failing. The screws are also written to `floor_9_screws.txt`.
* Examples 8 and 9: report ok, 48 of 48 connectors, 44 of 44 contacts, 0 faceted, 396 of 396 and 384 of 384 dowel bores exact. Element BReps are unchanged because the screws make no cut.
* Tests: `floor_elements` (with the new `check_screws`), `brep_drill`, `joint_elements` and `interaction_ownership` all pass. Every run used `tools/run_guarded.sh -t 10 -m 4`.
* Cost: example 9's connector step takes 235 ms, up from 149 ms, because of the toe-screw aim search.
