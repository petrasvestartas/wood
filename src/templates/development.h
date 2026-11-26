#pragma once
#include "wood_session.h"
#if __has_include("src/templates/folding/fold_unfolding.h")
#include "src/templates/folding/fold_unfolding.h"
#endif

using namespace session_cpp;
using namespace wood_session;

// ═══════════════════════════════════════════════════════════════════════════
// Ruled and curved-crease strip development
// ═══════════════════════════════════════════════════════════════════════════

/// Isometric triangle-strip development of one ruled curved plate or a continuous multi-strip curved-crease sheet.
class Development {
public:
    size_t samples = 64; // Rulings per strip used to unfold the meridians.
    Polyline outline; // Closed 2D outer cutting outline in the xy plane.
    Polyline inner_outline; // Closed 2D inner-face outline when built from a FaceCurvedElement.
    Polyline bottom; // Closed 2D bottom-face outline (z = 0) of an unfolded FaceCurvedElement.
    Polyline top; // Closed 2D top-face outline (projected to z = 0) of an unfolded FaceCurvedElement.
    std::vector<Line> rulings; // Straight 2D ruling lines across the unfolded strip.
    std::vector<Polyline> creases; // Interior 2D curved creases when built from three or more meridians.
    double thickness = 0.0; // mm, recovered uniform plate thickness of a FaceCurvedElement.
    double bevel_a = 0.0; // mm, miter bevel offset |t * cot(alpha)| along crease a.
    double bevel_b = 0.0; // mm, miter bevel offset |t * cot(alpha)| along crease b.
    double distortion = 0.0; // Largest relative quad diagonal change, 0 on an exact developable sheet.
    double twist = 0.0; // Largest |ruling . (tangent_a x tangent_b)| of unit vectors, 0 on a developable strip.
    std::shared_ptr<FaceCurvedElement> piece; // Closed 6-patch BRep flat solid piece on z in [0, thickness].

    // ═══════════════════════════════════════════════════════════════════════════
    // Constructors
    // ═══════════════════════════════════════════════════════════════════════════

    /// Construct an empty development.
    Development() = default;

    /// Unfold the ruled strip between curves a and b with the given number of rulings.
    Development(const NurbsCurve& a, const NurbsCurve& b, size_t samples = 64);

    /// Unfold the zero-strain neutral mid-surface and 3D miter bevels of a curved timber plate into a flat solid piece.
    explicit Development(
        const FaceCurvedElement& plate,
        size_t samples = 64,
        const std::string& name = "flat_piece"
    );

    /// Unfold a continuous single-sheet chain of ruled strips across shared curved meridians.
    explicit Development(const std::vector<NurbsCurve>& meridians, size_t samples = 64);

    // ═══════════════════════════════════════════════════════════════════════════
    // Scene export
    // ═══════════════════════════════════════════════════════════════════════════

    /// Place flat 2D cutting outlines side by side in group "flat" beside session's 3D elements.
    static void add_flat(
        WoodSession& session,
        const std::vector<Development>& developments,
        double gap = 400.0
    );

private:
    /// Point at given distances from two flat points, on the side of their line away from a third point.
    static Point third_corner(
        const Point& from,
        const Point& to,
        double from_distance,
        double to_distance,
        const Point& away
    );
};

inline Development::Development(const NurbsCurve& a, const NurbsCurve& b, size_t samples)
    : Development(std::vector<NurbsCurve>{a, b}, samples) {
}

