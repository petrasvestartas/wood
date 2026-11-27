# Edge Raw {#elements_edge_raw}

[TOC]

A natural timber trunk or tree fork modelled along an axis as a round or polygonal beam, with an optional scanned mesh and point cloud attached from a stock database by a transformation matrix.

## Constructors

```cpp
EdgeRawElement(const Line& axis, double radius, const Mesh& raw_mesh = Mesh(), const PointCloud& raw_pointcloud = PointCloud(), const Xform& raw_xform = Xform::identity(), const std::string& name = "edge_raw");

static std::shared_ptr<EdgeRawElement> round(const Line& axis, double radius, double chord_tolerance = 0.2, const std::string& name = "edge_raw");
static std::shared_ptr<EdgeRawElement> from_stock(const Line& axis, double radius, const std::string& stock_name, double roll_degrees = 0.0, bool center_along_axis = true, const std::string& name = "edge_raw");

// Stock attachment on an existing beam proxy
Xform align_stock(const Plane& stock_plane = Plane::xy_plane(), double stock_length = 0.0, double roll_degrees = 0.0) const;
void attach_raw(const Mesh& mesh, const PointCloud& pointcloud = PointCloud(), const Xform& xform = Xform::identity());
void attach_stock(const std::string& stock_name, double roll_degrees = 0.0, bool center_along_axis = true);
```

## A scanned trunk along an axis

A Rossiniere spruce log (`raw_log_1`) aligned onto an 1800 mm straight axis from `data/raw/`, carrying both its watertight surface mesh and its co-registered RGB laser scan.

![](elements/element_edge_raw.png)

\include{lineno} elements/element_edge_raw.cpp

## Proxy, point cloud, trunk and fork

Four stages side by side: a fast round beam proxy (`use_raw_mesh = false`), the proxy with its scanned RGB `PointCloud` placed by `raw_xform`, the reconstructed trunk mesh (`raw_log_3`), and a bifurcated tree fork (`raw_fork_0`) with its secondary `branch_axes`.

![](elements/element_edge_raw_proxy.png)

\include{lineno} elements/element_edge_raw_proxy.cpp

## A session of several elements

Two bifurcated spruce forks turned inward and trimmed flat at base and crown, carrying a naturally swept raw log girder.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_session.gif) | ![](elements/element_edge_raw_session.png) |  |

\include{lineno} elements/element_edge_raw_session.cpp

## Holes

Two dowel holes bored across a scanned spruce log (`raw_log_0`): `JointElement::drill` cuts exact rational cylindrical NURBS bores through the irregular bark.

![](elements/element_edge_raw_holes.png)

\include{lineno} elements/element_edge_raw_holes.cpp

## Cut by a plane

A bifurcated spruce fork (`raw_fork_1`) trimmed at its butt, upper stem and inclined side branch by three `JointCutPlaneElement` interactions.

![](elements/element_edge_raw_cut_plane.png)

\include{lineno} elements/element_edge_raw_cut_plane.cpp

## Assign joints

Two crossing members modelled first as round-section beams, connected to scanned spruce trunks (`raw_log_0` and `raw_log_1`) by `attach_stock` and joined by a pegged cross-lap whose boolean cutters extend past the bark overhang.

| assembly | still | |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_joints.gif) | ![](elements/element_edge_raw_joints.png) |  |

\include{lineno} elements/element_edge_raw_joints.cpp

## Typical Dimensions

Diameters D, tapers and branch angles of debarked roundwood logs and structural tree forks; bark and knot swell add 15 to 40 mm radially over the fitted proxy cylinder.

| Member | Species | Diameter D mm | Length m | Taper / fork angle |
| --- | --- | --- | --- | --- |
| Small-diameter thinnings (`data/raw/raw_log_*`) | Norway spruce (*Picea abies*) | 95 to 175 | 0.9 to 4.5 | 5 to 12 mm / m, sweep to 35 mm |
| Structural roundwood columns and chords | Spruce, pine, Douglas fir, larch | 140 to 360 | 3.0 to 12.0 | 6 to 10 mm / m |
| Truss web forks (`data/raw/raw_fork_*`) | Norway spruce (*Picea abies*) | 110 to 180 stem, 80 to 140 branch | 1.0 to 2.4 | 18° to 45° between branches |
| Heavy Y-columns and arch forks | Beech, oak, ash, Douglas fir | 220 to 500 stem, 150 to 350 branch | 2.5 to 6.5 | 25° to 60° between branches |

Boolean joinery cutters extend radially past the nominal proxy radius by `raw_overshoot` (typically 20 to 45 mm) and axially past the chord ends by `raw_end_extension`, so no bark sliver remains after fabrication cuts.

Sources: [Vestartas (2021)](https://raw.githubusercontent.com/petrasvestartas/storage/refs/heads/main/images/papers/2021_phd_thesis.pdf), chapters 5.3, 6.4 and appendix A.3 (Rossiniere spruce truss); Mollica & Self (2016), *Advances in Architectural Geometry* (Hooke Park Wood Chip Barn); Bukauskas et al. (2019), *Form-Fitting Strategies for Diversity-Tolerant Design*.
