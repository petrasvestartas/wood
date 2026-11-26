#pragma once
#include "src/templates/developable_folding/developable_folding_guide.h"

using namespace session_cpp;
using namespace wood_session;

namespace wood_developable_folding {

// ═══════════════════════════════════════════════════════════════════════════
// QuadGrid and FoldMeasures
// ═══════════════════════════════════════════════════════════════════════════

/// Rows of points along a folded sheet: row k a polyline along the curve, quad (k, i) between rows k and k + 1 from point i to i + 1.
using QuadGrid = std::vector<std::vector<Point>>;

/// What the quads of every stage measure against their crease pattern, the checks the examples print.
struct FoldMeasures {
    double planarity =
        0.0; // Largest distance of a quad's corner from the plane of the others over its diagonal.
    double kawasaki =
        0.0; // Radians, largest alternating sum of the sector angles at an inner vertex, 0 when flat-foldable.
    double developability =
        0.0; // Radians, largest sum of the sector angles at an inner vertex minus 2 pi, 0 when developable.
    double stretch = 0.0; // mm, largest change of an edge's length from the crease pattern.
    std::array<double, 2> openings = {
        0.0, 0.0
    }; // Radians, the smallest and largest opening at any crease of any stage.
    size_t crossings = 0; // Pairs of quads of a stage crossing each other, neighbours aside.
    double distortion = 0.0; // Largest relative length error of a curved plate's flat development, 0 without curved plates.
};

// ═══════════════════════════════════════════════════════════════════════════
// SpaceCurveRidge
// ═══════════════════════════════════════════════════════════════════════════

/// The constant-angle discrete curved fold along a space polyline (Tachi 2011): its discrete ribbon, its V ridge of planar quads, their crease pattern and the one-DOF folding.
class SpaceCurveRidge {
public:
    /// The ribbon width wide along curve, zig-zagged by zigzag, and its V ridge opened by angle.
    SpaceCurveRidge(const Polyline& curve, double width, double angle, double zigzag);

    /// The crease: the curve's vertices, every other moved along its curvature vector when zig-zagged.
    const std::vector<Point>& crease() const;

    /// Three rows: one edge of the ribbon, the crease along its middle and the other edge, the ribbon's rulings from Eq. 6.
    const QuadGrid& ribbon() const;

    /// Three rows: arm a, the crease and arm b, the arms at half the opening to the ribbon on either side, trimmed where neighbours meet.
    const QuadGrid& ridge() const;

    /// The ridge developed into the xy plane.
    const QuadGrid& pattern() const;

    /// The crease pattern folded to opening about its crease, one degree of freedom, its middle quad of arm a where the ridge's is.
    QuadGrid folded(double opening) const;

    // ═══════════════════════════════════════════════════════════════════════════
    // Stages
    // ═══════════════════════════════════════════════════════════════════════════

    /// The checks over every stage against the crease pattern pattern.
    static FoldMeasures measure(const std::vector<QuadGrid>& stages, const QuadGrid& pattern);

    /// Adds the curve, the zig-zagged crease when it differs and the ribbon as a quad mesh, groups curve and ribbon.
    static void
    add_curve(WoodSession& session, const Polyline& curve, const SpaceCurveRidge& ridge);

    /// Adds every stage's quads and creases side by side along x and the crease pattern under them, and returns every stage's shift.
    static std::vector<Vector>
    add_quads(WoodSession& session, const std::vector<QuadGrid>& stages, const QuadGrid& pattern);

    /// Adds a mitered plate thickness thick on every quad of every stage moved by its shift, the first stage's put in one by one.
    static void add_plates(
        WoodSession& session,
        const std::vector<QuadGrid>& stages,
        const std::vector<Vector>& shifts,
        double thickness
    );

    /// Adds every arm of every stage moved by its shift as one curved plate thickness thick off its outer side, the first stage's put in one by one, and returns their largest development distortion.
    static double add_arms(
        WoodSession& session,
        const std::vector<QuadGrid>& stages,
        const std::vector<Vector>& shifts,
        double thickness
    );

private:
    std::vector<Point> _crease; // The crease's vertices.
    QuadGrid _ribbon; // The ribbon's edges and the crease.
    QuadGrid _ridge; // Arm a, the crease, arm b at the opening.
    QuadGrid _pattern; // The ridge developed flat.

    /// The curve's vertices, every other moved zigzag along its curvature vector and the others against it.
    static std::vector<Point> compute_crease(const Polyline& curve, double zigzag);

    /// The ribbon width wide: the middle edge's frame from its neighbours, every next ruling solved from the last frame by Eq. 6.
    QuadGrid compute_ribbon(double width) const;

    /// The V ridge: per edge two half-planes at half of angle to the ribbon's quad, every ruling where neighbouring half-planes meet.
    QuadGrid compute_ridge(double width, double angle) const;

    /// The ridge developed into the xy plane by its sector angles and lengths, vertex by vertex.
    QuadGrid compute_pattern() const;

    /// The points of row 0 to 2 at vertex next of folded, from the quads at vertex vertex already placed.
    std::array<Point, 3> compute_step(const QuadGrid& folded, size_t vertex, size_t next) const;

    /// The quad mesh of grid, quad (k, i) wound from row k to row k + 1.
    static Mesh to_mesh(const QuadGrid& grid, const std::string& name);

    /// Radians between two vectors, 0 to pi.
    static double angle(const Vector& a, const Vector& b);

    /// Unit vector at sector angles first and second to the unit vectors a and b, on the side sign of a x b.
    static Vector
    compute_ray(const Vector& a, double first, const Vector& b, double second, double sign);

    /// The point of a flat quad with corner at, rays from and to, placed on the folded corner with rays along and toward.
    static Point place(
        const Point& point,
        const Point& at,
        const Vector& from,
        const Vector& to,
        const Point& corner,
        const Vector& along,
        const Vector& toward
    );

    /// Whether the segment from start to end passes through the triangle a b c.
    static bool
    pierces(const Point& start, const Point& end, const Point& a, const Point& b, const Point& c);
};

}
