#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a timber beam.
class Beam : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE =
        "Beam"; // The element_type this beam is written under.
    Polyline axis; // Centreline, one segment per span, in world space.
    std::vector<double>
        radii; // Section half-width per segment; a segment without one takes part in no joint.
    std::vector<Vector>
        directions; // Section up direction per segment; a segment without one takes the contact normal.
    int allowed_type =
        -1; // Which contacts this beam accepts: 0 crossing only, 1 side-to-end and end-to-end, -1 any.
    std::vector<Plane>
        cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.
    double extension =
        0.0; // Length the solid runs past each axis end: the stock an end joint at the axis end cuts back.
    std::vector<Polyline> profile; // Section loops in the profile frame.

protected:
    /// The solid trimmed by its cut planes as a Mesh, before the solid cuts.
    Mesh trimmed_mesh() const override;

    /// The solid trimmed by its cut planes as a BRep, before the solid cuts.
    BRep trimmed_brep() const override;

public:
    /// The axis start: z along the first span, x along the first section's first edge.
    std::optional<Plane> base_plane() const override;

    /// An empty beam: no axis, no radius.
    Beam();

    /// A beam of one radius along its whole axis; `name` is the type flag face_contacts() filters on.
    Beam(const Polyline& axis, double radius, const std::string& name = "beam");

    /// A beam with a radius and, optionally, an up direction per segment.
    Beam(
        const Polyline& axis,
        const std::vector<double>& radii,
        const std::vector<Vector>& directions,
        int allowed_type = -1,
        const std::string& name = "beam"
    );

    /// A beam of one profile along its whole axis, the radii its half-width; a direction per segment when given.
    Beam(
        const Polyline& axis,
        const std::vector<Polyline>& profile,
        const std::vector<Vector>& directions = {},
        const std::string& name = "beam"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// Two beams crossing at their middles, "first" along x and "second" turned `angle_degrees` from it in plan.
    static std::array<std::shared_ptr<Beam>, 2>
    pair_crossing(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);

    /// Two beams in a tee, "first" along x through the origin and "second" ending on its axis, turned `angle_degrees` from it in plan, each `length` long and `radius` in half-width, the second's solid running one radius past its axis ends, to the first's far face, the stock its through tenon is kept from: the side-to-end pair a top-side joint (top_side) is cut into.
    static std::array<std::shared_ptr<Beam>, 2>
    pair_tee(double angle_degrees = 90.0, double radius = 75.0, double length = 1200.0);

    /// Creates two beams meeting end to end at the specified angle.
    static std::array<std::shared_ptr<Beam>, 2> pair_end_to_end(
        double angle_degrees = 0.0,
        double radius = 75.0,
        double length = 1200.0,
        double extension = 150.0
    );

    /// The beam an Element tagged "Beam" describes, same guid; a missing payload leaves the axis empty.
    static std::shared_ptr<Beam> from_element(Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The radius of one segment, 0 when the beam has none for it.
    double radius(int segment) const;

    /// True when the beam carries an up direction for that segment.
    bool has_direction(int segment) const;

    /// One closed outline per axis vertex.
    std::vector<Polyline> sections() const;

    /// The parametric shape alone, before joints or cuts as a Mesh; computed on first access and cached independently until invalidate_geometry() or place().
    const Mesh& element_geometry_mesh() const override;

    /// The parametric shape alone, before joints or cuts as a BRep; computed on first access and cached independently until invalidate_geometry() or place().
    const BRep& element_geometry_brep() const override;

    /// One plane per face of the model solid with a Newell normal, so a concave cap (a W.
    std::vector<Plane> compute_planes() const override;

    /// A copy moved by xform from the parameters alone, guid and name kept.
    std::shared_ptr<Beam> transformed(const Xform& xform) const;

    /// Moves the solid.
    void place(const Xform& xform) override;

protected:
    /// Writes the model geometry.
    void compute_geometry_mesh_impl() override;

    /// Write the model BRep and the element features into the session slot.
    void compute_geometry_brep_impl() override;

    /// Refresh the dimensions and geometry features while preserving session features.
    void compute_geometry_features();

public:
    /// The kernel's cached box of the solid.
    using Element::aabb;

    /// The box of the axis widened by the largest radius or half the profile's larger side, inflated on each side.
    AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, radii, directions, allowed type and cuts as JSON.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, radii, directions, allowed type and cuts as wood_proto.Beam bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<Beam>(*this);
    }

    /// Registers the "Beam" factory with the kernel, so Session::pb_load rebuilds beams.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "Beam(name, segments, radii)".
    std::string str() const override;
};

}
