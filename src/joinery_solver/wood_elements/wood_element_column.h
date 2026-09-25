#pragma once

#include "pch.h"

namespace wood_session {

/// A column: a solid that knows its own axis, the section it is cut from and the planes that trim it.
class Column : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Column"; // The element_type this column is written under.
    session_cpp::Line axis; // Centreline, base to head, in world space.
    session_cpp::Polyline section; // Closed cross-section about the axis base; empty when unknown.
    std::vector<session_cpp::Plane> cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.
    std::vector<session_cpp::Polyline> profile; // Section loops in the profile frame the section was placed from, loop 0 the outline, then holes; empty when the section was given.
    double rotation = 0.0; // Degrees the profile x axis turns from world x about the axis.

private:
    mutable std::optional<session_cpp::Mesh> _element_geometry_mesh; // Cache of the mesh form.
    mutable std::optional<session_cpp::BRep> _element_geometry_brep; // Cache of the brep form.
    mutable std::optional<session_cpp::Mesh> _model_geometry_mesh; // Cache of the cut mesh form.
    mutable std::optional<session_cpp::BRep> _model_geometry_brep; // Cache of the cut brep form.

public:
    /// An empty column: no solid, a zero-length axis, no section.
    Column();

    /// A column from its axis and its section: the solid is the section swept along the axis; `name` is the type flag face_contacts() filters on.
    Column(const session_cpp::Line& axis, const session_cpp::Polyline& section, const std::string& name = "column");

    /// A column from its axis and a profile placed at the axis base in the plane perpendicular to it, x along world x projected then turned by rotation degrees; holes make it hollow.
    Column(const session_cpp::Line& axis, const std::vector<session_cpp::Polyline>& profile, double rotation = 0.0, const std::string& name = "column");

    /// A column from its solid, its axis and its section; the solid stays as given while the section is empty, else it is rebuilt from the section. `name` is the type flag face_contacts() filters on.
    Column(
        const session_cpp::Mesh& solid,
        const session_cpp::Line& axis,
        const session_cpp::Polyline& section,
        const std::string& name = "column"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The column an Element tagged "Column" describes, same guid; a missing payload leaves axis, section and cuts default.
    static std::shared_ptr<Column> from_element(session_cpp::Element element);

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

    /// One plane per face of the model solid with a Newell normal, so a concave cap (a W, a T) faces the right way for contact detection.
    std::vector<session_cpp::Plane> compute_planes() const override;

    /// Drops the cached solids and marks the Element slot stale; call after assigning the axis, the section or the cuts by hand.
    void invalidate_geometry() override;

    /// A copy moved by xform from the parameters alone, guid and name kept: axis, section, cuts, features and insertion vectors moved, no solid until one is asked for; nullptr for a mirror.
    std::shared_ptr<Column> transformed(const session_cpp::Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the axis, the section and the cuts, and drops the cached solids.
    void place(const session_cpp::Xform& xform) override;

protected:
    /// Writes the model geometry, the section lofted along the axis and cut by every plane in cuts, onto the Element in the requested form (the given solid stays when the section is empty) with the axis and section features, keeping the joint and contact features the session put there; WoodSession::pb_dump calls it for every stale column.
    void compute_geometry_mesh_impl() override;

    /// Write the model BRep and the element features into the session slot.
    void compute_geometry_brep_impl() override;

    /// Refresh the dimensions and geometry features while preserving session features.
    void compute_geometry_features();

public:

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, section and cuts as JSON: axis, section, cuts, type; the protobuf message printed; a payload in the kernel's JSON from older files is still read.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, section and cuts as wood_proto.Column bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the solid when there is one, else of the axis and the section, inflated on each side.
    session_cpp::AABB aabb(double inflate) const;

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override {
        return std::make_shared<Column>(*this);
    }

    /// Registers the "Column" factory with the kernel, so Session::pb_load rebuilds columns.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Column(name, axis_length, section_pts)".
    std::string str() const override;
};

} // namespace wood_session
