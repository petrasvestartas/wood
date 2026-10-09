# Profiles {#elements_profile}

[TOC]

The section profiles a Beam or a Column sweeps: loops in the section frame, loop 0 the outline, further loops holes.

## Constructors

```cpp
std::vector<Polyline> profile_rectangle(double width, double depth)
std::vector<Polyline> profile_round(double diameter, int segments = 16)
std::vector<Polyline> profile_w(double width, double depth, double flange, double web)
std::vector<Polyline> profile_hss(double width, double depth, double thickness)
std::vector<Polyline> profile_double(double width, double depth, double gap)
std::vector<Polyline> profile_slab_band(double width, double depth)
std::vector<Polyline> profile_t(double width, double depth, double web, double flange)
```

## Every profile

![Every profile](elements/element_profile.png)

Each profile swept along an 800 beam: rectangle, round, W, hollow, double, slab band and T.

| Parameter | What it changes in 3D |
| --- | --- |
| `width`, `depth` | the section across and up |
| `diameter`, `segments` | the round section and its number of flats |
| `flange`, `web` | the thickness of the W's or T's flanges and web |
| `thickness` | the wall of the hollow section |
| `gap` | the space between the two members of a double section |

\include{lineno} elements/element_profile.cpp
