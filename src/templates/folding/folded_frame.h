#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// FoldedFrame
// ═══════════════════════════════════════════════════════════════════════════

/// A frame or arch of triangular folded plates, half a bay one zigzag fold from the foot to the crown.
class FoldedFrame : public WoodSession {
public:
    const FoldedFrameGuide guide; // What it is built from; every element is in the session.

    /// The folded frame of the guide as the session named name.
    explicit FoldedFrame(
        const FoldedFrameGuide& guide = FoldedFrameGuide(), const std::string& name = "folded_frame"
    );

    /// Not copied, as a session is not.
    FoldedFrame(const FoldedFrame&) = delete;

    /// Not assigned, as it is not copied.
    FoldedFrame& operator=(const FoldedFrame&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

    /// The zigzag's triangles, every point with the last point on each rail.
    std::vector<std::array<Point, 3>> compute_triangles() const;

    /// The mesh of every bay: the zigzag's triangles mirrored over mid-span and the bay's centre, facing out.
    Mesh compute_mesh() const;
};

}
