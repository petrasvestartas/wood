# Profiles {#elements_profile}

[TOC]

Standard 2D cross-section loop generators swept by `Beam` and `Column`. Loop `0` is the outer perimeter and subsequent loops are interior holes.

## Constructors

```cpp
std::vector<Polyline> profile_rectangle(double width, double depth);
std::vector<Polyline> profile_round(double diameter, int segments = 16);
std::vector<Polyline> profile_w(double width, double depth, double flange, double web);
std::vector<Polyline> profile_hss(double width, double depth, double thickness);
std::vector<Polyline> profile_slab_band(double width, double depth);
std::vector<Polyline> profile_t(double width, double depth, double web, double flange);
```

## Parameters

| Parameter | Role |
| --- | --- |
| `width`, `depth` | Overall section width and depth |
| `diameter`, `segments` | Round section diameter and polygon facet count |
| `flange`, `web` | Flange and web thicknesses for W and T sections |
| `thickness` | Wall thickness of hollow structural sections (`profile_hss`) |

## Every profile

All six standard section profiles swept along 800 mm beam segments: rectangle, round, W, hollow (`hss`), slab band, and T.

![](elements/element_profile.png)

```cpp
const std::vector<Polyline> loops = profile_w(200.0, 300.0, 20.0, 12.0);
scene.add(std::make_shared<Beam>(axis, loops, std::vector<Vector>{}, "w"));
```

<details><summary>Example code: elements/element_profile.cpp</summary>

\include{lineno} elements/element_profile.cpp

</details>
