# Joint from photos 2: the five-member node, step log

Written as the work goes. Earlier entries are never edited; a correction is a new entry.

Photos, short names used below:

| name | file | view |
|---|---|---|
| A | 24308ed9 | oblique, from above the node, X of four arms |
| B | 04cf22bb | oblique, lower, from the side, shows the round column and the broad sides |
| C | 63f5ec96 | straight at the node along the column axis, close |

The node is assembled in all three. Nothing is shown apart. The underside is not shown at all. That is the main limit of this job.

The user's description: 5 elements into one node, 1 column, 4 beams, 2 beams form the main scarf-like joint, the 2 others are secondary and meet those beams with a dovetail-like joint.

## Step 1: first reading of the photos, before any zoom or measuring

Looked at: the three photos, nothing else.

Concluded:

- The column is round, about twice a beam's width across. In C we look straight down its axis. Call that axis Z, the side we see the top.
- Four beams leave the node in plan as a cross at 90 degrees. Their tops look flush with the top of the column's head. Their sections look taller than wide (B).
- The column's head has a cross-shaped slot, leaving four corner posts, seen as end grain with one rounded outer edge each. Pencil lines and carpenter's numbers are on the posts and beams.
- Each beam is narrowed to about 0.7 of its width where it enters the slot, leaving a shoulder close to the column's circle.
- Main beams, left and right in C, marked 1 and 2. Each ends in a long tongue reaching across the whole node to the far shoulder of the other. The tongues lie side by side, the seam between them runs at a small slant, so each tongue is wider at its tip than at its root. That is the scarf: the seam is a wedge that locks them against pulling apart along the beam.
- Secondary beams, up and down in C, marked 3 and 4. Each ends against a main beam's tongue with one dovetail, the tail wider at its tip, not centred on the beam.
- The whole plan looks unchanged when turned 180 degrees about the column axis.

Assembly, as I read it now: every joint face seen from the top is a wall parallel to Z. So I expect everything goes in from above: main beams into the slot first, secondary beams last. NOT seen: whether the dovetail pockets go through the height, how deep the slot is, what the underside looks like.

Unsure:

- Slot depth, beam height, and whether the beams stop on the slot's floor.
- Whether the seam or the dovetails also taper in height. All I can see of them is the top.
- Why the edges along the seams in B look stepped. It could be real, or just the fine grain.
- How this differs from a two-member tile. It looks like it does not fit one: one column and four beams share one space.

Next: zoom on the seams and the posts, then measure C.

## Step 2: close-ups

Looked at: 2x crops of the plan (C), the seams (A, B), the column (A, B).

- The stepped edges along the seams in B are grain ridges meeting a gap, not cut steps. Nothing to model.
- WRONG in step 1, by omission: I took the slot as just a cross. Each beam's FULL width also enters the circle a little before its shoulder, so the column head has a shallow housing for each beam as well. The post's outline shows it: beyond the beam's side line the post runs on to the circle, inside it the post stops at the shoulder.
- The strip seen in A and B, which looks like a lath running into the node, is the column's own round surface between two beams' sides. It ends where the beam's underside leaves the column. So its length is the beam's height. That gives the height from a view that sees it side on: about 2 W, far taller than the plan suggests. The beams are about 1:2.
- Column in B: a plain cylinder. Its far end is seen as a rim, not its end face.
- Tips of the dovetails have rounded corners, so they were milled. Small gaps show at both tongue tips, at the slot walls and around the tails. Nominal fit assumed.
- Marks on the wood, 1 and 2 on the main tongues, 3 and 4 on the secondary beams, and a matching mark on each post. So I name the members by them.


## Step 3: the round column against the house rule

The rule: round means exact BRep. wood's feature path rebuilds planar faces and round holes only, and `Column` takes a polyline section, so a slotted round column is not supported today. A many-sided polygon would break the rule.

Tried the kernel's native `BRep::boolean_difference` on a cylinder, in a throwaway probe:

