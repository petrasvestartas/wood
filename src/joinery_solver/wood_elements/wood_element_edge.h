#pragma once

#include "pch.h"
#include "wood_element.h"

using namespace session_cpp;

namespace wood_session {

/// Element that represents an edge: a timber member along an axis, straight, curved or of varying section.
class EdgeElement : public WoodElement {
public:
    std::vector<Plane>
        cuts; // Planes the solid is cut by, each keeping the side its normal points to; call invalidate_geometry() after assigning.

    using WoodElement::WoodElement;

    /// One plane per face of the model solid with a Newell normal.
    std::vector<Plane> compute_planes() const override;

    /// The straight line from its first to its last point; zero length for an edge without an axis.
    virtual Line chord() const = 0;

protected:
    /// The solid trimmed by its cut planes as a Mesh, before the solid cuts.
    Mesh trimmed_mesh() const override;

    /// The solid trimmed by its cut planes as a BRep, before the solid cuts.
    BRep trimmed_brep() const override;
};

}
