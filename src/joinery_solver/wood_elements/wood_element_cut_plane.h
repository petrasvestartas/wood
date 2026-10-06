#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// A cutting plane as an element: drawn as a square of size on its plane, it cuts other elements by its feature, add_interaction(cut_plane, element, cut_plane->feature()), the element keeping the side the normal points to.
class CutPlane : public WoodElement {
public:
    static constexpr std::string_view ELEMENT_TYPE = "CutPlane"; // The element_type this plane is written under.
    Plane plane; // The cutting plane; what it cuts keeps the side its normal points to.
    double size = 1000.0; // The side of the square it is drawn as.

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// A cutting plane drawn as a square of size.
    explicit CutPlane(const Plane& plane, double size = 1000.0, const std::string& name = "cut_plane");

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The plane feature it cuts another element by.
    std::shared_ptr<InteractionFeaturePlane> feature() const;

    /// The square on the plane, one face.
    const Mesh& element_geometry_mesh() const override;

    /// The square on the plane, one face.
    const BRep& element_geometry_brep() const override;

    /// The square: a plane is not cut.
    const Mesh& model_geometry_mesh() const override;

    /// The square: a plane is not cut.
    const BRep& model_geometry_brep() const override;

    /// ELEMENT_TYPE.
    std::string element_type_name() const override {
        return std::string(ELEMENT_TYPE);
    }

    /// A copy with a fresh guid, the polymorphic copy a Session makes.
    std::shared_ptr<Element> clone() const override {
        return std::make_shared<CutPlane>(*this);
    }

protected:
    void compute_geometry_mesh_impl() override;
    void compute_geometry_brep_impl() override;
};

} // namespace wood_session