- with one 30-face cross-shaped tool: threw, and uncaught it killed the process, 0xC0000409.
- with one plain box: "Interference trim lost a required original raw event".
- with the cylinder turned 0.7 rad so its seam lies in a post: "Curve intersection exceeded its subdivision or floating-point resolution limit".

Concluded: kernel booleans can not cut this cylinder. Kernel changes are out of scope for this job, so I will not patch them. Next: the head is an extrusion, so try building it exactly with no boolean at all.

## Step 4: measuring the plan, and what kind of joint this is

Read off a 2x crop of C, W = beam width = 995 px, plan centre taken from the slots' walls:

| what | measured | taken as |
|---|---|---|
| beam width, four arms | 995 to 998 px | W |
| tenon width, both directions | 700 to 705 px | 0.70 W |
| column radius, eight arc points | 987 to 1012 px, mean 992 | 1.00 W, so diameter 2 W |
| shoulder from the column axis, four arms | 775 to 800 px | 0.80 W |
| seam slope over the tongues | 150 in 1550 | 1 in 10, 5.7 deg |
| tongue tips from the axis | 0.76 and 0.78 W | at the shoulder, 0.80 W, so nominal fit, the 2 to 4 mm gap not kept |
| dovetail centre from the axis | 0.14 and 0.15 W, opposite sides | 0.145 W |
| dovetail root, depth | 0.286 W, 0.146 W | as measured |
| dovetail flare, per side | 0.14 to 0.21, mean 0.176 | 10 deg |

Height: from step 2, about 2 W. To be checked with cameras. Slot depth: NOT visible. Taken equal to the beam height, so the beams sit on the slot's floor.

Decision: this does NOT fit the two-member tile. A plate tile has one male and one female in one box; here five solids share one space, each with its own cut. So it is built as the wood rules say a joint is built: one element, `JointBeam::column_head`, a connector with one set of cutters per target, passed to the five members with one `add_interaction` each. Its plan is drawn in units of W about the column axis, the same way a tile is.

Decision on the column: keep the Manifold mesh for every cut and measurement, and rebuild an exact BRep from that mesh's own faces, no kernel boolean: a whole cylinder up to the slot's floor, then each post's faces, arcs and cylinder patches made exact.

## Step 5: state of the FIRST attempt, before any fix

Did: wrote `Column::round`, the exact column BRep, `JointBeam::column_head`, the example. First build failed on two missing includes, in shared wood_core sources, so briefly the other job could not have built it either. Fixed within minutes. Built, ran, shot one view drawn apart.

What it showed, unfixed:

- All five members closed solids. The two main beams equal in volume, 9.43e6 mm3, the two secondary equal, 9.12e6. No fit test yet.
- Structure as read from the photos: four posts, cross slot, shoulders, tongues, tails.
- A defect in the first exact column: vertical lines down the plain body. A chord strip on a post is one face from base to head, so its patch overlapped the lower cylinder. Not a shape error, a drawing one, but it breaks the look of a round column. Fix: patches start at the slot's floor.
- Beam and column lengths still provisional.

## Step 6: cameras, height, lengths

Did: solved cameras for A and B from top-plane corners only, plan as measured from C, model unchanged. Then read off what C can not show.

| photo | corners | mean | worst |
|---|---|---|---|
| A | 15 | 1.6 mm | 3.2 mm |
| B | 8 | 2.0 mm | 3.6 mm |

B first read 10 mm on average, worst 26. That was one corner I had misread, `S3r`, the bottom-right shoulder. Without it the mean fell from 39 to 9.8 px. It is left out, not corrected, so B rests on 8 corners.

- WRONG in step 2: height about 2 W. Measured on four vertical edges of arm ends in B: 176 to 189 mm, mean 183 to 186 for any focal length from 5550 up. Taken as 185.
- Arm reach and column length are NOT well fixed. They trade off against the focal length, which B can not fix: at f 5550 the column is 435 and the main arm 357, at 9000 it is 405 and 385. A is the better conditioned photo: main 374 and 395, secondary 465 and 464. Taken: main 385, secondary 465, column 425. Treat them as within 5 percent.
- The secondary beams are longer than the main ones, ratio about 0.83, stable in both photos. I had read all four as equal.

