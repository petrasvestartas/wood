# Floor pins: report

Branch `floor-pins`, 2026-10-02, on `eff7463` (dev `578fd8f` + the marks). The rule is in
`docs/floor_parametric_model.md` section 8.1.

## Commits

| commit | what |
|---|---|
| `9598b78` | `JointBeam::headed_pins` with `pre_drill` (proto field 23, `generated/element_joint.pb.*` regenerated with protoc 36.2, only that message changed), `WoodSession::pre_drill_lines`, the five pin relations in `relationships()`, `check_pins`, examples 8 and 9, `check_pins` in `tests/floor_elements.cpp`, the docs |
| this one | this report |

## Data model

Each pin line is stored once, on its connector (`connector_pins_<i>`). The connector names every
member the pin passes as a target and makes no cut. `WoodSession::pre_drill_lines(guid)` reads a
member's lines from the connectors that name it, so both sides of a joint read the same line. The
`pre_drill` flag, the lines and the targets are serialised in the Joint proto, and the session graph
holds an edge from the connector to every target. The viewer draws each pin as a Pin child
`<connector>_pin_<i>`, an exact cylinder of radius 2.

## Pins per kind

| kind | rule | square | 3000 x 2400 |
|---|---|---|---|
| `pin_rib_beam` | outer rib into the seam beam, along the beam, at h/4 and h/2 (above the tie pocket) | 16 | 16 |
| `pin_beam_mitre` (red) | seam beam into the oculus beam, along the oculus beam from the seam plane | 16 | 16 |
| `pin_rib_corner` (blue) | along the inner rib axis through the beam corner into the rib end | 16 | 16 |
| `pin_ring` | ring pinwheel corners: through ring q along ring q + 1 | 8 | 8 |
| `pin_oculus` | toe pins from the ring's inner face into the quarters' oculus beams, beyond the wedge | 16 | 16 |
| total | 12 per quarter + 24 oculus | 72 in 36 connectors | 72 in 36 connectors |

Every location has 2 horizontal pins at two heights of the 197 mm joint depth. At an oculus corner,
pins that cross in plan sit at different sevenths of the depth: red and blue interleave, and the
two quarters' mitre pins at a seam differ.

## The oculus rule

The ring beams meet each other only at the pinwheel butts. These are end-on-side joints like an outer
rib on a seam beam, so they get the same rule: pins along the butting beam (`pin_ring`).

The ring meets the quarters' oculus beams side to side, along the oculus wedge. A 200 mm pin cannot
go square across 120 mm of timber, and the wedge fills the middle of the joint. So the pins are toe
pins in the free ends of the contact, driven from the ring's inner face, which can still be reached
from the oculus.

The bottom wedges and the inner plate are 27 mm layers and take no pin.

Assembly order: pin each quarter and the ring as separate assemblies, join them, then drive the
oculus pins.

## Distances (both floors)

* closest two pin axes: 28.143 mm (8 mm required)
* closest pin to a pin bore (surfaces, each bore run on by its overshoot): 137.272 mm
* closest pin to a pocket or connector part (surfaces, each pocket checked within its own member): 5.441 mm
* every pin is held over its full 200.000 mm by the members it names
* 36 of 36 pin contacts verified by the kernel's search

## Reported misfits

None. No pin needed shortening or dropping, and every joint is deep enough for its two levels.

Worth knowing, though not a misfit: the blue pins follow the inner rib axis through the beam
corner, so each passes three members. The seam beam's end, the oculus beam (at least 24.9 mm of the
pin) and the rib are all named as targets.

## Gates

* Example 1: dump byte-identical, 193 records with 0 failing against compas_tf.
* Examples 2, 4, 5, 6 and 8 (both modes): every dump and console unchanged, apart from timing lines. Example 8's pins go to a new file, `floor_8_pins.txt`.
* Example 9 (both modes): every existing record unchanged, with pin lines added. Against `model_rectangle.txt`: 0 failing. The pins are also written to `floor_9_pins.txt`.
* Examples 8 and 9: report ok, 48 of 48 connectors, 44 of 44 contacts, 0 faceted, 396 of 396 and 384 of 384 pin bores exact. Element BReps are unchanged because the pins make no cut.
* Tests: `floor_elements` (with the new `check_pins`), `brep_drill`, `joint_elements` and `interaction_ownership` all pass. Every run used `tools/run_guarded.sh -t 10 -m 4`.
* Cost: example 9's connector step takes 235 ms, up from 149 ms, because of the toe-pin aim search.
