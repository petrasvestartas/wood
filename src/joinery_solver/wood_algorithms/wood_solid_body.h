#pragma once

#include "pch.h"
#include "aabb.h"
#include "mesh.h"
#include "xform.h"

namespace manifold {
class Manifold;
}

using namespace session_cpp;

namespace wood_session {

/// The wood two solids share: its volume and its mean thickness, twice the volume over its surface.
struct SharedWood {
    double volume = 0.0;
    double thickness = 0.0;
};

/// A closed solid converted once, so it can be placed and intersected again and again.
class SolidBody {
public:
    /// The solid of mesh; empty when the mesh is not closed.
    explicit SolidBody(const Mesh& mesh);

    /// Whether the mesh was closed.
    bool is_solid() const;

    /// The bounds where the mesh stands.
    const AABB& box() const;

    /// The bounds of the solid placed by placed.
    AABB box_at(const Xform& placed) const;

    /// The wood this solid placed by placed shares with other placed by other_placed.
    SharedWood shared_with(const SolidBody& other, const Xform& placed, const Xform& other_placed = Xform()) const;

private:
    std::shared_ptr<const manifold::Manifold> solid;
    AABB bounds;
};

}