inline Development::Development(
    const FaceCurvedElement& plate,
    size_t samples,
    const std::string& name
)
    : samples(std::max<size_t>(samples, 4)), distortion(0.0), twist(0.0) {

    const size_t n = this->samples;
    std::vector<Point> oa(n + 1);
    std::vector<Point> ob(n + 1);
    std::vector<Point> ia(n + 1);
    std::vector<Point> ib(n + 1);
    std::vector<Point> na(n + 1);
    std::vector<Point> nb(n + 1);

    for (size_t k = 0; k <= n; k++) {
        const double frac = static_cast<double>(k) / static_cast<double>(n);
        oa[k] = plate.outer_a.point_at(plate.outer_a.domain_start() + (plate.outer_a.domain_end() - plate.outer_a.domain_start()) * frac);
        ob[k] = plate.outer_b.point_at(plate.outer_b.domain_start() + (plate.outer_b.domain_end() - plate.outer_b.domain_start()) * frac);
        ia[k] = plate.inner_a.point_at(plate.inner_a.domain_start() + (plate.inner_a.domain_end() - plate.inner_a.domain_start()) * frac);
        ib[k] = plate.inner_b.point_at(plate.inner_b.domain_start() + (plate.inner_b.domain_end() - plate.inner_b.domain_start()) * frac);
        na[k] = Point(0.5 * (oa[k][0] + ia[k][0]), 0.5 * (oa[k][1] + ia[k][1]), 0.5 * (oa[k][2] + ia[k][2]));
        nb[k] = Point(0.5 * (ob[k][0] + ib[k][0]), 0.5 * (ob[k][1] + ib[k][1]), 0.5 * (ob[k][2] + ib[k][2]));
    }

    // Recover thickness and local miter bevel offsets along each 3D ruling
    std::vector<double> delta_a(n + 1, 0.0);
    std::vector<double> delta_b(n + 1, 0.0);
    double thickness_sum = 0.0;

    for (size_t k = 0; k <= n; k++) {
        const size_t prev = k == 0 ? 0 : k - 1;
        const size_t next = k == n ? n : k + 1;
        const Vector tan_a = (na[next] - na[prev]).normalized();
        const Vector tan_b = (nb[next] - nb[prev]).normalized();
        Vector ruling = nb[k] - na[k];
        if (ruling.normalize_self())
            twist = std::max(twist, std::abs(ruling.dot(tan_a.cross(tan_b))));

        Vector strip_normal = tan_a.cross(ruling);
        if (!strip_normal.normalize_self())
            strip_normal = (oa[k] - ia[k]).normalized();
        if ((oa[k] - ia[k]).dot(strip_normal) < 0.0)
            strip_normal = strip_normal * -1.0;

        const double t_a = std::abs((oa[k] - ia[k]).dot(strip_normal));
        const double t_b = std::abs((ob[k] - ib[k]).dot(strip_normal));
        thickness_sum += 0.5 * (t_a + t_b);
        delta_a[k] = (oa[k] - ia[k]).dot(ruling);
        delta_b[k] = (ob[k] - ib[k]).dot(ruling);
    }

    thickness = thickness_sum / static_cast<double>(n + 1);
    const size_t mid = n / 2;
    bevel_a = std::abs(delta_a[mid]);
    bevel_b = std::abs(delta_b[mid]);

    // Unfold zero-strain neutral mid-surface into z = 0 preserving quad sector angles and edge lengths
    std::vector<Point> flat_na(n + 1);
    std::vector<Point> flat_nb(n + 1);
    flat_na[0] = Point(0.0, 0.0, 0.0);
    flat_nb[0] = Point(0.0, na[0].distance(nb[0]), 0.0);

    for (size_t k = 1; k <= n; k++) {
        const Point away_start = k == 1 ? Point(-1.0, 0.0, 0.0) : flat_nb[k - 2];
        flat_na[k] = third_corner(
            flat_na[k - 1],
            flat_nb[k - 1],
            na[k - 1].distance(na[k]),
            nb[k - 1].distance(na[k]),
            away_start
        );
        flat_nb[k] = third_corner(
            flat_nb[k - 1],
            flat_na[k],
            nb[k - 1].distance(nb[k]),
            na[k].distance(nb[k]),
            flat_na[k - 1]
        );
        const double diag_3d = na[k - 1].distance(nb[k]);
        if (diag_3d > 1e-9)
            distortion = std::max(
                distortion,
                std::abs(flat_na[k - 1].distance(flat_nb[k]) - diag_3d) / diag_3d
            );
    }

    rulings.reserve(n + 1);
    std::vector<Point> flat_inner_a(n + 1);
    std::vector<Point> flat_inner_b(n + 1);
    std::vector<Point> flat_outer_a(n + 1);
    std::vector<Point> flat_outer_b(n + 1);
    std::vector<Point> top_2d_a(n + 1);
    std::vector<Point> top_2d_b(n + 1);

    for (size_t k = 0; k <= n; k++) {
        rulings.push_back(Line::from_points(flat_na[k], flat_nb[k]));
        Vector r2d = flat_nb[k] - flat_na[k];
        if (!r2d.normalize_self())
            r2d = Vector(0.0, 1.0, 0.0);

        flat_inner_a[k] = flat_na[k] + r2d * (-0.5 * delta_a[k]);
        flat_inner_b[k] = flat_nb[k] + r2d * (-0.5 * delta_b[k]);
        top_2d_a[k] = flat_na[k] + r2d * (0.5 * delta_a[k]);
        top_2d_b[k] = flat_nb[k] + r2d * (0.5 * delta_b[k]);
        flat_outer_a[k] = Point(top_2d_a[k][0], top_2d_a[k][1], thickness);
        flat_outer_b[k] = Point(top_2d_b[k][0], top_2d_b[k][1], thickness);
    }

    std::vector<Point> bottom_loop = flat_inner_a;
    bottom_loop.insert(bottom_loop.end(), flat_inner_b.rbegin(), flat_inner_b.rend());
    bottom_loop.push_back(flat_inner_a.front());
    bottom = Polyline(bottom_loop);

    std::vector<Point> top_loop = top_2d_a;
    top_loop.insert(top_loop.end(), top_2d_b.rbegin(), top_2d_b.rend());
    top_loop.push_back(top_2d_a.front());
    top = Polyline(top_loop);

    outline = top;
    inner_outline = bottom;

    // Ensure positive outward closed_mesh() winding on the flat solid piece
    const Vector tan_2d = flat_outer_a[mid + 1] - flat_outer_a[mid - 1];
    const Vector across_2d = flat_outer_b[mid] - flat_outer_a[mid];
    const Vector up_2d = flat_outer_a[mid] - flat_inner_a[mid];
    if (across_2d.cross(tan_2d).dot(up_2d) < 0.0) {
        std::swap(flat_outer_a, flat_outer_b);
        std::swap(flat_inner_a, flat_inner_b);
    }

    piece = std::make_shared<FaceCurvedElement>(
        NurbsCurve::create_interpolated(flat_outer_a, CurveNurbsKnotStyle::Uniform),
        NurbsCurve::create_interpolated(flat_outer_b, CurveNurbsKnotStyle::Uniform),
        NurbsCurve::create_interpolated(flat_inner_a, CurveNurbsKnotStyle::Uniform),
        NurbsCurve::create_interpolated(flat_inner_b, CurveNurbsKnotStyle::Uniform),
        name
    );
}

