#pragma once

#include "pch.h"
#include "wood_element_geometry.h"

using namespace session_cpp;

namespace wood_session {

/// A timber element: its shape built from its own parameters, its stock that shape with every block other elements glue on, and its model the stock trimmed by its own planes with every solid other elements take away. The solid features come only through WoodSession::add_interaction(source, element, InteractionFeatureSolid).
class WoodElement : public Element {
public:
    std::vector<InteractionFeatureSolid> solid_features; // The solid features other elements put on it, each in the element's frame naming its source: the adds make its stock, the subtracts cut its model.
    std::vector<InteractionFeaturePlane> plane_features; // The planes other elements cut it by, each in the element's frame naming its source, the element keeping the side the normal points to.

    using Element::Element;

    // ═══════════════════════════════════════════════════════════════════════════
    // Geometry
    // ═══════════════════════════════════════════════════════════════════════════

    /// The shape with every add feature united, in frame() on a 1e-6 mm grid so a glued block meets it on exactly one plane; the shape itself without add features.
    Mesh stock_mesh() const;

    /// The planes of the plane features.
    std::vector<Plane> feature_planes() const;

    /// The trimmed stock cut by the plane features, with every subtract feature applied as a Mesh; cached until invalidate_geometry() or place().
    const Mesh& model_geometry_mesh() const override;

    /// The trimmed stock cut by the plane features, with every subtract feature applied as a BRep, the drills exact cylinders; cached until invalidate_geometry() or place().
    const BRep& model_geometry_brep() const override;

    /// Drops the uncut and the cut solids and every other cache.
    void invalidate_geometry() override;

    /// The frame the add features are united in: the world xy plane unless the element has its own.
    virtual Plane frame() const;

protected:
    mutable std::optional<Mesh> _element_geometry_mesh; // Cache of the uncut mesh.
    mutable std::optional<BRep> _element_geometry_brep; // Cache of the uncut brep.

    /// The solid before the subtract features as a Mesh: the stock by default.
    virtual Mesh trimmed_mesh() const;

    /// The solid as a BRep when there are no solid features: the element alone by default.
    virtual BRep trimmed_brep() const;
};

} // namespace wood_session
