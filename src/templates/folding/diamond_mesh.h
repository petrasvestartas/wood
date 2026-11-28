#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// DiamondMesh
// ═══════════════════════════════════════════════════════════════════════════

/// A diamond shell: a NURBS surface split into rhombi of two triangles each, one plate per triangle.
class DiamondMesh : public WoodSession {
public:
    const DiamondMeshGuide guide; // What it is built from; every element is in the session.

    /// The diamond pattern of the guide as the session named name.
    explicit DiamondMesh(
        const DiamondMeshGuide& guide = DiamondMeshGuide(), const std::string& name = "diamond_mesh"
    );

    /// Not copied, as a session is not.
    DiamondMesh(const DiamondMesh&) = delete;

    /// Not assigned, as it is not copied.
    DiamondMesh& operator=(const DiamondMesh&) = delete;

    /// The welded triangle mesh, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The welded triangle mesh, one face per plate.

    /// Per cell four triangles around its centre to the next cell's centre, six on the first row, welded.
    Mesh compute_mesh() const;
};

}