inline Development::Development(const std::vector<NurbsCurve>& meridians, size_t samples)
    : samples(std::max<size_t>(samples, 2)), distortion(0.0), twist(0.0) {

    if (meridians.size() < 2)
        return;

    const size_t count = meridians.size();
    std::vector<std::vector<Point>> grid(count, std::vector<Point>(this->samples + 1));
    std::vector<std::vector<Vector>> tangents(count, std::vector<Vector>(this->samples + 1));

    // sample all meridians and measure ruling twist on each strip
    for (size_t m = 0; m < count; m++) {
        const NurbsCurve& curve = meridians[m];
        const double start = curve.domain_start();
        const double span = curve.domain_end() - start;

        for (size_t k = 0; k <= this->samples; k++) {
            const double t = start + span * static_cast<double>(k) / static_cast<double>(this->samples);
            grid[m][k] = curve.point_at(t);
            tangents[m][k] = curve.tangent_at(t).normalized();
        }
    }

    for (size_t m = 0; m + 1 < count; m++)
        for (size_t k = 0; k <= this->samples; k++) {
            Vector ruling = grid[m + 1][k] - grid[m][k];

            if (ruling.normalize_self())
                twist = std::max(
                    twist,
                    std::abs(ruling.dot(tangents[m][k].cross(tangents[m + 1][k])))
                );
        }

    // unfold the first strip with its first ruling along +y
    std::vector<std::vector<Point>> flat(count, std::vector<Point>(this->samples + 1));
    flat[0][0] = Point(0.0, 0.0, 0.0);
    flat[1][0] = Point(0.0, grid[0][0].distance(grid[1][0]), 0.0);

    for (size_t k = 1; k <= this->samples; k++) {
        const Point away_start = k == 1 ? Point(-1.0, 0.0, 0.0) : flat[1][k - 2];
        flat[0][k] = third_corner(
            flat[0][k - 1],
            flat[1][k - 1],
            grid[0][k - 1].distance(grid[0][k]),
            grid[1][k - 1].distance(grid[0][k]),
            away_start
        );
        flat[1][k] = third_corner(
            flat[1][k - 1],
            flat[0][k],
            grid[1][k - 1].distance(grid[1][k]),
            grid[0][k].distance(grid[1][k]),
            flat[0][k - 1]
        );
        const double diagonal = grid[0][k - 1].distance(grid[1][k]);

        if (diagonal > 1e-9)
            distortion = std::max(
                distortion,
                std::abs(flat[0][k - 1].distance(flat[1][k]) - diagonal) / diagonal
            );
    }

    if (count == 2) {
        rulings.reserve(this->samples + 1);
        for (size_t k = 0; k <= this->samples; k++)
            rulings.push_back(Line::from_points(flat[0][k], flat[1][k]));
    }

    // continue unfolding subsequent strips across each shared curved meridian
    for (size_t m = 1; m + 1 < count; m++) {
        flat[m + 1][0] = third_corner(
            flat[m][0],
            flat[m][1],
            grid[m][0].distance(grid[m + 1][0]),
            grid[m][1].distance(grid[m + 1][0]),
            flat[m - 1][0]
        );

        for (size_t k = 1; k <= this->samples; k++) {
            flat[m + 1][k] = third_corner(
                flat[m + 1][k - 1],
                flat[m][k],
                grid[m + 1][k - 1].distance(grid[m + 1][k]),
                grid[m][k].distance(grid[m + 1][k]),
                flat[m][k - 1]
            );
            const double diag_a = grid[m][k - 1].distance(grid[m + 1][k]);
            const double diag_b = grid[m][k].distance(grid[m + 1][k - 1]);

            if (diag_a > 1e-9)
                distortion = std::max(
                    distortion,
                    std::abs(flat[m][k - 1].distance(flat[m + 1][k]) - diag_a) / diag_a
                );

            if (diag_b > 1e-9)
                distortion = std::max(
                    distortion,
                    std::abs(flat[m][k].distance(flat[m + 1][k - 1]) - diag_b) / diag_b
                );
        }

        creases.push_back(Polyline(flat[m]));
    }

    // closed outer boundary of the unfolded strip or multi-strip sheet
    std::vector<Point> boundary = flat[0];

    for (size_t m = 1; m + 1 < count; m++)
        boundary.push_back(flat[m][this->samples]);

    boundary.insert(boundary.end(), flat[count - 1].rbegin(), flat[count - 1].rend());

    for (size_t m = count - 2; m > 0; m--)
        boundary.push_back(flat[m][0]);

    boundary.push_back(flat[0][0]);
    outline = Polyline(boundary);
}

