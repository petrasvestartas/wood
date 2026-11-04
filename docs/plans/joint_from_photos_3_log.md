# Joint from photos 3: the keyed box connector, step log

Written as the work goes. Earlier entries are never edited; a correction is a new entry.

Reference images, all of someone else's built project. The mechanism is taken from them, the proportions are my own. None of their artwork goes in the docs.

| name | file | what |
|---|---|---|
| P1 | f2e8f391 | four stages of assembly, on white, 735 x 687 px, so small |
| P2 | 0f7124be | exploded building with labels, 331 x 331 px |
| P3 | 905cb73e | line drawing of the same building |

The user's words: connect two plates top to top using such a mechanism, "the first photos explains it the most that it contains a third box connector that has keys to lock it after inserting".

## Step 1: first reading, before any zoom

Looked at: the three images, nothing else.

Concluded from P1, panel by panel:

- Stage 1, top left: two plywood plates stand apart, faces parallel. Each has a square hole through its face. Beside them lies a long block, the connector, with a shallow slot in one side and two smaller blocks crossing it near one end. A separate flat key lies below.
- Stage 2, top right: the long block is pushed through both holes, sticking out both sides. The plates still stand apart with a gap. The slot in the block shows at its left end. The key still lies aside.
- Stage 3, bottom left: the two plates are now pushed together, face to face. Key still aside.
- Stage 4, bottom right: the key is in, in the slot, on top of the block, on the left side of the plates. Everything locked.

The mechanism as I read it: the connector goes through both mortises. A shoulder on one side stops it, a crossing piece that can not pass the hole. A key dropped into a slot on the other side stops it coming back, and because the slot is tapered or tight it also pulls the plates together. In P2 the label reads "wood wedge connector" and in P3 the holes are square.

P3 and P2 add: in the building these are wall plates, faces against each other at the shared edge? Not sure. In P3 each wall panel has a square hole and a mechanical fastener at its edge. The holes go through the panel. So there the boxes sit in the holes of a wall and something else, maybe the top plate. P2 labels "wedge connector" near the walls.

Top to top: faces together, which is what the user asks for: wood's top_top contact. Stage 3 and 4 show exactly that.

Unsure:

- How many parts the long block is. It may be one solid with a shoulder, or a box with loose keys on both sides, since the user says "keys" and P1 stage 1 shows blocks crossing it at one end.
- Whether the crossing pieces in stage 1 are glued in or loose, and in which order they go.
- Whether the visible key is a wedge. It looks parallel at this size.
- What stops the block moving across the mortise, presumably the mortise itself.

Next: enlarge each panel.

## Step 2: enlarged panels, which changed the reading

Looked at: each panel at 4x to 5x. The image is only 735 px, so edges are soft; a reading here is good to about 2 px of the original.

- WRONG in step 1: "two smaller blocks crossing it near one end". It is ONE key, already sitting in the connector, passing through it and sticking out of both narrow faces. Same part as the loose one. So there are three loose parts besides the plates: the box and two identical keys. The user's "keys" is plural, which fits.
- WRONG in step 1: "a shallow slot in one side". It is a through mortise, running across the box from one narrow face to the other. It shows on both narrow faces, and in stage 2 the wood's dark inside shows between the plates. It is one long mortise, from near the left end to the right key's far side, running through both plates. So BOTH keys sit in the same mortise, one each side of the plates.
- WRONG in step 1: "a shoulder on one side stops it". The first key is the shoulder.
- WRONG in step 1: "it looks parallel". Stage 4: the left key is 175 px wide at the top and 130 px at the bottom in my 5x crop. The right key tapers too, on its outer flank. The flank against the plate stays straight. The stage 1 loose key shows the same slant, though perspective hides how much. P2 says "wedge connector". So they are wedges, wide end up, on both sides.
- The thin line along the box's top edge in stages 2 to 4, left end, is a lamination seam. Not a cut. The box itself looks like glued layers; modelled as one solid. Inferred.
- The plate's hole in stage 1 is seen very obliquely. P3 shows the same kind of hole square. Size: fits the box section with no visible gap.

The sequence as now read:

1. First key in the box, at the far end of the mortise.
2. Box slid through both plates, plain end first, until the first key stops on the near plate. The plates can still be apart.
3. Far plate pushed along the box against the near one. That uncovers the mortise on the far side.
4. Second key driven in against the far plate. The two wedges pinch the plates between them.

Rough proportions read from stages 1 and 3, camera height unknown, so about 15 percent, in plate thicknesses t: box 9.6 long, 2.3 wide, 1.7 deep, mortise 0.7 to 0.9 thick, key 1.8 wide at the box's face, sticking out about 0.5. I am told to use my own proportions, so these only set the order of size. No camera was solved here: with my own proportions a residual would mean nothing.

NOT shown, inferred:

- The mortise's end walls. Only their top edge can be seen. If they are square, a wedge touches them along one line. I will slant them to match the key, so the wedge bears on a face.
- Whether the keys' thin faces fit the mortise tight. Taken as tight.
- What the mortise does between the plates: shown open, modelled open.

P2 and P3 add nothing to the mechanism beyond the word wedge and square holes in panels.

## Step 3: reading the code, and what it decided

- This is no plate tile: a tile has a male and a female, and here three more solids exist. So it is a connector, `JointBeam::keyed_box`, built like `butterfly_key`: `parts` are the box and two keys, `cutters` the plates' mortises, one `add_interaction` per plate. It takes a top_top contact. Colours and names come with the connector.
- Obstacle: a connector's `solid_features` cut every part, so the box's mortise would have swallowed the keys inside it. Fix in `part_features`: a cut that would leave nothing of a part is another part's. Chosen over a new protobuf field. This changes a shared function for every connector. `joint_beams` runs the others.
- The keys' directions go on the end of `insertions`.