## Step 7: test results, the mesh check, and what they do not cover

`joint_beams`, new `check_column_head`, first run, at the photographed size:

| measure | result |
|---|---|
| seated overlap, all 10 pairs | 0.000 mm3 |
| column head filled | 5810415 of 5810415 mm3, no void |
| first tongue, by hand | 951811 of 951811 mm3 |
| any beam lowered 300 past any member, in 10 mm steps | 0.000 mm3, so all 24 orders of the four beams go in |
| least overlap of a 2 mm move any way but up, others seated | 1641 mm3 |
| scarf pulled 2 mm along the beam, the two main beams alone | 5846 mm3, so the seam holds |
| the same pair moved 2 mm along the seam | 0.000 mm3, so the seam alone does NOT hold that |
| that move against the column | 3424 mm3, the slot's walls stop it |

Assembly answer: everything drops in along the column, every wall being parallel to it. Nothing fixes an order. The carpenter's numbers 1, 2, 3, 4 suggest main beams then secondary beams, and that order works, but in the model so does any other. Holds only if the walls run the full height, which the photos do not show. If the pockets are blind or the tails stop short, the secondary beams must go last, and the underside will differ.

Mesh check: all 15 corners used in step 6 are built vertices, worst 0.000 mm, and the built extents are the intended 385, 465, 425, 185. The bridle's mistake is not repeated here.

New defects found while looking at the sheets and thinking past the photographed size:

- The faint lines along the column body are still there. My step 5 fix was incomplete: a chord cut by a slot wall gives an L-shaped sliver, not a quad, which fell through as a plain wall down to the base.
- `column_head` reaches a fixed 1.4 W with the slot cutter, so a column wider than 2.8 W would not be cut through. Holds only near the photographed proportion. Nothing shows this at the measured size.
- The test ran one size, axis-aligned. Not good enough.

## Step 8: after the fixes, and one claim withdrawn

- The sliver fix worked: the column body is smooth, one line left, the cylinder's seam.
- `column_head` now takes the slot's reach from the column's real radius, and returns null for beams with no profile, where it used to give empty cutters silently.
- `check_column_head` now runs five cases. All pass:

| beam | radius | turned | seated | lowered clear | least blocked | scarf pulled | along seam | column stops it |
|---|---|---|---|---|---|---|---|---|
| 100 x 185 | 100 | 0 | 0.000 | 0.000 | 1641 | 5846 | 0.000 | 3424 |
| 100 x 185 | 100 | 30 | 0.000 | 0.000 | 1641 | 5846 | 0.000 | 3424 |
| 80 x 200 | 120 | 0 | 0.000 | 0.000 | 1363 | 5040 | 0.000 | 4684 |
| 120 x 150 | 100 | 45 | 0.000 | 0.000 | 1639 | 5700 | 0.000 | 2921 |
| 100 x 185 | 300 | 0 | 0.000 | 0.000 | 1641 | 5846 | 0.000 | 11166 |

All in mm3. The head filled and the tongue by hand match at every size. The turned case gives the same numbers as the unturned one, as it must. The sizes are the model's, not a photographed object's.

- WITHDRAWN: I called the column BRep exact in step 4 and in the tests' "exact faces" line. Measured: `is_solid` 1, but that is only the whole cylinder under the head. BRep volume 9.50e6 mm3, true 8.95e6. Its mesh is not closed. It is true arcs and cylinder patches drawn side by side, right to the eye, with no facets, and NOT a sewn solid. So "round means BRep" is met for the look and not for the topology. The closed Manifold mesh is what every number here was measured on, never this BRep. The doc comment on `Column::model_geometry_brep` now says so.
- `Column::radius` is not in the protobuf. It survives `place` and `transformed`, but a column read back from a .pb loses it and falls back to the polygon. Adding a field needs a proto regeneration, left out here.
- Regression: `joint_library` 124 of 135, the same 11 failing variants as before this job, ids 14 and 15. `joint_beams` passes.

