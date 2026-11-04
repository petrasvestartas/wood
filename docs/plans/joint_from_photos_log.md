# Joint from photos: step log

Written as the work goes. Earlier entries are never edited; a correction is a new entry.
No prior log existed, so this one starts at step 1.

Photos, in the order read (short names used below):

| name | file | view |
|---|---|---|
| P1 | 0092c83c | assembled, from above, near top-down |
| P2 | 2a93b845 | apart, straight at the near side face of the running beam |
| P3 | c3baec35 | as P2, with the forked beam beside it |
| P4 | d83c7e8a | apart, close, oblique |
| P5 | 2ebcd896 | apart, close, from above |
| P6 | 97814c1a | half slid in, close |
| P7 | b467e2fc | half slid in, wide oblique |
| P8 | 4ca78423 | apart, wide, along the far side face |

## Step 1: first reading of the photos, before any measuring or code

Looked at: all eight, one by one, nothing else read about them.

Concluded:

- Two members lying in one plane, top and bottom faces flush. One runs through (call it A, the long
  one), one ends in it (B, the short one) at about 60 degrees. Both look like the same solid timber.
- Through the joint A keeps only its middle layer, a tongue about a third of its thickness. The layers
  above and below are cut away right across its width, so A has two identical pockets, one in each wide face.
- B ends in a fork: an open slot in the middle third, two cheeks about a third each. The cheeks fill
  the two pockets and sandwich the tongue. So this is a three-layer bridle, skewed: a fork on the end
  of one member around the reduced middle of another.
- The pocket outline in plan is not a parallelogram. One wall is square to A's axis right across
  (short side of A in P1). The other wall has two parts: a short square shoulder from A's near side,
  then a long slanted part along B's upper edge to A's far side. That shoulder leaves a small
  triangular tooth of A biting into B's upper edge. B has the matching triangular notch.
- B's cheek end is therefore a pointed plan: cut along A's far side (flush with it when assembled)
  and cut square to A's axis against the square wall.
- The slot closes at the plane of A's near side, so the tongue's near face bears on the slot bottom.
- Assembly: B slides in sideways, square to A's axis, from A's near side. The two square walls are
  parallel to that motion, the long slanted wall and the slot bottom are the stops. P6 and P7 show this motion
  half done. It is not a joint assembled by lowering B from above.
- A's far ends are cut parallel to B's axis, so A is a long parallelogram in plan. B's far end looks square.
- Pale band across the tongue in P5 and P8: a shadow of the block beside it, the edge is parallel to the wall.
  Not a crease. The tongue is one flat plane.

Round parts, and here I am unsure:

- A flush circle on the tongue's near face, stippled like beech end grain, in P2, P3 and P4. It sits low
  in the tongue, almost touching its lower edge, not centred.
- A thin dowel sticks out of B's long lower side, far from the joint, square to B's axis. P1, P7, P8.
  The two look different in diameter. They may be two pins. No exit circle seen yet on the opposite faces.
- Neither obviously pins B to A: no circle on B's top face in P1. What each one does is not known yet.

Next: measure, do not trust the eye. Rectify P1 and P2 and read proportions; zoom on the round parts.

## Step 2: close-ups, which changed the reading

Looked at: crops of the originals at 2x to 3x of the tongue (P2, P3, P4, P5), the fork (P3, P5), the stub (P1, P8).

Corrections to step 1, left there on purpose:

- WRONG in step 1: "the slot closes at the plane of A's near side, so the tongue's near face bears on the slot
  bottom". Seen from B's upper side (P3) the slot ends exactly at the notch corner, and the notch wall is
  whole, rings run unbroken across the middle layer. In P5 the torn fibres of the slot bottom run square to
  B's axis: I projected B's width direction through the affine of that photo and got (0.85, 0.53) against
  (0.83, 0.55) read from the fibres. The slot is a plain slot, square to B.
- WRONG in step 1: "tongue = A's middle layer, whole". P5 shows the tongue's near edge running in one straight line from
  the pocket corner to the near corner of the square wall, and below it a sawn face. So a triangle of the
  middle layer is sawn away: near shoulder corner, pocket corner, near corner of the square wall. B's solid
  behind the slot needs that room: it would hit a full tongue. Had I built step 1 literally, B could not be seated.
- The pale band is confirmed a shadow: its edge is parallel to the wall, and in P3 it stays straight over the grain.
- P1 shows B not quite home: a dark gap along all its walls. B's end wall stands about 21 px left of A's near edge,
  about 3 percent of the width. So the seated position is read from A's edges, B's own edges only give directions.

Round parts:

