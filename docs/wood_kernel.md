# Architecture {#architecture}

[TOC]

Wood is a C++23 timber joinery solver and parametric structure library built on [session_cpp](https://github.com/petrasvestartas/session_cpp). Elements store their uncut stock geometry alongside directed cutting interactions in WoodSession, evaluating boolean-cut meshes and exact NURBS BReps on demand.

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
class WoodSession : public Session {
public:
    Settings settings;

    void add(const std::shared_ptr<Element>& element, const std::shared_ptr<TreeNode>& parent = nullptr);
    std::shared_ptr<Interaction> add_interaction(const std::shared_ptr<Element>& source, const std::shared_ptr<Element>& target, const std::shared_ptr<Interaction>& interaction);
    WoodSession get_branch(const std::string& group_name) const;
    void graft(const WoodSession& other, const std::shared_ptr<TreeNode>& parent = nullptr);
    void compute_breps();
    void pb_dump(const std::string& filename);
};
```

## Stock and Cut Geometry {#arch_stock_vs_cut}

![](diagrams/wood_elements_taxonomy.webp)

Every WoodElement caches two lazy geometric states in both Mesh and BRep form: uncut stock (`element_geometry_mesh()`, `element_geometry_brep()`) built once and placed via scene transforms, and cut solids (`model_geometry_mesh()`, `model_geometry_brep()`) recomputed when interactions invalidate the element.

| Element | Defining Geometry | Stock Representation |
| --- | --- | --- |
| Plate | Planar bottom and top Polyline loops | Bottom face 0, top face 1, side faces j + 2 |
| PlateCurved | Four boundary NurbsCurve rails | Exact 6-patch ruled NURBS solid |
| Beam | Polyline axis, section radii, up vectors, side count | Rectangular or polygonal/round swept timber prism |
| BeamCurved | Centreline NurbsCurve, width, height, lamellae | Curved glulam with RMF frames and 6-patch NURBS BRep |
| BeamVariable | Axis Line, station Polylines | Multi-section lofted prism or parabolic rib |
| Column | Axis, frame, width | Square shaft with head blocks |
| Support | Plane, base_width, head_diameter, pin_length | Two steel base plates and three anchor pins |
| Block / Pin / ConnectorPart | Mesh / cylinder axis and radius / connector plate | Connector solids and dowels nested under joints |
| CutPlane | Plane | Half-space clip plane |

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

Hosted features are evaluated lazily in `WoodElement::model_geometry_mesh()` for closed Manifold meshes and `WoodElement::model_geometry_brep()` for exact NURBS BReps.

| Feature | Plate Action | Beam / Column / Block Action | Exact BRep |
| --- | --- | --- | --- |
| InteractionFeaturePlane | Clips body by half-space plane | Clips mesh by half-space plane | `BRep::cut_by_plane` |
| InteractionFeatureSolid | Manifold boolean subtract or add + `drills` | Manifold boolean subtract or add + `drills` | Merged planar faces + analytical NURBS bores (`drilled_brep`) |
| InteractionFeaturePlate | 2D outline merge, mortise holes, solid cutters, and `drill_lines` | Oriented unit-box joinery transferred to beam faces | Planar `brep_between_loops` or `solid_features_brep` |
| InteractionFeatureBeam | — | Axis-contact solid cutter meshes, butt planes, and `drill_lines` | Exact planar faces and cylindrical NURBS bores |

## Serialization {#arch_serialization}

- Mesh mode: `session.pb_dump(pb_path("live"))` writes cut meshes, BReps for round/drilled members, polyline overlays, and the scene tree hierarchy.
- Exact BRep mode: `session.compute_breps()` evaluates `compute_geometry_brep()` on every cut member, connector part, pin, curved member, and support so cylindrical bores and curved surfaces serialize as exact NURBS BReps.