inline void Development::add_flat(
    WoodSession& session,
    const std::vector<Development>& developments,
    double gap
) {

    if (developments.empty())
        return;

    double max_x = -std::numeric_limits<double>::infinity();
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();

    for (const std::shared_ptr<Element>& element : session.elements())
        for (const Point& corner : element->aabb().corners()) {
            min_x = std::min(min_x, corner[0]);
            max_x = std::max(max_x, corner[0]);
            min_y = std::min(min_y, corner[1]);
        }

    const double base_y = std::isfinite(min_y) ? min_y : 0.0;
    const double margin = std::isfinite(max_x) ? std::max(gap, 0.12 * (max_x - min_x)) : 0.0;
    double cursor_x = (std::isfinite(max_x) ? max_x : 0.0) + margin;
    const std::shared_ptr<TreeNode> group = session.group_named("flat");

    for (size_t i = 0; i < developments.size(); i++) {
        double low_x = std::numeric_limits<double>::infinity();
        double high_x = -std::numeric_limits<double>::infinity();
        double low_y = std::numeric_limits<double>::infinity();

        for (const Point& pt : developments[i].outline.get_points()) {
            low_x = std::min(low_x, pt[0]);
            high_x = std::max(high_x, pt[0]);
            low_y = std::min(low_y, pt[1]);
        }

        const Xform shift = Xform::translation(cursor_x - low_x, base_y - low_y, 0.0);
        Polyline placed = developments[i].outline.transformed(shift);
        placed.name = fmt::format("flat_{}", i);
        session.add_polyline(placed, group);

        for (size_t c = 0; c < developments[i].creases.size(); c++) {
            Polyline crease = developments[i].creases[c].transformed(shift);
            crease.name = fmt::format("flat_crease_{}_{}", i, c);
            session.add_polyline(crease, group);
        }

        cursor_x += (high_x - low_x) + gap;
    }
}

inline Point Development::third_corner(
    const Point& from, const Point& to, double from_distance, double to_distance, const Point& away
) {

    const double base = from.distance(to);
    const double along =
        (from_distance * from_distance - to_distance * to_distance + base * base) / (2.0 * base);
    const double across = std::sqrt(std::max(0.0, from_distance * from_distance - along * along));

    Vector axis = to - from;
    axis.normalize_self();
    const Vector side(-axis[1], axis[0], 0.0);
    const Point foot = from + axis * along;
    const Point left = foot + side * across;
    const Point right = foot - side * across;

    return left.distance(away) > right.distance(away) ? left : right;
}
