# Architecture {#architecture}

[TOC]

Wood is a C++20 timber joinery solver and parametric structure library built on [session_cpp](https://github.com/petrasvestartas/session_cpp). Elements store their uncut stock geometry alongside directed cutting interactions in WoodSession, evaluating boolean-cut meshes and exact NURBS BReps on demand.

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

## Scene Graph {#arch_scene_graph}

![](diagrams/wood_architecture.webp)

WoodSession wraps a session containing a visual tree hierarchy and a directed interaction graph.

| Structure | Representation | Role |
| --- | --- | --- |
| Session::tree | TreeNode hierarchy | Scene layers and parent-child ownership |
| Session::graph | Directed Element and Interaction graph | Contact edges and boolean cutting dependencies |

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

## Stock and Cut Geometry {#arch_stock_vs_cut}

![](diagrams/wood_elements_taxonomy.webp)

Every Element caches two lazy geometric states in both Mesh and BRep form: uncut stock (`element_geometry_mesh()`, `element_geometry_brep()`) built once and transformed by `place(Xform)`, and cut solids (`model_geometry_mesh()`, `model_geometry_brep()`) recomputed when interactions mark the element dirty.

| Element | Defining Geometry | Stock Representation |
| --- | --- | --- |
| Plate | Planar bottom and top Polyline loops | Bottom face 0, top face 1, side faces j + 2 |
| PlateCurved | Four boundary NurbsCurve rails | Exact 6-patch ruled NURBS solid |
| Beam | Axis Line, section Polyline, up Vector | Extruded prism along axis |
| BeamCurved | Centreline NurbsCurve, width, height, lamellae | Curved glulam with RMF frames and 6-patch NURBS BRep |
| BeamVariable | Axis Line, station Polylines | Multi-section lofted prism or parabolic rib |
| Column | Axis, frame, width | Square shaft with head blocks |
| Support | Plane, base_width, head_diameter, pin_length | Two steel base plates and three anchor pins |
| Block / Pin | Mesh / cylinder axis and radius | Connector solids and dowels nested under joints |
| CutPlane / Profile | Plane / planar cutting polylines | Half-space clip plane and 2D cutting loops |

## Contact Detection {#arch_contact_detection}

![](diagrams/wood_joint_unit_box.webp)

All contact queries operate on uncut stock so joint geometry does not depend on cutting order.

| Query | Return Type | Construction |
| --- | --- | --- |
| compute_face_contact(a, b, min_area) | InteractionContactFace | Coplanar opposing faces clipped to 2D intersection polygon and local frame |
| compute_face_contacts(min_area) | void | BVH broad-phase + pairwise face contact search across session elements |
| compute_cross_contact(a, b) | InteractionContactCross | 4-plane intersection volume along the crossing line of two plates |
| compute_axis_contact(a, b) | InteractionContactAxis | Closest segment between element axes and oriented feature boxes |

## Boolean and BRep Cutting {#arch_boolean_cutting}

![](diagrams/wood_boolean_cutting.webp)

Incoming feature edges are evaluated in `Element::cut()` for meshes and `Element::cut_brep()` for exact NURBS BReps.

| Feature | Plate Action | Beam / Column / Block Action | Exact BRep |
| --- | --- | --- | --- |
| InteractionFeaturePlane | Shifts outline edges or clips body | Clips mesh by half-space plane | BRep::trimmed_by_plane |
| InteractionFeatureProfile | 2D outline diff or pocket subtract | Extruded cutter prism boolean subtract | BRep::cut_profile |
| InteractionFeatureSolid | MeshBoolean subtract or add | MeshBoolean subtract or add | Merged planar BRep faces |
| InteractionFeatureJoint | Outline merge, pockets, and drill_lines | Solid cutter meshes and drill_lines | Analytical cylindrical bores via BRep::cut_cylinder |

## Serialization

- Mesh mode: `session.pb_dump(pb_path("live"))` writes cut meshes, polyline overlays, and the scene tree hierarchy.
- Exact BRep mode: `session.compute_breps()` populates `session.lookup.brep` so cylindrical bores and curved members serialize as exact NURBS surfaces.
