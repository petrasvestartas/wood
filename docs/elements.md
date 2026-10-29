# Elements {#elements}

[TOC]

Timber members, auxiliary cutters, and parametric joints under `src/joinery_solver/wood_elements/`. Every element stores its uncut stock (`element_geometry_*`) and applies incoming `WoodSession::add_interaction(source, target, interaction)` edges to compute its cut solid (`model_geometry_*`).

## Shape Elements

Click any shape element card below to jump directly to its constructor reference, parameters, and C++ example code:

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_plate.html">
    <img src="element_plate.png" alt="Plate">
    <span class="wood-card-title">Plate</span>
    <span class="wood-card-sub">Planar timber panel (`wood_element_plate.h`)</span>
  </a>
  <a class="wood-card" href="elements_plate_curved.html">
    <img src="element_plate_curved.png" alt="PlateCurved">
    <span class="wood-card-title">PlateCurved</span>
    <span class="wood-card-sub">6-patch NURBS curved timber panel (`wood_element_plate_curved.h`)</span>
  </a>
  <a class="wood-card" href="elements_beam.html">
    <img src="element_beam.png" alt="Beam">
    <span class="wood-card-title">Beam</span>
    <span class="wood-card-sub">Prismatic timber beam member (`wood_element_beam.h`)</span>
  </a>
  <a class="wood-card" href="elements_beam_curved.html">
    <img src="element_beam_curved.png" alt="BeamCurved">
    <span class="wood-card-title">BeamCurved</span>
    <span class="wood-card-sub">Curved glulam NURBS BRep beam with lamellae (`wood_element_beam_curved.h`)</span>
  </a>
  <a class="wood-card" href="elements_beam_variable.html">
    <img src="element_beam_variable.png" alt="BeamVariable">
    <span class="wood-card-title">BeamVariable</span>
    <span class="wood-card-sub">Tapered / variable cross-section beam (`wood_element_beam_variable.h`)</span>
  </a>
  <a class="wood-card" href="elements_column.html">
    <img src="element_column.png" alt="Column">
    <span class="wood-card-title">Column</span>
    <span class="wood-card-sub">Vertical post member with head &amp; base cuts (`wood_element_column.h`)</span>
  </a>
  <a class="wood-card" href="elements_block.html">
    <img src="element_block.png" alt="Block">
    <span class="wood-card-title">Block</span>
    <span class="wood-card-sub">Polyhedral timber block, head, or voussoir (`wood_element_block.h`)</span>
  </a>
  <a class="wood-card" href="elements_support.html">
    <img src="element_support.png" alt="Support">
    <span class="wood-card-title">Support</span>
    <span class="wood-card-sub">Boundary support plinth &amp; abutment solid (`wood_element_support.h`)</span>
  </a>
  <a class="wood-card" href="elements_cut_plane.html">
    <img src="element_cut_plane.png" alt="CutPlane">
    <span class="wood-card-title">CutPlane</span>
    <span class="wood-card-sub">Planar half-space trim cutter (`wood_element_cut_plane.h`)</span>
  </a>
  <a class="wood-card" href="elements_profile.html">
    <img src="element_profile.webp" alt="Profiles">
    <span class="wood-card-title">Profiles</span>
    <span class="wood-card-sub">Parametric 2D cross-section polylines (`wood_profile.h`)</span>
  </a>
  <a class="wood-card" href="elements_pin.html">
    <img src="element_pin.png" alt="Pin">
    <span class="wood-card-title">Pin</span>
    <span class="wood-card-sub">Cylindrical dowel &amp; headed fastener pin (`wood_element_pin.h`)</span>
  </a>
</div>