## Step 9: comparison, plain verdict

Sheets in `docs/plans/joint_from_photos_2/`, photo beside viewer, cropped and set to one size only. Directions from the solved cameras. The photos converge harder than the viewer's camera.

| sheet | tests | verdict |
|---|---|---|
| compare_c_plan | the plan, straight down the column | Matches: seam, tongues, offset tails, shoulders, four posts. Mean 1.6 mm on A's 15 corners, same plan |
| compare_a_above | above, oblique | Matches: the plan, the strip of column between beams, proportions |
| compare_b_side | low, side | Matches in layout. Beam ends land less well, see below |
| overlay_a | built model over A | Good within about 10 mm at the arm ends |
| overlay_b | built model over B | Node good, arm ends off by up to about 30 mm in the photo's perspective |

Reason for overlay_b: B's 8 corners all lie in the small top area of the node, and they fix its long-range perspective poorly. The focal length came out at the top of the searched range, so 7000 was used for the picture. This is a weak camera, not proven error in the arms, but it is also why the lengths carry 5 percent.

NOT shown by the photos, inferred:

- The whole underside. Nothing below the tops is seen in the joint.
- Slot depth, taken as the beam height. Tongues, seam and tails taken as straight through the height. Had the dovetails tapered or stopped, the assembly answer would change.
- That the column ends square at both ends and stops at 425: its far end is only seen as a rim.
- Any peg or wedge. None is visible; the scarf has to be held by the column's walls and the top of the node is open, so something must hold the beams down in service. Not modelled.
- The assembly order. See step 7: the model does not fix one.

Not modelled, seen: rounded tail corners, the 2 to 4 mm gaps at the tongue tips and around the tails, eased edges. Carpenter's marks are not modelled.

## Step 10: answer to "did it work the first try"

For the plan of the joint, yes: the first build's plan needed no change and fits A to 1.6 mm, all five solids closed, and later tests found no overlap.

No for the rest:

- Height: my close-up reading of 2 W was wrong. Measured 1.85 W from B, before the fit test.
- The four arms: I first read them equal. The secondary arms are longer.
- The exact column took three goes: one vertical-line defect, an incomplete fix, then the sliver fix. And it is still not a sewn solid.
- `column_head` held only near the photographed proportion until the reach was fixed.
- First build failed on two missing includes, in shared wood_core sources.

## Step 11: the user's marked line on the post, stopped for the kernel fix

The user marked a line down a post's round face in compare_a_above and asked for a clean boolean. Looked at: that sheet at 3x, beside compare_b_side.

- Two different lines, not one. On the plain body, the cylinder's own seam: `create_cylinder` has one seam edge, and `drill_brep` places it wherever the frame's x axis falls. On the post, a line along the middle of the round face, from the slot's floor to the head. That one is mine: the post is built from cylinder patches, and there is a boundary between two of them. The body in that same view shows no line, so it is not the seam carrying on.
- Not traced: why the post's strips split into two runs there. Suspected, not proved: the angle wrap or the height grouping in `round_cut_brep`. Stopped, because that code is due to be replaced.
- Sewn solid: not close. Step 8 stands. `round_cut_brep` lays faces side by side with no shared edges, volume 9.50e6 against 8.95e6, mesh open. Sewing it was not started.

Plan changed by the main session: `brep-boolean` is fixing `BRep::boolean_difference` in session_cpp, and the column will be rebuilt through the real boolean then. Nothing changed in the code for this step. So the lines are still there, and the page's note stands.

## Step 12: column_head from one column and four beams, at any angle

