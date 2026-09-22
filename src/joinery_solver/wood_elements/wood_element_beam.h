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

private:
    bool _geometry_synced = false; // True while the Element slot holds the solid of the current axis and radii.

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

    /// Marks the Element slot stale; call after assigning the axis, radii or directions by hand.
    void invalidate_geometry();

    /// True once compute_geometry() wrote the solid of the current axis onto the Element; false after any invalidation.
    bool geometry_synced() const { return _geometry_synced; }

    /// Writes the solid swept through sections() onto the Element with the axis and section features, keeping the joint features the session put there; WoodSession::pb_dump calls it for every stale beam.
    void compute_geometry();

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the axis widened by the largest radius, inflated on each side.
    session_cpp::AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, radii, directions and allowed type as JSON, with type; the protobuf message printed; a payload in the kernel's JSON from older files is still read.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, radii, directions and allowed type as wood_proto.Beam bytes: what the kernel carries in element_data.
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
