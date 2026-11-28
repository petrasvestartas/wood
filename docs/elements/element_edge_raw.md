# Edge Raw {#elements_edge_raw}

[TOC]

A natural roundwood trunk or bifurcated tree fork modelled along a structural axis as a fast polygonal round-section beam proxy, with an unprocessed 3D-scanned mesh, RGB point cloud and secondary branch skeleton attached from a stock database by a rigid transformation matrix `raw_xform`.

## Constructors

```cpp
EdgeRawElement(const Line& axis, double radius, const Mesh& raw_mesh = Mesh(), const PointCloud& raw_pointcloud = PointCloud(), const Xform& raw_xform = Xform::identity(), const std::string& name = "edge_raw");

static std::shared_ptr<EdgeRawElement> round(const Line& axis, double radius, double chord_tolerance = 0.2, const std::string& name = "edge_raw");
static std::shared_ptr<EdgeRawElement> from_stock(const Line& axis, double radius, const std::string& stock_name, double roll_degrees = 0.0, bool center_along_axis = true, const std::string& name = "edge_raw");
static std::shared_ptr<EdgeRawElement> from_registered_stock(const std::string& stock_name, const Xform& xform, double radius, const std::optional<Line>& proxy_axis = std::nullopt, const std::string& name = "edge_raw");

// Stock alignment, secondary fork branches and bark clearance
std::shared_ptr<EdgeRawElement> branch_proxy(double branch_radius = 0.0, const std::optional<Line>& proxy_axis = std::nullopt, size_t branch_index = std::numeric_limits<size_t>::max()) const;
Xform align_stock(const Plane& stock_plane = Plane::xy_plane(), double stock_length = 0.0, double roll_degrees = 0.0) const;
void attach_raw(const Mesh& mesh, const PointCloud& pointcloud = PointCloud(), const Xform& xform = Xform::identity());
void attach_stock(const std::string& stock_name, double roll_degrees = 0.0, bool center_along_axis = true);
double raw_overshoot(double margin = 20.0) const;
double raw_end_extension(double margin = 20.0) const;
```

## A scanned trunk along an axis

A Rossiniere Norway spruce trunk (`raw_log_1`) aligned onto an 1800 mm straight axis from `data/raw/`, carrying both its closed watertight surface mesh and its co-registered RGB laser scan.

![](elements/element_edge_raw.png)

\include{lineno} elements/element_edge_raw.cpp

## Scan, beam proxy and attached raw timber

Three stages of the raw-timber workflow side by side on the same four-member portal node: co-registered RGB laser-scan point clouds aligned onto structural axes by `raw_xform` (left), joinery solved rapidly on octagonal round-section beam proxies (`use_raw_mesh = false`, middle), and the identical four-sided shouldered mortise-and-tenon, notched oblique thrust and eccentric conical cross-lap carved through the irregular spruce trunks (`use_raw_mesh = true`, right).

![](elements/element_edge_raw_proxy.png)

\include{lineno} elements/element_edge_raw_proxy.cpp

## Holes

Vertical, horizontal and inclined dowel bores across an irregular scanned trunk (`raw_log_0`): `JointElement::drill` cuts exact rational cylindrical NURBS surfaces through the bark overhang.

![](elements/element_edge_raw_holes.png)

\include{lineno} elements/element_edge_raw_holes.cpp

## Cut by a plane

A bifurcated spruce fork (`raw_fork_1`) trimmed flat across its butt, upper stem and inclined side branch by three `JointCutPlaneElement` interactions.

![](elements/element_edge_raw_cut_plane.png)

\include{lineno} elements/element_edge_raw_cut_plane.cpp

## Conical cross-lap joints

Eccentric crossing roundwood trunks joined by conical cross-lap tiles (`cross_half_lap` and `cross_sliced_half_lap` via `JointBeamElement::from_contact`) at an axis offset $\Delta z = 56\text{ mm}$ on $D = 128\text{ mm}$ logs, keeping over two-thirds of each timber section intact while stretching the conical pocket mouth clear of the bark: a six-cut extended conical cross-lap locked by a central cylindrical wooden pin (left), and a nine-cut sliced conical cross-lap at $60^\circ$ locked by two diagonal wooden pins (right).

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_cross.gif) | ![](joint_library/assembly_element_edge_raw_cross.png) | ![](elements/element_edge_raw_cross.png) |

\include{lineno} elements/element_edge_raw_cross.cpp

## Top-to-side and end-to-end scarf joints

Top-to-side and colinear end-to-end joinery on scanned spruce trunks: a four-sided shouldered blind mortise-and-tenon post with automatic bark-seat surfacing and an inclined notched oblique thrust strut landing on one raw carrier (left), beside a plain half-lap scarf splice locked by two inclined wooden pins (right).

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_joints.gif) | ![](joint_library/assembly_element_edge_raw_joints.png) | ![](elements/element_edge_raw_joints.png) |

\include{lineno} elements/element_edge_raw_joints.cpp

## Parallel flattened trunks with butterfly keys

