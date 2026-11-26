#include "src/templates/folding/diagonal_fold.h"
#include "src/templates/folding/fold_mesh.h"
#include "src/templates/template_chamfer.h"

namespace wood_folding {

// ═══════════════════════════════════════════════════════════════════════════
// Cross fold
// ═══════════════════════════════════════════════════════════════════════════
void add_diagonal_fold(
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

// ═══════════════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════════════
DiagonalFold::DiagonalFold(const DiagonalFoldGuide& guide, const std::string& name)
    : WoodSession(name), guide(guide) {

    // mesh: the sixteen triangles of the cross fold
    FoldMesh fold;
    add_diagonal_fold(fold, Point(0.0, 0.0, 0.0), 0.5 * guide.size, guide.heights);
    _mesh = fold.mesh();
    add_mesh(_mesh, add_group("mesh"));

    // folds: the diagonals, the cross and the lines from middle to middle, every edge of the mesh inside the plan
    const std::shared_ptr<TreeNode> folds = add_group("folds");
    const double half = 0.5 * guide.size;

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

    for (const std::shared_ptr<FacePlanarElement>& plate :
         offset_plates(_mesh, guide.thickness, sides))
        add(plate, plates);

    // assembly: the four sectors of four plates, each folded on its diagonal, leaned on each other
    const std::vector<std::shared_ptr<FacePlanarElement>> placed =
        get_elements_numbered<FacePlanarElement>("plate");
    std::vector<size_t> keys;
    for (size_t i = 0; i < placed.size(); i++)
        keys.push_back(i / 4);
    assemblies = {AssemblyModule::module(name, AssemblyModule::modules_of("sector", placed, keys))};
}

// ═══════════════════════════════════════════════════════════════════════════
// Results and steps
// ═══════════════════════════════════════════════════════════════════════════
const Mesh& DiagonalFold::mesh() const {
    return _mesh;
}

}
