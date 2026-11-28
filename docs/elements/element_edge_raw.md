# Edge Raw {#elements_edge_raw}

[TOC]

A natural roundwood trunk, curved stem or bifurcated tree fork modelled along a structural axis as a fast polygonal round-section beam proxy, with an unprocessed 3D-scanned mesh, RGB point cloud and secondary branch skeleton attached from a stock database by a rigid transformation matrix `raw_xform` or parallel-transport curve warp.

## Constructors

```cpp
EdgeRawElement(const Line& axis, double radius, const Mesh& raw_mesh = Mesh(), const PointCloud& raw_pointcloud = PointCloud(), const Xform& raw_xform = Xform::identity(), const std::string& name = "edge_raw");

static std::shared_ptr<EdgeRawElement> round(const Line& axis, double radius, double chord_tolerance = 0.2, const std::string& name = "edge_raw");
static std::shared_ptr<EdgeRawElement> from_stock(const Line& axis, double radius, const std::string& stock_name, double roll_degrees = 0.0, bool center_along_axis = true, const std::string& name = "edge_raw");
static std::shared_ptr<EdgeRawElement> from_registered_stock(const std::string& stock_name, const Xform& xform, double radius, const std::optional<Line>& proxy_axis = std::nullopt, const std::string& name = "edge_raw");
static std::shared_ptr<EdgeRawElement> from_bent_stock(const Polyline& curve_axis, double radius, const std::string& stock_name, double roll_degrees = 0.0, const std::string& name = "edge_raw");

// Stock alignment, secondary fork branches, local curved tangent proxies and bark clearance
std::shared_ptr<EdgeRawElement> branch_proxy(double branch_radius = 0.0, const std::optional<Line>& proxy_axis = std::nullopt, size_t branch_index = std::numeric_limits<size_t>::max()) const;
std::shared_ptr<EdgeRawElement> segment_proxy(const Point& near_point, double half_span = 200.0) const;
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

Three stages of the raw-timber workflow side by side on the same four-member portal node: co-registered RGB laser-scan point clouds aligned onto structural centreline axes by `raw_xform` (left), interlocking carpentry solved rapidly on octagonal round-section beam proxies without carrying dense bark meshes through every calculation (`use_raw_mesh = false`, middle), and the identical flared dovetail T-bridle (`dovetail_t_bridle`), pegged oblique-thrust mortise-and-tenon (`notched_oblique_thrust_pegged_mortise_tenon`) and eccentric conical cross-lap carved through the irregular spruce trunks with cutting volumes extended past scan and bark uncertainty while keeping mating faces exact (`use_raw_mesh = true`, right).

![](elements/element_edge_raw_proxy.png)

\include{lineno} elements/element_edge_raw_proxy.cpp

## Holes

Vertical, horizontal and inclined cylindrical bores across an irregular scanned trunk (`raw_log_0`): `JointElement::drill` cuts exact rational cylindrical NURBS surfaces cleanly past bark swell, serving both structural hardwood dowels and the three-hole local reference datums that tie a physical fork or log to its digital scan in the robot cell and on the assembly jig.

![](elements/element_edge_raw_holes.png)

\include{lineno} elements/element_edge_raw_holes.cpp

## Cut by a plane

A bifurcated spruce fork (`raw_fork_1`) trimmed flat across its butt, upper stem and inclined side branch by three `JointCutPlaneElement` interactions.

![](elements/element_edge_raw_cut_plane.png)

\include{lineno} elements/element_edge_raw_cut_plane.cpp

## Conical cross-lap joints

Eccentric crossing roundwood trunks joined at 60° by conical cross-lap tiles (`cross_half_lap` and `cross_sliced_half_lap` via `JointBeamElement::from_contact`) at an axis offset dz = 56 mm on D = 128 mm logs, keeping over two-thirds of each timber section intact while stretching the conical pocket mouth clear of the bark: a six-cut extended conical cross-lap locked by a central cylindrical wooden pin (left), and a nine-cut sliced conical cross-lap locked by two crossed diagonal wooden pins (right).

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_cross.gif) | ![](joint_library/assembly_element_edge_raw_cross.png) | ![](elements/element_edge_raw_cross.png) |

\include{lineno} elements/element_edge_raw_cross.cpp

## Tusk tenon, dovetail brace, Jupiter scarf and dowelled lap

All primary connection topologies on 3D-scanned spruce trunks (Thesis Fig. 5.3 A–E): a horizontally wedged through tusk tenon post (`horizontally_wedged_tusk_tenon`, with spot-faced entry and exit seats and twin opposing horizontal hardwood keys) beside an inclined pegged stopped-dovetail half-lap strut (`pegged_angled_stopped_dovetail_half_lap`) on one raw carrier (left), a colinear *Trait de Jupiter* hooked cogged scarf (`cogged_lap_folding_wedges_splice`) locked by full-height opposing folding wedges and two cylindrical pins (bottom right), and a top-to-top overlapping raw-log splice with a surfaced contact saddle and four vertical wooden dowels (top right, Fig. 5.3 D).

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_joints.gif) | ![](joint_library/assembly_element_edge_raw_joints.png) | ![](elements/element_edge_raw_joints.png) |

\include{lineno} elements/element_edge_raw_joints.cpp

## Castellated shear-cog and butterfly-key slab

Side-to-side slab joinery on four parallel scanned spruce trunks surfaced flat along their top deck while preserving natural round bark underneath and on the outer flanks: five alternating 22 mm castellated longitudinal shear cogs interlock adjacent logs along every seam, reinforced by transverse cylindrical wooden dowels and staggered inlaid double-dovetail butterfly keys.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_parallel.gif) | ![](joint_library/assembly_element_edge_raw_parallel.png) | ![](elements/element_edge_raw_parallel.png) |

\include{lineno} elements/element_edge_raw_parallel.cpp

## Cantilevered grooved stave bench

Cantilevered raw-wood wall-to-deck bench prototype (Thesis Fig. 5.22, 5.23 and 5.44): four vertical scanned spruce wall logs with surfaced inner cheeks and minimally milled rectangular top tenons support four cantilevered deck staves surfaced flat on top while keeping natural round bark underneath, interlocked along every longitudinal seam by stepped rabbet grooves and 10 mm radius transverse wooden dowels.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_bench.gif) | ![](joint_library/assembly_element_edge_raw_bench.png) | ![](elements/element_edge_raw_bench.png) |

\include{lineno} elements/element_edge_raw_bench.cpp

## Side-to-top dovetail pinwheel, dowel-nut and panelled bay

Side-to-top reciprocal raw-wood framing and recessed roof panelling (Thesis Fig. 5.45, 5.46, 5.47 and 9.8): a four-log cyclic reciprocal pinwheel with upward-oriented stopped half-dovetails and an upward-projecting fourth-corner dovetail closure allowing collision-free vertical assembly (left), beside a panelled raw-wood bay combining Dowel-Nut T-joints (spot-faced saddle + axial dowel anchored by an intersecting vertical cross-plug), three-tooth serrated finger pockets, and recessed planar timber panels seated flush into surfaced log rebates (right).

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_side_top_nexorade.gif) | ![](joint_library/assembly_element_edge_raw_side_top_nexorade.png) | ![](elements/element_edge_raw_side_top_nexorade.png) |

\include{lineno} elements/element_edge_raw_side_top_nexorade.cpp

## Triangular tree-fork space truss

The seven-member Rossiniere triangular space truss reconstructed from three 3D-scanned spruce forks (`raw_fork_0..2`) and four straight logs (`raw_log_0..3`): chord proxies solve four stop-splayed cogged scarf splices (`stop_splayed_cogged_scarf`) combining sloped scarf tables, central tension hooks and pairs of cylindrical wooden pins, while `EdgeRawElement::branch_proxy` solves three cyclic pegged stopped-dovetail half-lap fork-branch joints (`pegged_angled_stopped_dovetail_half_lap`) triangulating all three chords into a collision-free assembly sequence.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_truss.gif) | ![](joint_library/assembly_element_edge_raw_truss.png) | ![](elements/element_edge_raw_truss.png) |

\include{lineno} elements/element_edge_raw_truss.cpp

## Reciprocal conical nexorade

A six-log raw-wood reciprocal nexorade where four transverse spruce logs weave alternately over and under two tip-to-butt longitudinal trunks around three cyclic windows. All eight eccentric crossings (dz = ±54 mm, pocket depth under D / 3) use six-cut conical cross-lap joints whose sloped cheeks allow each woven rung to rotate 30° into seat about its midpoint before locking with cylindrical wooden pins.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_nexorade.gif) | ![](joint_library/assembly_element_edge_raw_nexorade.png) | ![](elements/element_edge_raw_nexorade.png) |

\include{lineno} elements/element_edge_raw_nexorade.cpp

## Doubly curved bent gridshell

Arched whole-timber gridshell (`EdgeRawElement::from_bent_stock`, Thesis Fig. 5.58 B and 5.59): six scanned spruce trunks warped along doubly curved shell centrelines cross at nine eccentric nodes (dz = 54 mm), where local straight tangent proxies (`segment_proxy`) cut six-plane conical cross-lap saddles and radial cylindrical hardwood pins while preserving over two-thirds of every curved trunk section.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_bent_shell.gif) | ![](joint_library/assembly_element_edge_raw_bent_shell.png) | ![](elements/element_edge_raw_bent_shell.png) |

\include{lineno} elements/element_edge_raw_bent_shell.cpp

## Two-layer vertical reciprocal frame

Vertical two-layer raw-wood reciprocal prototype (Thesis Fig. 5.60 and 5.61): four upright scanned spruce columns mortised into a timber base slab carry conical cross-lap pockets on both their front (`-Y`) and back (`+Y`) faces, clamping four staggered inclined horizontal raw-wood rails with sixteen transverse hardwood pins.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_two_layer.gif) | ![](joint_library/assembly_element_edge_raw_two_layer.png) | ![](elements/element_edge_raw_two_layer.png) |

\include{lineno} elements/element_edge_raw_two_layer.cpp

## Spatial reciprocal dome and hyperboloid tower

Single- and double-ring spatial reciprocal roundwood shells (Thesis Fig. 5.62 A and 9.21): a nine-log hexagonal reciprocal dome crowned by a three-bar triangular ring (left) beside a ten-log hyperboloid reciprocal funnel tower (right), joined at every eccentric crossing by six-cut conical cross-laps and radial wooden pins.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_dome.gif) | ![](joint_library/assembly_element_edge_raw_dome.png) | ![](elements/element_edge_raw_dome.png) |

\include{lineno} elements/element_edge_raw_dome.cpp

## Segmented three-row staggered raw-timber arch

IBOIS segmented three-row staggered raw-timber arch (Thesis Fig. 8.7, 8.9, 8.16 and 8.17): ten scanned spruce log segments sawn flat only on their two vertical side cheeks while retaining natural round bark on both extrados and intrados, interlocked end-to-end within each ply by three-tenon radial finger butt joints and side-by-side across every staggered overlap by three parallel longitudinal cheek grooves and transverse wooden dowels.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_arch.gif) | ![](joint_library/assembly_element_edge_raw_arch.png) | ![](elements/element_edge_raw_arch.png) |

\include{lineno} elements/element_edge_raw_arch.cpp

## Pointed Gothic tree-fork portal nave

Two-bay pointed tree-fork portal nave (Thesis Appendix A Fig. A.3 B–C): four upright bifurcated spruce Y-fork columns (`raw_fork_1`) tilt outward so both limbs of each natural fork engage the inclined pointed Gothic arch rafters in pegged stopped-dovetail half-lap joints, meeting at pointed crown bridles and tied longitudinally by raw-wood purlins in eccentric conical cross-laps.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_fork_arch.gif) | ![](joint_library/assembly_element_edge_raw_fork_arch.png) | ![](elements/element_edge_raw_fork_arch.png) |

\include{lineno} elements/element_edge_raw_fork_arch.cpp

## Bifurcated fork pavilion

Four bifurcated spruce Y-columns (`raw_fork_0` and `raw_fork_2`) turned inward to support two longitudinal log girders through fourteen interlocking joints: every vertical fork stem cradles its girder in a flared dovetail T-bridle (`dovetail_t_bridle`), every inclined fork branch braces into the girder through an oblique-thrust mortise-and-tenon (`oblique_thrust_mortise_tenon`) locked by a transverse wooden pin, and three raw rafters span across above in six eccentric conical cross-laps with vertical pins.

| assembly | apart | still |
|---|---|---|
| ![](joint_library/assembly_element_edge_raw_session.gif) | ![](joint_library/assembly_element_edge_raw_session.png) | ![](elements/element_edge_raw_session.png) |

\include{lineno} elements/element_edge_raw_session.cpp

## Typical Dimensions

Diameters D, tapers and fork angles of debarked roundwood logs and naturally grown structural tree forks across ground-to-crown harvesting; bark, sweep and knot swell add 15 to 40 mm radially over the fitted proxy cylinder.

| Member | Species | Diameter D mm | Length m | Taper / fork angle |
| --- | --- | --- | --- | --- |
| Small-diameter thinnings (`data/raw/raw_log_*`) | Norway spruce (*Picea abies*), fir | 95 to 175 | 0.9 to 4.5 | 5 to 12 mm / m, sweep to 35 mm |
| Curved / swept structural stems (`from_bent_stock`) | Spruce, larch, oak, sweet chestnut | 110 to 280 | 1.5 to 6.0 | Natural sweep 40 to 250 mm |
| Truss web and crown forks (`data/raw/raw_fork_*`) | Norway spruce, European beech (*Fagus sylvatica*) | 110 to 220 stem, 80 to 160 branch | 1.0 to 2.8 | 18° to 45° between limbs |
| Heavy Y-columns and Gothic arch forks | Beech, oak, ash, Douglas fir | 220 to 500 stem, 150 to 350 branch | 2.5 to 6.5 | 25° to 60° between limbs |

### Joinery allowances on irregular roundwood

| Joint family | Eccentricity / cut depth | Raw-bark allowance | Fasteners |
| --- | --- | --- | --- |
| Conical cross-lap (`cross_half_lap`, `cross_sliced_half_lap`, `segment_proxy`) | Axis offset dz = 0.44 to 0.50 D, pocket depth under D / 3 (over 2 D / 3 section preserved) | Conical pocket mouth stretched past local bark overhang | 16 to 20 mm central or crossed diagonal hardwood pins |
| Dovetail T-bridle & wedged tusk tenon (`dovetail_t_bridle`, `horizontally_wedged_tusk_tenon`) | Full-depth bridle tines or 0.36 D protruding through-tenon | Squared entering cheeks + spot-faced carrier entry and exit seats | Twin opposing horizontal hardwood keys or dovetail bridle |
| Side-to-top half-dovetails, dowel-nut & serrated fingers | Stopped dovetail 0.72 D, 3-tooth finger tenons, recessed panel rebates | Spot-faced cheek saddles + top panel rebates | Axial bolts into vertical cylindrical dowel-nuts, hardwood pins |
| Hooked cogged, Jupiter & radial finger arch scarfs | Lap length 2.0 to 2.9 D or 3-tenon radial finger butt | Radial `raw_overshoot` + sawn side cheeks keeping bark extrados/intrados | Folding wedges, longitudinal cheek grooves and transverse dowels |
| Castellated shear-cog, rabbeted bench & butterfly-key slabs | 12 to 22 mm longitudinal seam cogs/rabbets, key depth 0.22 D | Surfaced top deck, natural bark soffit and milled wall tenons | Inlaid flared butterfly keys + 20 mm transverse wooden dowels |

Sources: [Vestartas (2021)](https://raw.githubusercontent.com/petrasvestartas/storage/refs/heads/main/images/papers/2021_phd_thesis.pdf), chapters 2, 5.2–5.5, 8.2–8.3, 9.1–9.4 and appendices A.3–A.4 (Rossiniere spruce fork truss, cantilevered grooved stave bench, side-to-top dowel-nut nexorade, segmented three-row raw-timber arch, conical reciprocal shells and pointed fork arch nave); Vestartas, Rezaei Rad & Weinand (2021), *Robotically-Fabricated Nexorades from Whole Timber*; Rogeau, Latteur & Weinand (2021), *An Integrated Design Tool for Timber Plate Structures to Generate Joints Geometry, Fabrication Toolpath, and Robot Trajectories*; Mollica & Self (2016), *Tree Fork Truss: Geometric Strategies for Exploiting Inherent Material Form*, *Advances in Architectural Geometry* (Hooke Park Wood Chip Barn); Zachary Mollica, [*From tree scan to building component*](https://zacharymolli.ca/from-tree-scan-to-building-component/), [*Ground to Crown*](https://zacharymolli.ca/ground-to-crown/) and [*Weird Wood Index*](https://zacharymolli.ca/weird-wood-index/); Bauer & Pauli (2024), *Joinery Compendium*.