The job: the only inputs are the column and four beams, in any order, and every cut comes from each beam's own axis. Before this the factory took its frame from `first` alone, and a rotated beam silently got a wrong joint.

What it now reads from the members: the head end of the column, each beam's near end and direction, its width, height and level from its actual solid, the main pair, each secondary's side. `first` to `fourth` are gone; the arguments are `a` to `d`.

What each cut follows: half planes in the beam's own direction, in a frame on the tongue it meets. Each secondary's tenon, tail and shoulder stay square to its own axis; the tail's tip is square to the beam and its root lies on the tongue's edge. So the four posts come out unequal, with nothing set per angle. A secondary's shoulder is the larger of the default and what clears the main beam.

Changed default: the shoulder is now a share of the column's radius, not of the beam's width. At the photographed size that is the same 80.

**Can the mains turn too? No.** Two tongues share one straight slot and bear on each other along the seam. If the mains are not in line, the tongues' sides are not parallel and no slot fits both. That is why the in-line limit is tight, 0.1 degrees. The secondary pair is what turns.

A decision the geometry could not make: at an exact cross both pairs are equally in line. My first tie-break took the other pair as mains, a valid joint, and my test noticed only because a tongue's volume was wrong. The rule now: the shorter pair, as in the photographed node, then the pair most along world x. A user wanting the other pair has no switch yet.

Defects found along the way, in this first attempt:

- The second secondary's column cuts were placed in the wrong frame. The new post check refused it.
- A secondary's slot stopped exactly where the main slot began, leaving a skin of column between them, near zero volume, so no volume check could see it. It joined posts together. The post count found it. The slots now overlap.
- A batch cut of overlapping prisms leaves posts touching at a sliver, with the same volume. Posts are counted from one prism at a time.

Stock: a beam too short gains its own section through a new `gains` field, hosted as an add feature by `add_interaction`, the way a butt adds stock; too long is cut back. `gains` is not written to the protobuf; the target keeps what it was given. A beam stopping outside the column's surface is refused, not extended.

## Step 13: measured, at this stable point

`joint_beams`, all on the Manifold mesh:

- Five sizes and turns: seated 0.000 mm3, head filled, lowering clear 0.000, first tongue matches the hand volume, the scarf held and only the column stopping the seam slide.
- All 24 orders of the same four beams agree within 0.000 mm3.
- Beams stopping 90 short of the axis, running 60 past it, or drawn the other way, and the column drawn head down: agree within 0.000 mm3.
- Sweep, both secondaries turned together, 1 degree steps: valid from 67 to 113 degrees, 47 angles, at 100 x 185 on a 200 column. At each: seated, filled, lowered, least blocked above 10 mm3.
- The limit is the post, not the dovetail. At 66 degrees a post is 480 mm2 against `min_post` 0.05 W2 = 500. At 90 degrees a post is 1912 mm2. **That limit is my choice, not a law.** A smaller `min_post` widens the range; how far was not measured.
- Each side on its own: 4 of 6 asymmetric pairs join. The other two refuse, naming the beam, at 60 degrees. A narrower secondary, 70 wide, joins.
- Refused, each naming the beam and reason: no pair in line, both secondaries on one side, a beam stopping short, tops not flush, a dovetail outside its tenon, a tilted beam, a beam missing the axis.
- `joint_library` 124 of 135, unchanged. `joint_pins` passes.

Not proved: blocked moves here are single beams moved 2 mm, along the main and each beam's own axes, not the group test the keyed box got. A column off vertical is not tested. Tilted beams are refused, as agreed. Beams must aim at the column's axis within 1 mm. All of it is on the mesh.

Looked at the rotated example in the viewer, plan and isometric, because the keyed box taught me not to trust green: tenons, tails and shoulders follow each beam, and the posts are unequal. It reads right.

The column's BRep is still the pieced one from step 8, so the rotated picture is INTERIM: it carries the same line and the same non-solid BRep, and must be taken again once `brep-boolean` lands. `round_cut_brep` should then be deleted.
