#pragma once
#include "src/templates/developable_folding/developable_folding_guide.h"
#include "src/templates/development.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CurvedFold
// ═══════════════════════════════════════════════════════════════════════════

/// A curved folded tent roof: fold strips between NURBS ridges and valleys rising into a tower, with a thickness one curved lamella each.
class CurvedFold : public WoodSession {
public:
    const CurvedFoldGuide
        guide; // The roof's sizes, folds and thickness; every strip, lamella and piece is in the session.

    /// The roof of the guide as the session named name: its meridians, strips and pattern, with a thickness its lamellas and flat pieces.
    explicit CurvedFold(
        const CurvedFoldGuide& guide = CurvedFoldGuide(), const std::string& name = "curved_fold"
    );

    /// Not copied, as a session is not.
    CurvedFold(const CurvedFold&) = delete;

    /// Not assigned, as it is not copied.
    CurvedFold& operator=(const CurvedFold&) = delete;

    /// Largest relative length error of any strip's flat development.
    double distortion() const {
        return _distortion;
    }

private:
    double _distortion = 0.0; // Largest relative diagonal error of any strip's flat development.

    /// Per ring from the eaves up its star of ridge and valley points on the trumpet curve, the outer surface.
    std::vector<std::vector<Point>> compute_rings() const;

    /// The same points on the lamellas' inner faces, where the two strips beside each, offset by the thickness, meet.
    std::vector<std::vector<Point>>
    compute_inner_rings(const std::vector<std::vector<Point>>& stars) const;

    /// Every ridge and valley a NURBS curve through the rings, drawn in the group meridians.
    std::vector<NurbsCurve> add_meridians(const std::vector<std::vector<Point>>& stars);

    /// Adds every fold strip, the ruled surface between two meridians, as a quad mesh and returns the box around them.
    AABB add_strips(const std::vector<NurbsCurve>& meridians);

    /// Adds every fold strip as lamella_<j>: one curved plate between two meridians, its inner face between the inner ones.
    void add_lamellas(
        const std::vector<NurbsCurve>& meridians, const std::vector<std::vector<Point>>& inner_stars
    );

    /// Adds every plate unrolled into its flat piece on a cutting sheet, group flat.
    void add_flat();
};

}