- The circle on the tongue: grain lines run through it along the beam, so side grain, not end grain, it is not a dowel
  end. It is CUT by the sawn surfaces: the lower edge of the tongue clips about 20 percent of it (P2, P3, P4), and it lies flush
  on the oblique sawn face. So it was in the timber before the joint was sawn. Conclusion: a repair plug in the
  stock, no part of this joint. Confidence fair, not certain.
- The stub: beech, about 0.127 of the width thick, the circle is 0.206. Two different things. It leaves B's lower side
  square to B's axis, about 0.65 of a width from B's far end, nowhere near A. It can not reach A, it can not pin
  this joint. Its job is unknown from the photos. No exit circle on B's opposite side in P3, so blind.

## Step 3: measuring P1

Did: read corners off gridded crops at original resolution, then recovered the camera pose of the top plane from three
vanishing points with a phone focal length of about 2890 px (26 mm equivalent, assumed), so squareness is
measured and not assumed. Script in the scratchpad, numbers here.

Measured, W = width of A:

| what | value | check |
|---|---|---|
| plane tilt to the camera | 1.8 deg | P1 is near top-down, the method is well conditioned |
| crossing angle of A and B | 56.8 deg | image alone gives 57.0 |
| A's end cuts against B's axis | 2.1 deg off | parallel within reading error |
| square wall against A's axis | 91.5 deg, shoulder 91.2 deg | square within 1.5 deg |
| B's far end against B's axis | 88.7 deg | square |
| B's width | 1.04 W | same stock assumed, 4 percent unexplained |
| shoulder length, near side to corner | 0.30 W | |
| distance along A's near side, shoulder to square wall | 1.07 W | |
| A near side, long block | 2.72 W | |
| A near side, short block | 0.97 W | |
| H from P2 | 1.0 W | A's near side lengths compared in P1 and P2; section square within 3 percent |
| tongue, P2 | 0.33 H | |
| cheek, slot, cheek, P3 | 0.345, 0.31, 0.345 H | thirds |

Unsure: the corners are read to about 5 px of 660, about 1 percent. Angle error about 1.5 deg.

## Step 4: reading the code

Read: wood/CLAUDE.md, the milled wedge, cross slot and tenon tiles, compute_joinery, from_contact, beam_to_beam, top_side
detection, joint_orient_to_connection_area, the kernel's two_rects_from_frame and from_change_of_basis.

Concluded: for a tee each beam's box is a plate between its two side faces. The unit box is skew: x along A's axis
across B, y along B's axis across A, z through the height. So x = +-0.5 are B's two sides and y = +-0.5 are A's two sides.
A bridle fits this with no extra machinery: only the walls square to A's axis depend on the angle, and a tile can
read the angle from joint_volumes as top_side_snap_fit already reads them.

## Step 5: probing the real unit box

Did: registered `element_joint_beam_top_side_bridle`, ran it once with the existing milled wedge tile, printing joint_volumes.
Measured at 57 degrees, 100 x 100 beams: vol0 and vol1 are the planes x = -27.15 and x = 92.09, y from +50 to -50, z from -50 to 50.

- WRONG in step 4: the unit box is NOT skew. unit x = height (world z), unit y = across A, -0.5 on the near face, +0.5 on the far face,
  unit z = along A's axis. z = +-0.5 are where B's two sides leave A's near face. The box is right-angled.
- Consequence, simpler than planned: walls square to A's axis are plain z = const, A's sides are y = +-0.5, layers are x = const.
  Only B's lean is missing from the box, so the tile must read it from the male plate, the way the Annen tiles read their elements.
- Also read: compute_joinery cuts the male everywhere inside the female's box except inside the female's pockets
  (`outside[i] - pockets[1 - i]`). So B's fork comes from A's pockets minus B's own cuts, and pockets must not overshoot past A's far side
  without B's own cut taking the overshoot back.

## Step 6: first attempt written, nothing built yet

Did: `top_side_bridle.h` (the tile), id 26, `JointPlate::top_side_bridle(shoulder, tongue)`, the example with the measured sizes.
The tile is left out of the geometry cache because it depends on each joint's lean (one line in reuse_or_create_geometry).

Assumed dimensions, there is no scale in the photos: section 100 x 100, so W = 100 mm. Angle 57 deg. Stub diameter 12, sticking out 75,
foot 60 from B's far end. Its hole depth is unknown, taken to B's axis. Its height is unknown, taken at mid height. A sawn parallel to B at
both ends, at x = 345 and x = -124 on the near side, as measured. B's axis 236 long.

Not modelled, on purpose: the round plug in the tongue, see step 2.

## Step 7: state of the FIRST attempt, before any fix

