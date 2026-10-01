#pragma once

#include "pch.h"
#include "wood_element_geometry.h"

namespace wood_session {

/// A beam whose section changes along it: closed sections, one per station, lofted in order along a straight axis; a rib under a parabola or a beam between two slanted faces.
class BeamVariable : public session_cpp::Element {
public:
    std::vector<SolidCut> solid_cuts;
    static constexpr std::string_view ELEMENT_TYPE = "BeamVariable"; // The element_type this beam is written under.
    session_cpp::Line axis; // Straight reference line from the first section to the last; contact detection tells end faces from side faces by it.
    std::vector<session_cpp::Polyline> sections; // Closed rings with one point count, one per station in axis order.
    std::vector<session_cpp::Plane> cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.

private:
    mutable std::optional<session_cpp::Mesh> _element_geometry_mesh; // Cache of the mesh form.
    mutable std::optional<session_cpp::BRep> _element_geometry_brep; // Cache of the brep form.
    mutable std::optional<session_cpp::Mesh> _model_geometry_mesh; // Cache of the cut mesh form.
    mutable std::optional<session_cpp::BRep> _model_geometry_brep; // Cache of the cut brep form.

public:
    /// An empty beam: no axis, no sections.
    BeamVariable();

    /// A beam lofted through its sections along the axis; `name` is the type flag face_contacts() filters on.
    BeamVariable(const session_cpp::Line& axis, const std::vector<session_cpp::Polyline>& sections, const std::string& name = "beam_variable");

    // ═══════════════════════════════════════════════════════════════════════════
    // Static constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The beam an Element tagged "BeamVariable" describes, same guid; a missing payload leaves the axis and sections empty.
    static std::shared_ptr<BeamVariable> from_element(session_cpp::Element element);

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The parametric shape alone, before joints or cuts as a Mesh: one face per section cap and one per side strip whose quads share a plane, else a face per quad.
    const session_cpp::Mesh& element_geometry_mesh() const override;

    /// The parametric shape alone, before joints or cuts as a BRep of planar faces.
    const session_cpp::BRep& element_geometry_brep() const override;

    /// The shape with joints or cuts applied as a Mesh; computed on first access and cached independently until invalidate_geometry() or place().
    const session_cpp::Mesh& model_geometry_mesh() const override;

    /// The shape with joints or cuts applied as a BRep; computed on first access and cached independently until invalidate_geometry() or place().
    const session_cpp::BRep& model_geometry_brep() const override;

    /// One plane per face of the model solid with a Newell normal.
    std::vector<session_cpp::Plane> compute_planes() const override;

    /// Drops the cached solids and marks the Element slot stale; call after assigning the axis, sections or cuts by hand.
    void invalidate_geometry() override;

    /// A copy moved by xform from the parameters alone, guid and name kept; nullptr for a mirror.
    std::shared_ptr<BeamVariable> transformed(const session_cpp::Xform& xform) const;

    /// Moves the solid, the features and the insertion vectors, then the axis, sections and cuts, and drops the cached solids.
    void place(const session_cpp::Xform& xform) override;

protected:
    /// Writes the model mesh and the element features into the session slot; WoodSession::pb_dump calls it for every stale beam.
    void compute_geometry_mesh_impl() override;

    /// Writes the model BRep and the element features into the session slot.
    void compute_geometry_brep_impl() override;

    /// Refresh the geometry features while preserving session features.
    void compute_geometry_features();

public:
    /// The kernel's cached box of the solid.
    using session_cpp::Element::aabb;

    /// The box of the solid, inflated on each side; the sections' box when there is no mesh.
    session_cpp::AABB aabb(double inflate) const;

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
    std::shared_ptr<session_cpp::Element> clone() const override {
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
