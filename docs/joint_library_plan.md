# Joint library: plan for the unfinished joints {#elements_joint_library_plan}

[TOC]

State on 2026-11-21: 400 of the 407 joints of the Joinery Compendium (Bauer, Pauli 2024) have a record in `data/joints/*.yaml`;
7 have none and 14 are simplified. Each joint below has what is wrong or missing, the reference, and the step that solves it.
A joint is done when its record passes `tests/joint_assembly.cpp`, its picture is checked against the book's projections
(`tools/compare_joint_projections.py`) and it is committed with CI green.

## 1. Missing joints that the record format can already express

| Page | Joint | Reading | Step |
|---|---|---|---|
| 744 | Wedged four-way splayed cogged joint | Two pairs of beams in a post, splayed cogs, wedged | Extend p. 742 to four beams at two heights |

Kawai status (2026-11-21): every one of the six wedges is cut correctly on its own (288 000 mm3 each), but a member's
three wedges meet only along the diagonal, and the one cut that takes all three away (`JointElement::removal`, shells side by
side, subtracted by Manifold) leaves the member with one finger. Uniting the steps with a Manifold union first gives a mesh that
is not closed; a core prism about the diagonal did not help. The fix belongs in the solid boolean, not in the records.

References: Kawai tsugite, [woodgears.ca](https://woodgears.ca/puzzles/3way_joint.html), [Instructables](https://www.instructables.com/Kawai-Tsugite-Amazing-Three-way-Japanese-Joint).

## 2. Simplified joints to correct

| Page | Joint | Simplification | Step |
|---|---|---|---|
| 92, 98 | Folding-wedge splices | One blind tapered key | Two folding wedges (wedge parts with a flat face) |
| 750 | Four-way rod tenon | Pegs missing | Pegs as parts, their holes through the rods by a P step |
| 748 | Three-way rod tenon | Haunch wedge missing | A wedge part in the haunch |
| 538, 540 | Spline tenon joints | Spline without its dovetail | The spline as a prism part with a dovetail section |
| 154 | Scarf | Hidden rabbet missing | A rabbet region on both members |
| 218 | Scarf | Undercut missing | An undercut plane in the cog |
| 88 | Cogged lap with floating piece | Its tenon drawn as a loose tenon | The tenon as a second prism part fixed to the piece |
| 364, 842 | Canvas stretcher key, radial dovetail | Approximate shapes | Re-read the projections, fix the planes |

## 3. Joints that need more than straight moves

| Page | Joint | Why | Step |
|---|---|---|---|
| 792 | Keyhole batten | Lugs drop through the keyholes, then slide | A two-leg insertion path in AssemblySequence |
| 736, 740, 742 | Two-way joints in a through post | Beams enter the post's slot, then drop onto each other | The same two-leg path, then the post runs on through |
| 730 | Chidori puzzle | Locks with a quarter turn of the round-notched stick | A turn as the last move, or the record marked as turned with the check skipped |
| 732 | Cross puzzle | Three identical A pieces do not come apart one at a time in my reading | Re-read the B pieces from the steps; if two pieces must move together, group moves in AssemblySequence |

Step 3 changes the kernel of the assembly check: a member's way in becomes a path of straight legs, each swept against the
members still in. It is proposed before it is built.

## 4. Review of the finished joints

Done on 2026-11-22: every one of the 400 records was compared with the book's spread (projections and cutting steps) and with
at least one external source, mostly the JAANUS dictionary entry for its Japanese name: 282 ok, 111 wrong, 7 unsure. The
wrong and unsure ones were corrected record by record, each passing the joint test and checked again against the book.
Still differing from the book, most because the assembly check moves each piece along one straight line:

| Page | Joint | What it needs |
|---|---|---|
| 110, 144, 150 | Splices with T-shaped tips | Two moves: along the beam by the wedge or pin gap, then across |
| 148 | Splice with an interrupted rabbet | Two moves past the tooth in the lip |
| 400, 572 | Housed dovetails in a closed pocket | Two moves: into the entry pocket, then along the carrier |
| 538 | Right-angle spline tenon | The spline's dovetail end: drop into the housing, then slide |
| 740, 742 | Two-way joints in a post | The post running on through: in along the beam, then down |
| 784, 790 | Blind dovetail edge, keyed batten | Into an escape pocket or slot, then sideways |
| 566 | Diagonally cut T-bridle | Turning only the post 45 degrees (roll turns both members) |
| 240 | Tapered dovetail with mitred shoulders | The book's way of assembly is unclear |

Two-step moves in AssemblySequence (a piece's way in as a path of straight legs) would settle all but the last two.


## 5. Corrections from the user's answers (2026-11-22)

| Page | Joint | Reading | State |
|---|---|---|---|
| 736 | Wedged two-way cogged lap | An upright post with two crossing through mortises; two thin boards, the same piece, one flipped, hooked inside the post; a wedge from each far face, under one board and over the other | Rebuilt (`upright`, `carrier_end`); board b goes in over a and drops onto its hook, written in `moves` |
| 650 | Four-part cogged cross lap | Four identical pieces: two slide together along their length into one bar (half-width tongues, a cog over the partner's tongue tip), the other two the same turned; the upper bar goes down onto the lower as one | Built; the pair moving as one is written in `moves` |

A record with `moves` names a way in the assembly check cannot take yet (two legs, or two pieces at once); the test skips its
come-apart check, as for `sealed`.
| 242, 404 | Kawai joint, splice and corner | From the user's STEP model: the end cube split by three planes through its body diagonal and the midpoints of its edges into six 60 degree sectors over the middle third of the diagonal; each piece keeps its root corner and three alternate sectors, the other piece the rest, sliding apart along the diagonal. The corner is the same cube with the second body on a face next to the first | Built; the removed sectors overlap by 0.01 mm about the diagonal so their shells never share an edge (the old one-finger boolean) |

A way in can now be a path: `insertions: "v; m1; m2"` approaches along v, then makes the moves m1, m2 (JointElement::target_legs),
and AssemblySequence sweeps a member back along those moves before sliding it out, so such a joint is checked for real
(pp. 400, 572: into a keyhole mortise in the side face, then along the beam into the undercut). Turns (Chidori, p. 730) are
not paths yet.
