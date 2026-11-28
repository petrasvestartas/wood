#pragma once
#include "src/templates/developable_folding/space_curve_ridge.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// SpaceCurveFold
// ═══════════════════════════════════════════════════════════════════════════

/// One constant-angle curved fold along a space polyline (Tachi 2011): a V ridge of planar quads folding with one degree of freedom, then plates.
class SpaceCurveFold : public WoodSession {
public:
    const SpaceCurveFoldGuide guide; // The curve, width, opening, zig-zag, stages and thickness; every quad and plate is in the session.

    /// The fold of the guide as the session named name: its curve, ribbon, quads, creases and crease pattern, with a thickness its plates.
    explicit SpaceCurveFold(
        const SpaceCurveFoldGuide& guide = SpaceCurveFoldGuide(),
        const std::string& name = "space_curve_fold"
    );

    /// Not copied, as a session is not.
    SpaceCurveFold(const SpaceCurveFold&) = delete;

    /// Not assigned, as it is not copied.
    SpaceCurveFold& operator=(const SpaceCurveFold&) = delete;

    /// The checks over every stage: planarity, Kawasaki, the openings, the edge lengths and the crossings.
    const FoldMeasures& measures() const {
        return _measures;
    }

private:
    FoldMeasures _measures; // What every stage's quads measure against the crease pattern.
};

}