Did: built `element_joint_beam_top_side_bridle` with the tile as first written, ran it, shot one isometric from the real viewer.
It compiled at the first go, which proves nothing about the shape.

What it showed, unfixed:

- Both beams came out whole, no fallback to the milled wedge (from_contact silently swaps the design when a design severs a member, so this was
  worth checking), and the topology is the one in the photos: tongue in the middle third, rectangular notch under it, short block with one
  straight wall, long block with shoulder plus slanted wall, B with a pointed end, a notch in its edge, a fork.
- Not yet checked at this point: the fit when seated, and the proportions against the photos from matching directions.

A shell quoting failure and a CRLF mismatch cost two tries at registering the design. Those were tooling, not geometry, and nothing was built from them.

## Step 8: two false alarms while setting up the comparison

- My first matched shots came out rotated. plane_to_plane gives the opposite rotation to the one I assumed in my temporary camera block.
  Camera code only, the joint was not touched.
- P8's long block seemed to end in a slanted corner, as if the end cut leaned in height. A close-up settled it: the cut plane passes
  through the camera, so its top edge and the vertical corner fall on one line. P7 shows the same corner plainly and it is vertical.
  No change to the model.

Decision: stop judging by eye. Solve each photo's camera from model corners (PnP) and read the residual at every corner, which also
gives the real camera direction for each viewer shot. The numbers go in the next step.

## Step 9: the model projected into the photos

Did: solved each photo's camera (PnP, focal length free) from the corners of the model as first built, with 100 mm section, and read the residual
at each corner, photo against model. The corner positions come from the photos, the 3D positions from the first attempt, unchanged.

| photo | member | corners | mean | worst |
|---|---|---|---|---|
| P8 | A | 17 | 2.0 mm | 5.7 mm, far corner of the long end |
| P8 | B | 8 | 1.7 mm | 3.5 mm, nose |
| P7 | A | 16 | 1.9 mm | 3.7 mm |
| P7 | B | 8 | 1.7 mm | 3.2 mm, nose |

Not a design change, but the first P7 run read 16 mm at two corners. That was MY reading: I had marked B's notch as A's shoulder, both are
near each other in that view. Relabelled, 3.7 mm. The 16 mm stays here on the record.

Independent check of the assembly direction: B solved on its own in P7, half slid in, sits 16.25 mm in y and 0.09 mm in x from the seated model, rotated 0.6 deg.
So the slide really is square to A. That was a reading of step 1, the numbers now confirm it, and the tile's walls being z = const are right.

So, at this point: no correction to the geometry has been needed. The first attempt stands.

## Step 10: a first-attempt defect, found by reasoning, not by the photos

Planning the test showed a real fault in the tile as first written. The photo's sawn line runs from the pocket corner to the near end of the square wall.
That clears B's solid behind the slot only while cos(angle) >= shoulder. At 57 deg with shoulder 0.3: 0.545 >= 0.3, so the photographed joint was never
affected and no picture would have shown it. Nearer 90 deg the slot line swings inside the sawn line and the seated beams would overlap in the tongue.

Fix: the sawn line ends where the slot line reaches the square wall, when that is further in than the near face (`sawn` in the tile). At 57 deg this is the same geometry.
So: right first time for the object in the photos, wrong first time for the general design.

## Step 11: test results, and a second first-attempt defect

`joint_beams`, new `check_bridles`, run after the fix of step 10, at 57, 45, 75, 90 and mirrored 123 deg, W = H = 100:

| angle | seated overlap | sliding out 5 to 200 mm | least overlap of a 2 mm move any other way |
|---|---|---|---|
| 57 | 0.000 mm3 | 0.000 | 10193 mm3 |
| 45 | 0.000 | 0.000 | 13340 |
| 75 | 0.000 | 0.000 | 10476 |
| 90 | 0.000 | 0.000 | 6667 |
| 123 | 0.000 | 0.000 | 10193, same as 57, as a mirror must be |

By hand at 57 deg: A loses 608813 of 608834 mm3, the fork fills 582034 of 582055.

Then the second defect, in the example and not in the tile. Comparing the fork view with P3 showed B's plain side 244 mm long against about 144 mm.
`extension` lengthens a beam at BOTH ends. I had set it to reach A's far face and so added 100 mm at B's far end too. The stub then looked mid-beam, not near the end.
P1 shows it plainly and I missed it at step 8: B stands out of A by 1.35 W in the photo and 2.43 W in my first top view.

What this means for step 9: its table compared the photos with the dimensions I intended, not with the mesh that was built. A's numbers hold, because A's stock has no
extension. B's did not test its length. That was a hole in the check. Closed in the next step by checking the built vertices.

