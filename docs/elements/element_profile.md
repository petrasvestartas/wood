# Profiles {#elements_profile}

[TOC]

Standard 2D cross-section loop generators swept by `EdgeStraightElement` and `Column`. Loop `0` is the outer perimeter and subsequent loops are interior holes.

## Constructors

```cpp
std::vector<Polyline> profile_rectangle(double width, double depth);
std::vector<Polyline> profile_round(double diameter, int segments = 16);
std::vector<Polyline> profile_w(double width, double depth, double flange, double web);
std::vector<Polyline> profile_hss(double width, double depth, double thickness);
std::vector<Polyline> profile_slab_band(double width, double depth);
std::vector<Polyline> profile_t(double width, double depth, double web, double flange);
```

## Every profile

All six standard section profiles swept along 800 mm beam segments: rectangle, round, W, hollow (`hss`), slab band, and T.

![](elements/element_profile.png)

\include{lineno} elements/element_profile.cpp
