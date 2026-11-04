# Beam Column Head {#elements_joint_beam_column_head}

[TOC]

Five members in one node: a round column keeps four posts around a slot, two main beams meet in it in a scarf of two tapered tongues, and two secondary beams end on those tongues in one dovetail each. It is one joint element with one interaction per target, not a two-member tile.

## Constructors

```cpp
static std::shared_ptr<Column> Column::round(const Line& axis, double radius, double chord_tolerance = 0.02, const std::string& name = "column");
static std::shared_ptr<JointBeam> JointBeam::column_head(const Column& column, const Beam& a, const Beam& b, const Beam& c, const Beam& d, const Beam* main_beam = nullptr, double tenon = 0.7, double shoulder = 0.8, double seam_slope = 0.1, double dovetail_offset = 0.145, double dovetail_root = 0.286, double dovetail_depth = 0.146, double dovetail_flare = 0.176, double min_wood = 0.08, double min_post = 0.05);
```

Give it one round column and four beams, in any order. It reads everything else from the members: which end of each beam is the near one, each beam's section and level, the main pair (the two beams in line) and the side each secondary beam is on. At a plain cross both pairs are in line: the shorter pair is taken as the main one, then the pair running most along world x, unless `main_beam` names a beam of the pair to use. Beam axes can simply end on the column's axis. A beam too long is cut back and one too short gains stock, as long as it reaches the column. It throws, naming the beam and the reason, when no valid joint exists.

Sizes are shares of a beam's own width, except the shoulder, which is a share of the column's radius. The beams' tops must be flush, and the slot is as deep as the beams are tall.

- `tenon`: the width of each beam inside the column.
- `shoulder`: how far each shoulder stands from the column's axis, also where the tongues end. A secondary beam's shoulder moves out as far as it must to clear the main beam.
- `seam_slope`: the scarf seam's run across per unit along.
- `dovetail_offset`, `dovetail_root`, `dovetail_depth`, `dovetail_flare`: the tail's place across its beam, its width at the root, its depth, and its widening per unit of depth.
- `min_wood`: the least wood left in a tongue behind a dovetail.
- `min_post`: the least area of a post, in square beam widths.

## Column Head
A node of a 200 round column and four 100 x 185 beams, rebuilt from photographs of a made node and drawn apart along the column, the way each beam comes out. Every wall runs along the column, so the four beams drop in from above in any order; the scarf resists pulling apart, and the column's walls stop it sliding along its seam.

![](elements/element_joint_beam_column_head.png)

The round column's BRep draws true arcs but is not a sewn solid: its faces are built from the closed mesh, because `BRep::boolean_difference` fails on a cylinder. Volumes and fit are measured on the mesh.

\include{lineno} elements/element_joint_beam_column_head.cpp

## Turned Secondary Beams
The same call with the secondary beams turned, one at `75°` to the main pair and the other at `80°` the other side. Each beam's tenon, dovetail and shoulder are computed from its own axis, so the four posts come out unequal.

The main pair must stay in line, because both tongues share one straight slot. The secondary beams may turn, each on its own. For 100 x 185 beams on a 200 column turned together the range is `67°` to `113°`, set by `min_post`: past that a post drops below 500 mm². Lowering `min_post` opens it only to `65°` and `115°`, the last angles with one solid post: further on a post splits, and by `47°` it is gone. The range is measured for these proportions only. Beams must be square to the column and aimed at its axis; a tilted beam is refused.

![](elements/element_joint_beam_column_head_rotated.png)

\include{lineno} elements/element_joint_beam_column_head_rotated.cpp
