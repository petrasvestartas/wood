#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// A column: a solid that knows its own axis, the section it is cut from and the planes that trim it.
class Column : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Column"; // The element_type this column is written under.
    Line axis; // Centreline, base to head, in world space.
    Polyline section; // Closed cross-section about the axis base; empty when unknown.
    std::vector<Plane> cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.
    std::vector<Polyline> profile; // Section loops in the profile frame the section was placed from, loop 0 the outline, then holes; empty when the section was given.
    double rotation = 0.0; // Degrees the profile x axis turns from world x about the axis.

protected:
    /// The stock trimmed by its cut planes as a Mesh, before the subtract features.
    Mesh trimmed_mesh() const override;

    /// The swept section trimmed by its cut planes as a BRep, when it has no solid features.
    BRep trimmed_brep() const override;

    /// The frame at the axis base, x along the section's first side and z along the axis: where the blocks glued to the column are united.
    Plane frame() const override;

public:
    /// An empty column: no solid, a zero-length axis, no section.
    Column();

    /// A column from its axis and its section: the solid is the section swept along the axis; `name` is the type flag face_contacts() filters on.
    Column(const Line& axis, const Polyline& section, const std::string& name = "column");

    /// A column from its axis and a profile placed at the axis base in the plane perpendicular to it, x along world x projected then turned by rotation degrees; holes make it hollow.
    Column(const Line& axis, const std::vector<Polyline>& profile, double rotation = 0.0, const std::string& name = "column");

    /// A column from its solid, its axis and its section; the solid stays as given while the section is empty, else it is rebuilt from the section. `name` is the type flag face_contacts() filters on.
    Column(
        const Mesh& solid,
        const Line& axis,
        const Polyline& section,
        const std::string& name = "column"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A square column on its axis: the square of side from the corner frame's origin along its x and y axes, at the axis base, swept along the axis.
    static std::shared_ptr<Column> square(const Line& axis, const Plane& corner, double side, const std::string& name = "column");

    /// The column an Element tagged "Column" describes, same guid; a missing payload leaves axis, section and cuts default.
    static std::shared_ptr<Column> from_element(Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The parametric shape alone, before joints or cuts as a Mesh; computed on first access and cached independently until invalidate_geometry() or place().
    const Mesh& element_geometry_mesh() const override;

    /// The parametric shape alone, before joints or cuts as a BRep; computed on first access and cached independently until invalidate_geometry() or place().
    const BRep& element_geometry_brep() const override;


    /// One plane per face of the model solid with a Newell normal, so a concave cap (a W, a T) faces the right way for contact detection.
    std::vector<Plane> compute_planes() const override;

    /// A copy moved by xform from the parameters alone, guid and name kept: axis, section, cuts, features and insertion vectors moved, no solid until one is asked for; nullptr for a mirror.
    std::shared_ptr<Column> transformed(const Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the axis, the section and the cuts, and drops the cached solids.
    void place(const Xform& xform) override;

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
    using Element::aabb;

    /// The box of the solid when there is one, else of the axis and the section, inflated on each side.
    AABB aabb(double inflate) const;

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
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
