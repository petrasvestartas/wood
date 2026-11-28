#pragma once
#include "wood_session.h"

using namespace laika_cpp;
using namespace wood_session;

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// FoldPoint
// ═══════════════════════════════════════════════════════════════════════════

/// Where a variation folds a side of the truncated pyramid, as shares of a plan line and of the height.
struct FoldPoint {
    double along; // Share of the line's plan length from the base edge.
    double rise; // Share of the height.
};

// ═══════════════════════════════════════════════════════════════════════════
// DiagonalHeights
// ═══════════════════════════════════════════════════════════════════════════

/// The heights of a cross fold spanned diagonally over a square plan.
struct DiagonalHeights {
    double corner; // mm, the four corners.
    double middle; // mm, the four edges' middles.
    double diagonal; // mm, the fold point on every diagonal.
    double along; // Where that point lies on the diagonal, a share of corner to centre.
    double centre; // mm, the centre.
};

// ═══════════════════════════════════════════════════════════════════════════
// ReflexFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A folded plate shell: a profile carried along a cross section, folded at every cross-section point, in mm.
struct ReflexFoldGuide {
    Polyline cross_section = arch(); // The polyline the profile is carried along.
    Polyline profile = zigzag(); // The polyline folded at every cross-section point.
    double thickness = 10.0; // Every plate.
    double chamfer_bottom = 20.0; // Cut back at the bottom outline's corners.
    double chamfer_top = 20.0; // Cut back at the top outline's corners.
    double chamfer_angle = 180.0; // Degrees, corners sharper than this are chamfered.

    /// A 2 m arch of five points in the xz plane.
    static Polyline arch() {
        return Polyline({
            {0.0, 0.0, 0.0},
            {232.466867, 0.0, 578.230273},
            {966.431048, 0.0, 738.362403},
            {1714.771039, 0.0, 604.197646},
            {2037.199246, 0.0, 4.784134},
        });
    }

    /// A 1.6 m zigzag along -y in the xy plane.
    static Polyline zigzag() {
        return Polyline({
            {0.0, 0.0, 0.0},
            {-77.091582, -89.779688, 0.0},
            {-19.195901, -179.559376, 0.0},
            {-89.503039, -269.339064, 0.0},
            {-25.912208, -359.118752, 0.0},
            {-89.664508, -448.898440, 0.0},
            {-19.472054, -538.678128, 0.0},
            {-77.485143, -628.457816, 0.0},
            {-0.817868, -718.237504, 0.0},
            {-54.785307, -808.017192, 0.0},
            {26.377605, -897.796881, 0.0},
            {-25.921340, -987.576569, 0.0},
            {55.558217, -1077.356257, 0.0},
            {1.964204, -1167.135945, 0.0},
            {79.167116, -1256.915633, 0.0},
            {21.364954, -1346.695321, 0.0},
            {91.462653, -1436.475009, 0.0},
            {27.192231, -1526.254697, 0.0},
            {89.830774, -1616.034385, 0.0},
        });
    }

