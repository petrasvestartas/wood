#pragma once

#include "pch.h"
#include "wood_element_edge.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a beam whose section changes along it.
class EdgeVariableElement : public EdgeElement {
public:
    static constexpr std::string_view ELEMENT_TYPE =
        "EdgeVariableElement"; // The element_type this beam is written under.
    static constexpr std::string_view LEGACY_ELEMENT_TYPE =
        "BeamVariable"; // The element_type wood wrote before, still accepted on read.
    Line
        axis; // Straight reference line from the first section to the last; contact detection tells end faces from side faces by it.
    std::vector<Polyline>
        sections; // Closed rings with one point count, one per station in axis order.

    /// The axis start: z along the axis, x along the first section's first edge.
    std::optional<Plane> base_plane() const override;

    /// An empty beam: no axis, no sections.
    EdgeVariableElement();

    /// A beam lofted through its sections along the axis; `name` is the type flag face_contacts() filters on.
    EdgeVariableElement(
        const Line& axis,
        const std::vector<Polyline>& sections,
        const std::string& name = "beam_variable"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A straight beam between two end sections, its axis from the first section's centroid to the last's; the ends may be slanted.
    static std::shared_ptr<EdgeVariableElement>
    between(const Polyline& first, const Polyline& last, const std::string& name = "beam_variable");

    /// A straight beam of rectangular section, `length` along the horizontal `direction` from `start`.
    static std::shared_ptr<EdgeVariableElement>
    box(const Point& start,
        const Vector& direction,
        double length,
        double width,
        double height,
        const std::string& name = "beam");

    /// Two beams along x touching side by side: "left" on negative y, "right" on positive y.
    static std::array<std::shared_ptr<EdgeVariableElement>, 2>
    pair_side_by_side(double length = 1600.0, double width = 200.0, double height = 300.0);

    /// Two beams along x end to end: "first" ends where "second" starts.
    static std::array<std::shared_ptr<EdgeVariableElement>, 2>
    pair_end_to_end(double length = 1000.0, double width = 200.0, double height = 300.0);

    /// A "beam" along x and a "joist" as long, running along y from the middle of its side.
    static std::array<std::shared_ptr<EdgeVariableElement>, 2> pair_tee(
        double length = 600.0,
        double width = 120.0,
        double joist_width = 160.0,
        double height = 300.0
    );

    /// The beam an Element tagged "EdgeVariableElement" describes, same guid; a missing payload leaves the axis and sections empty.
    static std::shared_ptr<EdgeVariableElement> from_element(Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Outlines
    // ═══════════════════════════════════════════════════════════════════════════

    /// The top face's outline, as a plate's top.
    Polyline top() const;

    /// The bottom face's outline.
    Polyline bottom() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The parametric shape alone, before joints or cuts as a Mesh.
    const Mesh& element_geometry_mesh() const override;

    /// The parametric shape alone, before joints or cuts as a BRep of planar faces.
    const BRep& element_geometry_brep() const override;

    /// The axis.
    Line chord() const override {
        return axis;
    }

    /// A copy moved by xform from the parameters alone, guid and name kept; nullptr for a mirror.
    std::shared_ptr<EdgeVariableElement> transformed(const Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the axis, sections and cuts, and drops the cached solids.
    void place(const Xform& xform) override;

protected:
    /// Writes the model mesh and the element features into the session slot; WoodSession::pb_dump calls it for every stale beam.
    void compute_geometry_mesh_impl() override;

    /// Writes the model BRep and the element features into the session slot.
    void compute_geometry_brep_impl() override;

    /// The axis and every section as geometry features, the session features kept.
    void compute_geometry_features();

public:
    /// The kernel's cached box of the solid.
    using Element::aabb;

    /// The box of the solid, inflated on each side; the sections' box when there is no mesh.
    AABB aabb(double inflate) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // JSON
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, sections and cuts as JSON: the protobuf message printed.
    nlohmann::ordered_json element_data_jsondump() const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Protobuf
    // ═══════════════════════════════════════════════════════════════════════════

    /// The axis, sections and cuts as wood_proto.EdgeVariableElement bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<EdgeVariableElement>(*this);
    }

    /// Registers the "EdgeVariableElement" factory with the kernel, so Session::pb_load rebuilds these beams.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "EdgeVariableElement(name, sections, faces)".
    std::string str() const override;
};

}
