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

private:
    mutable std::optional<session_cpp::ElementGeometry> _element_geometry_mesh; // Cache of the mesh form.
    mutable std::optional<session_cpp::ElementGeometry> _element_geometry_brep; // Cache of the brep form.
    mutable std::optional<session_cpp::ElementGeometry> _model_geometry_mesh; // Cache of the cut mesh form.
    mutable std::optional<session_cpp::ElementGeometry> _model_geometry_brep; // Cache of the cut brep form.

public:
    /// An empty column: no solid, a zero-length axis, no section.
    Column();

    /// A column from its axis and its section: the solid is the section swept along the axis; `name` is the type flag face_contacts() filters on.
    Column(const session_cpp::Line& axis, const session_cpp::Polyline& section, const std::string& name = "column");

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
    static std::shared_ptr<Column> from_element(const session_cpp::Element& element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The parametric shape alone, the section lofted along the axis, never cut; a mesh when true, a BRep when false; cached per form until invalidate_geometry().
    const session_cpp::ElementGeometry& element_geometry(bool mesh_or_brep = true) const;

    /// The shape cut by every plane in cuts, the element geometry while there are none; a mesh when true, a BRep when false; cached per form until invalidate_geometry().
    const session_cpp::ElementGeometry& model_geometry(bool mesh_or_brep = true) const;

    /// The loft of the section along the axis as a mesh or as faces, empty without a section of three points and a positive length.
    session_cpp::ElementGeometry compute_element_geometry(bool mesh_or_brep) const;

    /// The element geometry cut by every plane in cuts, as a mesh or as faces.
    session_cpp::ElementGeometry compute_model_geometry(bool mesh_or_brep) const;

    /// Drops the cached solids and marks the Element slot stale; call after assigning the axis, the section or the cuts by hand.
    void invalidate_geometry() override;

    /// A copy moved by xform from the parameters alone, guid and name kept: axis, section, cuts, features and insertion vectors moved, no solid until one is asked for; nullptr for a mirror.
    std::shared_ptr<Column> transformed(const session_cpp::Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the axis, the section and the cuts, and drops the cached solids.
    void place(const session_cpp::Xform& xform) override;

protected:
    /// Writes the model geometry, the section lofted along the axis and cut by every plane in cuts, onto the Element in the requested form (the given solid stays when the section is empty) with the axis and section features, keeping the joint and contact features the session put there; WoodSession::pb_dump calls it for every stale column.
    void compute_geometry_impl(bool mesh_or_brep) override;

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
    std::string element_type_name() const override { return std::string(ELEMENT_TYPE); }

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the solid when there is one, else of the axis and the section, inflated on each side.
    session_cpp::AABB aabb(double inflate) const;

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Column>(*this); }

    /// Registers the "Column" factory with the kernel, so Session::pb_load rebuilds columns.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Column(name, axis_length, section_pts)".
    std::string str() const override;
};

} // namespace wood_session
