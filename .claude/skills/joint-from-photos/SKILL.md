---
name: joint-from-photos
description: Rebuilds a physical timber joint from photos as a real wood joint - tile in the library, applied to beams, example, picture, page, test - and proves the result against the photos. Use when the user gives photos of a joint or node and asks to reconstruct it, regenerate it, or make a joint tile from it. No scale, drawing or CAD file needed.
---

# Joint from photos

The photos are read by the model, which reads corners off grid lines by eye and types them in. Ordinary code (OpenCV `solvePnP`, NumPy) only solves cameras to check the result. No photogrammetry, structure from motion, depth, segmentation, edge detection or image-to-3D service is involved, and the model is never fitted to the images. The result is only trusted once numbers from the photos and from the built mesh agree.

Read `wood/CLAUDE.md` first. Delegate this to its own agent with the photos and the user's words, and no reading of the joint: that keeps the test honest.

## Log

Keep `wood/docs/plans/joint_from_photos_<n>_log.md`, written as you go. Each step: what you looked at, what you concluded, what you are unsure of, what was measured. A correction is a new entry; earlier entries are never edited. Name the photos P1, P2... at the top.

## Steps

1. **First reading.** Read every photo before anything else. Write down the members, what is removed from each, the fit and the assembly direction. Mark round parts and anything you cannot explain as unknown.
2. **Attack the reading with close-ups.** Crop the originals at 2x to 3x around every feature. Check grain and fibre direction, whether ring lines run unbroken across a surface, and whether an edge is a cut, a crease or a shadow (a shadow's edge stays straight over the grain). Ask whether the members could actually be seated as you read them. Expect this step to overturn part of step 1.
3. **Measure.** Read corners off gridded crops at original resolution. Recover the camera pose of a flat face from vanishing points, assuming a phone focal length, so squareness is measured and not assumed. Tabulate everything in units of one member's width, with an error estimate. Check a known relation as you go, such as two parallel cuts.
4. **Read the code and probe it.** Read the nearest existing tiles, `compute_joinery`, `from_contact` and the top_side detection. Then run the example once with an existing tile and print `joint_volumes`: the real unit box settles the axes, the code alone may not.
5. **Write the tile.** Follow the library: one concept in its own tile header, a named `JointPlate` factory with its own parameters, the example with one pair of members drawn apart, `pb_dump` to a scene of its own name when another job shares the tree. State every assumed dimension. There is no scale in a photo.
6. **Record the first attempt before fixing anything.** Build, run, shoot one isometric, and write down its state. Compiling proves nothing about the shape.
7. **Project the model into the photos.** Solve each photo's camera (PnP, focal length free) from model corners and read the residual at every corner, in mm. A large residual may be a mislabelled corner: check before changing the model. Solve a half-assembled member on its own: its pose against the seated model proves the assembly direction.
8. **Check the built mesh, not the intended sizes.** Dump the vertices and confirm every corner used in step 7 is a built vertex. Otherwise the table compares the photos with your intention.
9. **Think past the photographed angle.** Ask which constructions hold only near the measured values, then test at other angles and mirrored: seated overlap 0, free sliding out along the insertion direction, every other direction blocked, and one volume checked by hand. For a node of several members, test the assembly order.
10. **Compare and report.** Put viewer screenshots from the solved camera directions beside the photos, and give each sheet a plain verdict. List what still differs, what the photos do not show and what was inferred. Say honestly whether it worked the first time, naming the defects found and how.

## Traps seen so far

- A beam's `extension` lengthens it at both ends.
- `from_contact` silently swaps the design when a design severs a member: check that the tile you wrote is the one used.
- A tile that reads a member's lean must stay out of the geometry cache.
- `compute_joinery` cuts the male everywhere inside the female's box except its pockets, so the pockets decide the fork.
- Round parts in old timber may be repairs, not joinery: if saw cuts run through a circle, it was there before the joint.
- Phone photos converge harder than the viewer's camera: compare shapes, not outlines pixel for pixel.
- An assumed phone focal length can be well off (2890 px assumed, 4600 to 5000 px solved): treat step 3's angles as readings and let step 7 correct them.

## Pictures

Real viewer screenshots, cropped only, by `tools/screenshot_viewer.mjs` and `tools/crop_viewer_shot.py`. Round parts are exact BRep. When other work is running, use your own `SHOT_PORT`, your own targets, and take and release only your own build lock.
