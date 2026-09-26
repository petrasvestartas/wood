#pragma once

#include "pch.h"

namespace wood_session {

/// A curved beam: a cross-section swept along a smooth central axis, turned at every station so its up direction follows a given vector, a lamella's surface normal; the solid is one BRep of smooth faces that kinks only along the section's corners and at the two ends.
class BeamCurved : public session_cpp::Element {
public:
    static constexpr std::string_view ELEMENT_TYPE = "BeamCurved"; // The element_type this beam is written under.
    session_cpp::NurbsCurve axis; // Central axis, a cubic interpolated through the stations, in world space.
    std::vector<double> parameters; // Axis parameter of every station.
    std::vector<session_cpp::Vector> directions; // Section up direction at every station.
    session_cpp::Polyline section; // Closed cross-section in the station frame: x across the axis, y up.

private:
    mutable std::optional<session_cpp::Mesh> _element_geometry_mesh; // Cache of the mesh form.
    mutable std::optional<session_cpp::BRep> _element_geometry_brep; // Cache of the brep form.

public:
    /// An empty beam: no axis, no section.
    BeamCurved();

    /// A beam through points, the up direction at each, and the section every station carries; the axis is the chord-length cubic through the points.
    BeamCurved(
        const std::vector<session_cpp::Point>& points,
        const std::vector<session_cpp::Vector>& directions,
        const session_cpp::Polyline& section,
        const std::string& name = "beam_curved"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The beam an Element tagged "BeamCurved" describes, same guid; a missing payload leaves it empty.
    static std::shared_ptr<BeamCurved> from_element(session_cpp::Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The station frame at axis parameter t: origin on the axis, x across it (up cross the tangent), y the up direction made normal to the tangent, z the tangent.
    session_cpp::Plane frame(double t, const session_cpp::Vector& up) const;

    /// The section placed at every station, counter-clockwise about the tangent.
    std::vector<session_cpp::Polyline> sections() const;

    /// One cubic rail per section corner through that corner of every station, all four sharing the axis parameters.
    std::vector<session_cpp::NurbsCurve> rails() const;

    /// The sweep through sections() as a Mesh, every vertex on a rail: what contacts and clash checks read.
    const session_cpp::Mesh& element_geometry_mesh() const override;

    /// The sweep as one closed BRep: a ruled face between each two neighbouring rails and a planar cap at each end.
    const session_cpp::BRep& element_geometry_brep() const override;

    /// One plane per face of the swept mesh with a Newell normal.
    std::vector<session_cpp::Plane> compute_planes() const override;

    /// Drops the cached solids and marks the Element slot stale; call after assigning the axis, the stations or the section by hand.
    void invalidate_geometry() override;

    /// Moves the solid, the features and the insertion vectors, then the axis and the directions, and drops the cached solids.
    void place(const session_cpp::Xform& xform) override;

protected:
    /// Writes the swept mesh onto the Element with the axis and section features, keeping the features the session put there.
    void compute_geometry_mesh_impl() override;

    /// Writes the swept BRep onto the Element with the axis and section features.
    void compute_geometry_brep_impl() override;

    /// Refresh the axis and section features while preserving session features.
    void compute_geometry_features();

public:

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, parameters, directions and section as wood_proto.BeamCurved bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<session_cpp::Element> clone() const override {
        return std::make_shared<BeamCurved>(*this);
    }

    /// Registers the "BeamCurved" factory with the kernel, so Session::pb_load rebuilds curved beams.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "BeamCurved(name, stations, section_pts)".
    std::string str() const override;
};

} // namespace wood_session
