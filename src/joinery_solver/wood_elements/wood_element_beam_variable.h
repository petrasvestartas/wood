#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// A beam whose section changes along it: closed sections, one per station, lofted in order along a straight axis; a rib under a parabola or a beam between two slanted faces.
class BeamVariable : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE = "BeamVariable"; // The element_type this beam is written under.
    Line axis; // Straight reference line from the first section to the last; contact detection tells end faces from side faces by it.
    std::vector<Polyline> sections; // Closed rings with one point count, one per station in axis order.
    std::vector<Plane> cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.

protected:
    /// The solid trimmed by its cut planes as a Mesh, before the solid cuts.
    Mesh trimmed_mesh() const override;

    /// The solid trimmed by its cut planes as a BRep, before the solid cuts.
    BRep trimmed_brep() const override;

public:
    /// An empty beam: no axis, no sections.
    BeamVariable();

    /// A beam lofted through its sections along the axis; `name` is the type flag face_contacts() filters on.
    BeamVariable(const Line& axis, const std::vector<Polyline>& sections, const std::string& name = "beam_variable");

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A straight beam between two end sections, its axis from the first section's centroid to the last's; the ends may be slanted.
    static std::shared_ptr<BeamVariable> between(const Polyline& first, const Polyline& last, const std::string& name = "beam_variable");

    /// The beam an Element tagged "BeamVariable" describes, same guid; a missing payload leaves the axis and sections empty.
    static std::shared_ptr<BeamVariable> from_element(Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The parametric shape alone, before joints or cuts as a Mesh: one face per section cap and one per side strip whose quads share a plane, else a face per quad.
    const Mesh& element_geometry_mesh() const override;

    /// The parametric shape alone, before joints or cuts as a BRep of planar faces.
    const BRep& element_geometry_brep() const override;



    /// One plane per face of the model solid with a Newell normal.
    std::vector<Plane> compute_planes() const override;


    /// A copy moved by xform from the parameters alone, guid and name kept; nullptr for a mirror.
    std::shared_ptr<BeamVariable> transformed(const Xform& xform) const;

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

    /// The axis, sections and cuts as wood_proto.BeamVariable bytes: what the kernel carries in element_data.
    std::string element_data_dumps() const override;

    /// ELEMENT_TYPE, the tag the kernel writes and the registry reads.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<BeamVariable>(*this);
    }

    /// Registers the "BeamVariable" factory with the kernel, so Session::pb_load rebuilds these beams.
    static void register_type();

    // ═══════════════════════════════════════════════════════════════════════════
    // String
    // ═══════════════════════════════════════════════════════════════════════════

    /// "BeamVariable(name, sections, faces)".
    std::string str() const override;
};

} // namespace wood_session