| Element | Header | Examples |
| --- | --- | --- |
| [Plate](@ref elements_plate) | `wood_element_plate.h` | `element_plate`, `element_plate_holes`, `element_plate_session`, `element_plate_assign_joints` |
| [PlateCurved](@ref elements_plate_curved) | `wood_element_plate_curved.h` | `element_plate_curved` |
| [Beam](@ref elements_beam) | `wood_element_beam.h` | `element_beam` |
| [BeamCurved](@ref elements_beam_curved) | `wood_element_beam_curved.h` | `element_beam_curved` |
| [BeamVariable](@ref elements_beam_variable) | `wood_element_beam_variable.h` | `element_beam_variable`, `element_beam_variable_cut` |
| [Column](@ref elements_column) | `wood_element_column.h` | `element_column`, `element_column_session` |
| [Block](@ref elements_block) | `wood_element_block.h` | `element_block` |
| [Support](@ref elements_support) | `wood_element_support.h` | `element_support` |
| [CutPlane](@ref elements_cut_plane) | `wood_element_cut_plane.h` | `element_beam_variable_cut` |
| [Profiles](@ref elements_profile) | `wood_profile.h` | `element_profile` |
| [Pin](@ref elements_pin) | `wood_element_pin.h` | `element_pin` |

## Joint Elements

Joint elements cut and connect pairs of member elements (`Plate`, `Beam`, `BeamVariable`, `Column`, `Block`) and optionally own connector keys, wedges, side plates, or [`Pin`](@ref elements_pin) dowels. Click any joint element family card below to jump directly to its presets and C++ examples:

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="elements_joint.html">
    <img src="element_joint.png" alt="Joint">
    <span class="wood-card-title">Joint</span>
    <span class="wood-card-sub">Analytical drill bores &amp; boolean solid cutters (`element_joint_drill`, `_cutter`)</span>
  </a>
  <a class="wood-card" href="elements_joint_plate.html">
    <img src="element_joint_plate.png" alt="JointPlate">
    <span class="wood-card-title">JointPlate</span>
    <span class="wood-card-sub">Unit-box `[-0.5, 0.5]^3` plate joinery &amp; parameters (`element_joint_plate_parameters`)</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_side.html">
    <img src="element_joint_plate_top_side.png" alt="JointPlate top_side">
    <span class="wood-card-title">JointPlate top_side</span>
    <span class="wood-card-sub">T-joint edge-to-face tenons, Annen tenons, snap-fit &amp; milled wedge</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_inplane.html">
    <img src="element_joint_plate_side_side_inplane.png" alt="JointPlate side_side_inplane">
    <span class="wood-card-title">JointPlate side_side_inplane</span>
    <span class="wood-card-sub">Coplanar plate seams: fingers, dovetails, butterfly/tooth keys &amp; Hilti</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_outplane.html">
    <img src="element_joint_plate_side_side_outplane.png" alt="JointPlate side_side_outplane">
    <span class="wood-card-title">JointPlate side_side_outplane</span>
    <span class="wood-card-sub">Folded corner miters: fingers, tenons &amp; Vidy linked tenons</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_side_side_rotated.html">
    <img src="element_joint_plate_side_side_rotated.png" alt="JointPlate side_side_rotated">
    <span class="wood-card-title">JointPlate side_side_rotated</span>
    <span class="wood-card-sub">Twisted seams: miter slices, hooks, diamond keys &amp; side removal</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_cross.html">
    <img src="element_joint_plate_cross.png" alt="JointPlate cross">
    <span class="wood-card-title">JointPlate cross</span>
    <span class="wood-card-sub">Intersecting plate cross slots &amp; half-laps (`InteractionContactCross`)</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_top_top.html">
    <img src="element_joint_plate_top_top.png" alt="JointPlate top_top">
    <span class="wood-card-title">JointPlate top_top</span>
    <span class="wood-card-sub">Stacked face-to-face drilled pin arrays (`centre_pin` .. `rectangle_pins`)</span>
  </a>
  <a class="wood-card" href="elements_joint_plate_border.html">
    <img src="element_joint_plate_border.png" alt="JointPlate border">
    <span class="wood-card-title">JointPlate border</span>
    <span class="wood-card-sub">Single-plate boundary edge profile (`compute_border_contact`)</span>
  </a>
  <a class="wood-card" href="elements_joint_beam.html">
    <img src="element_joint_beam.png" alt="JointBeam">
    <span class="wood-card-title">JointBeam</span>
    <span class="wood-card-sub">Axis-contact feature volumes, plane butts &amp; face-contact connectors</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_cross.html">
    <img src="element_joint_beam_cross.png" alt="JointBeam cross">
    <span class="wood-card-title">JointBeam cross</span>
    <span class="wood-card-sub">Crossing beam half-laps, sliced half-laps &amp; Brussels half-laps</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_top_side.html">
    <img src="element_joint_beam_top_side.png" alt="JointBeam top_side">
    <span class="wood-card-title">JointBeam top_side</span>
    <span class="wood-card-sub">Beam ending on another member (`top_side_milled_wedge`)</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_end_to_end.html">
    <img src="element_joint_beam_end_to_end.png" alt="JointBeam end to end">
    <span class="wood-card-title">JointBeam end to end</span>
    <span class="wood-card-sub">Collinear &amp; angled beam scarfs (`splayed_scarf`, `rotated_miter`, `jupiter_scarf`)</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_butt.html">
    <img src="element_joint_beam_butt.png" alt="JointBeam butt">
    <span class="wood-card-title">JointBeam butt</span>
    <span class="wood-card-sub">Automatic or forced plane butts (`butt`, `butt_side_side`, `butt_top_side`)</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_connectors.html">
    <img src="element_joint_beam_connectors.png" alt="JointBeam connectors">
    <span class="wood-card-title">JointBeam connectors</span>
    <span class="wood-card-sub">Wedges, let-in plates, rectangle plates &amp; butterfly keys</span>
  </a>
  <a class="wood-card" href="elements_joint_beam_pins.html">
    <img src="element_joint_beam_pins.png" alt="JointBeam pins">
    <span class="wood-card-title">JointBeam pins</span>
    <span class="wood-card-sub">Centred pins &amp; headed pins across `InteractionContactFace`</span>
  </a>
