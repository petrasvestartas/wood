#include "src/templates/developable_folding/curved_miura.h"

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
CurvedMiura::CurvedMiura(const CurvedMiuraGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // ridge: the discrete ribbon along the curve (Eq. 6), its V ridge at the opening and the ridge developed flat
    const SpaceCurveRidge ridge(guide.curve, guide.width, guide.angle, guide.zigzag);

    // lengths: per arm the ruling at every vertex that keeps every row parallel to the crease, the width across at the middle
    const std::array<std::vector<double>, 2> lengths = compute_lengths(ridge.pattern());

    // pattern: the ridge's crease pattern repeated across, every other strip on one arm's rulings
    const QuadGrid pattern = compute_tessellation(ridge.pattern(), lengths);

    // stages: the ridge folded to every deployment's opening and repeated across the same way, the crease pattern fixed
    std::vector<QuadGrid> stages;
    for (const double deployment : guide.deployments)
        stages.push_back(compute_tessellation(ridge.folded(deployment * guide.angle), lengths));

    // checks: planarity, Kawasaki, the openings, the edge lengths and the crossings of every stage
    _measures = SpaceCurveRidge::measure(stages, pattern);

    // geometry: the curve and the ribbon, every stage's quads and creases side by side, the crease pattern under them
    SpaceCurveRidge::add_curve(*this, guide.curve, ridge);
    const std::vector<Vector> shifts = SpaceCurveRidge::add_quads(*this, stages, pattern);

    // materialization: the quads alone without a thickness
    if (guide.thickness == 0.0)
        return;

    // plates: every quad of every stage a mitered plate, the first stage's put in one by one
    SpaceCurveRidge::add_plates(*this, stages, shifts, guide.thickness);
}

// ═══════════════════════════════════════════════════════════════════════════
// Tessellation
// ═══════════════════════════════════════════════════════════════════════════
std::array<std::vector<double>, 2> CurvedMiura::compute_lengths(const QuadGrid& strip) const {

    const size_t count = strip[1].size();
    const size_t middle = (count - 1) / 2;
    std::array<std::vector<double>, 2> lengths;

    for (size_t arm = 0; arm < 2; arm++) {
        const std::function<double(size_t, size_t)> sine = [&](size_t vertex, size_t edge) {
            const Vector ruling = (strip[2 * arm][vertex] - strip[1][vertex]).normalized();
            const Vector tangent = (strip[1][edge + 1] - strip[1][edge]).normalized();
            return ruling.cross(tangent).magnitude();
        };

        // a quad's two rulings reach the same distance off its crease edge, the parallel row's
        std::vector<double> length(count);
        length[middle] = guide.width / sine(middle, middle);
        for (size_t i = middle; i + 1 < count; i++)
            length[i + 1] = length[i] * sine(i, i) / sine(i + 1, i);
        for (size_t i = middle; i > 0; i--)
            length[i - 1] = length[i] * sine(i, i - 1) / sine(i - 1, i - 1);
        lengths[arm] = length;
    }

    return lengths;
}

QuadGrid CurvedMiura::compute_tessellation(
    const QuadGrid& strip, const std::array<std::vector<double>, 2>& lengths
) const {

    const size_t count = strip[1].size();
    const std::array<size_t, 2> counts = {guide.strips / 2, guide.strips - guide.strips / 2};
    std::array<QuadGrid, 2> sides = {QuadGrid{strip[1]}, QuadGrid{strip[1]}};

    // per side: the first strip on its own arm's rulings, the next on the other arm's reversed, and so on
    for (size_t side = 0; side < 2; side++)
        for (size_t j = 0; j < counts[side]; j++) {
            const size_t arm = (side + j) % 2;
            const double sign = j % 2 == 0 ? 1.0 : -1.0;
            const std::vector<Point> last = sides[side].back();
            std::vector<Point> row;
            for (size_t i = 0; i < count; i++) {
                const Vector ruling = (strip[2 * arm][i] - strip[1][i]).normalized();
                row.push_back(last[i] + ruling * (sign * lengths[arm][i]));
            }
            sides[side].push_back(row);
        }

    // rows: arm a's side from its outermost in, the crease, arm b's side out
    QuadGrid rows(sides[0].rbegin(), sides[0].rend());
    rows.insert(rows.end(), sides[1].begin() + 1, sides[1].end());
    return rows;
}

}
