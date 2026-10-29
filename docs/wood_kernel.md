# Architecture {#architecture}

[TOC]

`wood` is a C++20 timber joinery solver and parametric structure library built on [`session_cpp`](https://github.com/petrasvestartas/session_cpp). Elements store their uncut stock geometry alongside directed cutting interactions in `WoodSession`, evaluating boolean-cut meshes and exact NURBS BReps on demand.

![](diagrams/wood_architecture.webp)

<div class="wood-card-grid cols-4">
  <a class="wood-card" href="#arch_scene_graph">
    <img src="wood_architecture.webp" alt="Scene Graph">
    <span class="wood-card-title">1. Scene Graph</span>
    <span class="wood-card-sub">WoodSession tree hierarchy &amp; directed interaction graph</span>
  </a>
  <a class="wood-card" href="#arch_stock_vs_cut">
    <img src="wood_elements_taxonomy.webp" alt="Stock vs Cut Geometry">
    <span class="wood-card-title">2. Stock vs Cut</span>
    <span class="wood-card-sub">Lazy Mesh &amp; NURBS BRep caching across all 11 shape elements</span>
  </a>
  <a class="wood-card" href="#arch_contact_detection">
    <img src="wood_joint_unit_box.webp" alt="Contact Detection">
    <span class="wood-card-title">3. Contact Detection</span>
    <span class="wood-card-sub">Face, cross, axis &amp; border contact queries on uncut stock</span>
  </a>
  <a class="wood-card" href="#arch_boolean_cutting">
    <img src="wood_boolean_cutting.webp" alt="Boolean and BRep Cutting">
    <span class="wood-card-title">4. Boolean &amp; BRep Cutting</span>
    <span class="wood-card-sub">Plane, profile, solid &amp; joint feature evaluation</span>
  </a>
</div>
<div style="display:none">
![](diagrams/wood_joint_unit_box.webp)
</div>

```cpp
WoodSession session("model");
const std::shared_ptr<InteractionContactFace> contact = session.compute_face_contact(a, b);
const std::shared_ptr<JointPlate> joint = JointPlate::side_side_outplane_fingers(8, 0.5);
joint->orient(contact, {a, b});
session.add(joint, joints_group);
session.add_interaction(joint, a, joint->interaction(0));
session.add_interaction(joint, b, joint->interaction(1));
session.pb_dump(pb_path("live"));
```

## 1. Scene Graph {#arch_scene_graph}

`WoodSession` wraps a `session_cpp::Session` containing two synchronized structures:

| Structure | Representation | Role |
| --- | --- | --- |
| `Session::tree` | `TreeNode` hierarchy | Visual scene layers (`plates`, `beams`, `joints`, `contacts`) and parent-child ownership (`JointBeam` → `Block`/`Pin` children) |
| `Session::graph` | Directed graph of `Element` vertices and `Interaction` edges | Geometric dependencies: contact edges record shared faces or axes; feature edges cut target stock solids |

```cpp
class WoodSession {
public:
    Session session;
    Settings settings;

    void add(const std::shared_ptr<Element>& element, const std::shared_ptr<TreeNode>& parent = nullptr);
    void add_interaction(const std::shared_ptr<Element>& source, const std::shared_ptr<Element>& target, const std::shared_ptr<Interaction>& interaction);
    WoodSession get_branch(const std::string& group_name) const;
    void graft(const WoodSession& other, const std::shared_ptr<TreeNode>& parent = nullptr);
    void compute_breps();
    void pb_dump(const std::string& filename);
};
```

## 2. Stock vs Cut Geometry {#arch_stock_vs_cut}

Every `Element` caches two lazy geometric states in both `Mesh` and `BRep` representations:

| State | Methods | Cache Invalidation |
| --- | --- | --- |
| **Uncut Stock** | `element_geometry_mesh()`, `element_geometry_brep()` | Built once via `compute_element_geometry()`; transformed in-place by `place(Xform)` |
| **Cut Solid** | `model_geometry_mesh()`, `model_geometry_brep()` | Recomputed on demand when `is_dirty` is set by `WoodSession::add_interaction` |

![](diagrams/wood_elements_taxonomy.webp)

| Element | Defining Geometry | Stock `BRep` / `Mesh` |
| --- | --- | --- |
| `Plate` | `bottom`, `top` planar `Polyline` loops | Face `0` bottom, face `1` top, side faces `j + 2` along edges |
| `PlateCurved` | `outer_a`, `outer_b`, `inner_a`, `inner_b` (`NurbsCurve`) | Exact 6-patch ruled NURBS solid (`BRep::from_nurbssurfaces`) |
| `Beam` | `axis` (`Line`), `profile` (`Polyline`), `z` (`Vector`) | Extruded prism in `Plane::from_line_direction(axis, z)` |
| `BeamCurved` | `centreline` (`NurbsCurve`), `width`, `height`, `directions`, `num_width`, `num_height` | Single/double-curved glulam with RMF twist, lamella stack, and 6-patch NURBS `BRep` |
| `BeamVariable` | `axis` (`Line`), `profiles` (`vector<Polyline>`), `stations` | Multi-section lofted prism or parabolic soffit rib |
| `Column` | `axis`, `frame`, `width` | Square shaft + `head_blocks` (`SolidOperation::add`) |
| `Support` | `plane`, `base_width`, `head_diameter`, `pin_length` | Two steel plates + 3 anchor pins |
| `Block` / `Pin` | `Mesh` / `(head, direction, radius, length)` | Auxiliary connector parts nested under `JointBeam` or `Column` |
| `CutPlane` / `Profile` | `Plane` / `(Plane, vector<Polyline>)` | Half-space clip plane / 2D cutting loops |

## 3. Contact Detection {#arch_contact_detection}

All contact queries operate on **uncut stock** (`element_geometry_*`) so joint geometry is independent of cutting order:

| Query | Return Type | Geometric Construction |
| --- | --- | --- |
| `compute_face_contact(a, b, min_area)` | `shared_ptr<InteractionContactFace>` | Coplanar opposing faces → 2D Clipper2 intersection polygon, local `frame`, `thickness`, and `type` (`side_side`, `side_top`, `top_top`, `end_side`, `end_end`, `end_top`) |
| `compute_face_contacts(min_area)` | `void` (populates `Session::graph`) | BVH broad-phase (`BVH::from_boxes`) + pairwise face contact search across all session elements |
| `compute_cross_contact(a, b)` | `shared_ptr<InteractionContactCross>` | 4-plane intersection volume (`cross_line`, `volumes[0..1]`, `plane_0`, `plane_1`) where two plates cross |
| `compute_axis_contact(a, b)` | `shared_ptr<InteractionContactAxis>` | Closest segment between element axes + oriented feature boxes |

## 4. Boolean and BRep Cutting {#arch_boolean_cutting}

![](diagrams/wood_boolean_cutting.webp)

Incoming `InteractionFeature` edges are dispatched in `Element::cut()` (mesh) and `Element::cut_brep()` (exact BRep):

| Feature Class | `Plate` Action | `Beam` / `Column` / `Block` Action | Exact `BRep` Representation |
| --- | --- | --- | --- |
| `InteractionFeaturePlane` | `trim_by_plane` (shifts outline edges or slices body) | `BooleanPolyline::clip_mesh_by_plane` | `BRep::trimmed_by_plane` (exact planar cap) |
| `InteractionFeatureProfile` | Edge-crossing loop → 2D outline `BooleanPolyline` diff; interior loop → pocket subtract | Lofted cutter prism along plane normal → `MeshBoolean` subtract | `BRep::cut_profile` (analytical planar extrusion subtract) |
| `InteractionFeatureSolid` | `MeshBoolean::compute` (`subtract` or `add`) | `MeshBoolean::compute` (`subtract` or `add`) | `BRep::from_polylines` + coplanar face merge |
| `InteractionFeatureJoint` | Outline merge (`side_swap`, `edge_insertion`) + solid pockets + `drill_lines` | Solid cutter meshes + `drill_lines` | Analytical `NurbsSurface::create_cylinder` bores via `BRep::cut_cylinder` |

## 5. Serialization

- **Mesh mode (default):** `session.pb_dump(pb_path("live"))` writes every visible `Element`'s `model_geometry_mesh()`, polyline overlays, and `Session::tree` hierarchy.
- **Exact BRep mode:** `session.compute_breps()` populates `session.lookup.brep` with `model_geometry_brep()` so cylindrical bores and curved plates serialize as exact NURBS surfaces.
