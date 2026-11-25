# Face Curved {#elements_face_curved}

[TOC]

A closed curved plate of six NURBS patches, bounded by four meridians.

## Constructors

```cpp
FaceCurvedElement(const NurbsCurve& outer_a, const NurbsCurve& outer_b, const NurbsCurve& inner_a, const NurbsCurve& inner_b, const std::string& name = "plate_curved");
```

## Barrel vault strip

A 120 mm thick strip between two outer quarter arcs of radius 2000 mm, 600 mm apart, and two inner arcs of radius 1880 mm.

![](elements/element_face_curved.png)

\include{lineno} elements/element_face_curved.cpp

## A session of several elements

Three strips of a barrel vault side by side.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_face_curved_session.gif) | ![](elements/element_face_curved_session.png) |  |

\include{lineno} elements/element_face_curved_session.cpp

## Cut by a plane

A cutting plane is a joint; the strip keeps the side its normal points to.

![](elements/element_face_curved_cut_plane.png)

\include{lineno} elements/element_face_curved_cut_plane.cpp

## Assign joints

Two strips side by side, pins centred across their contact.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_face_curved_joints.gif) | ![](elements/element_face_curved_joints.png) |  |

\include{lineno} elements/element_face_curved_joints.cpp

## Typical Dimensions

| Curved panel | Thickness mm | Width x length m | Area m² | Radius m | Source |
| --- | --- | --- | --- | --- | --- |
| Cross-laminated, Unterrainer | 60 to 280 | 2.95 x 13.50 | 39.8 | from 2 | [Unterrainer](https://www.holzbau-unterrainer.at/brettsperrholz), [Timber-Online](https://www.timber-online.net/wood_products/2021/07/cross-laminated-timber-reinvented.html) |
| Veneer lumber bent along the face grain | t | | | from 450 t | [Kerto](https://www.metsagroup.com/globalassets/metsa-wood/attachments/kerto-lvl-manual/en/kerto-manual-lvl-curved-structures.pdf) |
| Veneer lumber with cross plies bent across the face grain | t | | | from 350 t | [Kerto](https://www.metsagroup.com/globalassets/metsa-wood/attachments/kerto-lvl-manual/en/kerto-manual-lvl-curved-structures.pdf) |
