#pragma once
#include "wood_session.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// The heights of a cross fold spanned diagonally over a square plan .
struct DiagonalHeights {
    double corner; // mm, the four corners.
    double middle; // mm, the four edges' middles.
    double diagonal; // mm, the fold point on every diagonal.
    double
        along; // Where that point lies on the diagonal, as a share of its length from the corner to the centre.
    double centre; // mm, the centre.
};

/// The sixteen triangles of one cross-folded square centred on centre, half its side half, gathered into fold.
inline void add_diagonal_fold(
    FoldMesh& fold, const Point& centre, double half, const DiagonalHeights& heights
) {

    const std::array<Point, 4> corners = {
        Point(centre[0] - half, centre[1] - half, heights.corner),
        Point(centre[0] + half, centre[1] - half, heights.corner),
        Point(centre[0] + half, centre[1] + half, heights.corner),
        Point(centre[0] - half, centre[1] + half, heights.corner),
    };
    const Point top(centre[0], centre[1], heights.centre);

    for (size_t i = 0; i < 4; i++) {
        const Point& corner = corners[i];
        const Point& before = corners[(i + 3) % 4];
        const Point& after = corners[(i + 1) % 4];
        const Point middle_before(
            0.5 * (corner[0] + before[0]), 0.5 * (corner[1] + before[1]), heights.middle
        );
        const Point middle_after(
            0.5 * (corner[0] + after[0]), 0.5 * (corner[1] + after[1]), heights.middle
        );
        const double x = corner[0] + (centre[0] - corner[0]) * heights.along;
        const double y = corner[1] + (centre[1] - corner[1]) * heights.along;
        const Point diagonal(x, y, heights.diagonal);
        fold.face({corner, middle_before, diagonal});
        fold.face({corner, diagonal, middle_after});
        fold.face({middle_before, top, diagonal});
        fold.face({diagonal, top, middle_after});
    }
}

/// Cross-folded surfaces spanned diagonally over a square plan, four sectors folded on their diagonals.
class DiagonalFold : public WoodSession {
public:
    const double size; // mm, the square plan's side, centred on the origin.
    const DiagonalHeights
        heights; // mm, the corners, edge middles, diagonal fold points and centre.
    const double thickness; // mm, every plate.

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

public:
    /// The cross fold over the square as the session named name.
    explicit DiagonalFold(
        double size = 18000.0,
        const DiagonalHeights& heights = {3300.0, 0.0, 6200.0, 0.5, 3300.0},
        double thickness = 120.0,
        const std::string& name = "diagonal_fold"
    );

    /// Not copied, as a session is not.
    DiagonalFold(const DiagonalFold&) = delete;

    /// Not assigned, as it is not copied.
    DiagonalFold& operator=(const DiagonalFold&) = delete;

    /// The folded triangle mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;
};

inline DiagonalFold::DiagonalFold(
    double size, const DiagonalHeights& heights, double thickness, const std::string& name
)
    : WoodSession(name), size(size), heights(heights), thickness(thickness) {

    // mesh: the sixteen triangles of the cross fold
    FoldMesh fold;
    add_diagonal_fold(fold, Point(0.0, 0.0, 0.0), 0.5 * size, heights);
    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: the diagonals, the cross and the lines from middle to middle, every edge of the mesh inside the plan
    const std::shared_ptr<TreeNode> folds = add_group("folds");
    const double half = 0.5 * size;

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (!start || !end)
            continue;

        const bool on_side = std::abs(std::abs((*start)[0]) - half) < 1.0 &&
                             std::abs(std::abs((*end)[0]) - half) < 1.0;
        const bool on_front = std::abs(std::abs((*start)[1]) - half) < 1.0 &&
                              std::abs(std::abs((*end)[1]) - half) < 1.0;

        if (!on_side && !on_front)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, the edges on the plan's sides cut flush
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(-half, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        Plane::from_point_normal(Point(half, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        Plane::from_point_normal(Point(0.0, -half, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, half, 0.0), Vector(0.0, 1.0, 0.0)),
    };

    for (const std::shared_ptr<Plate>& plate : offset_plates(_mesh, thickness, sides))
        add(plate, plates);
}

inline const Mesh& DiagonalFold::mesh() const {
    return _mesh;
}
