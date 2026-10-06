#pragma once

#include "pch.h"
#include "wood_element_geometry.h"

using namespace session_cpp;

namespace wood_session {

/// A timber element: its solid built from its own parameters, then trimmed by its own shape and cut by the solid cuts the joints put on it. The uncut solid is cached here; the cut one in the kernel's model caches.
class WoodElement : public Element {
public:
    std::vector<InteractionFeatureSolid> solid_cuts; // Solids and drills the joints cut out of it, in the element's frame.

    using Element::Element;

    /// The trimmed solid with the solid cuts applied as a Mesh; cached until invalidate_geometry() or place().
    const Mesh& model_geometry_mesh() const override;

    /// The trimmed solid with the solid cuts applied as a BRep, the drills exact cylinders; cached until invalidate_geometry() or place().
    const BRep& model_geometry_brep() const override;

    /// Drops the uncut and the cut solids and every other cache.
    void invalidate_geometry() override;

protected:
    mutable std::optional<Mesh> _element_geometry_mesh; // Cache of the uncut mesh.
    mutable std::optional<BRep> _element_geometry_brep; // Cache of the uncut brep.

    /// The solid before the solid cuts as a Mesh: the element alone by default.
    virtual Mesh trimmed_mesh() const;

    /// The solid before the solid cuts as a BRep: the element alone by default.
    virtual BRep trimmed_brep() const;
};

}
