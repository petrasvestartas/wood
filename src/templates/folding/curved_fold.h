#pragma once
#include "wood_session.h"

using namespace session_cpp;
using namespace wood_session;

/// A curved folded tent roof after Knie's Zauberhut, Rapperswil (Blumer Lehmann, Carlos Martinez Architekten, 2020): a roof of 32 m
/// diameter rising in a trumpet curve into a tower 26 m high, folded into sectors whose valleys run down to the columns and whose
/// ridges end in a gable over the eaves, the folds shrinking into the tower. Every ridge and valley is a smooth NURBS curve, a meridian
/// interpolated through the rings, each ring the same star of ridge and valley points scaled by the trumpet curve's radius and lifted
/// to its height; every fold strip between two neighbouring meridians is one lamella, a PlateCurved: a closed curved solid whose outer
/// face is the ruled NURBS surface between the two meridians, whose inner face is the one between the inner meridians, where the two
/// neighbouring strips offset by the thickness meet, whose side faces run along the folds, shared with the neighbour as a miter, and
/// whose caps close it at the eaves and at the top. No facet, no triangle: the lamellas are curved, as the 5 cm sheathing on the
/// uniaxially curved glulam of the built roof.
///
/// Fields: the plan, the curve, the fold and the thickness; the meridians and the lamellas are in the session.
class CurvedFold : public WoodSession {
public:
    const size_t sectors; // Folds around the axis, a ridge and a valley each.
    const double radius; // mm, the eaves' radius at the ridges.
    const double top_radius; // mm, the tower's radius at the top.
    const double eave; // mm, the valleys' height at the eaves, on the columns.
    const double height; // mm, the tower's top.
    const double gable; // mm, how far a ridge rises over its valleys at the eaves, the fold's depth, scaling with the radius up the tower.
    const double inset; // The valleys' radius inside the ridges', a fraction of the radius.
    const size_t rings; // Rings from the eaves to the top the meridians are interpolated through.
    const double thickness; // mm, every lamella.

    /// The curved fold as the session named name.
    explicit CurvedFold(
        size_t sectors = 12,
        double radius = 16000.0,
        double top_radius = 3500.0,
        double eave = 4000.0,
        double height = 26000.0,
        double gable = 4000.0,
        double inset = 0.1,
        size_t rings = 14,
        double thickness = 120.0,
        const std::string& name = "curved_fold"
    );

    /// Not copied, as a session is not.
    CurvedFold(const CurvedFold&) = delete;

    /// Not assigned, as it is not copied.
    CurvedFold& operator=(const CurvedFold&) = delete;

private:
    /// Per ring from the eaves up its star of 2 sectors points, a ridge and a valley in turn: the trumpet curve's radius, flat at the
    /// eaves and vertical at the tower, its height, the fold's depth and the valleys' inset scaled with the radius; the top ring cut flat
    /// by the plane at the tower's height, so the lamellas end level.
    std::vector<std::vector<Point>> compute_rings() const;

    /// The same points on the lamellas' inner faces: at every point the line where the two strips beside it, offset inwards by the
    /// thickness, meet, cut by the point's level.
    std::vector<std::vector<Point>> compute_inner_rings(const std::vector<std::vector<Point>>& stars) const;
};

inline CurvedFold::CurvedFold(
    size_t sectors,
    double radius,
    double top_radius,
    double eave,
    double height,
    double gable,
    double inset,
    size_t rings,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      sectors(sectors),
      radius(radius),
      top_radius(top_radius),
      eave(eave),
      height(height),
      gable(gable),
      inset(inset),
      rings(std::max<size_t>(rings, 3)),
      thickness(thickness) {

    const size_t count = 2 * sectors;
    const std::vector<std::vector<Point>> stars = compute_rings();
    const std::vector<std::vector<Point>> inner_stars = compute_inner_rings(stars);

    // meridians: every ridge and valley a NURBS curve through the rings, on the outer and on the inner face
    std::vector<NurbsCurve> outer;
    std::vector<NurbsCurve> inner;
    const std::shared_ptr<TreeNode> meridians = add_group("meridians");

    for (size_t j = 0; j < count; j++) {
        std::vector<Point> outer_points;
        std::vector<Point> inner_points;

        for (size_t k = 0; k < stars.size(); k++) {
            outer_points.push_back(stars[k][j]);
            inner_points.push_back(inner_stars[k][j]);
        }

        outer.push_back(NurbsCurve::create_interpolated(outer_points));
        inner.push_back(NurbsCurve::create_interpolated(inner_points));
        add_polyline(Polyline(outer_points), meridians);
    }

    // lamellas: every fold strip one curved plate between its two meridians, its inner face between the inner ones, capped at both ends
    const std::shared_ptr<TreeNode> lamellas = add_group("lamellas");

    for (size_t j = 0; j < count; j++) {
        const size_t next = (j + 1) % count;
        add(std::make_shared<PlateCurved>(outer[j], outer[next], inner[j], inner[next], fmt::format("lamella_{}", j)), lamellas);
    }
}

inline std::vector<std::vector<Point>> CurvedFold::compute_rings() const {

    const size_t count = 2 * sectors;
    std::vector<std::vector<Point>> stars;

    for (size_t k = 0; k < rings; k++) {
        const double t = static_cast<double>(k) / static_cast<double>(rings - 1);
        const double ring_radius = top_radius + (radius - top_radius) * (1.0 - t) * (1.0 - t);
        const double ring_height = eave + (height - eave) * t;
        const double fade = ring_radius / radius;
        std::vector<Point> star;

        for (size_t j = 0; j < count; j++) {
            const double angle = 2.0 * Tolerance::PI * static_cast<double>(j) / static_cast<double>(count);
            const bool ridge = j % 2 == 0;
            const double r = ridge ? ring_radius : ring_radius * (1.0 - inset);
            const double z = k + 1 == rings ? height : ring_height + (ridge ? gable * fade : 0.0);
            star.push_back(Point(r * std::cos(angle), r * std::sin(angle), z));
        }

        stars.push_back(star);
    }

    return stars;
}

inline std::vector<std::vector<Point>> CurvedFold::compute_inner_rings(const std::vector<std::vector<Point>>& stars) const {

    const size_t count = 2 * sectors;
    std::vector<std::vector<Point>> inner;

    for (size_t k = 0; k < stars.size(); k++) {
        // the band below the last ring, the one above every other
        const size_t band = std::min(k, stars.size() - 2);
        const std::vector<Point>& low = stars[band];
        const std::vector<Point>& high = stars[band + 1];
        std::vector<Point> ring;

        for (size_t j = 0; j < count; j++) {
            // the strips before and after the point, their planes moved inwards by the thickness
            std::array<Plane, 2> offsets;

            for (size_t side = 0; side < 2; side++) {
                const size_t a = (j + count - 1 + side) % count;
                const size_t b = (a + 1) % count;
                Vector normal = (high[a] - low[a]).cross(low[b] - low[a]);
                normal.normalize_self();
                offsets[side] = Plane::from_point_normal(low[a] + normal * thickness, normal);
            }

            const std::optional<Line> fold = Intersection::plane_plane(offsets[0], offsets[1]);
            const Plane level = Plane::from_point_normal(stars[k][j], Vector(0.0, 0.0, 1.0));
            const std::optional<Point> point = fold ? Intersection::line_plane(*fold, level, false) : std::nullopt;
            ring.push_back(point.value_or(stars[k][j]));
        }

        inner.push_back(ring);
    }

    return inner;
}
