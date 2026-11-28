#pragma once
#include "src/templates/folding/folding_guide.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// ConicalFold
// ═══════════════════════════════════════════════════════════════════════════

/// A folded plate with conical folding.
class ConicalFold : public WoodSession {
public:
    const ConicalFoldGuide guide; // What it is built from; every element is in the session.

    /// The conical fold of the guide as the session named name.
    explicit ConicalFold(
        const ConicalFoldGuide& guide = ConicalFoldGuide(), const std::string& name = "conical_fold"
    );

    /// Not copied, as a session is not.
    ConicalFold(const ConicalFold&) = delete;

    /// Not assigned, as it is not copied.
    ConicalFold& operator=(const ConicalFold&) = delete;

    /// The folded mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

    /// The profile at x cut height high: a trapezoid per fold, the full zigzag at the depth.
    Polyline compute_profile(double x, double height) const;

    /// The mesh: per fold its two flank pentagons and its two cut triangles, every face facing -z.
    Mesh compute_mesh() const;

    /// The corners of a face, reversed when they face +z.
    static std::vector<Point> facing_down(std::vector<Point> corners);
};

}