Side-to-side slab joinery on four parallel scanned spruce trunks surfaced flat along their top plane and longitudinal contact seams while preserving natural round bark underneath and on the outer flanks, locked across every seam by staggered inlaid double-dovetail butterfly keys and transverse cylindrical wooden dowels.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_parallel.gif) | ![](joint_library/assembly_element_edge_raw_parallel.png) | ![](elements/element_edge_raw_parallel.png) |

\include{lineno} elements/element_edge_raw_parallel.cpp

## Triangular tree-fork space truss

The seven-member Rossiniere triangular space truss reconstructed from three 3D-scanned spruce forks (`raw_fork_0..2`) and four straight logs (`raw_log_0..3`): chord proxies solve four plain half-lap scarf splices with pairs of cylindrical wooden pins, while `EdgeRawElement::branch_proxy` solves three cyclic angled four-sided shouldered blind mortise-and-tenon web joints triangulating all three chords into a collision-free seven-step assembly sequence.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_truss.gif) | ![](joint_library/assembly_element_edge_raw_truss.png) | ![](elements/element_edge_raw_truss.png) |

\include{lineno} elements/element_edge_raw_truss.cpp

## Reciprocal conical nexorade

A six-log raw-wood reciprocal nexorade where four transverse spruce logs weave alternately over and under two tip-to-butt longitudinal trunks around three cyclic windows. All eight eccentric crossings ($\Delta z = \pm 54\text{ mm}$, pocket depth $< \frac{1}{3} D$) use six-cut conical cross-lap joints whose sloped cheeks allow each woven rung to rotate $30^\circ$ into seat about its midpoint before locking with cylindrical wooden pins.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_nexorade.gif) | ![](joint_library/assembly_element_edge_raw_nexorade.png) | ![](elements/element_edge_raw_nexorade.png) |

\include{lineno} elements/element_edge_raw_nexorade.cpp

## Bifurcated fork pavilion

Four bifurcated spruce Y-columns (`raw_fork_0` and `raw_fork_2`) turned inward and trimmed level at base and crown so both stem and branch cradle two longitudinal log girders, spanned across by three raw rafters seated in eccentric conical cross-laps and pinned onto the girders.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_session.gif) | ![](joint_library/assembly_element_edge_raw_session.png) | ![](elements/element_edge_raw_session.png) |

\include{lineno} elements/element_edge_raw_session.cpp

## Typical Dimensions

Diameters $D$, tapers and fork angles of debarked roundwood logs and structural tree forks; bark, sweep and knot swell add 15 to 40 mm radially over the fitted proxy cylinder.

| Member | Species | Diameter D mm | Length m | Taper / fork angle |
| --- | --- | --- | --- | --- |
| Small-diameter thinnings (`data/raw/raw_log_*`) | Norway spruce (*Picea abies*) | 95 to 175 | 0.9 to 4.5 | 5 to 12 mm / m, sweep to 35 mm |
| Structural roundwood columns and chords | Spruce, pine, Douglas fir, larch | 140 to 360 | 3.0 to 12.0 | 6 to 10 mm / m |
| Truss web forks (`data/raw/raw_fork_*`) | Norway spruce (*Picea abies*) | 110 to 180 stem, 80 to 140 branch | 1.0 to 2.4 | 18° to 45° between branches |
| Heavy Y-columns and arch forks | Beech, oak, ash, Douglas fir | 220 to 500 stem, 150 to 350 branch | 2.5 to 6.5 | 25° to 60° between branches |

### Joinery allowances on irregular roundwood

| Joint family | Eccentricity / cut depth | Raw-bark allowance | Fasteners |
| --- | --- | --- | --- |
| Conical cross-lap (`cross_half_lap`, `cross_sliced_half_lap`) | Axis offset $\Delta z \approx 0.44\text{–}0.50\,D$, pocket depth $< \frac{1}{3}\,D$ ($> \frac{2}{3}$ section preserved) | Conical pocket mouth stretched past local bark overhang | 16 to 20 mm central or crossed diagonal hardwood pins |
| Four-sided shouldered fork tenon (`fully_shouldered_blind_mortise_tenon`) | Mortise depth $0.40\text{–}0.50\,D$, tenon thickness $\frac{1}{3}\,D$ | Flat carrier shoulder seat shaved within landing footprint (`raw_overshoot`) | Optional cross-peg or self-locking truss triangulation |
| Plain half-lap scarf splice (`half_lap_splice`) | Lap length $1.8\text{–}2.5\,D$, mid-depth plane | Radial `raw_overshoot` + axial `raw_end_extension` | Pair of 16 to 20 mm cylindrical wooden pins |
| Flared butterfly key slab (`dovetail_butterfly_key`) | Key depth $0.20\text{–}0.25\,D$, staggered across seams | Flattened top datum and milled longitudinal contact seams | Inlaid hardwood butterfly keys + transverse dowels |

Sources: [Vestartas (2021)](https://raw.githubusercontent.com/petrasvestartas/storage/refs/heads/main/images/papers/2021_phd_thesis.pdf), chapters 5.3, 5.5, 9.4 and appendices A.3–A.4 (Rossiniere spruce fork truss and reciprocal conical cross-lap prototype); Mollica & Self (2016), *Advances in Architectural Geometry* (Hooke Park Wood Chip Barn); Bukauskas et al. (2019), *Form-Fitting Strategies for Diversity-Tolerant Design*.
