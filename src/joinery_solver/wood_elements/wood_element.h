#pragma once

#include "pch.h"
#include "wood_element_geometry.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents a timber member.
class WoodElement : public Element {
public:
    std::vector<InteractionFeatureSolid>
        solid_features; // The solid features other elements put on it.
    std::vector<InteractionFeaturePlane> plane_features; // The planes other elements cut it by.

    using Element::Element;

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// Returns the shape united with its add features on the frame grid.
    Mesh stock_mesh() const;

    /// The planes of the plane features.
    std::vector<Plane> feature_planes() const;

    /// The trimmed stock cut by the plane features, with every subtract feature applied as a Mesh; cached until invalidate_geometry() or place().
    const Mesh& model_geometry_mesh() const override;

    /// The trimmed stock cut by the plane features.
    const BRep& model_geometry_brep() const override;

    /// Drops the uncut and the cut solids and every other cache.
    void invalidate_geometry() override;

    /// The frame the add features are united in: the world xy plane unless the element has its own.
    virtual Plane frame() const;

    /// The plane the element is laid out on, drawn as an attribute in the viewer; none unless the element type has one.
    virtual std::optional<Plane> base_plane() const;

protected:
    mutable std::optional<Mesh> _element_geometry_mesh; // Cache of the uncut mesh.
    mutable std::optional<BRep> _element_geometry_brep; // Cache of the uncut brep.

    /// The solid before the subtract features as a Mesh: the stock by default.
    virtual Mesh trimmed_mesh() const;

    /// The solid as a BRep when there are no solid features: the element alone by default.
    virtual BRep trimmed_brep() const;
};

}
