#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a curved plate: the strip between two meridians, a solid closed by six exact NURBS patches, its outer face the ruled surface between the two outer meridians, its inner face the one between the two inner meridians, a side face along each fold ruled between the outer and the inner meridian there, and a cap at each end between the rulings. Two plates built on the same meridians share their side face as mitered planar plates do; the solid is a BRep, never a mesh, and its mesh is tessellated from it.
class PlateCurved : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE = "PlateCurved"; // The element_type this plate is written under.
    NurbsCurve outer_a; // The first meridian on the outer face.
    NurbsCurve outer_b; // The second meridian on the outer face, running the same way.
    NurbsCurve inner_a; // The first meridian on the inner face, under outer_a.
    NurbsCurve inner_b; // The second meridian on the inner face, under outer_b.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// The curved plate between the outer meridians outer_a and outer_b and the inner meridians inner_a and inner_b, all four running the same way.
    PlateCurved(
        const NurbsCurve& outer_a,
        const NurbsCurve& outer_b,
        const NurbsCurve& inner_a,
        const NurbsCurve& inner_b,
        const std::string& name = "plate_curved"
    );

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The solid tessellated from its BRep; computed on first access and cached until invalidate_geometry() or place().
    const Mesh& element_geometry_mesh() const override;

    /// The closed solid of six patches: outer, inner, the two sides and the two caps, every edge shared by two faces; computed on first access and cached until invalidate_geometry() or place().
    const BRep& element_geometry_brep() const override;

    /// Moves the four meridians and drops every cached solid.
    void place(const Xform& xform) override;

    /// ELEMENT_TYPE.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<PlateCurved>(*this);
    }

protected:
    void compute_geometry_mesh_impl() override;
    void compute_geometry_brep_impl() override;

private:
    /// The six patches, outer, inner, side a, side b, start cap and end cap, each ruled between two of the plate's boundary curves so neighbouring patches meet on the same curve.
    std::array<NurbsSurface, 6> compute_patches() const;

    /// The closed BRep of the patches: its corners the vertices, its meridians and rulings the edges, each patch a face on its whole domain wound outwards.
    BRep compute_solid(const std::array<NurbsSurface, 6>& patches) const;
};

} // namespace wood_session
