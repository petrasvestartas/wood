#pragma once

#include "pch.h"


namespace wood_session {

/// Merged cut outlines of a plate, per face: [0] the outer boundary, [1..] holes.
struct Features {
    std::vector<session_cpp::Polyline> top; // Top face: the outer outline first, then one outline per hole.
    std::vector<session_cpp::Polyline> bottom; // Bottom face: the outer outline first, then one outline per hole.
};

/// A timber plate: a bottom and a top outline, one side face per edge, and the joints cut into it. It carries two geometries, as a compas_model element does: element_geometry_mesh() / element_geometry_brep() give the plate alone, the loft of its two outlines, never cut; model_geometry_mesh() / model_geometry_brep() give the plate with its joints cut in, the loft of the merged outlines, the one to inspect and the one pb_dump writes. Neither is lofted until asked for.
class Plate : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Plate"; // The element_type this plate is written under.
    static constexpr std::string_view LEGACY_ELEMENT_TYPE = "WoodElement"; // The element_type wood wrote before, still accepted on read.
    std::vector<session_cpp::Polyline> polylines; // Face outlines: [0] bottom, [1] top, [2..] one closed quad per side.
    std::vector<session_cpp::Plane> planes; // One plane per outline, normals pointing out of the plate.
    double thickness = 0.0; // Distance between the bottom and the top plane.
    bool reversed = false; // True when the constructor reversed both outlines to make the bottom normal point away from the top.
    Features features; // Merged cut outlines after compute_features; empty before. Call invalidate_geometry() after assigning.
    std::vector<int> feature_types; // Joint type per face from the joints_types sidecar, indexed like polylines; empty lets the solver decide. The annen and vidy datasets only.

private:
    mutable std::optional<session_cpp::Mesh> _element_geometry_mesh; // Cache of the mesh form.
    mutable std::optional<session_cpp::BRep> _element_geometry_brep; // Cache of the brep form.
    mutable std::optional<session_cpp::Mesh> _model_geometry_mesh; // Cache of the mesh form.
    mutable std::optional<session_cpp::BRep> _model_geometry_brep; // Cache of the brep form.

public:
    /// An empty plate: no outlines, no planes, nothing to loft.
    Plate();

    /// A plate from its bottom and top outline; `name` is the type flag face_contacts() filters on.
    Plate(const session_cpp::Polyline& bottom, const session_cpp::Polyline& top, const std::string& name = "plate");

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A rectangular plate: the kernel's rectangle at `origin` along `x_axis` and `y_axis` as the bottom outline, moved by `thickness` for the top.
    static std::shared_ptr<Plate> from_rectangle(const session_cpp::Point& origin, const session_cpp::Vector& x_axis, const session_cpp::Vector& y_axis, double width, double height, const session_cpp::Vector& thickness, const std::string& name = "plate");

    /// The plate an Element written by pb_dumps() describes, same guid; an element without the outline payload comes back empty.
    static std::shared_ptr<Plate> from_element(session_cpp::Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The parametric shape alone, before joints or cuts as a Mesh; computed on first access and cached independently until invalidate_geometry() or place().
    const session_cpp::Mesh& element_geometry_mesh() const override;

    /// The parametric shape alone, before joints or cuts as a BRep; computed on first access and cached independently until invalidate_geometry() or place().
    const session_cpp::BRep& element_geometry_brep() const override;

    /// The shape with joints or cuts applied as a Mesh; computed on first access and cached independently until invalidate_geometry() or place().
    const session_cpp::Mesh& model_geometry_mesh() const override;

    /// The shape with joints or cuts applied as a BRep; computed on first access and cached independently until invalidate_geometry() or place().
    const session_cpp::BRep& model_geometry_brep() const override;

    /// Swaps bottom and top, outlines and planes, and drops every cache the kernel and the plate hold; detection asks for it when a joint wants the other face first.
    void flip();

    /// Drops all cached lofts and marks the Element slot stale; the merge calls it after filling features, and so must anyone assigning polylines or features by hand.
    void invalidate_geometry() override;

    /// A copy moved by xform from the members alone, never the constructor: outlines, planes, merged features, element features and insertion vectors moved, thickness, reversed and feature types kept, guid and name too; no loft until one is asked for; nullptr for a mirror.
    std::shared_ptr<Plate> transformed(const session_cpp::Xform& xform) const;

    /// Moves the solid, the element features and the insertion vectors, then the outlines, planes and merged features, and drops all cached lofts.
    void place(const session_cpp::Xform& xform) override;

protected:
    /// Writes the model geometry (cached, lofted here at the latest), the dimensions and the face features onto the Element, keeping the joint and contact features the session put there, the slot the session file and the viewer read; WoodSession::pb_dump calls it for every stale plate, so nothing lofts until a file is written or a geometry is asked for.
    void compute_geometry_mesh_impl() override;

    /// Write the model BRep and the element features into the session slot.
    void compute_geometry_brep_impl() override;

    /// Refresh the dimensions and geometry features while preserving session features.
    void compute_geometry_features();

public:

    /// Outline extent in the plate's own frame, thickness in z.
    session_cpp::Vector nominal_dimensions() const;

    /// One ElementFeature per face with a joint type ("joint_type_<code>") or cut outlines ("cut"); [0] bottom, [1] top, [2..] sides.
    std::vector<session_cpp::ElementFeature> face_features() const;

protected:
    /// The plate's own outlines, so Element::polylines() agrees with the solver's view.
    std::vector<session_cpp::Polyline> compute_polylines() const override {
        return polylines;
    }

    /// The plate's own planes, so Element::planes() agrees with the solver's view.
    std::vector<session_cpp::Plane> compute_planes() const override {
        return planes;
    }

public:
    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The outline payload as JSON: bottom, reversed, top, type; the protobuf message printed; a payload in the kernel's JSON from older files is still read.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The outline payload as wood_proto.Plate bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// The kernel's cached box of the lofted geometry.
    using session_cpp::Element::aabb;

    /// The box of every outline, inflated on each side; valid before the plate is lofted.
    session_cpp::AABB aabb(double inflate) const;

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override {
        return std::make_shared<Plate>(*this);
    }

    /// Registers the "Plate" factory (and the legacy "WoodElement" tag) with the kernel, so Session::pb_load rebuilds plates.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Plate(name, polylines, thickness)".
    std::string str() const override;

    /// "Plate(name, polylines, planes, reversed, thickness, features)".
    std::string repr() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Dataset overrides - the annen and vidy datasets only
    // ═══════════════════════════════════════════════════════════════════════════

    /// The insertion vectors the Element holds, read-only.
    using session_cpp::Element::insertion_vectors;

    /// The insertion vectors the Element holds, one per face from the insertion_vectors sidecar, writable by the solver.
    std::vector<session_cpp::Vector>& insertion_vectors() {
        return _insertion_vectors;
    }
};

} // namespace wood_session
