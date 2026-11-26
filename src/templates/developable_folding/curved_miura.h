#pragma once
#include "src/templates/developable_folding/space_curve_ridge.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CurvedMiura
// ═══════════════════════════════════════════════════════════════════════════

/// Generalized Miura-ori bent and twisted along a space polyline (Tachi 2011): the V ridge repeated across in strips, one degree of freedom, then plates.
class CurvedMiura : public WoodSession {
public:
    const CurvedMiuraGuide guide; // The curve, width, opening, zig-zag, strips, stages and thickness; every quad and plate is in the session.

    /// The tessellation of the guide as the session named name: its curve, ribbon, quads, creases and crease pattern, with a thickness its plates.
    explicit CurvedMiura(
        const CurvedMiuraGuide& guide = CurvedMiuraGuide(), const std::string& name = "curved_miura"
    );

    /// Not copied, as a session is not.
    CurvedMiura(const CurvedMiura&) = delete;

    /// Not assigned, as it is not copied.
    CurvedMiura& operator=(const CurvedMiura&) = delete;

    /// The checks over every stage: planarity, Kawasaki, the openings, the edge lengths and the crossings.
    const FoldMeasures& measures() const {
        return _measures;
    }

private:
    FoldMeasures _measures; // What every stage's quads measure against the crease pattern.

    /// Per arm of strip the length of its ruling at every vertex that keeps a row along it parallel to the crease, the width across at the middle.
    std::array<std::vector<double>, 2> compute_lengths(const QuadGrid& strip) const;

    /// Rows from strip's crease out to both sides, a strip on its own arm's rulings, the next on the other arm's reversed, by lengths.
    QuadGrid
    compute_tessellation(const QuadGrid& strip, const std::array<std::vector<double>, 2>& lengths) const;
};

}
