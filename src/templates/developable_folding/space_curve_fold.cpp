#include "src/templates/developable_folding/space_curve_fold.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
SpaceCurveFold::SpaceCurveFold(const SpaceCurveFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // ridge: the discrete ribbon along the curve (Eq. 6), its V ridge at the opening and the ridge developed flat
    const SpaceCurveRidge ridge(guide.curve, guide.width, guide.angle, guide.zigzag);

    // stages: the crease pattern folded to every deployment's opening, the rulings and the pattern fixed
    std::vector<QuadGrid> stages;
    for (const double deployment : guide.deployments)
        stages.push_back(ridge.folded(deployment * guide.angle));

    // checks: planarity, Kawasaki, the openings, the edge lengths and the crossings of every stage
    _measures = SpaceCurveRidge::measure(stages, ridge.pattern());

    // geometry: the curve and the ribbon, every stage's quads and creases side by side, the crease pattern under them
    SpaceCurveRidge::add_curve(*this, guide.curve, ridge);
    const std::vector<Vector> shifts = SpaceCurveRidge::add_quads(*this, stages, ridge.pattern());

    // materialization: the quads alone without a thickness
    if (guide.thickness == 0.0)
        return;

    // plates: every quad of every stage a mitered plate, the first stage's put in one by one
    SpaceCurveRidge::add_plates(*this, stages, shifts, guide.thickness);
}

}