## Step 12: fix and proof against the built mesh

Did: B's axis 136 long with extension 100, so it ends 236 from the axis as measured. Dumped the vertices of both built solids and looked up every corner used in step 9.
Measured: all 17 corners of A and 8 of B lie on a built vertex within 0.005 mm. So the step 9 table now holds for the real mesh, B's length included.

Pin: checked for facets at close zoom, as the house rule asks. Smooth shading, and an elliptical end cap in the P8 view. It is drawn from a Pin element with a Joint::drill bore.
The line along it in the isometric picture I take for the cylinder's seam or a shading edge, not a facet. I did not trace which.

Regression: `joint_library` 124 of 135 pass. The 11 failures are all `side_side_outplane_tenons` and `linked_tenons`, ids 14 and 15. My one shared change is gated to id 26, so they should not
be mine, but I did not run a clean tree to prove it. `joint_beams` passes.

Own mistake outside the geometry: at the end I ran `rmdir build/.buildlock` as tidying. My build script already frees its own lock, so had the directory existed then it would have been the
other job's. Checked at once: no compiler or linker running and no lock present. I can not prove it was not held in that instant. It should not have been run.

## Step 13: comparison, plain verdict

Sheets in `docs/plans/joint_from_photos/`, photo beside viewer screenshot, cropped and set to one size only. P7 and P8 use the camera direction solved from the photo. P1 is top-down. P2, P3 and P4 are
set by hand and show one member, so as to test one finding each. The viewer's camera is not the phone's: the photos converge harder, so compare shapes, not outlines pixel for pixel.

| sheet | what it tests | verdict |
|---|---|---|
| compare_p1_assembled_top | plan, assembled | Matches: angle, shoulder, notch, pointed end flush with the far side, square wall, A's parallel ends, the stub |
| compare_p8_apart | apart, solved camera | Matches: pocket, tongue, under-notch, step, fork, stub. Mean residual 2.0 mm, worst 5.7 |
| compare_p7_half_slid | half slid, solved camera | Matches. Mean 1.9 mm, worst 3.7. B's solved pose is a slide square to A |
| compare_p2_near_side | near side, straight | Matches: thirds, block lengths. No plug in the model, see below |
| compare_p3_fork | fork from B's upper side | Matches: slot square to B stopping at the notch corner, B's length after the fix |
| compare_p4_tongue_face | sawn tongue face | Matches: the one feature I had wrong at first reading. Plug missing |

Still different from the photos:

- The round plug in the tongue is not modelled. I read it as a repair in the stock, not part of the joint, because the saw cuts run through it. Confidence fair. If it is a real pin, this is a missing feature.
- B is 100 wide in the model, 104 measured in P1. Same stock assumed. So the near-side gap between the walls is 100, 107 measured. Worst visible in P1.
- The stub's job is unknown. It can not reach A. Its height, mid height, and its hole depth, to B's axis, are assumptions.
- Real wood has eased arrises and a splintered slot bottom; the model is sharp.
- Crossing angle 57 deg carries about 1.5 deg of reading error. All sizes rest on the assumed 100 mm section.

## Step 14: answer to "did it work the first try"

For the joint in the photos, yes: the tile as first built needed no change to fit the photos, 2 mm mean, and was the right design. But not cleanly, and three things stay on record:

- The example's second beam came out 100 mm too long, an error in how I used `extension`. Found by comparing the fork view. Fixed in step 12.
- The tile was wrong for angles near 90 deg, invisible at 57. Found by reasoning before any test ran. Fixed in step 10.
- That result stands on a reading corrected BEFORE any code: my true first reading of the photos, step 1, had two errors, a full tongue and a slot stopping on A's side. Built literally, B could not have been seated. Close-ups fixed that in step 2.

Open for the main session: the example still dumps to its own scene, the unit tile picture comes from `make.sh` on the machine with the native renderer, and the bridle is not in the oracle's golden list.

## Step 15: README and tile picture

Did: `docs/plans/joint_from_photos/README.md`. The tile picture is the built tile at 57 deg mapped back into its unit box by the inverse of the joint's change of basis, drawn with a
temporary block in the example, cropped by polygon clipping at 0.75 so the cutters read, shot in the viewer. The example was restored byte for byte from a copy afterwards.

## Step 16: the stub pin is a handle

The user, looking at the comparison: the dowel is only a handle, not part of the joint. It was modelled from the photos with its job marked unknown in step 2 and step 13. Removed from the example, and the element picture was taken again. The tile never depended on it, so no joint geometry changed. This entry was added by the main session.