## Step 4: state of the FIRST attempt, before any fix

Did: wrote the factory, the example, `check_keyed_boxes`. Built first go. Example ran. Test passed all five cases on its first run: 40 and 40, 24 and 24, 30 and 50, other sizes, plates lying flat.

Measured at 40 and 40: seated overlap 0.000 mm3, all four stages clear 0.000, past home 576, key stops on plate 1920, second key before the plates meet 13440, least blocked 576, box 1385472 of 1385472 by hand.

Why I did not stop there: a pass on the first run proves little when I wrote the test too. Reading it critically:

- It moved one piece at a time. The claim is "nothing comes out", and a plate and a key could leave TOGETHER. Weak test, changed to every group of pieces.
- Its check that a box with both keys can not enter used one hand-picked position, and a sliver could pass. Changed to a sweep.
- Nothing refused a straight key, which would fall through, or a box too big for the plates. The factory built both. Changed: returns null.

None of these is yet known to hide a fault. Nothing has been looked at in the viewer yet.

## Step 5: harder test, and a real defect only the picture found

The harder test passed at once: every group of the five pieces, 30 groups, 6 ways, 180 moves: 174 blocked, least 576 mm3 at 40 and 40, and the 6 free moves are exactly a key or both straight out, seen from either side. The sweep for a box with both keys gives 38400 mm3. The straight key, the mortise wider than the box and a box past the plates are refused.

Then the first look in the viewer: the mortise opens on the box's SIDE. Keys go in horizontally, across the plates' width, not down from above as in P1. Cause: I took the key direction from `top_edge`, whose longest edge in the upper half of a 320 x 400 contact is a vertical one. So the first attempt was WRONG in orientation, and the example showed it.

180 checks could not see it, because the mechanism works in any orientation, and my test plates were square, where `top_edge` happened to give the right answer. Green tests, wrong joint.

Fix: on standing plates the keys come out straight up, world z in the contact's plane; flat plates fall back to the contact's longest edge. The test plates are now 300 x 400 and it checks the direction, so this can not come back unseen.

## Step 6: after the fix, final numbers

`check_keyed_boxes`, 300 x 400 plates, each case against the built solids. All in mm3. Every case: all pieces closed, seated overlap 0.000, all four stages clear 0.000, 174 blocked moves, the 6 free ones are only keys straight out, the box, a key and the plate match the hand volumes.

| plates | case | past home | key stops on plate | second key too soon | least blocked |
|---|---|---|---|---|---|
| 40, 40 | defaults, standing | 576 | 1920 | 13440 | 576 |
| 24, 24 | defaults, standing | 207 | 691 | 2903 | 207 |
| 30, 50 | defaults, standing | 324 | 1080 | 5670 | 324 |
| 40, 40 | larger box, 0.2 taper | 1728 | 2880 | 38880 | 1728 |
| 40, 40 | defaults, flat | 576 | 1920 | 13440 | 576 |

- Sequence order, proved: the second key can not go in before the plates meet, and the box can not enter with both keys in.
- Without the second key the first plate slides off and the box backs out, both 0.000.
- Wedge: raised 8 mm a key leaves 1.00 mm of play at 0.125 taper, clear at 0.9 of that and blocked at 1.5.
- What the test does NOT prove: friction, wood strength, or how far a real key can be driven. Nominal fit, no tolerance. The moves are straight lines; turning a piece out is not tested. The first key is not shown to be blocked from going all the way through by friction, only by the taper.

The example's plates are 320 x 400, the test's 300 x 400. Mesh check: the volumes above are of the built solids, nothing is compared to intended sizes.

Shared change: `part_features`. `joint_beams` and `joint_pins` pass.

Found while checking that: `floor_elements` crashes, exit 0xC00000FD, a stack overflow, in `check_floors_in_scene`, right after the drill feature line. Tested: with the old `part_features` loop put back it crashes at the same place, so the shared change is not the cause, and `keyed_box` is not on that path. NOT established: whether it dates from my column or bridle commits or earlier. `Column::round` is the only earlier change of mine there that I would suspect, and it does not run for a floor's square columns. Not fixed, not investigated further; reported.

## Step 7: comparison, plain verdict

Sheet: `docs/plans/joint_from_photos_3/compare_four_stages.jpg`. Top row: P1, someone else's project, scaled up, source stated on the sheet. Bottom row: real viewer screenshots of this joint from nearly the same direction. Layout of loose parts is mine.

| stage | what it shows | verdict |
|---|---|---|
| 1 | plates apart, box with first key, loose key | Matches: mortise holes, long mortise, seated key, wedge |
| 2 | box through both, stopped by first key, plates apart | Matches |
| 3 | plates together, mortise uncovered | Matches |
| 4 | second key in | Matches: two keys pinching the plates, wide end up |

Differs, on purpose or not:

- Proportions are my own: box 7.4 t long against about 9.6 in P1, plates smaller against the box, narrower keys.
- Colour, plywood and all surfaces are not comparable. No camera solved, so no residuals.
- P1's keys are tight in by hand. Mine stop exactly home.
- The docs picture is from the back isometric, because the near plate hid the first key from the default one. One line added to `tools/screenshot_element_docs.sh`.

NOT shown by the images, inferred: slanted mortise end walls, tight key faces, the box's glue-up modelled as one solid, wedge angle within about 30 percent since the image is so small, and both keys being identical.

## Step 8: did it work the first try

No. The mechanism did: the part list, sequence and wedge action needed no change once the close-ups had corrected my first reading, which had four errors.

The joint itself was wrong in the first attempt: its keys entered sideways on tall plates. All tests passed on that wrong joint. Only the picture found it, and square test plates had hidden it. That is the lesson of this job, and it is in the skill's traps list.
