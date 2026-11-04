# Hexagonal shell: finger size log

The user's words: "For this example make finger joints 2x narrower: update whole example. I think you take plate thickness as example but there should be a way to change dimensions of the joint to scale in the volume so that fingers would not be so big."

Written as the work went. Corrections are new entries.

## Step 1: what sets a finger's size today

The user's guess is right for the depth.

- Depth across the seam: the joint volume. `side_side_in_plane` offsets the seam by half the plate thickness each way, so the volume is one plate thickness wide (49.6 mm here) and the outlines fill it. Nothing else reaches that number.
- Count along the seam: `ceil(length / division_length)`, the first value of the joint's row in `joint_parameters`.
- `joint_scale` does nothing here. Only unit-scale designs read it.

The dataset, `data/inplane_hexshell.yml`:

- row 0 is `40, 0.5, 1`: division length 40, joint id 1, `side_side_inplane_dovetails`.
- `joint_volume_extension: -5, 0, -10`.

My first assumption was a row of 300 with the fingers design. Both were wrong; the file says 40 and id 1.

## Step 2: the plain way to size it fails on a fold

The obvious handle is a negative `joint_volume_extension` width. Measured on the seated shell, overlap summed over every pair of plates:

| Case | Depth mm | Divisions | Overlap mm3 |
|---|---|---|---|
| dataset, width -5, division 40 | 39.0 | 5.1 | 4,441 |
| width 0, division 40 | 48.9 | 5.1 | 0.000 |
| width -14.9, division 20 | same small depth | 9.6 | 16,624 |

I expected 0 on the dataset as it stands. It is not: the dataset's own -5 already overlaps. A shrunk volume shrinks in each plate's own tilted plane, so on a folded seam the two plates' cuts part. The shell is folded at every seam, so this is not a corner case.

`joint_scale[0]` was the other candidate. `hello.yml` sets it to 2.0 with in-plane joints and has a golden, so reusing it would move that golden.

## Step 3: the setting

`joint_depth_scale` in `Settings`, default 1, read from yaml, field 17 of `settings.proto`; 0 in an older file reads as 1.

`scale_joint_depth` scales the in-plane unit-box outlines in x before they are mapped onto the volumes. Both plates map the same scaled outlines through the same volume, so a wall of constant x stays common to both, and the depth changes without the plates parting.

Regenerating the protobuf needed protoc 36.2, which is not on this machine: taken from the official release into WSL.

`rust/src/proto/wood_proto.rs` was not regenerated. It was already another job's dirty file.

## Step 4: applied to the three examples, not to the dataset

`examples/2_contacts.cpp`, `3_features.cpp`, `4_traversal.cpp`, after `yaml_load`:

- `joint_parameters[0] = 20`: 2x narrower, twice as many.
- `joint_volume_extension = {0, 0, -10}`: width no longer shrunk.
- `joint_depth_scale = 0.4`: 0.4 of the full 48.9, which is half the dataset's 39.0.

`data/inplane_hexshell.yml` is unchanged, so `main_all_datasets` and the reference tests read the same file as before, with the same 4,441 mm3 overlap.

Measured, `tests/joint_elements.cpp`, `check_joint_depth_scale`:

| Case | Depth mm | Divisions | Overlap mm3 | Closed | Wood mm3 |
|---|---|---|---|---|---|
| new settings | 19.5 | 9.6 | 0.000 | 23 of 23 | 95,356,956 |
| full volume | 48.9 | 5.1 | 0.000 | | 95,356,956 |

The same wood as the full volume, because every finger removed from one plate is added to its neighbour.

Regressions after the setting: `joint_elements` passes, `reference_datasets` 48 of 49 (`hex_block_rossiniere`, the known miss; `hello` and `inplane_hexshell` match), `joint_library` 124 of 135 as before, `main_session_round_trip` passes.

## Step 5: pictures, first pass

Regenerated `ex_2`, `ex_3`, `ex_4`, `wood_architecture`, and the landing picture, for which no tool existed.

Two defects, both found by looking and neither by a test.

