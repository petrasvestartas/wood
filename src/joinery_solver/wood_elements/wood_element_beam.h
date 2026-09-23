#pragma once

#include "pch.h"

namespace wood_session {


/// A timber beam: a polyline axis with a square section of one radius per segment, joined to other beams where their axes come within reach; WoodSession::compute_axis_contacts and compute_beam_features find the pairs and cut four volume rectangles at each.
class Beam : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Beam"; // The element_type this beam is written under.
    session_cpp::Polyline axis; // Centreline, one segment per span, in world space.
    std::vector<double> radii; // Section half-width per segment; a segment without one takes part in no joint.
    std::vector<session_cpp::Vector> directions; // Section up direction per segment; a segment without one takes the contact normal.
    int allowed_type = -1; // Which contacts this beam accepts: 0 crossing only, 1 side-to-end and end-to-end, -1 any.
    std::vector<session_cpp::Plane> cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.

private:
    mutable std::optional<session_cpp::ElementGeometry> _element_geometry_mesh; // Cache of the mesh form.
    mutable std::optional<session_cpp::ElementGeometry> _element_geometry_brep; // Cache of the brep form.
    mutable std::optional<session_cpp::ElementGeometry> _model_geometry_mesh; // Cache of the cut mesh form.
    mutable std::optional<session_cpp::ElementGeometry> _model_geometry_brep; // Cache of the cut brep form.

public:
    /// An empty beam: no axis, no radius.
    Beam();

    /// A beam of one radius along its whole axis; `name` is the type flag face_contacts() filters on.
    Beam(const session_cpp::Polyline& axis, double radius, const std::string& name = "beam");

    /// A beam with a radius and, optionally, an up direction per segment.
    Beam(
        const session_cpp::Polyline& axis,
        const std::vector<double>& radii,
        const std::vector<session_cpp::Vector>& directions,
        int allowed_type = -1,
        const std::string& name = "beam"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The beam an Element tagged "Beam" describes, same guid; a missing payload leaves the axis empty.
    static std::shared_ptr<Beam> from_element(const session_cpp::Element& element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The radius of one segment, 0 when the beam has none for it.
    double radius(int segment) const;

    /// True when the beam carries an up direction for that segment.
    bool has_direction(int segment) const;

    /// One closed square per axis vertex, half-width the segment's radius, along the bisector at an interior vertex; empty when the beam has no radius.
    std::vector<session_cpp::Polyline> sections() const;

    /// The parametric shape alone, the solid swept through sections(), never cut; a mesh when true, a BRep when false; cached per form until invalidate_geometry().
    const session_cpp::ElementGeometry& element_geometry(bool mesh_or_brep = true) const;

    /// The shape cut by every plane in cuts, the element geometry while there are none; a mesh when true, a BRep when false; cached per form until invalidate_geometry().
    const session_cpp::ElementGeometry& model_geometry(bool mesh_or_brep = true) const;

    /// The sweep through sections() as a mesh or as faces, empty when the beam has no radius.
    session_cpp::ElementGeometry compute_element_geometry(bool mesh_or_brep) const;

    /// The element geometry cut by every plane in cuts, as a mesh or as faces.
    session_cpp::ElementGeometry compute_model_geometry(bool mesh_or_brep) const;

    /// Drops the cached solids and marks the Element slot stale; call after assigning the axis, radii, directions or cuts by hand.
    void invalidate_geometry() override;

    /// A copy moved by xform from the parameters alone, guid and name kept: axis, directions, cuts, features and insertion vectors moved, a segment without a direction given xform·z when xform tilts z; no solid until one is asked for; nullptr for a mirror.
    std::shared_ptr<Beam> transformed(const session_cpp::Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the axis, the directions (filled as transformed() does) and the cuts, and drops the cached solids.
    void place(const session_cpp::Xform& xform) override;

protected:
    /// Writes the model geometry, the sweep through sections() cut by every plane in cuts, onto the Element in the requested form with the axis and section features, keeping the joint and contact features the session put there; WoodSession::pb_dump calls it for every stale beam.
    void compute_geometry_impl(bool mesh_or_brep) override;

public:

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the axis widened by the largest radius, inflated on each side.
    session_cpp::AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, radii, directions, allowed type and cuts as JSON, with type; the protobuf message printed; a payload in the kernel's JSON from older files is still read.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, radii, directions, allowed type and cuts as wood_proto.Beam bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override { return std::string(ELEMENT_TYPE); }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override { return std::make_shared<Beam>(*this); }

    /// Registers the "Beam" factory with the kernel, so Session::pb_load rebuilds beams.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Beam(name, segments, radii)".
    std::string str() const override;
};

} // namespace wood_session
