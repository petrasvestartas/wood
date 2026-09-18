#pragma once

#include "pch.h"

namespace wood_session {

class WoodSession;

/// A timber beam: a polyline axis with a square section of one radius per segment, joined to other beams where their axes come within reach. Its joint volumes are the four rectangles Beam::joint_volumes cuts at every contact.
class Beam : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "Beam"; // The element_type this beam is written under.
    session_cpp::Polyline axis; // Centreline, one segment per span, in world space.
    std::vector<double> radii; // Section half-width per segment; a segment without one takes part in no joint.
    std::vector<session_cpp::Vector> directions; // Section up direction per segment; a segment without one takes the contact normal.
    int allowed_type = -1; // Which contacts this beam accepts: 0 crossing only, 1 side-to-end and end-to-end, -1 any.

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

    /// Every axis contact among the beams within `min_distance` as four joint volume rectangles of `volume_length`, in a WoodSession that holds the axes under "BeamAxes" and the rectangles under "JointVolumes"; `cross_or_side_to_end` is the parameter that separates a crossing from an end contact, `flip_male` rotates the male rectangle corners.
    static WoodSession joint_volumes(
        const std::vector<std::shared_ptr<Beam>>& beams,
        double min_distance,
        double volume_length,
        double cross_or_side_to_end,
        int flip_male
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The radius of one segment, 0 when the beam has none for it.
    double radius(int segment) const;

    /// True when the beam carries an up direction for that segment.
    bool has_direction(int segment) const;

    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the axis widened by the largest radius, inflated on each side.
    session_cpp::AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, radii, directions and allowed type as JSON, with type.
    std::string element_data_dumps() const override;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

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