First, `ex_4_traversal.webp` still showed the old fingers. `4_traversal.exe` and `2_contacts.exe` were a day old: `run_example` runs what is built and does not build. The picture had changed by 16 bytes and git listed it as modified. Rebuilt and retaken.

Second, four seams on the far side of the ridge turned into nearly solid black patches on the landing picture and the sheet. The old picture has hatching at the same four places, so the isometric view looks almost along those seams, and twice as many fingers closed the hatching up. It is the camera, not the model.

## Step 6: the camera

The viewer's own orbit, `SHOT_ORBIT`, right-drag, nothing processed. Tried +60 and -60 pixels, 17 degrees each way, on the cut shell. Both resolved the patches. +60 keeps the shell facing the page most evenly, and every far seam reads as fingers.

`tools/render_architecture_pictures.py` now holds it as `SHELL_ORBIT`, for the three examples and the sheet, and has a `landing` step so the landing picture can be made again. `ex_2_contacts.webp` changed only because of this, to stay the same view as the others.

By eye at full size:

- `landing_architecture.webp`: every seam reads as fingers, no dark line, no patch.
- Sheet panel 4, cut plates: clean.
- `ex_3`, `ex_4`, sheet panel 3 at 0.4 opacity: the fingers read on every near seam. The far rim is still dense where the front and back seams stack through the transparent plates, most at the top centre. The rectangles stay separate at full size in `ex_3` and `ex_4`. On the sheet, which is scaled to half width, a few spots there are close to solid. This comes with drawing a through view of twice as many fingers; I did not thin the lines.

## Step 7: text

No page gave the old sizes. `docs/wood-cleanup-2026-09-17.md` quotes the width -5 seam lengths as a dated record and stays. One sentence added to `docs/examples.md` under Plate Joinery on the two settings.

While adding round-trip checks I found `check_joint_depth_scale();` sharing a line with the next statement in `tests/joint_elements.cpp`, left by my insert script. Fixed. The test now also checks the setting survives a file and that a file without it reads as 1.

## Step 8: correction, what "2x narrower" meant

I read "narrower" as the width along the seam and doubled the count. That was wrong. The user drew it in red: a tall square tooth becoming a low tooth of the same width. Narrower here means shallower: half the reach into the plate, the width along the edge and the count as they were.

Steps 4 to 6 record what I built on that wrong reading.

Changes:

- The three examples no longer set `joint_parameters[0]`. The dataset's 40 stays.
- `joint_volume_extension = {0, 0, -10}` and `joint_depth_scale = 0.4` stay.
- `tests/joint_elements.cpp` now requires the same divisions as the full volume, not nearly twice as many.

The setting itself did not change. The user's sentence about scaling the joint in the volume was the job all along.

Measured, `check_joint_depth_scale`:

| Case | Depth mm | Divisions | Overlap mm3 | Closed | Wood mm3 |
|---|---|---|---|---|---|
| dataset | 39.0 | 5.1 | 4,441 | | |
| full volume | 48.9 | 5.1 | 0.000 | | 95,356,956 |
| shallow fingers | 19.5 | 5.1 | 0.000 | 23 of 23 | 95,356,956 |
| same depth by a shrunk volume | | | 14,273 | | |

The last row replaces the 16,624 of step 2, which was at division 20.

## Step 9: the camera again

With the original count, I shot the cut shell from the original view and from +60. In the original view the four far seams are small hatched patches, as in the picture before this job, not the solid black of step 5. Those patches came from doubling the count, not from the depth.

So the reason for `SHELL_ORBIT` is gone and it is removed from the tool, with the `orbit` argument. The pictures keep their old angle. `ex_2_contacts.webp` is back to byte-identical with git. The `landing` step stays, since the landing picture had no recipe.

Retaken and read at full size: `ex_3_features`, `ex_4_traversal`, `wood_architecture`, `landing_architecture`. Low square teeth of the old width on every near seam, no dark line. The far rim in the transparent shots is less dense than in the first version of this job: the rectangles stay separate, and no spot on the sheet goes solid.