</div>

| Joint Element | Header | Examples |
| --- | --- | --- |
| [Joint](@ref elements_joint) | `wood_element_joint.h` | `element_joint_drill`, `element_joint_cutter` |
| [JointPlate](@ref elements_joint_plate) | `wood_element_joint_plate.h` | `element_joint_plate_parameters` |
| [JointPlate top_side](@ref elements_joint_plate_top_side) | `wood_element_joint_plate.h` | `element_joint_plate_top_side_three_tenons` .. `_snap_fit`, `_custom` |
| [JointPlate side_side_inplane](@ref elements_joint_plate_side_side_inplane) | `wood_element_joint_plate.h` | `element_joint_plate_side_side_inplane_three_fingers` .. `_tooth_keys`, `_custom`, `_hilti`, `_hilti_angles` |
| [JointPlate side_side_outplane](@ref elements_joint_plate_side_side_outplane) | `wood_element_joint_plate.h` | `element_joint_plate_side_side_outplane_three_fingers` .. `_linked_tenons_wall`, `_capped_fingers`, `_notch`, `_custom` |
| [JointPlate side_side_rotated](@ref elements_joint_plate_side_side_rotated) | `wood_element_joint_plate.h` | `element_joint_plate_side_side_rotated_miter`, `_hooks`, `_keys`, `_custom`, `_side_removal` |
| [JointPlate cross](@ref elements_joint_plate_cross) | `wood_element_joint_plate.h` | `element_joint_plate_cross_slot`, `_custom` |
| [JointPlate top_top](@ref elements_joint_plate_top_top) | `wood_element_joint_plate.h` | `element_joint_plate_top_top_centre_pin` .. `_rectangle_pins`, `_custom` |
| [JointPlate border](@ref elements_joint_plate_border) | `wood_element_joint_plate.h` | `element_joint_plate_border` |
| [JointBeam](@ref elements_joint_beam) | `wood_element_joint_beam.h` | `element_joint_beam_wedge` |
| [JointBeam cross](@ref elements_joint_beam_cross) | `wood_element_joint_beam.h` | `element_joint_beam_cross_slot` .. `_brussels_half_lap` |
| [JointBeam top_side](@ref elements_joint_beam_top_side) | `wood_element_joint_beam.h` | `element_joint_beam_top_side_milled_wedge` |
| [JointBeam end to end](@ref elements_joint_beam_end_to_end) | `wood_element_joint_beam.h` | `element_joint_beam_side_side_inplane_splayed_scarf`, `_cross_splayed_scarf`, `_side_side_rotated_miter`, `_jupiter_scarf` |
| [JointBeam butt](@ref elements_joint_beam_butt) | `wood_element_joint_beam.h` | `element_joint_beam_butt`, `_butt_plates` |
| [JointBeam connectors](@ref elements_joint_beam_connectors) | `wood_element_joint_beam.h` | `element_joint_beam_wedge`, `_rectangle_plate`, `_butterfly_key` |
| [JointBeam pins](@ref elements_joint_beam_pins) | `wood_element_joint_beam.h` | `element_joint_beam_centred_pins`, `_headed_pins` |