    /// The default arch and zigzag, 10 mm plates with 20 mm chamfers.
    static ReflexFoldGuide reflex_fold() {
        return ReflexFoldGuide();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// DiamondMeshGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A diamond shell: a NURBS surface split into rhombi of two triangles each, in mm.
struct DiamondMeshGuide {
    NurbsSurface surface = arch(); // The surface the pattern is laid on.
    int u_divisions = 8; // Cells across u.
    int v_divisions = 4; // Cells along v.
    double thickness = 10.0; // Every plate.
    double chamfer = 1.0; // Cut back at the corners of both outlines.
    double chamfer_angle = 180.0; // Degrees, corners sharper than this are chamfered.

    /// A bicubic arch, 3000 across, 5000 along v and 1500 high.
    static NurbsSurface arch() {

        const double width = 3000.0;
        const double length = 5000.0;
        const double height = 1500.0;
        std::vector<Point> points;

        for (int i = 0; i < 4; i++) {
            const double z = (i == 1 || i == 2) ? height * 4.0 / 3.0 : 0.0;

            for (int j = 0; j < 4; j++)
                points.push_back({width * i / 3.0, length * j / 3.0, z});
        }

        NurbsSurface surface = NurbsSurface::create(false, false, 3, 3, 4, 4, points);
        surface.set_domain(0, 0.0, width);
        surface.set_domain(1, 0.0, length);
        surface.transpose();

        return surface;
    }

    /// An 8 x 4 pattern on the arch, 40 mm plates with 10 mm chamfers.
    static DiamondMeshGuide diamond_mesh() {

        DiamondMeshGuide guide;
        guide.thickness = 40.0;
        guide.chamfer = 10.0;

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// PrismaticFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A singly folded plate: a fold profile carried straight along the span, in mm.
struct PrismaticFoldGuide {
    Polyline profile = zigzag(4, 1500.0, 1200.0); // In yz at x 0, along +y, a fold per inner point.
    double span = 12000.0; // Along x, the folds' length between the end supports.
    double thickness = 120.0; // Every plate and diaphragm.
    size_t diaphragms = 3; // Under the folds, one at each end, the rest between; 0 for none.
    double edge_flange = 400.0; // The flange folded on at each free edge; 0 for none.

    /// The zigzag of ridges ridges between valleys on z 0, every plate fold_width across in plan and depth high.
    static Polyline zigzag(size_t ridges, double fold_width, double depth) {

        std::vector<Point> points;

        for (size_t i = 0; i <= 2 * ridges; i++)
            points.push_back(
                Point(0.0, fold_width * static_cast<double>(i), i % 2 == 1 ? depth : 0.0)
            );

        return Polyline(points);
    }

    /// The polygon of folds folds over width, its corners on a circular arc rising rise.
    static Polyline polygon(size_t folds, double width, double rise) {

        // the arch through both supports and the crown: its radius and its centre under the crown
        const double half = 0.5 * width;
        const double radius = (half * half + rise * rise) / (2.0 * rise);
        const double opening = std::asin(half / radius);
        std::vector<Point> points;

        for (size_t i = 0; i <= folds + 1; i++) {
            const double angle =
                -opening + 2.0 * opening * static_cast<double>(i) / static_cast<double>(folds + 1);
            points.push_back(Point(
                0.0, half + radius * std::sin(angle), rise - radius + radius * std::cos(angle)
            ));
        }

        return Polyline(points);
    }

    /// Four ridges 1500 mm wide and 1200 mm deep over 12 m, three diaphragms, 400 mm edge flanges.
    static PrismaticFoldGuide prismatic_fold() {
        return PrismaticFoldGuide();
    }

    /// Seven folds on an arc 12 m wide and 4 m high over 12 m, a diaphragm at each end, no flanges.
    static PrismaticFoldGuide folded_barrel() {

        PrismaticFoldGuide guide;
        guide.profile = polygon(7, 12000.0, 4000.0);
        guide.diaphragms = 2;
        guide.edge_flange = 0.0;

        return guide;
    }

    /// The Vidy hall: three ridges 2200 mm wide and deep over 14 m, 45 mm plates, no diaphragms or flanges.
    static PrismaticFoldGuide vidy_hall() {

        PrismaticFoldGuide guide;
        guide.profile = zigzag(3, 2200.0, 2200.0);
        guide.span = 14000.0;
        guide.thickness = 45.0;
        guide.diaphragms = 0;
        guide.edge_flange = 0.0;

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// CounterFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A folded plate whose profile changes along the span, every warped strip folded on one diagonal, in mm.
struct CounterFoldGuide {
    std::vector<Polyline> stations = counter_running(
        4, 1500.0, 1500.0, 13500.0
    ); // Profiles along +x, each in a yz plane along +y, one point count.
    int folding = 0; // Warped strips: 0 ridge to ridge, 1 ridge to valley, 2 alternating.
    double thickness = 120.0; // Every plate.

    /// A zigzag of ridges ridges at x, its valleys on elevation, counter starting it on a ridge.
    static Polyline zigzag(
        double x, size_t ridges, double fold_width, double depth, double elevation, bool counter
    ) {

        std::vector<Point> points;

        for (size_t i = 0; i <= 2 * ridges; i++) {
            const bool ridge = (i % 2 == 1) != counter;
            points.push_back(
                Point(x, fold_width * static_cast<double>(i), elevation + (ridge ? depth : 0.0))
            );
        }

        return Polyline(points);
    }

    /// Counter-running folding: the zigzag at the start, its counter profile at the end.
    static std::vector<Polyline>
    counter_running(size_t ridges, double fold_width, double depth, double span) {
        return {
            zigzag(0.0, ridges, fold_width, depth, 0.0, false),
            zigzag(span, ridges, fold_width, depth, 0.0, true),
        };
    }

    /// The centre fold elevated over the edge folds, its ridges middle_depth over the edges' ridges.
    static std::vector<Polyline> elevated_centre(
        size_t ridges, double fold_width, double depth, double span, double middle_depth
    ) {
        return {
            zigzag(0.0, ridges, fold_width, depth, 0.0, true),
            zigzag(0.5 * span, ridges, fold_width, middle_depth, depth, false),
            zigzag(span, ridges, fold_width, depth, 0.0, true),
        };
    }

    /// Four ridges 1500 mm wide and deep running counter over 13.5 m, ridge to ridge.
    static CounterFoldGuide counter_fold() {
        return CounterFoldGuide();
    }

    /// The centre fold 1500 mm over the edge folds over 15 m, ridge to ridge.
    static CounterFoldGuide elevated_fold() {

        CounterFoldGuide guide;
        guide.stations = elevated_centre(4, 1500.0, 1500.0, 15000.0, 1500.0);

        return guide;
    }

    /// The Vidy hall: three ridges 2200 mm wide and deep, the centre 2200 mm over the edges over 14 m, 45 mm plates.
    static CounterFoldGuide vidy_hall() {

        CounterFoldGuide guide;
        guide.stations = elevated_centre(3, 2200.0, 2200.0, 14000.0, 2200.0);
        guide.thickness = 45.0;

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// ConicalFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A folded plate with conical folding, in mm.
struct ConicalFoldGuide {
    size_t folds = 4; // Folds across, a ridge each between two valleys.
    double fold_width = 1500.0; // Every flank across in plan, half a fold from valley to ridge.
    double depth = 1500.0; // The ridges over the valleys at mid-span.
    double end_depth = 600.0; // The cut over the valleys at either end.
    double span = 14400.0; // Along x.
    double thickness = 120.0; // Every plate.

    /// Four folds over 14.4 m, the ridges 600 mm high at the supports and 1500 mm at mid-span.
    static ConicalFoldGuide conical_fold() {
        return ConicalFoldGuide();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// FoldedFrameGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A frame or arch of triangular folded plates, half a bay one zigzag fold from the foot to the crown, in mm.
struct FoldedFrameGuide {
    Polyline zigzag = two_hinged_zigzag(
        30000.0, 4500.0, 5800.0, 6600.0, 8750.0, 2900.0, 2200.0
    ); // Half a bay from the foot to the crown.
    double bay = 4500.0; // Every bay along y.
    size_t bays = 4; // Bays along y.
    double thickness = 120.0; // Every plate.

    /// The two-hinged frame's zigzag, ridge-to-ridge folding: columns eaves high, the knees bulging out bulge.
    static Polyline two_hinged_zigzag(
        double span,
        double bay,
        double eaves,
        double ceiling,
        double ridge,
        double knee,
        double bulge
    ) {

        const double centre = 0.5 * bay;
        const double middle = 0.5 * span;

        return Polyline({
            Point(0.0, centre, 0.0),
            Point(0.0, 0.0, 0.0),
            Point(-bulge, centre, knee),
            Point(0.0, 0.0, eaves),
            Point(middle, centre, ridge),
            Point(middle, 0.0, ceiling),
        });
    }

    /// The A-frame's zigzag, ridge-to-valley folding: two legs to an apex, their diagonals crossing at the waist.
    static Polyline a_frame_zigzag(
        double span,
        double bay,
        double apex,
        double ridge,
        double knee,
        double bulge,
        double waist_in,
        double waist
    ) {

        const double centre = 0.5 * bay;

        return Polyline({
            Point(0.0, 0.0, 0.0),
            Point(-bulge, centre, knee),
            Point(waist_in, centre, waist),
            Point(0.5 * span, 0.0, apex),
            Point(0.5 * span, centre, ridge),
        });
    }

    /// The three-hinged octagonal arch's zigzag at the proportions, height high.
    static Polyline three_hinged_zigzag(double span, double bay, double height) {

        const double centre = 0.5 * bay;

        return Polyline({
            Point(0.0, centre, 0.0),
            Point(-0.075 * span, 0.0, 0.29 * height),
            Point(-0.23 * span, centre, 0.29 * height),
            Point(-0.115 * span, 0.0, 0.59 * height),
            Point(-0.195 * span, centre, 0.67 * height),
            Point(0.27 * span, 0.0, 0.95 * height),
            Point(0.27 * span, centre, height),
            Point(0.5 * span, 0.0, height),
            Point(0.5 * span, centre, height),
        });
    }

    /// A 30 m two-hinged portal of four 4.5 m bays, ridge to ridge.
    static FoldedFrameGuide two_hinged_frame() {
        return FoldedFrameGuide();
    }

    /// A 21 m A-frame of four 3.85 m bays, its apex 12.7 m high under an 18.1 m ridge, ridge to valley.
    static FoldedFrameGuide a_frame() {

        FoldedFrameGuide guide;
        guide.zigzag =
            a_frame_zigzag(21000.0, 3850.0, 12700.0, 18100.0, 6000.0, 1450.0, 6100.0, 4000.0);
        guide.bay = 3850.0;

        return guide;
    }

    /// A 21.5 m octagonal three-hinged arch 20.5 m high of five 5 m bays.
    static FoldedFrameGuide three_hinged_arch() {

        FoldedFrameGuide guide;
        guide.zigzag = three_hinged_zigzag(21500.0, 5000.0, 20500.0);
        guide.bay = 5000.0;
        guide.bays = 5;

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// SingleFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Singly folded surfaces over a regular polygon plan, every sector folded once on its ridge, in mm.
struct SingleFoldGuide {
    size_t sides = 4; // Sides of the regular polygon plan.
    double radius = 12728.0; // The plan's circumradius, its corners on the ground.
    double overhang = 0.0; // Every edge's middle off the edge, outward when positive.
    double gable = 5400.0; // The height of every edge's middle, the ridges' outer ends.
    double apex = 6900.0; // The centre's height, the ridges' inner end.
    double thickness = 120.0; // Every plate.

    /// A square plan, vertical gables 5.4 m high, the ridges rising to 6.9 m at the centre.
    static SingleFoldGuide square_fold() {
        return SingleFoldGuide();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// DiagonalFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Cross-folded surfaces spanned diagonally over a square plan, four sectors folded on their diagonals, in mm.
struct DiagonalFoldGuide {
    double size = 18000.0; // The square plan's side, centred on the origin.
    DiagonalHeights heights = {
        3300.0, 0.0, 6200.0, 0.5, 3300.0
    }; // The corners, edge middles, diagonal fold points and centre.
    double thickness = 120.0; // Every plate.

    /// An 18 m square, the diagonal ridges folded up to 6.2 m, the corners and centre at 3.3 m.
    static DiagonalFoldGuide cross_fold() {
        return DiagonalFoldGuide();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// FoldCompositionGuide
// ═══════════════════════════════════════════════════════════════════════════

/// A field of crosswise folded square cells, a cross fold over the block in the middle, in mm.
struct FoldCompositionGuide {
    size_t columns = 6; // Cells along x, even, the block in the middle.
    size_t rows = 4; // Cells along y, even, the block in the middle.
    double cell = 12000.0; // Every cell's side.
    double low = 0.0; // The low nodes' height, the field standing on them.
    double high = 4600.0; // The high nodes' height.
    double peak = 6500.0; // Every cell's peak over its centre.
    DiagonalHeights star = {
        8000.0, 0.0, 15000.0, 0.5, 8000.0
    }; // The cross fold over the block in the middle.
    double thickness = 120.0; // Every plate.

    /// Six by four 12 m cells on ground and 4.6 m nodes, a 15 m star over the 2 x 2 block in the middle.
    static FoldCompositionGuide fold_composition() {
        return FoldCompositionGuide();
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// TruncatedPyramidGuide
// ═══════════════════════════════════════════════════════════════════════════

/// The pyramid and truncated pyramid on a square plan, its sides folded at edge and corner points, in mm.
struct TruncatedPyramidGuide {
    double half = 9000.0; // Half the base square's side, centred on the origin.
    double top = 3000.0; // Half the top square's side, 0 for an apex.
    double height = 6800.0; // The top's height.
    std::optional<FoldPoint> edge_fold; // Every base edge's middle moved in and raised, or none.
    std::optional<FoldPoint> corner_fold; // Every corner line bent at this point, or none.
    double thickness = 120.0; // Every plate.

    /// Four triangles over an 18 m square to an apex 6 m high.
    static TruncatedPyramidGuide pyramid() {

        TruncatedPyramidGuide guide;
        guide.top = 0.0;
        guide.height = 6000.0;

        return guide;
    }

    /// Four trapezoids over an 18 m square and a 6 m top 6.8 m high.
    static TruncatedPyramidGuide truncated_pyramid() {
        return TruncatedPyramidGuide();
    }

    /// The truncated pyramid, every base edge's middle moved in 21 % and raised to 54 % of the height.
    static TruncatedPyramidGuide folded_truncated_pyramid() {

        TruncatedPyramidGuide guide;
        guide.edge_fold = FoldPoint{0.21, 0.54};

        return guide;
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// TriangleFoldGuide
// ═══════════════════════════════════════════════════════════════════════════

/// Folded triangular surfaces over a regular polygon plan, a gable on every edge, in mm.
struct TriangleFoldGuide {
    size_t sides = 4; // Sides of the regular polygon plan.
    double radius = 12000.0; // The plan's circumradius, its corners on the ground.
    double middle = 5000.0; // The edges' middles' height, the gables' peaks.
    double centre = 6500.0; // The centre's height; at the middles' one flat face.
    double thickness = 120.0; // Every plate.

    /// A square plan, gables 5 m high, the centre at 6.5 m.
    static TriangleFoldGuide triangle_square() {
        return TriangleFoldGuide();
    }
};

}
