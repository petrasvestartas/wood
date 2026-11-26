#pragma once
#include "src/templates/cylindrical_folding/cylindrical_folding_guide.h"
#include "src/templates/development.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CurvedFoldedBeam
// ═══════════════════════════════════════════════════════════════════════════

/// Closed curved-folded beams side by side.
class CurvedFoldedBeam : public WoodSession {
public:
    const CurvedFoldedBeamGuide
        guide; // The span and section the beams are built from; every element is in the session.

    /// The beams of the guide as the session named name.
    explicit CurvedFoldedBeam(
        const CurvedFoldedBeamGuide& guide = CurvedFoldedBeamGuide(),
        const std::string& name = "curved_folded_beam"
    );

    /// Not copied, as a session is not.
    CurvedFoldedBeam(const CurvedFoldedBeam&) = delete;

    /// Not assigned, as it is not copied.
    CurvedFoldedBeam& operator=(const CurvedFoldedBeam&) = delete;

    /// The largest relative length error of any plate's development, the proof every plate is cut flat.
    double distortion() const;

private:
    double _distortion = 0.0; // The largest relative length error of any plate's development.

    /// The corner at the top right sampled at equal x: the crease and the inner crease a thickness inside both plates.
    std::array<std::vector<Point>, 2> compute_corner() const;

    /// The circular arc through base at the supports and base + change at mid-span: its value and slope at x.
    std::pair<double, double> profile(double base, double change, double x) const;

    /// The point a thickness inside a profile at the arc's parameter u: its x and its distance from the axis.
    std::pair<double, double> inside_point(double base, double change, double u) const;

    /// The inner profile's distance from the axis at x, by bisection on the arc's parameter.
    double inner_distance(double base, double change, double x) const;

    /// Every beam, beam_<b> its group: its four plates and its webs.
    void add_beams(const std::array<std::vector<Point>, 2>& corner);

    /// Beam b's four plates, beam_<b>_<k>.
    void add_plates(
        const std::array<std::vector<Point>, 2>& corner,
        size_t b,
        const std::shared_ptr<TreeNode>& group
    );

    /// Beam b's webs, beam_<b>_<4 + w>: flat diaphragms at even stations, their corners on the inner crease.
    void add_webs(const NurbsCurve& inner_crease, size_t b, const std::shared_ptr<TreeNode>& group);
};

}
