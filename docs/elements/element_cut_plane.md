# CutPlane {#elements_cut_plane}

[TOC]

An infinite cutting plane displayed as a square quad; `add_interaction(cut_plane, element, cut_plane->feature())` trims the target element and keeps the half-space in the direction of the plane normal.

## Constructors

```cpp
explicit CutPlane(const Plane& plane, double size = 1000.0, const std::string& name = "cut_plane");
std::shared_ptr<InteractionFeaturePlane> feature() const;
```

## Parameters

| Parameter | Default | Role |
| --- | --- | --- |
| `plane` | — | Cutting plane origin and normal; the target keeps the side pointed to by the normal |
| `size` | `1000` | Side length of the visual display square in mm |

## Cutting a variable beam

Two inclined 900 mm `CutPlane` elements trimming both ends of a parabolic variable-depth beam.

![](elements/element_beam_variable_cut.png)

```cpp
const std::shared_ptr<CutPlane> start = std::make_shared<CutPlane>(
    Plane::from_point_normal({250.0, 0.0, -300.0}, {1.0, 0.0, 0.5}), 900.0, "start"
);
scene.add(start);
scene.add_interaction(start, rib, start->feature());
```

<details><summary>Example code: elements/element_beam_variable_cut.cpp</summary>

\include{lineno} elements/element_beam_variable_cut.cpp

</details>
