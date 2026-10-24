#pragma once
#include "wood_session.h"
#include "src/templates/folding/diagonal_fold.h"
#include "src/templates/template_chamfer.h"

using namespace session_cpp;
using namespace wood_session;

/// A field of crosswise folded square cells after Engel (p. 230), a diagonal cross fold over the two by two block in its middle.
class FoldComposition : public WoodSession {
public:
    const size_t columns; // Cells along x, even, the block in the middle.
    const size_t rows; // Cells along y, even, the block in the middle.
    const double cell; // mm, every cell's side.
    const double low; // mm, the low nodes' height, the field standing on them.
    const double high; // mm, the high nodes' height.
    const double peak; // mm, every cell's peak over its centre.
    const DiagonalHeights star; // mm, the heights of the cross fold over the block in the middle.
    const double thickness; // mm, every plate.

private:
    Mesh _mesh; // The folded mesh, the plates' outer faces.

public:
    /// The composition as the session named name.
    explicit FoldComposition(
        size_t columns = 6,
        size_t rows = 4,
        double cell = 12000.0,
        double low = 0.0,
        double high = 4600.0,
        double peak = 6500.0,
        const DiagonalHeights& star = {8000.0, 0.0, 15000.0, 0.5, 8000.0},
        double thickness = 120.0,
        const std::string& name = "fold_composition"
    );

    /// Not copied, as a session is not.
    FoldComposition(const FoldComposition&) = delete;

    /// Not assigned, as it is not copied.
    FoldComposition& operator=(const FoldComposition&) = delete;

    /// The folded triangle mesh, the plates' outer faces, one face per plate.
    const Mesh& mesh() const;

private:
    /// The node at column i and row j of the field's grid, low or high by the chessboard, the block's corners raised.
    Point compute_node(size_t i, size_t j) const;
};

inline FoldComposition::FoldComposition(
    size_t columns,
    size_t rows,
    double cell,
    double low,
    double high,
    double peak,
    const DiagonalHeights& star,
    double thickness,
    const std::string& name
)
    : WoodSession(name),
      columns(columns),
      rows(rows),
      cell(cell),
      low(low),
      high(high),
      peak(peak),
      star(star),
      thickness(thickness) {

    // cells: four triangles from the corner nodes to the peak, the block's cells left to the star
    FoldMesh fold;
    const size_t block_i = columns / 2 - 1;
    const size_t block_j = rows / 2 - 1;

    for (size_t i = 0; i < columns; i++)
        for (size_t j = 0; j < rows; j++) {
            if (i >= block_i && i < block_i + 2 && j >= block_j && j < block_j + 2)
                continue;

            const std::array<Point, 4> corners = {
                compute_node(i, j),
                compute_node(i + 1, j),
                compute_node(i + 1, j + 1),
                compute_node(i, j + 1),
            };
            const Point top(0.5 * (corners[0][0] + corners[2][0]), 0.5 * (corners[0][1] + corners[2][1]), peak);

            for (size_t k = 0; k < 4; k++)
                fold.face({corners[k], corners[(k + 1) % 4], top});
        }

    // star: the cross fold spanned diagonally over the block, its corners the block's raised corner nodes
    const Point block_centre = compute_node(block_i + 1, block_j + 1);
    add_diagonal_fold(fold, Point(block_centre[0], block_centre[1], 0.0), cell, star);
    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh inside the field
    const std::shared_ptr<TreeNode> folds = add_group("folds");
    const double half_x = 0.5 * cell * static_cast<double>(columns);
    const double half_y = 0.5 * cell * static_cast<double>(rows);

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (!start || !end)
            continue;

        const bool on_side = std::abs(std::abs((*start)[0]) - half_x) < 1.0
            && std::abs(std::abs((*end)[0]) - half_x) < 1.0;
        const bool on_front = std::abs(std::abs((*start)[1]) - half_y) < 1.0
            && std::abs(std::abs((*end)[1]) - half_y) < 1.0;

        if (!on_side && !on_front)
            add_polyline(Polyline({*start, *end}), folds);
    }

    // plates: one plate per face by the exact offset, the edges on the field's sides cut flush
    const std::shared_ptr<TreeNode> plates = add_group("plates");
    const std::vector<Plane> sides = {
        Plane::from_point_normal(Point(-half_x, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        Plane::from_point_normal(Point(half_x, 0.0, 0.0), Vector(1.0, 0.0, 0.0)),
        Plane::from_point_normal(Point(0.0, -half_y, 0.0), Vector(0.0, 1.0, 0.0)),
        Plane::from_point_normal(Point(0.0, half_y, 0.0), Vector(0.0, 1.0, 0.0)),
    };

    for (const std::shared_ptr<Plate>& plate : offset_plates(_mesh, thickness, sides))
        add(plate, plates);
}

inline const Mesh& FoldComposition::mesh() const {
    return _mesh;
}

inline Point FoldComposition::compute_node(size_t i, size_t j) const {

    // height: the block's corners raised to the star's, else high at odd i + j and low at even
    const size_t block_i = columns / 2 - 1;
    const size_t block_j = rows / 2 - 1;
    const bool block_corner = (i == block_i || i == block_i + 2) && (j == block_j || j == block_j + 2);
    const double z = block_corner ? star.corner : ((i + j) % 2 == 1 ? high : low);
    const double x = cell * (static_cast<double>(i) - 0.5 * static_cast<double>(columns));
    const double y = cell * (static_cast<double>(j) - 0.5 * static_cast<double>(rows));
    return Point(x, y, z);
}
