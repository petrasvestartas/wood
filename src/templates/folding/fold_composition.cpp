#include "src/templates/folding/fold_composition.h"
#include "src/templates/folding/diagonal_fold.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
FoldComposition::FoldComposition(const FoldCompositionGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // cells: four triangles from the corner nodes to the peak, the block's cells left to the star
    FoldMesh fold;
    const size_t block_i = guide.columns / 2 - 1;
    const size_t block_j = guide.rows / 2 - 1;

    for (size_t i = 0; i < guide.columns; i++)
        for (size_t j = 0; j < guide.rows; j++) {
            if (i >= block_i && i < block_i + 2 && j >= block_j && j < block_j + 2)
                continue;

            const std::array<Point, 4> corners = {
                compute_node(i, j),
                compute_node(i + 1, j),
                compute_node(i + 1, j + 1),
                compute_node(i, j + 1),
            };
            const Point top(
                0.5 * (corners[0][0] + corners[2][0]),
                0.5 * (corners[0][1] + corners[2][1]),
                guide.peak
            );

            for (size_t k = 0; k < 4; k++)
                fold.face({corners[k], corners[(k + 1) % 4], top});
        }

    // star: the cross fold spanned diagonally over the block, its corners the block's raised corner nodes
    const Point block_centre = compute_node(block_i + 1, block_j + 1);
    add_diagonal_fold(fold, Point(block_centre[0], block_centre[1], 0.0), guide.cell, guide.star);
    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: every edge of the mesh inside the field
    const std::shared_ptr<TreeNode> folds = add_group("folds");
    const double half_x = 0.5 * guide.cell * static_cast<double>(guide.columns);
    const double half_y = 0.5 * guide.cell * static_cast<double>(guide.rows);

    for (const std::pair<size_t, size_t>& edge : _mesh.edges()) {
        const std::optional<Point> start = _mesh.vertex_point(edge.first);
        const std::optional<Point> end = _mesh.vertex_point(edge.second);

        if (!start || !end)
            continue;

        const bool on_side = std::abs(std::abs((*start)[0]) - half_x) < 1.0 &&
                             std::abs(std::abs((*end)[0]) - half_x) < 1.0;
        const bool on_front = std::abs(std::abs((*start)[1]) - half_y) < 1.0 &&
                              std::abs(std::abs((*end)[1]) - half_y) < 1.0;

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

    for (const std::shared_ptr<FacePlanarElement>& plate :
         offset_plates(_mesh, guide.thickness, sides))
        add(plate, plates);

    // assembly: the star of sixteen plates from its four sectors, then the pyramids of four plates around it
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    std::vector<size_t> keys;
    for (size_t i = 0; i < placed.size(); i++)
        keys.push_back(i / 4);
    const std::vector<AssemblyModule> cells = AssemblyModule::modules_of("pyramid", placed, keys);
    std::vector<AssemblyModule> sectors(cells.end() - 4, cells.end());
    for (size_t i = 0; i < sectors.size(); i++)
        sectors[i].name = fmt::format("sector_{}", i);
    std::vector<AssemblyModule> whole = {AssemblyModule::module("star", sectors)};
    whole.insert(whole.end(), cells.begin(), cells.end() - 4);
    assemblies = {AssemblyModule::module(name, whole)};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& FoldComposition::mesh() const {
    return _mesh;
}

Point FoldComposition::compute_node(size_t i, size_t j) const {

    // height: the block's corners raised to the star's, else high at odd i + j and low at even
    const size_t block_i = guide.columns / 2 - 1;
    const size_t block_j = guide.rows / 2 - 1;
    const bool block_corner =
        (i == block_i || i == block_i + 2) && (j == block_j || j == block_j + 2);
    const double z = block_corner ? guide.star.corner : ((i + j) % 2 == 1 ? guide.high : guide.low);
    const double x =
        guide.cell * (static_cast<double>(i) - 0.5 * static_cast<double>(guide.columns));
    const double y = guide.cell * (static_cast<double>(j) - 0.5 * static_cast<double>(guide.rows));
    return Point(x, y, z);
}

}
