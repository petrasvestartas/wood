#pragma once
#include "src/templates/cylindrical_folding/cylindrical_folding_guide.h"
#include "src/templates/development.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_cylindrical_folding {

// ═══════════════════════════════════════════════════════════════════════════
// CornerFan
// ═══════════════════════════════════════════════════════════════════════════

/// A fan of developable strips folded from one bottom corner of a square on parabolic creases to its two far sides.
class CornerFan : public WoodSession {
public:
    const CornerFanGuide
        guide; // The square and folds the fan is built from; every element is in the session.

    /// The fan of the guide as the session named name.
    explicit CornerFan(
        const CornerFanGuide& guide = CornerFanGuide(), const std::string& name = "corner_fan"
    );

    /// Not copied, as a session is not.
    CornerFan(const CornerFan&) = delete;

    /// Not assigned, as it is not copied.
    CornerFan& operator=(const CornerFan&) = delete;

    /// The largest relative length error of any strip's development, 0 for an exact cylinder.
    double distortion() const;

private:
    double _distortion = 0.0; // The largest relative length error of any strip's development.

    /// Every fold's top on the square's two far sides, equally spaced, high and low in turn.
    std::vector<Point> compute_tops() const;

    /// Every fold's parabola from three points, the corner, its lifted middle and its top, cut at foot.
    std::vector<NurbsCurve> compute_creases(const std::vector<Point>& tops) const;

    /// Every fold's inner crease, from its foot to its top end, where the two strips' inner faces meet on their bisector.
    std::vector<NurbsCurve> compute_inners(const std::vector<Point>& tops) const;

    /// The inner point of fold k at s: a thickness under both strips on their bisector, square under the one at a free fold.
    Point inner_point(const std::vector<Point>& tops, size_t k, double s) const;

    /// The s near start where fold k's inner point lies on the plane, by bisection.
    double
    inner_on(const std::vector<Point>& tops, size_t k, const Plane& plane, double start) const;

    /// Every strip, strip_<k> in the group strips, between two folds' creases and inner creases.
    void add_strips(const std::vector<NurbsCurve>& creases, const std::vector<NurbsCurve>& inners);

    /// The fold's parabola at s, from the corner at 0 to its top at 1.
    Point crease_point(const Point& top, double s) const;

    /// The fold's parabola tangent at s.
    Vector crease_tangent(const Point& top, double s) const;

    /// The unit normal under the strip from top to next at the fold through top, at s.
    static Vector strip_normal(const Vector& tangent, const Point& top, const Point& next);
};

}
