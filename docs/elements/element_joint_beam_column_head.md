# Beam Column Head {#elements_joint_beam_column_head}

[TOC]

Five members in one node: a round column keeps four posts around a cross slot, two main beams meet in it in a scarf of two tapered tongues, and two secondary beams end on those tongues in one dovetail each. It is one joint element with one interaction per target, not a two-member tile.

## Constructors

```cpp
static std::shared_ptr<Column> Column::round(const Line& axis, double radius, double chord_tolerance = 0.02, const std::string& name = "column");
static std::shared_ptr<JointBeam> JointBeam::column_head(const Column& column, const Beam& first, const Beam& second, const Beam& third, const Beam& fourth, double tenon = 0.7, double shoulder = 0.8, double seam_slope = 0.1, double dovetail_offset = 0.145, double dovetail_root = 0.286, double dovetail_depth = 0.146, double dovetail_flare = 0.176);
```

Sizes are shares of the beam width. The beams must be built from a profile, which gives their width and height; without one the factory returns null. Tops are flush with the column's head, and the slot is as deep as the beams are tall.

- `tenon`: the width of each beam inside the column.
- `shoulder`: how far each shoulder stands from the column's axis, also where the tongues end.
- `seam_slope`: the scarf seam's run across per unit along.
- `dovetail_offset`, `dovetail_root`, `dovetail_depth`, `dovetail_flare`: the tail's place along the tongue, its width at the root, its depth, and its widening per unit of depth.

## Column Head
A node of a 200 round column and four 100 x 185 beams, rebuilt from photographs of a made node and drawn apart along the column, the way each beam comes out. Every wall runs along the column, so the four beams drop in from above in any order; the scarf resists pulling apart, and the column's walls stop it sliding along its seam.

![](elements/element_joint_beam_column_head.png)

\include{lineno} elements/element_joint_beam_column_head.cpp