<div style="display:none">
![](elements/element_plate.png)
![](elements/element_plate_curved.png)
![](elements/element_beam.png)
![](elements/element_beam_curved.png)
![](elements/element_beam_variable.png)
![](elements/element_column.png)
![](elements/element_block.png)
![](elements/element_support.png)
![](elements/element_cut_plane.png)
![](elements/element_profile.webp)
![](elements/element_pin.png)
![](elements/element_joint.png)
![](elements/element_joint_plate.png)
![](elements/element_joint_plate_top_side.png)
![](elements/element_joint_plate_side_side_inplane.png)
![](elements/element_joint_plate_side_side_outplane.png)
![](elements/element_joint_plate_side_side_rotated.png)
![](elements/element_joint_plate_cross.png)
![](elements/element_joint_plate_top_top.png)
![](elements/element_joint_plate_border.png)
![](elements/element_joint_beam.png)
![](elements/element_joint_beam_cross.png)
![](elements/element_joint_beam_top_side.png)
![](elements/element_joint_beam_end_to_end.png)
![](elements/element_joint_beam_butt.png)
![](elements/element_joint_beam_connectors.png)
![](elements/element_joint_beam_pins.png)
</div>

- @subpage elements_plate
- @subpage elements_plate_curved
- @subpage elements_beam
- @subpage elements_beam_curved
- @subpage elements_beam_variable
- @subpage elements_column
- @subpage elements_block
- @subpage elements_support
- @subpage elements_cut_plane
- @subpage elements_profile
- @subpage elements_joint
- @subpage elements_joint_plate
- @subpage elements_joint_plate_top_side
- @subpage elements_joint_plate_side_side_inplane
- @subpage elements_joint_plate_side_side_outplane
- @subpage elements_joint_plate_side_side_rotated
- @subpage elements_joint_plate_cross
- @subpage elements_joint_plate_top_top
- @subpage elements_joint_plate_border
- @subpage elements_joint_beam
- @subpage elements_joint_beam_cross
- @subpage elements_joint_beam_top_side
- @subpage elements_joint_beam_end_to_end
- @subpage elements_joint_beam_butt
- @subpage elements_joint_beam_connectors
- @subpage elements_joint_beam_pins
- @subpage elements_pin

## Joint Wiring Pattern

![](diagrams/wood_joint_unit_box.webp)

```cpp
const std::shared_ptr<InteractionContactFace> contact = scene.compute_face_contact(lower, upper);
const std::shared_ptr<JointBeam> pins = JointBeam::centred_pins(*lower, *upper, *contact);
scene.add(pins);
scene.add_interaction(pins, lower, pins->interaction(0));
scene.add_interaction(pins, upper, pins->interaction(1));
```

| Joint Class | Input Contact | Target Modification |
| --- | --- | --- |
| [Joint](@ref elements_joint) | Axis, mesh, or 2D profile | Drills analytical cylinder bores, or subtracts, unions, or intersects a solid |
| [JointPlate](@ref elements_joint_plate) | `InteractionContactFace` or `InteractionContactCross` | Maps unit-box `[-0.5, 0.5]^3` outlines and drills onto both plates |
| [JointBeam](@ref elements_joint_beam) (contact or butt) | `InteractionContactAxis` or member endpoints | Cuts lap and scarf feature volumes or plane butts |
| [JointBeam](@ref elements_joint_beam) (connector) | `InteractionContactFace` | Cuts pockets and pin bores; nests child `Block`s and [`Pin`](@ref elements_pin)s |
