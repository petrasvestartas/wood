# Beam Keyed Box {#elements_joint_beam_keyed_box}

[TOC]

Two plates held face to face by a box through a mortise in both, locked by two wedge keys: a third part that needs no glue, screws or access to the plates' edges. It is one joint element, with the box and the keys as its parts and one interaction per plate.

## Constructors

```cpp
static std::shared_ptr<JointBeam> JointBeam::keyed_box(const Plate& a, const Plate& b, const InteractionContactFace& contact, double box_width = 2.4, double box_depth = 1.6, double mortise = 0.6, double key_width = 1.5, double key_taper = 0.125, double key_proud = 0.5, double head = 1.2);
```

Sizes are shares of the first plate's thickness, so the joint follows the plates; the second plate may be thicker or thinner. The box stands at the centre of the contact. It returns null when the sizes leave nothing, the keys do not taper, or the box would leave the plates.

- `box_width`, `box_depth`: the box's section, and so the mortise in each plate. Width runs the way the keys go in.
- `mortise`: the thickness of the long mortise across the box, and of the keys.
- `key_width`: a key's width where it enters the box.
- `key_taper`: how much a key narrows per unit of depth. It must be above zero: this is what stops a key at home and pinches the plates.
- `key_proud`: how far a key stands out of the box at each end.
- `head`: the wood left beyond the mortise at each end of the box.

On standing plates the keys go in straight down; on plates lying flat, along the contact's longest edge.

## Assembly

The order is forced by the geometry:

1. The first key goes into the box, at the far end of the long mortise.
2. The box slides through both plates, plain end first, until that key stops on the second plate.
3. The first plate closes on the second, uncovering the mortise on its side.
4. The second key goes in. It can not go in earlier, because the first plate sits where it goes. The two wedges pinch the plates between them.

Once both keys are in, the only way anything moves is a key straight back out, wide end first.

## Keyed Box
A pair of 40 plates with a box and keys at the default sizes, drawn before its last step: the plates together on the box and the second key lifted out, since it only goes in once the plates are seated.

![](elements/element_joint_beam_keyed_box.png)

\include{lineno} elements/element_joint_beam_keyed_box.cpp
